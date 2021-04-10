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
#include "sendvdr.h"

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

OSRHandler::OSRHandler(BrowserClient *bc, int width, int height, bool showPlayer, bool fullscreen) {
    CONSOLE_TRACE("Create OSRHandler and open shared memory");

    videoPlayer = nullptr;
    this->showPlayer = showPlayer;
    this->fullscreen = fullscreen;

    if (!showPlayer) {
        // create encoder as early as possible
        encoder = new Encoder();
    }

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

    if (videoPlayer != nullptr) {
        videoPlayer->stop();
        delete videoPlayer;
        videoPlayer = nullptr;
    }
}

void OSRHandler::osdClearVideo(int x, int y, int width, int height) {
    clearX = x;
    clearY = y;
    clearWidth = width;
    clearHeight = height;
}

bool OSRHandler::enableEncoder() {
    if (showPlayer) {
        if (videoPlayer == nullptr) {
            videoPlayer = new VideoPlayer(fullscreen);

            // get VDR volume
            SendToVdrString(CMD_STATUS, "VOLUME");
        }
    } else {
        encoder->enable();
    }

    isVideoStarted = true;

    return true;
}

void OSRHandler::disableEncoder() {
    isVideoStarted = false;

    if (showPlayer) {
        if (videoPlayer != nullptr) {
            videoPlayer->stop();
            delete videoPlayer;
            videoPlayer = nullptr;
        }
    } else {
        encoder->disable();
    }
}

void OSRHandler::flushEncoder() {
    if (!showPlayer) {
        encoder->flush();
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
    // CONSOLE_TRACE("OnPaint called: width: {}, height: {}, dirtyRects: {},  videoStarted: {}", width, height, dirtyRects.size(), isVideoStarted);

    if (isVideoStarted) {
        if (!startpts) {
            startpts = av_gettime();
        }

        if (showPlayer) {
            if (!videoPlayer->addVideoFrame(width, height, (uint8_t *) buffer, av_gettime() - startpts)) {
                isVideoStarted = false;
            }
        } else if (!encoder->addVideoFrame(width, height, (uint8_t *) buffer, av_gettime() - startpts)) {
            // encoder is not running anymore
            isVideoStarted = false;
        }

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

        // delete parts of the OSD where a video shall be visible
        /*
        for (uint32_t i = 0; i < (uint32_t)(width * height); ++i) {
            if (buf[i] == 0xfffe2e9a) {
                buf[i] = 0x00000000;
            }
        }
        */

        SendToVdrOsd("OSDU", w, h);
    } else {
        // reset
        sharedMemory.setMode(shmpWriteMode, Data);
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

    if (showPlayer) {
        videoPlayer->setAudioParameters(channels, params.sample_rate);
    } else {
        encoder->setAudioParameters(channels, params.sample_rate);
    }

    // it is possible, that the page bypasses all events regarding video start.
    // Just to be sure, signal VDR that video playing shall be started
    SendToVdrString(CMD_STATUS, "PLAY_VIDEO:");
}

void OSRHandler::OnAudioStreamPacket(CefRefPtr<CefBrowser> browser, const float **data, int frames, int64 pts) {
    if (isVideoStarted) {
        if (!startpts) {
            startpts = av_gettime();

            // CONSOLE_ERROR("StartPts in Audio: {}", startpts);
        }

        // CONSOLE_ERROR("AudioPts: {}", pts * 1000 - startpts);
        if (showPlayer) {
            if (!videoPlayer->addAudioFrame(data, frames, pts * 1000 - startpts)) {
                isVideoStarted = false;
            }
        } else if (!encoder->addAudioFrame(data, frames, pts * 1000 - startpts)) {
            // encoder is not running anymore
            isVideoStarted = false;
        }
    }
}

void OSRHandler::OnAudioStreamStopped(CefRefPtr<CefBrowser> browser) {
    CONSOLE_DEBUG("OSRVideoHandler::OnAudioStreamStopped");
    // this event will also be triggered if video paused. => do nothing
}

void OSRHandler::OnAudioStreamError(CefRefPtr<CefBrowser> browser, const CefString &message) {
    CONSOLE_DEBUG("OSRVideoHandler::OnAudioStreamError: {}", message.ToString());
}