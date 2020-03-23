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
#include "browser.h"
// #include "videotranscode.h"

unsigned char CMD_OSD = 2;

BrowserClient* OSRHandler::browserClient;

OSRHandler::OSRHandler(BrowserClient *bc, int width, int height) {
    browserClient = bc;
    renderWidth = width;
    renderHeight = height;
}

OSRHandler::~OSRHandler() {
}

void OSRHandler::setRenderSize(int width, int height) {
    renderWidth = width;
    renderHeight = height;
}

void OSRHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
    rect = CefRect(0, 0, renderWidth, renderHeight);
}

void OSRHandler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height) {
    int bytes;

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
}
