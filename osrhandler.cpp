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

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>

#include "osrhandler.h"

OSRHandler::OSRHandler(int width, int height) {
    renderWidth = width;
    renderHeight = height;

    auto streamUrl = std::string("ipc:///tmp/vdrosr_stream.ipc");

    // bind socket
    if ((socketId = nn_socket(AF_SP, NN_PUSH)) < 0) {
        fprintf(stderr, "unable to create nanomsg socket\n");
    }

    if ((endpointId = nn_bind(socketId, streamUrl.c_str())) < 0) {
        fprintf(stderr, "unable to bind nanomsg socket to %s\n", streamUrl.c_str());
    }
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
    bytes = nn_send (socketId, &countOfDirtyRecs, sizeof(countOfDirtyRecs), 0);

    // iterate overall dirty recs and send the size and buffer
    for (unsigned long i = 0; i < countOfDirtyRecs; ++i) {
        // send coordinates and size
        auto x = dirtyRects[i].x;
        auto y = dirtyRects[i].y;
        auto h = dirtyRects[i].height;
        auto w = dirtyRects[i].width;

        bytes = nn_send (socketId, &x, sizeof(x), 0);
        bytes = nn_send (socketId, &y, sizeof(y), 0);
        bytes = nn_send (socketId, &w, sizeof(w), 0);
        bytes = nn_send (socketId, &h, sizeof(h), 0);

        // send buffer
        for (auto j = 0; j < h; ++j) {
            bytes = nn_send(socketId, img + (width * j + x), 4 * w, 0);
        }
    }
}
