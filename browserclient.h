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
#include "include/wrapper/cef_message_router.h"

#include "osrhandler.h"

class BrowserControl;

class HbbtvCurl {
public:
    HbbtvCurl();
    ~HbbtvCurl();

    std::string ReadContentType(std::string url, CefRequest::HeaderMap headers);
    void LoadUrl(std::string url, CefRequest::HeaderMap headers);
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

class JavascriptHandler : public CefMessageRouterBrowserSide::Handler {
public:
    JavascriptHandler();
    ~JavascriptHandler();

    bool OnQuery(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         int64 query_id,
                         const CefString& request,
                         bool persistent,
                         CefRefPtr<Callback> callback) override;

    void OnQueryCanceled(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 int64 query_id) override;

    void SetBrowserControl(BrowserControl *ctl) { this->browserControl = ctl; }

private:
    int socketId;
    int endpointId;
    BrowserControl *browserControl;

};

class BrowserClient : public CefClient,
                      public CefRequestHandler,
                      public CefLoadHandler,
                      public CefResourceHandler,
                      public CefResourceRequestHandler,
                      public CefLifeSpanHandler {

private:
    CefRefPtr<CefRenderHandler> m_renderHandler;
    CefRefPtr<CefMessageRouterBrowserSide> browser_side_router;

    std::string exePath;
    HbbtvCurl hbbtvCurl;
    bool debugMode;
    bool injectJavascript;

    JavascriptHandler *handler;

    std::string responseContent;
    std::map<std::string, std::string> responseHeader;
    size_t offset;
    std::string redirectUrl;

    std::map<std::string, std::string> mimeTypes;

    // default HTML mode
    int mode = 1;

    bool loadingStart;

    void injectJs(CefRefPtr<CefBrowser> browser, std::string url, bool sync, bool headerStart, std::string htmlid);
    void injectJsModule(CefRefPtr<CefBrowser> browser, std::string url, std::string htmlid);

    inline std::string readFile(const std::string path) {
        std::ostringstream buf; std::ifstream input (path.c_str()); buf << input.rdbuf(); return buf.str();
    }

public:
    explicit BrowserClient(OSRHandler *renderHandler, bool debug);
    ~BrowserClient();

    void setLoadingStart(bool loading);
    void setBrowserControl(BrowserControl *ctl) { this->handler->SetBrowserControl(ctl); }

    // getter for the different handler
    CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }

    CefRefPtr<CefRenderHandler> GetRenderHandler() override;
    CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool is_navigation, bool is_download, const CefString& request_initiator, bool& disable_default_handling) override;
    CefRefPtr<CefResourceHandler> GetResourceHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request) override;

    // CefClient
    bool OnProcessMessageReceived(CefRefPtr< CefBrowser > browser, CefRefPtr< CefFrame > frame, CefProcessId source_process, CefRefPtr< CefProcessMessage > message) override;

    // CefRequestHandler
    CefResourceRequestHandler::ReturnValue OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefRequestCallback> callback) override;
    bool OnBeforeBrowse(CefRefPtr< CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool user_gesture, bool is_redirect ) override;
    void OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, CefRequestHandler::TerminationStatus status) override;

    // CefResourceHandler
    bool ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) override;
    void GetResponseHeaders(CefRefPtr<CefResponse> response, int64 &response_length, CefString &redirectUrl) override;
    bool ReadResponse(void *data_out, int bytes_to_read, int &bytes_read, CefRefPtr<CefCallback> callback) override;

    // bool CanGetCookie(const CefCookie &cookie) override { return CefResourceHandler::CanGetCookie(cookie); }
    // bool CanSetCookie(const CefCookie &cookie) override { return CefResourceHandler::CanSetCookie(cookie); }
    void Cancel() override { /* we do not cancel the request */ }

    // CefLoadHandler
    void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, TransitionType transition_type) override;
    void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override;
    void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl) override;

    // CefLifeSpanHandler
    void OnBeforeClose(CefRefPtr< CefBrowser > browser) override;

    //
    void initJavascriptCallback();
    void SetHtmlMode() { mode = 1; };
    void SetHbbtvMode() { mode = 2; };

IMPLEMENT_REFCOUNTING(BrowserClient);
};

#endif // BROWSERCLIENT_H
