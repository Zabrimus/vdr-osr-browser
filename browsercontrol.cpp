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
#include "include/cef_process_message.h"
#include "include/cef_base.h"
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include "globaldefs.h"
#include "browser.h"
#include "nativejshandler.h"

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
}

void BrowserControl::LoadURL(const CefString& url) {
    CONSOLE_TRACE("Current URL: {}, New URL: {}", browser->GetMainFrame()->GetURL().ToString(), url.ToString());

    //if (browser->GetMainFrame()->GetURL().compare(url)) {
        browser->StopLoad();
        browser->GetMainFrame()->LoadURL(url);
    //} else {
    //    browser->GetHost()->Invalidate(PET_VIEW);
    //}
}

void BrowserControl::PauseRender() {
    browser->GetHost()->WasHidden(true);
}

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
    if ((fromVdrSocketId = nn_socket(AF_SP, NN_PULL)) < 0) {
        fprintf(stderr, "BrowserControl: unable to create nanomsg socket\n");
        exit(1);
    }

    if (nn_bind(fromVdrSocketId, FROM_VDR_CHANNEL) < 0) {
        fprintf(stderr, "BrowserControl: unable to bind nanomsg socket to %s\n", FROM_VDR_CHANNEL);
    }

    while (isRunning) {
        char *buf = nullptr;
        int bytes;

        if ((bytes = nn_recv(fromVdrSocketId, &buf, NN_MSG, 0)) < 0) {
            fprintf(stderr, "BrowserControl: unable to read command from socket %d, %d, %s\n", bytes, nn_errno(), nn_strerror(nn_errno()));
        }

        if (bytes > 0) {
            if (strncmp("OSDU", buf, 4) != 0 && strncmp("APPURL", buf, 6) != 0) {
                CONSOLE_TRACE("Received command from VDR: {}", buf);
            }

            if (strncmp("OSDU", buf, 4) == 0) {
                browserClient->osdProcessed();
            } else if (strncmp("SENDOSD", buf, 7) == 0) {
                browserClient->osdProcessed();
                browser->GetHost()->Invalidate(PET_VIEW);
            } else if (strncmp("URL ", buf, 4) == 0 && bytes >= 5) {
                CefString url(buf + 4);
                LoadURL(url);
            } else if (strncmp("PAUSE", buf, 5) == 0) {
                PauseRender();
            } else if (strncmp("RESUME", buf, 6) == 0) {
                ResumeRender();
            } else if (strncmp("STOP", buf, 4) == 0) {
                Stop();
            } else if (strncmp("SIZE ", buf, 5) == 0 && bytes >= 6)  {
                if (browserClient->getDisplayMode() == HBBTV_MODE) {
                    CONSOLE_INFO("Command SIZE in HbbTV mode is not possible.");

                    browserClient->setRenderSize(1280, 720);
                    browser->GetHost()->WasResized();
                } else {
                    int w, h;
                    sscanf(buf + 4, "%d %d", &w, &h);

                    browserClient->setRenderSize(std::min(w, 1920), std::min(h, 1080));
                    browser->GetHost()->WasResized();
                }
            } else if (strncmp("ZOOM ", buf, 5) == 0) {
                if (browserClient->getDisplayMode() == HBBTV_MODE) {
                    CONSOLE_INFO("Command ZOOM in HbbTV mode is not possible.");

                    browser->GetHost()->SetZoomLevel(1);
                } else {
                    double level;
                    sscanf(buf + 4, "%lf", &level);

                    // calculate cef zoom level
                    double cefLevel = log(level) / log(1.2f);
                    browser->GetHost()->SetZoomLevel(cefLevel);
                }
            } else if (strncmp("JS ", buf, 3) == 0) {
                CefString call(buf + 3);

#ifdef DEBUG_JS
                printf("%s\n\n", call.ToString().c_str());
#endif

                auto frame = browser->GetMainFrame();
                frame->ExecuteJavaScript(call, frame->GetURL(), 0);
            } else if (strncmp("PING", buf, 4) == 0) {
                browserClient->SendToVdrPing();
            } else if (strncmp("KEY ", buf, 4) == 0) {
                sendKeyEvent(buf + 4);
            } else if (strncmp("MODE ", buf, 5) == 0) {
                int mode = -1;
                sscanf(buf + 4, "%d", &mode);

                if (mode == 1) {
                    browserClient->SetHtmlMode();
                } else if (mode == 2) {
                    browserClient->SetHbbtvMode();
                }
            } else if (strncmp("PLAYER_DETACHED", buf, 15) == 0) {
                browserClient->stop_video();

                CefString call = "window.cefStopVideo();";
                auto frame = browser->GetMainFrame();
                frame->ExecuteJavaScript(call, frame->GetURL(), 0);
            } else if (strncmp("CHANNEL ", buf, 8) == 0) {
                browserClient->SetCurrentChannel(std::string(buf + 8));
                browserClient->ClearAppUrl();
            } else if (strncmp("APPURL ", buf, 7) == 0) {
                // split string to get id and url
                std::string str(buf + 7);
                auto delimiterPos = str.find(":");
                auto id = str.substr(0, delimiterPos);
                auto u = str.substr(delimiterPos + 1);
                trim(id);
                trim(u);

                AddAppUrl(id, u);
                browserClient->AddAppUrl(id, u);
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
    /*
    // Test to send the key codes directly
    bool sendEvent = false;
    int windows_key_code;
    int native_key_code;

    if (strncmp(keyCode, "VK_LEFT", 7) == 0) {
        windows_key_code = 0x25;
        native_key_code = 0x71;
        sendEvent = true;
    } else if (strncmp(keyCode, "VK_RIGHT", 8) == 0) {
        windows_key_code = 0x27;
        native_key_code = 0x72;
        sendEvent = true;
    } else if (strncmp(keyCode, "VK_UP", 5) == 0) {
        windows_key_code = 0x26;
        native_key_code = 0x6F;
        sendEvent = true;
    } else if (strncmp(keyCode, "VK_DOWN", 5) == 0) {
        windows_key_code = 0x28;
        native_key_code = 0x74;
        sendEvent = true;
    }

    if (sendEvent) {
        CefKeyEvent key_event;

        key_event.windows_key_code = windows_key_code;
        key_event.native_key_code = native_key_code;
        key_event.modifiers = 0x00;

        key_event.type = KEYEVENT_RAWKEYDOWN;
        browser->GetHost()->SendKeyEvent(key_event);

        key_event.type = KEYEVENT_CHAR;
        browser->GetHost()->SendKeyEvent(key_event);

        key_event.type = KEYEVENT_KEYUP;
        browser->GetHost()->SendKeyEvent(key_event);
    } else {
     */
        // use javascript to send key codes
        std::ostringstream stringStream;

        stringStream << "window.cefKeyPress('" << keyCode << "');";

        auto script = stringStream.str();
        auto frame = browser->GetMainFrame();
        frame->ExecuteJavaScript(script, frame->GetURL(), 0);
    /*
    }
    */
}

void BrowserControl::AddAppUrl(std::string id, std::string url) {
    // construct the javascript command
    std::ostringstream stringStream;
    stringStream << "if (window._HBBTV_APPURL_) window._HBBTV_APPURL_.set('" << id << "','" << url << "');";
    auto script = stringStream.str();
    auto frame = browser->GetMainFrame();
    frame->ExecuteJavaScript(script, frame->GetURL(), 0);
}