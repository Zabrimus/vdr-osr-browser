#ifndef VDR_OSR_BROWSER_BROWSER_H
#define VDR_OSR_BROWSER_BROWSER_H

#include <fstream>
#include <thread>
#include <mutex>
#include <string>
#include <stack>
#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_callback.h"
#include "include/wrapper/cef_message_router.h"
#include "logger.h"
#include "osrhandler.h"
#include "globaldefs.h"

class BrowserClient;
class BrowserControl;

const uint8_t CMD_STATUS = 1;
const uint8_t CMD_OSD = 2;
const uint8_t CMD_VIDEO = 3;
const uint8_t CMD_PING = 5;

const int HTML_MODE = 1;
const int HBBTV_MODE = 2;

class HbbtvCurl {
public:
    HbbtvCurl();
    ~HbbtvCurl();

    std::string ReadContentType(std::string url, CefRequest::HeaderMap headers);
    void LoadUrl(std::string url, CefRequest::HeaderMap headers, long followLocation);
    static std::map<std::string, std::string> GetResponseHeader() { return response_header; }
    static std::string GetResponseContent() { return response_content; }
    std::string GetRedirectUrl() { return redirect_url; }
    long GetResponseCode() { return response_code; }

private:
    static size_t WriteContentCallback(void *contents, size_t size, size_t nmemb, void *userp);
    static size_t WriteHeaderCallback(void *contents, size_t size, size_t nmemb, void *userp);

    static std::map<std::string, std::string> response_header;
    static std::string response_content;
    long response_code;
    std::string redirect_url;
};

class JavascriptHandler : public CefMessageRouterBrowserSide::Handler {
public:
    JavascriptHandler();
    ~JavascriptHandler() override;

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
    void SetBrowserClient(BrowserClient *ctl) { this->browserClient = ctl; }

private:
    BrowserControl *browserControl;
    BrowserClient  *browserClient;

    // Application URL stack
    std::stack <std::string> applicationStack;
};

class BrowserClient : public CefClient,
                      public CefRequestHandler,
                      public CefLoadHandler,
                      public CefResourceHandler,
                      public CefResourceRequestHandler,
                      public CefLifeSpanHandler {

private:
    CefRefPtr<CefRenderHandler> videoRenderHandler;
    CefRefPtr<CefMessageRouterBrowserSide> browser_side_router;

    OSRHandler* osrHandler;

    std::string exePath;
    HbbtvCurl hbbtvCurl;
    bool injectJavascript;

    JavascriptHandler *handler;

    std::string responseContent;
    std::map<std::string, std::string> responseHeader;
    int responseCode;
    size_t offset;
    std::string redirectUrl;

    std::map<std::string, std::string> mimeTypes;

    // current channel
    std::string currentChannel;

    // all app ids and corresponding urls
    std::map<std::string, std::string> appUrls;

    // default HTML mode
    int mode = HTML_MODE;

    // sockets
    int toVdrSocketId;

    // heatbeat thread
    std::thread heartbeat_thread;
    bool heartbeat_running;
    void heartbeat();

    void injectJs(CefRefPtr<CefBrowser> browser, std::string url, bool sync, bool headerStart, std::string htmlid, bool insert = false);

    inline std::string readFile(const std::string path) {
        std::ostringstream buf; std::ifstream input (path.c_str()); buf << input.rdbuf(); return buf.str();
    }

public:
    explicit BrowserClient(spdlog::level::level_enum log_level);
    ~BrowserClient() override;

    void setBrowserControl(BrowserControl *ctl) { this->handler->SetBrowserControl(ctl); }

    // getter for the different handler
    CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }

    CefRefPtr<CefRenderHandler> GetRenderHandler() override;
    CefRefPtr<CefAudioHandler> GetAudioHandler() override;
    CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool is_navigation, bool is_download, const CefString& request_initiator, bool& disable_default_handling) override;
    CefRefPtr<CefResourceHandler> GetResourceHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request) override;

    // CefClient
    bool OnProcessMessageReceived(CefRefPtr< CefBrowser > browser, CefRefPtr< CefFrame > frame, CefProcessId source_process, CefRefPtr< CefProcessMessage > message) override;
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override;

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
    void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward);

    // CefLifeSpanHandler
    void OnBeforeClose(CefRefPtr< CefBrowser > browser) override;

    //
    void initJavascriptCallback();
    void SetHtmlMode() { mode = HTML_MODE; };
    void SetHbbtvMode() { mode = HBBTV_MODE; };
    int  getDisplayMode() { return mode; };
    void osdProcessed() { if (osrHandler != nullptr) osrHandler->osdProcessed(); };

    // transcode functions
    void start_video();
    void stop_video();
    void setVideoStarted();

    //
    void SendToVdrString(uint8_t messageType, const char* message);
    void SendToVdrOsd(const char* message, int width, int height);
    void SendToVdrPing();

    void setRenderSize(int width, int height) { osrHandler->setRenderSize(width, height); };
    static void eventCallback(std::string cmd);

    //
    void SetCurrentChannel(std::string channel) { currentChannel = channel; };
    std::string GetCurrentChannel() { return currentChannel; };

    //
    void AddAppUrl(std::string id, std::string url);
    std::map<std::string, std::string> GetAppUrls() { return appUrls; };
    void ClearAppUrl() { appUrls.clear(); };

    IMPLEMENT_REFCOUNTING(BrowserClient);
};

class BrowserControl {
public:
    explicit BrowserControl(CefRefPtr<CefBrowser> _browser, BrowserClient* client);
    ~BrowserControl();

    void LoadURL(const CefString& url);

    void PauseRender();
    void ResumeRender();

    void Start();
    void Stop();

    void BrowserBack();
    void BrowserStopLoad();

    void sendKeyEvent(const char* keyCode);
    void AddAppUrl(std::string id, std::string url);

private:
    CefRefPtr<CefBrowser> browser;
    BrowserClient *browserClient;

    bool isRunning;

    int fromVdrSocketId;

    std::string url;
};

class BrowserPaintUpdater {
    public:
        explicit BrowserPaintUpdater(CefRefPtr<CefBrowser> _browser);
        ~BrowserPaintUpdater();

    private:
        CefRefPtr<CefBrowser> browser;
        bool isRunning;

    public:
        void Start();
        void Stop();
};

#endif // VDR_OSR_BROWSER_BROWSER_H
