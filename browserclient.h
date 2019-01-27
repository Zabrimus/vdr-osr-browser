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

#include <fstream>
#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_render_handler.h"

#include "osrhandler.h"

class HbbtvCurl {
public:
    HbbtvCurl();
    ~HbbtvCurl();

    std::string ReadContentType(std::string url);
    void LoadUrl(std::string url, std::map<std::string, std::string>* header);
    std::map<std::string, std::string> GetResponseHeader() { return response_header; }
    std::string GetResponseContent() { return response_content; }
    std::string GetRedirectUrl() { return redirect_url; }

private:
    static size_t WriteContentCallback(void *contents, size_t size, size_t nmemb, void *userp);
    static size_t WriteHeaderCallback(void *contents, size_t size, size_t nmemb, void *userp);

    static std::map<std::string, std::string> response_header;
    static std::string response_content;
    std::string redirect_url;
};

class BrowserClient : public CefClient,
                      public CefRequestHandler,
                      public CefLoadHandler,
                      public CefResourceHandler {

private:
    CefRefPtr<CefRenderHandler> m_renderHandler;

    std::string exePath;

    bool hbbtv;
    HbbtvCurl hbbtvCurl;

    std::string responseContent;
    std::map<std::string, std::string> responseHeader;
    size_t offset;
    std::string redirectUrl;

    std::map<std::string, std::string> mimeTypes;

    bool loadingStart;

    void injectJs(CefRefPtr<CefBrowser> browser, std::string url, bool sync, bool headerStart);

    inline std::string readFile(const std::string path) {
        std::ostringstream buf; std::ifstream input (path.c_str()); buf << input.rdbuf(); return buf.str();
    }

public:
    explicit BrowserClient(OSRHandler *renderHandler, bool _hbbtv);

    void setLoadingStart(bool loading);

    // getter for the different handler
    CefRefPtr<CefRenderHandler> GetRenderHandler() override;
    CefRefPtr<CefRequestHandler> GetRequestHandler() override;
    CefRefPtr<CefResourceHandler> GetResourceHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request) override;
    CefRefPtr<CefLoadHandler> GetLoadHandler() override;

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
    void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, TransitionType transition_type) override;
    void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override;
    void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl) override;

    IMPLEMENT_REFCOUNTING(BrowserClient);
};

#endif // BROWSERCLIENT_H
