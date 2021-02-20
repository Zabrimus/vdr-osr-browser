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

    encoder = new Encoder("output.ts", true);
    encoder->startEncoder(nullptr);
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

    // FIXME: Video events müssen erkannt werden. Start/Stop und damit, ob der Encoder überhaupt benötigt wird.
    //        Im Moment ist er zum Test immer eingeschaltet.
    encoder->stopEncoder();

    if (encoder != nullptr) {
        encoder->stopEncoder();
        delete encoder;
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
    // CONSOLE_TRACE("OnPaint called: width: {}, height: {}, dirtyRects: {}", width, height, dirtyRects.size());

    /* FIXME: Den Encoder permanent laufen zu lassen ist keine gute Idee.
              Start und Stop von Videos müssen erkannt werden und der Encoder
              entsprechend gesteuert werden.
     */
    if (!startpts) {
        startpts = av_gettime();
    }

    encoder->addVideoFrame(1280, 720, (uint8_t *) buffer, av_gettime() - startpts);


    // hex = 0xAARRGGBB.
    // rgb(254, 46, 154) = #fe2e9a
    // fffe2e9a => 00fe2e9a

    if (shmp != nullptr) {
        int w = std::min(width, 1920);
        int h = std::min(height, 1080);

        // CONSOLE_TRACE("OnPaint: Try to get shm_mutex.lock");

        shm_mutex.lock();

        // CONSOLE_TRACE("OnPaint: Got shm_mutex.lock");

        memcpy(shmp, buffer, w * h * 4);

        // delete parts of the OSD where a video shall be visible
        uint32_t* buf = (uint32_t*)shmp;
        for (uint32_t i = 0; i < (uint32_t)(width * height); ++i) {
            if (buf[i] == 0xfffe2e9a) {
                buf[i] = 0x00fe2e9a;
            }
        }

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

bool OSRHandler::GetAudioParameters(CefRefPtr<CefBrowser> browser, CefAudioParameters &params) {
    auto ret = CefAudioHandler::GetAudioParameters(browser, params);

    // 48k default sample rate seems to better work with different input videos
    params.sample_rate = 48000;

    return ret;
}

void OSRHandler::OnAudioStreamStarted(CefRefPtr<CefBrowser> browser, const CefAudioParameters &params, int channels) {
    CONSOLE_TRACE("OSRVideoHandler::OnAudioStreamStarted: Sample rate: {}, Channel layout: {}, Frames per buffer: {}, Channels: {} ", params.sample_rate, params.channel_layout, params.frames_per_buffer, channels);
    encoder->setAudioParameters(channels, params.sample_rate);
}

void OSRHandler::OnAudioStreamPacket(CefRefPtr<CefBrowser> browser, const float **data, int frames, int64 pts) {
    if (!startpts) {
        startpts = av_gettime();
    }

    encoder->addAudioFrame(data, frames, pts * 1000 - startpts);
}

void OSRHandler::OnAudioStreamStopped(CefRefPtr<CefBrowser> browser) {
    CONSOLE_DEBUG("OSRVideoHandler::OnAudioStreamStopped");
}

void OSRHandler::OnAudioStreamError(CefRefPtr<CefBrowser> browser, const CefString &message) {
    CONSOLE_DEBUG("OSRVideoHandler::OnAudioStreamError: {}", message.ToString());
}

