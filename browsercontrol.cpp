/**
 *  CEF OSR implementation used für vdr-plugin-htmlskin
 *
 *  browsercontrol.cpp
 *
 *  (c) 2019 Robert Hannebauer
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#include <unistd.h>
#include <iostream>
#include <cmath>
#include "include/cef_v8.h"
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
#include "globaldefs.h"
#include "browser.h"

// #define DEBUG_JS

BrowserControl::BrowserControl(CefRefPtr<CefBrowser> _browser, BrowserClient* client) {
    browser = _browser;
    browserClient = client;

    browserClient->setBrowserControl(this);
}

BrowserControl::~BrowserControl() {
    if (fromVdrSocketId > 0) {
        nn_close(fromVdrSocketId);
    }
};

void BrowserControl::LoadURL(const CefString& url) {
    if (browser->GetMainFrame()->GetURL().compare(url)) {
        browserClient->setLoadingStart(true);
        browser->GetMainFrame()->LoadURL(url);
    }
}

void BrowserControl::PauseRender() {
    browser->GetHost()->WasHidden(true);
};

void BrowserControl::ResumeRender() {
    browser->GetHost()->WasHidden(false);
}

void BrowserControl::BrowserBack() {
    browser->GoBack();
}

void BrowserControl::BrowserStopLoad() {
    browser->StopLoad();
}

void BrowserControl::Start() {
    isRunning = true;

    // bind socket
    if ((fromVdrSocketId = nn_socket(AF_SP, NN_REP)) < 0) {
        fprintf(stderr, "BrowserControl: unable to create nanomsg socket\n");
        exit(1);
    }

    if ((fromVdrEndpointId = nn_bind(fromVdrSocketId, FROM_VDR_CHANNEL)) < 0) {
        fprintf(stderr, "BrowserControl: unable to bind nanomsg socket to %s\n", FROM_VDR_CHANNEL);
    }

    while (isRunning) {
        char *buf = nullptr;
        int bytes;

        if ((bytes = nn_recv(fromVdrSocketId, &buf, NN_MSG, 0)) < 0) {
            fprintf(stderr, "BrowserControl: unable to read command from socket %d, %d, %s\n", bytes, nn_errno(), nn_strerror(nn_errno()));
        }

        if (bytes > 0) {
            if (strncmp("URL", buf, 3) == 0 && bytes >= 5) {
                fprintf(stderr, "URL: %s\n", buf+4);

                CefString url(buf + 4);
                LoadURL(url);
            } else if (strncmp("PAUSE", buf, 5) == 0) {
                PauseRender();
            } else if (strncmp("RESUME", buf, 6) == 0) {
                ResumeRender();
            } else if (strncmp("STOP", buf, 4) == 0) {
                Stop();
            } else if (strncmp("SIZE", buf, 4) == 0 && bytes >= 6)  {
                int w, h;
                sscanf(buf + 4,"%d %d",&w, &h);

                browserClient->setRenderSize(std::min(w, 1920), std::min(h, 1080));
                browser->GetHost()->WasResized();
            } else if (strncmp("ZOOM", buf, 4) == 0) {
                double level;
                sscanf(buf + 4, "%lf", &level);

                // calculate cef zoom level
                double cefLevel = log(level) / log(1.2f);
                browser->GetHost()->SetZoomLevel(cefLevel);
            } else if (strncmp("JS", buf, 2) == 0) {
                CefString call(buf + 3);

#ifdef DEBUG_JS
                printf("%s\n\n", call.ToString().c_str());
#endif

                auto frame = browser->GetMainFrame();
                frame->ExecuteJavaScript(call, frame->GetURL(), 0);
            } else if (strncmp("PING", buf, 4) == 0) {
                // do nothing, only sends the response
            } else if (strncmp("KEY", buf, 3) == 0) {
                sendKeyEvent(buf + 4);
            } else if (strncmp("MODE", buf, 4) == 0) {
                int mode = -1;
                sscanf(buf + 4, "%d", &mode);

                if (mode == 1) {
                    browserClient->SetHtmlMode();
                } else if (mode == 2) {
                    browserClient->SetHbbtvMode();
                }
            } else if (strncmp("OSDU", buf, 4) == 0) {
                browserClient->osdProcessed();
            } else if (strncmp("TSU", buf, 3) == 0) {
                browserClient->frameProcessed();
            }
        }

        if (buf != nullptr) {
            nn_freemsg(buf);
        }
    }
}

void BrowserControl::Stop() {
    isRunning = false;
}

void BrowserControl::sendKeyEvent(const char* keyCode) {
    std::ostringstream stringStream;

    stringStream <<  "window.cefKeyPress('" << keyCode << "');";

    auto script = stringStream.str();
    auto frame = browser->GetMainFrame();
    frame->ExecuteJavaScript(script, frame->GetURL(), 0);
}