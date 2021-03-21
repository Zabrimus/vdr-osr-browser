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
#include <csignal>
#include <algorithm>
#include <sys/shm.h>
#include <unistd.h>
#include <mutex>
#include "browser.h"
#include "encoder.h"
#include "sharedmemory.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/time.h>
#ifdef __cplusplus
}
#endif

// #define DBG_MEASURE_TIME 1

// FULL HD
#define OSD_BUF_SIZE (1920 * 1080 * 4)
#define OSD_KEY 0xDEADC0DE

uint64_t startpts = 0;

BrowserClient* OSRHandler::browserClient;

OSRHandler::OSRHandler(BrowserClient *bc, int width, int height) {
    CONSOLE_TRACE("Create OSRHandler and open shared memory");

    // create encoder as early as possible
    encoder = new Encoder();;

    browserClient = bc;
    renderWidth = width;
    renderHeight = height;

    isVideoStarted = false;
}

OSRHandler::~OSRHandler() {
    if (encoder != nullptr) {
        encoder->disable();
        delete encoder;
        encoder = nullptr;
    }
}

void OSRHandler::osdClearVideo(int x, int y, int width, int height) {
    clearX = x;
    clearY = y;
    clearWidth = width;
    clearHeight = height;
}

bool OSRHandler::enableEncoder() {
    encoder->enable();
    isVideoStarted = true;

    return true;
}

void OSRHandler::disableEncoder() {
    isVideoStarted = false;
    encoder->disable();
}

void OSRHandler::flushEncoder() {
    encoder->flush();
}

void OSRHandler::setRenderSize(int width, int height) {
    renderWidth = width;
    renderHeight = height;
}

void OSRHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
    rect = CefRect(0, 0, renderWidth, renderHeight);
}

void OSRHandler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height) {
    // CONSOLE_TRACE("OnPaint called: width: {}, height: {}, dirtyRects: {},  videoStarted: {}", width, height, dirtyRects.size(), isVideoStarted);

    if (isVideoStarted) {
        if (!startpts) {
            startpts = av_gettime();
        }

        encoder->addVideoFrame(width, height, (uint8_t*)buffer, av_gettime() - startpts);

        // dont't send OSD update to VDR
        return;
    }

    int w = std::min(width, 1920);
    int h = std::min(height, 1080);

    if (sharedMemory.waitForWrite(Data) != -1) {
        uint32_t* buf = reinterpret_cast<uint32_t *>(sharedMemory.write((uint8_t *) buffer, w * h * 4, Data) + sizeof(int));

        if (clearX != 0 && clearY != 0 && clearHeight != width && clearHeight != height) {
            // clear part of the OSD
            for (auto i = 0; i < clearHeight; ++i) {
                for (auto j = clearX; j < clearX + clearWidth; ++j) {
                    *(buf + (clearY - 1 + i) * width + j) = 0;
                }
            }
        }

        browserClient->SendToVdrOsd("OSDU", w, h);
    } else {
        CONSOLE_ERROR("Unable to write OSD data to shared memory: {}", sharedMemory.getMode(Data));
    }
}

bool OSRHandler::GetAudioParameters(CefRefPtr<CefBrowser> browser, CefAudioParameters &params) {
    auto ret = CefAudioHandler::GetAudioParameters(browser, params);

    // 48k default sample rate seems to better work with different input videos
    params.sample_rate = 48000;

    return ret;
}

void OSRHandler::OnAudioStreamStarted(CefRefPtr<CefBrowser> browser, const CefAudioParameters &params, int channels) {
    CONSOLE_TRACE("OSRVideoHandler::OnAudioStreamStarted: Sample rate: {}, Channel layout: {}, Frames per buffer: {}, Channels: {} ", params.sample_rate, params.channel_layout, params.frames_per_buffer, channels);

    enableEncoder();

    encoder->setAudioParameters(channels, params.sample_rate);

    // it is possible, that the page bypasses all events regarding video start.
    // Just to be sure, signal VDR that video playing shall be started
    browserClient->SendToVdrString(CMD_STATUS, "PLAY_VIDEO:");
}

void OSRHandler::OnAudioStreamPacket(CefRefPtr<CefBrowser> browser, const float **data, int frames, int64 pts) {
    if (isVideoStarted) {
        if (!startpts) {
            startpts = av_gettime();

            // CONSOLE_ERROR("StartPts in Audio: {}", startpts);
        }

        // CONSOLE_ERROR("AudioPts: {}", pts * 1000 - startpts);
        encoder->addAudioFrame(data, frames, pts * 1000 - startpts);
    }
}

void OSRHandler::OnAudioStreamStopped(CefRefPtr<CefBrowser> browser) {
    CONSOLE_DEBUG("OSRVideoHandler::OnAudioStreamStopped");
    // this event will also be triggered if video paused. => do nothing
}

void OSRHandler::OnAudioStreamError(CefRefPtr<CefBrowser> browser, const CefString &message) {
    CONSOLE_DEBUG("OSRVideoHandler::OnAudioStreamError: {}", message.ToString());
}