/**
 *  CEF OSR implementation used für vdr-plugin-htmlskin
 *
 *  osrhandler.cpp
 *
 *  (c) 2019 Robert Hannebauer
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#include <cstdint>
#include <thread>
#include <algorithm>
#include <sys/shm.h>
#include <mutex>
#include "browser.h"

// #define DBG_MEASURE_TIME 1

// FULL HD
#define OSD_BUF_SIZE (1920 * 1080 * 4)
#define OSD_KEY 0xDEADC0DE

BrowserClient* OSRHandler::browserClient;

OSRHandler::OSRHandler(BrowserClient *bc, int width, int height) {
    CONSOLE_TRACE("Create OSRHandler and open shared memory");

    browserClient = bc;
    renderWidth = width;
    renderHeight = height;

    // init shared memory
    shmid = -1;
    shmp = nullptr;

    shmid = shmget(OSD_KEY, OSD_BUF_SIZE, 0666 | IPC_CREAT | IPC_EXCL) ;

    if (errno == EEXIST) {
        shmid = shmget(OSD_KEY, OSD_BUF_SIZE, 0666);
    }

    if (shmid == -1) {
        perror("Unable to get shared memory");
        return;
    }

    shmp = (uint8_t *) shmat(shmid, NULL, 0);
    if (shmp == (void *) -1) {
        perror("Unable to attach to shared memory");
        return;
    }
}

OSRHandler::~OSRHandler() {
    if (shmdt(shmp) == -1) {
        perror("Unable to detach from shared memory");
        return;
    }

    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        perror("Fehler in After IPC_RMID");
        // Either this process or VDR removes the shared memory
    }

    fprintf(stderr, "Delete shared memory\n");
}

void OSRHandler::setRenderSize(int width, int height) {
    renderWidth = width;
    renderHeight = height;
}

void OSRHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
    rect = CefRect(0, 0, renderWidth, renderHeight);
}

void OSRHandler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height) {
    if (shmp != nullptr) {
        if (width > 1920) {
            fprintf(stderr, "Warn: Width %d is too high. Maximum is 1920", width);
        }

        if (height > 1080) {
            fprintf(stderr, "Warn: Height %d is too high. Maximum is 1080", height);
        }

        int w = std::min(width, 1920);
        int h = std::min(height, 1080);

        // CONSOLE_TRACE("OnPaint: Try to get shm_mutek.lock");

        shm_mutex.lock();

        // CONSOLE_TRACE("OnPaint: Got shm_mutek.lock");

        memcpy(shmp, buffer, w * h * 4);
        browserClient->SendToVdrOsd("OSDU", w, h);

        // CONSOLE_TRACE("OnPaint: Send OSDU to VDR");

        // TEST
        /*
        static int i = 0;
        char *filename = nullptr;
        asprintf(&filename, "image_%d.rgba", i);
        FILE *f = fopen(filename, "wb");
        fwrite(buffer, width * height * 4, 1, f);
        fclose(f);

        char *pngfile = nullptr;
        asprintf(&pngfile, "gm convert -size 1280x720 -depth 8 %s %s.png", filename, filename);
        system(pngfile);

        free(filename);
        free(pngfile);
        pngfile = nullptr;
        filename = nullptr;
        ++i;
        */
        // TEST
    } else {
        CONSOLE_CRITICAL("Shared memory does not exists!");
    }
}