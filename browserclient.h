/**
 *  CEF OSR implementation used für vdr-plugin-htmlskin
 *
 *  browserclient.h
 *
 *  (c) 2019 Robert Hannebauer
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#ifndef BROWSERCLIENT_H
#define BROWSERCLIENT_H

#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_render_handler.h"

#include "osrhandler.h"

class BrowserClient : public CefClient, public CefLoadHandler {
private:
    CefRefPtr<CefRenderHandler> m_renderHandler;

public:
    explicit BrowserClient(OSRHandler *renderHandler);
    CefRefPtr<CefRenderHandler> GetRenderHandler() override;

    void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override;
    void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl) override;

    IMPLEMENT_REFCOUNTING(BrowserClient);
};

#endif // BROWSERCLIENT_H
