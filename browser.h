#ifndef BROWSER_H
#define BROWSER_H

#include <fstream>
#include <thread>
#include <mutex>
#include <string>
#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_render_handler.h"
#include "include/cef_callback.h"
#include "include/wrapper/cef_message_router.h"
#include "transcodeffmpeg.h"
#include "logger.h"

class BrowserClient;
class BrowserControl;

const int HTML_MODE = 1;
const int HBBTV_MODE = 2;

class OSRHandler : public CefRenderHandler {
private:
    int renderWidth;
    int renderHeight;

    int shmid;
    uint8_t *shmp;
    std::mutex shm_mutex;

    static BrowserClient *browserClient;

public:
    OSRHandler(BrowserClient *bc, int width, int height);
    ~OSRHandler() override;

    void setRenderSize(int width, int height);
    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) override;
    void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height) override;

    void osdProcessed() { CONSOLE_TRACE("Unlock shm_mutex"); shm_mutex.unlock(); };

    IMPLEMENT_REFCOUNTING(OSRHandler);
};

class HbbtvCurl {
public:
    static std::map<std::string, std::string> cookies;

public:
    HbbtvCurl();
    ~HbbtvCurl();

    std::string ReadContentType(std::string url, CefRequest::HeaderMap headers);
    void LoadUrl(std::string url, CefRequest::HeaderMap headers);
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
};

class BrowserClient : public CefClient,
                      public CefRequestHandler,
                      public CefLoadHandler,
                      public CefResourceHandler,
                      public CefResourceRequestHandler,
                      public CefLifeSpanHandler {

private:
    CefRefPtr<CefRenderHandler> renderHandler;
    CefRefPtr<CefMessageRouterBrowserSide> browser_side_router;
    OSRHandler* osrHandler;

    std::string exePath;
    HbbtvCurl hbbtvCurl;
    bool injectJavascript;

    JavascriptHandler *handler;

    TranscodeFFmpeg *transcoder;
    std::thread transcode_thread;

    std::string responseContent;
    std::map<std::string, std::string> responseHeader;
    int responseCode;
    size_t offset;
    std::string redirectUrl;

    std::map<std::string, std::string> mimeTypes;

    // current channel
    std::string currentChannel;

    // default HTML mode
    int mode = HTML_MODE;

    // sockets
    int toVdrSocketId;
    int toVdrEndpointId;

    bool loadingStart;

    void injectJs(CefRefPtr<CefBrowser> browser, std::string url, bool sync, bool headerStart, std::string htmlid, bool insert = false);

    inline std::string readFile(const std::string path) {
        std::ostringstream buf; std::ifstream input (path.c_str()); buf << input.rdbuf(); return buf.str();
    }

public:
    explicit BrowserClient(spdlog::level::level_enum log_level);
    ~BrowserClient() override;

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
    bool set_input_file(const char* input);
    int transcode();
    void pause_video();
    void resume_video();
    void stop_video();
    void seek_video(const char* ms);
    void speed_video(const char* speed);

    //
    void SendToVdrString(uint8_t messageType, const char* message);
    void SendToVdrVideoData(uint8_t* message, int size);
    void SendToVdrOsd(const char* message, int width, int height);

    void setRenderSize(int width, int height) { osrHandler->setRenderSize(width, height); };
    static int write_buffer_to_vdr(uint8_t *buf, int buf_size);
    static void eventCallback(std::string cmd);

    //
    void SetCurrentChannel(std::string channel) { currentChannel = channel; };
    std::string GetCurrentChannel() { return currentChannel; };

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

private:
    CefRefPtr<CefBrowser> browser;
    BrowserClient *browserClient;

    bool isRunning;

    int fromVdrSocketId;
    int fromVdrEndpointId;

    std::string url;
};


#endif // BROWSER_H
