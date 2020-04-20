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
#include <chrono>
#include <algorithm>
#include <sys/shm.h>
#include <mutex>
#include "browser.h"

// #define DBG_MEASURE_TIME 1

// FULL HD
#define OSD_BUF_SIZE (1920 * 1080 * 4)
#define OSD_KEY 0xDEADC0DE

uint8_t CMD_OSD = 2;

BrowserClient* OSRHandler::browserClient;

OSRHandler::OSRHandler(BrowserClient *bc, int width, int height) {
    browserClient = bc;
    renderWidth = width;
    renderHeight = height;

    // init shared memory
    shmid = -1;
    shmp = nullptr;

    shmid = shmget(OSD_KEY, OSD_BUF_SIZE, 0644 | IPC_CREAT | IPC_EXCL) ;

    if (errno == EEXIST) {
        shmid = shmget(OSD_KEY, OSD_BUF_SIZE, 0644);
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

        shm_mutex.lock();
        memcpy(shmp, buffer, w * h * 4);
        browserClient->SendToVdrString(CMD_OSD, "OSDU");
        browserClient->SendToVdrBuffer(&w, sizeof(w));
        browserClient->SendToVdrBuffer(&h, sizeof(h));
    }
}