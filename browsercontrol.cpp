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
#include "globaldefs.h"
#include "browser.h"
#include "nativejshandler.h"
#include "sharedmemory.h"
#include "keycodes.h"
#include "sendvdr.h"

// #define DEBUG_JS

BrowserControl::BrowserControl(CefRefPtr<CefBrowser> _browser, BrowserClient* client) {
    browser = _browser;
    browserClient = client;

    browserClient->setBrowserControl(this);
}

BrowserControl::~BrowserControl() {
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

    while (isRunning) {
        char *buf;

        if (sharedMemory.waitForRead(VdrCommand) == -1) {
            // got currently no new command
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        buf = sharedMemory.readString(VdrCommand);

        if (buf != nullptr && strlen(buf) > 0) {
            if (strncmp("APPURL", buf, 6) != 0) {
                CONSOLE_TRACE("Received command from VDR: {}", buf);
            }

            if (strncmp("SENDOSD", buf, 7) == 0) {
                browser->GetHost()->Invalidate(PET_VIEW);
            } else if (strncmp("URL ", buf, 4) == 0 && strlen(buf) >= 5) {
                CefString url(buf + 4);
                LoadURL(url);
            } else if (strncmp("PAUSE", buf, 5) == 0) {
                PauseRender();
            } else if (strncmp("RESUME", buf, 6) == 0) {
                ResumeRender();
            } else if (strncmp("STOP", buf, 4) == 0) {
                Stop();
            } else if (strncmp("SIZE ", buf, 5) == 0 && strlen(buf) >= 6)  {
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
                SendToVdrPing();
            } else if (strncmp("KEY ", buf, 4) == 0) {
                sendKeyEvent(buf + 4);
            } else if (strncmp("VOLUME ", buf, 7) == 0) {
                int volume;
                sscanf(buf + 7, "%d", &volume);

                char *cmd;
                asprintf(&cmd, "window.cefVideoVolume(%d);", volume);
                CefString call = cmd;
                auto frame = browser->GetMainFrame();
                frame->ExecuteJavaScript(call, frame->GetURL(), 0);
                free(cmd);
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

        sharedMemory.finishedReading(VdrCommand);
    }
}

void BrowserControl::Stop() {
    isRunning = false;
}

void BrowserControl::sendKeyEvent(const char* keyCode) {
    // TODO: Ein paar der Keycodes existieren nur in echten HbbTV-Apps. Für diese KeyEvents muss ich die Javascript-Funktion
    //  aufrufen. Die normalen Tastencodes können direkt gesendet werden.
    //  Evt. muss hier noch eine Anpassung erfolgen, falls bestimmte KeyEvents nicht richtig funktionieren.

    if ((strncmp(keyCode, "VK_BACK", 7) == 0) ||
            (strncmp(keyCode, "VK_RED", 7) == 0) ||
            (strncmp(keyCode, "VK_GREEN", 7) == 0) ||
            (strncmp(keyCode, "VK_YELLOW", 7) == 0) ||
            (strncmp(keyCode, "VK_BLUE", 7) == 0) ||
            (strncmp(keyCode, "VK_PLAY", 7) == 0) ||
            (strncmp(keyCode, "VK_PAUSE", 7) == 0) ||
            (strncmp(keyCode, "VK_STOP", 7) == 0) ||
            (strncmp(keyCode, "VK_FAST_FWD", 7) == 0) ||
            (strncmp(keyCode, "VK_REWIND", 7) == 0)) {

        // send key event via Javascript
        std::ostringstream stringStream;

        stringStream << "window.cefKeyPress('" << keyCode << "');";

        auto script = stringStream.str();
        auto frame = browser->GetMainFrame();
        frame->ExecuteJavaScript(script, frame->GetURL(), 0);
    } else {
        // send key event via code
        // FIXME: Was ist mit keyCodes, die nicht existieren?
        int windowsKeyCode = keyCodes[keyCode];
        CefKeyEvent keyEvent;

        keyEvent.windows_key_code = windowsKeyCode;
        keyEvent.modifiers = 0x00;

        keyEvent.type = KEYEVENT_RAWKEYDOWN;
        browser->GetHost()->SendKeyEvent(keyEvent);

        keyEvent.type = KEYEVENT_CHAR;
        browser->GetHost()->SendKeyEvent(keyEvent);

        keyEvent.type = KEYEVENT_KEYUP;
        browser->GetHost()->SendKeyEvent(keyEvent);
    }
}

void BrowserControl::AddAppUrl(std::string id, std::string url) {
    // construct the javascript command
    std::ostringstream stringStream;
    stringStream << "if (window._HBBTV_APPURL_) window._HBBTV_APPURL_.set('" << id << "','" << url << "');";
    auto script = stringStream.str();
    auto frame = browser->GetMainFrame();
    frame->ExecuteJavaScript(script, frame->GetURL(), 0);
}