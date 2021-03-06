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

void startEncoderThread(Encoder* encoder) {
    encoder->Start();
}

OSRHandler::OSRHandler(BrowserClient *bc, int width, int height) {
    CONSOLE_TRACE("Create OSRHandler and open shared memory");

    encoder = nullptr;

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

    isVideoStarted = false;
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

    if (encoder != nullptr) {
        encoder->stopEncoder();
        delete encoder;
    }
}

void OSRHandler::osdClearVideo(int x, int y, int width, int height) {
    clearX = x;
    clearY = y;
    clearWidth = width;
    clearHeight = height;
}

bool OSRHandler::enableEncoder() {
    if (encoder != nullptr) {
        return false;
    }

    // start encoder
    encoder = new Encoder(this, "movie/streaming");
    encoderThread = new std::thread(startEncoderThread, encoder);
    // encoderThread->detach();

    isVideoStarted = false;

    return true;
}

void OSRHandler::disableEncoder() {
    isVideoStarted = false;

    if (encoder != nullptr) {
        encoder->stopEncoder();
        encoderThread->join();

        delete encoder;
        encoder = nullptr;
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
    // CONSOLE_TRACE("OnPaint called: width: {}, height: {}, dirtyRects: {}, encoderInit: {}, videoStarted: {}", width, height, dirtyRects.size(), isEncoderInitialized, isVideoStarted);

    if (isVideoStarted) {
        if (!startpts) {
            startpts = av_gettime();

            // CONSOLE_ERROR("StartPts in Video: {}", startpts);
        }

        // CONSOLE_ERROR("VideoPts: {}", av_gettime() - startpts);

        encoder->addVideoFrame(width, height, (uint8_t*)buffer, av_gettime() - startpts);

        // dont't send OSD update to VDR
        return;
    }

    if (shmp != nullptr) {
        int w = std::min(width, 1920);
        int h = std::min(height, 1080);

        // CONSOLE_TRACE("OnPaint: Try to get shm_mutex.lock");

        shm_mutex.lock();

        // CONSOLE_TRACE("OnPaint: Got shm_mutex.lock");

        memcpy(shmp, buffer, w * h * 4);

        if (clearX != 0 && clearY != 0 && clearHeight != width && clearHeight != height) {
            CONSOLE_DEBUG("clearX: {}, clearY: {}, clearWidth: {}, clearHeight: {}, width: {}, heigth: {}", clearX, clearY, clearWidth, clearHeight, width, height);

            // clear part of the OSD
            uint32_t* buf = (uint32_t*)shmp;
            for (auto i = 0; i < clearHeight; ++i) {
                for (auto j = clearX; j < clearX + clearWidth; ++j) {
                    *(buf + (clearY - 1 + i) * width + j) = 0;
                }
            }
        }

        browserClient->SendToVdrOsd("OSDU", w, h);

        // CONSOLE_TRACE("OnPaint: Send OSDU to VDR");
    } else {
        CONSOLE_CRITICAL("Shared memory does not exists!");
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

    encoder->setAudioParameters(channels, params.sample_rate);
    if (encoder->startEncoder(nullptr) != 0) {
        CONSOLE_CRITICAL("Starting the encoder failed.");
        isVideoStarted = false;
    } else {
        isVideoStarted = true;
    }
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

    // this event could be fired before stop video has been received.
    if (encoder != nullptr) {
        disableEncoder();
        browserClient->SendToVdrString(CMD_STATUS, "STOP_VIDEO");
    }
}

void OSRHandler::OnAudioStreamError(CefRefPtr<CefBrowser> browser, const CefString &message) {
    CONSOLE_DEBUG("OSRVideoHandler::OnAudioStreamError: {}", message.ToString());
}

int OSRHandler::writeVideoToShm(uint8_t *buf, int buf_size) {
    if (shmp != nullptr) {
        int i = 0;
        int maxIteration = 50;

        // wait some time, maximal maxIteration*10 milliseconds
        while (*(uint8_t*)shmp != 0 && i < maxIteration) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ++i;
        }

        if (i < maxIteration) {
            memcpy(shmp + 1, &buf_size, sizeof(int));
            memcpy(shmp + sizeof(int) + 1, buf, buf_size);

            // trigger VDR update
            *(uint8_t*)shmp = 1;
        }
    }

    return buf_size;
}