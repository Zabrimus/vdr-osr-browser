/**
 *  CEF OSR implementation used für vdr-plugin-htmlskin
 *
 *  browsercontrol.h
 *
 *  (c) 2019 Robert Hannebauer
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#ifndef BROWSERCONTROL_H
#define BROWSERCONTROL_H

#include "include/cef_app.h"
#include "osrhandler.h"
#include "browserclient.h"

class BrowserControl {
public:
    explicit BrowserControl(CefRefPtr<CefBrowser> _browser, OSRHandler* osrHandler, BrowserClient* client);
    ~BrowserControl();

    void LoadURL(const CefString& url);

    void PauseRender();
    void ResumeRender();

    void Start(std::string socketUrl);
    void Stop();


private:
    CefRefPtr<CefBrowser> browser;
    OSRHandler* handler;
    BrowserClient *browserClient;

    bool isRunning;

    int socketId;
    int endpointId;

    std::string url;
};

#endif // BROWSERCONTROL_H
