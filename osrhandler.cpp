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
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>

#include "globals.h"
#include "osrhandler.h"

unsigned char CMD_OSD = 2;
unsigned char CMD_VIDEO = 3;

bool OSRHandler::streamToFfmpeg;

OSRHandler::OSRHandler(int width, int height) {
    renderWidth = width;
    renderHeight = height;
    streamToFfmpeg = false;
    videoTranscode = new VideoTranscode();
}

OSRHandler::~OSRHandler() {
    if (videoTranscode) {
        delete videoTranscode;
    }
}

void OSRHandler::setRenderSize(int width, int height) {
    renderWidth = width;
    renderHeight = height;
}

void OSRHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
    rect = CefRect(0, 0, renderWidth, renderHeight);
}

void OSRHandler::setStreamToFfmpeg(bool flag) {
    streamToFfmpeg = flag;

    if (flag) {
        videoReadThread = new std::thread(readEncodedVideo, videoTranscode);
        if (!videoTranscode->startStreaming()) {
            streamToFfmpeg = false;
            return;
        }
    } else {
        videoTranscode->stopStreaming();
        videoReadThread->join();
    }
}

void OSRHandler::readEncodedVideo(VideoTranscode *transcoder) {
    char* buffer[18800];

    while (OSRHandler::getStreamToFfmpeg()) {
        int size = transcoder->readEncoded(buffer, 18800);

        if (size < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        } else {
            // write to VDR
            nn_send(Globals::GetToVdrSocket(), &CMD_VIDEO, 1, 0);
            nn_send(Globals::GetToVdrSocket(), &buffer, size, 0);
        }
    }
}

void OSRHandler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height) {
    int bytes;

    auto img = (uint32_t*)buffer;

    if (streamToFfmpeg) {
        // send everything to FFmpeg
        videoTranscode->writeVideo(buffer, width, height);
    } else {
        // send everything to VDR

        // send count of dirty recs
        auto countOfDirtyRecs = dirtyRects.size();
        if (countOfDirtyRecs > 0) {
            nn_send(Globals::GetToVdrSocket(), &CMD_OSD, 1, 0);
            nn_send(Globals::GetToVdrSocket(), &countOfDirtyRecs, sizeof(countOfDirtyRecs), 0);

            // iterate overall dirty recs and send the size and buffer
            for (unsigned long i = 0; i < countOfDirtyRecs; ++i) {
                // send coordinates and size
                auto x = dirtyRects[i].x;
                auto y = dirtyRects[i].y;
                auto h = dirtyRects[i].height;
                auto w = dirtyRects[i].width;

                bytes = nn_send(Globals::GetToVdrSocket(), &x, sizeof(x), 0);
                bytes = nn_send(Globals::GetToVdrSocket(), &y, sizeof(y), 0);
                bytes = nn_send(Globals::GetToVdrSocket(), &w, sizeof(w), 0);
                bytes = nn_send(Globals::GetToVdrSocket(), &h, sizeof(h), 0);

                // send buffer
                for (auto j = y; j < y + h; ++j) {
                    bytes = nn_send(Globals::GetToVdrSocket(), img + (width * j + x), 4 * w, 0);
                }
            }
        }
    }
}
