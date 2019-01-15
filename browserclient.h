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

class BrowserClient : public CefClient,
                      public CefRequestHandler,
                      public CefLoadHandler,
                      public CefResourceHandler {

private:
    CefRefPtr<CefRenderHandler> m_renderHandler;

    bool hbbtv;

    std::string responseContent;
    std::map<std::string, std::string> responseHeader;
    size_t offset;
    std::string redirectUrl;

public:
    explicit BrowserClient(OSRHandler *renderHandler, bool _hbbtv);

    // getter for the different handler
    CefRefPtr<CefRenderHandler> GetRenderHandler() override;
    CefRefPtr<CefRequestHandler> GetRequestHandler() override;
    CefRefPtr<CefResourceHandler> GetResourceHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request) override;

    // CefRequestHandler
    ReturnValue OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefRequestCallback> callback) override;

    // CefResourceHandler
    bool ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) override;
    void GetResponseHeaders(CefRefPtr<CefResponse> response, int64 &response_length, CefString &redirectUrl) override;
    bool ReadResponse(void *data_out, int bytes_to_read, int &bytes_read, CefRefPtr<CefCallback> callback) override;

    bool CanGetCookie(const CefCookie &cookie) override { return CefResourceHandler::CanGetCookie(cookie); }
    bool CanSetCookie(const CefCookie &cookie) override { return CefResourceHandler::CanSetCookie(cookie); }
    void Cancel() override { /* we do not cancel the request */ }

    // CefLoadHandler
    void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override;
    void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl) override;

    IMPLEMENT_REFCOUNTING(BrowserClient);
};

#endif // BROWSERCLIENT_H
