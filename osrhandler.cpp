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
#define SHM_BUF_SIZE (1920 * 1080 * 4)
#define SHM_KEY 0xDEADC0DE

uint8_t CMD_OSD = 2;

BrowserClient* OSRHandler::browserClient;

OSRHandler::OSRHandler(BrowserClient *bc, int width, int height) {
    browserClient = bc;
    renderWidth = width;
    renderHeight = height;
    videoRendering = false;

    // init shared memory
    shmid = -1;
    shmp = nullptr;

    shmid = shmget(SHM_KEY, SHM_BUF_SIZE, 0644);

    if (errno == EEXIST) {
        printf("Existiert bereits....\n");
    }

    if (errno == ENOENT) {
        shmid = shmget(SHM_KEY, SHM_BUF_SIZE, 0644 | IPC_CREAT | IPC_EXCL) ;
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

#if 0
    int bytes;

    if (videoRendering) {

#ifdef DBG_MEASURE_TIME
        auto start = std::chrono::high_resolution_clock::now();
        browserClient->add_overlay_frame(width, height, (uint8_t*)buffer);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        printf("Process overlay image took %ld milliseconds, dirtyRecs = %ul, width = %d, height = %d.\n", duration.count(), dirtyRects.size(), width, height);
#else
        browserClient->add_overlay_frame(width, height, (uint8_t*)buffer);
#endif

        return;
    }

    auto img = (uint32_t*)buffer;

    // send count of dirty recs
    auto countOfDirtyRecs = dirtyRects.size();
    if (countOfDirtyRecs > 0) {
        browserClient->SendToVdrBuffer(CMD_OSD, &countOfDirtyRecs, sizeof(countOfDirtyRecs));

        // iterate overall dirty recs and send the size and buffer
        for (unsigned long i = 0; i < countOfDirtyRecs; ++i) {
            // send coordinates and size
            auto x = dirtyRects[i].x;
            auto y = dirtyRects[i].y;
            auto h = dirtyRects[i].height;
            auto w = dirtyRects[i].width;

            browserClient->SendToVdrBuffer(&x, sizeof(x));
            browserClient->SendToVdrBuffer(&y, sizeof(y));
            browserClient->SendToVdrBuffer(&w, sizeof(w));
            browserClient->SendToVdrBuffer(&h, sizeof(h));

            // send buffer
            for (auto j = y; j < y + h; ++j) {
                browserClient->SendToVdrBuffer(img + (width * j + x), 4 * w);
            }
        }
    }
#endif
}