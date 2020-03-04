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
#include <X11/keysym.h>
#include "include/cef_app.h"
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>

#include "browsercontrol.h"

// #define DEBUG_JS

BrowserControl::BrowserControl(CefRefPtr<CefBrowser> _browser, OSRHandler* osrHandler, BrowserClient* client) {
    browser = _browser;
    handler = osrHandler;
    browserClient = client;

    socketId = -1;
    endpointId = -1;
}

BrowserControl::~BrowserControl() = default;

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

void BrowserControl::Start(std::string socketUrl) {
    // bind socket
    if ((socketId = nn_socket(AF_SP, NN_REP)) < 0) {
        fprintf(stderr, "unable to create nanomsg socket\n");
        exit(1);
    }

    if ((endpointId = nn_bind(socketId, socketUrl.c_str())) < 0) {
        fprintf(stderr, "unable to bind nanomsg socket to %s\n", socketUrl.c_str());
        exit(2);
    }

    isRunning = true;

    while (isRunning) {
        char *buf = nullptr;
        int bytes;

        if ((bytes = nn_recv(socketId, &buf, NN_MSG, 0)) < 0) {
            fprintf(stderr, "unable to read command from socket\n");
        }

        if (bytes > 0) {
            bool successful = true;

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

                handler->setRenderSize(w, h);
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
                sendKeyEvent(buf + 3);
            } else if (strncmp("MODE", buf, 4) == 0) {
                int mode = -1;
                sscanf(buf + 4, "%d", &mode);

                if (mode == 1) {
                    browserClient->SetHtmlMode();
                } else if (mode == 2) {
                    browserClient->SetHbbtvMode();
                }
            } else {
                successful = false;
            }

            char *buffer;
            if (successful) {
                asprintf(&buffer, "ok");
                if ((bytes = nn_send(socketId, buffer, strlen(buffer) + 1, 0)) < 0) {
                    fprintf(stderr, "unable to send response\n");
                }
            } else {
                asprintf(&buffer, "unknown command '%s'\n", buf);
                if ((bytes = nn_send(socketId, buffer, strlen(buffer) + 1, 0)) < 0) {
                    fprintf(stderr, "unable to send response\n");
                }
            }
        }

        nn_freemsg(buf);
    }
}

void BrowserControl::Stop() {
    isRunning = false;
}

void BrowserControl::sendKeyEvent(const char* keyCode) {
    std::ostringstream stringStream;

    stringStream <<  "document.dispatchEvent(new KeyboardEvent('keydown',{'keyCode':window.";
    stringStream <<  keyCode;
    stringStream << "}))";

    auto script = stringStream.str();
    auto frame = browser->GetMainFrame();
    frame->ExecuteJavaScript(script, frame->GetURL(), 0);
}