/**
 *  CEF OSR implementation used für vdr-plugin-htmlskin
 *
 *  browserclient.cpp
 *
 *  (c) 2019 Robert Hannebauer
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#include <iostream>
#include <map>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <curl/curl.h>
#include "browserclient.h"
#include "include/wrapper/cef_helpers.h"
#include "globals.h"

// to enable much more debug data output to stderr, set this variable to true
static bool DumpDebugData = true;
#define DBG(a...) if (DumpDebugData) fprintf(stderr, a)

#define HEADERSIZE (4 * 1024)

struct MemoryStruct {
    char *memory;
    size_t size;
};

std::map<std::string, std::string> HbbtvCurl::response_header;
std::string HbbtvCurl::response_content;

HbbtvCurl::HbbtvCurl() {
    curl_global_init(CURL_GLOBAL_ALL);
}

HbbtvCurl::~HbbtvCurl() {
    curl_global_cleanup();
}

std::string HbbtvCurl::ReadContentType(std::string url) {
    char *headerdata = (char*)malloc(HEADERSIZE);
    memset(headerdata, 0, HEADERSIZE);

    CURL *curl_handle;
    CURLcode res;

    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, USER_AGENT);

    curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteHeaderCallback);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, headerdata);

    res = curl_easy_perform(curl_handle);

    if (res == CURLE_OK) {
        char *redir_url = nullptr;
        curl_easy_getinfo(curl_handle, CURLINFO_REDIRECT_URL, &redir_url);
        if (redir_url) {
            // clear buffer
            memset(headerdata, 0, HEADERSIZE);

            response_header.clear();

            // reload url
            redirect_url = redir_url;
            curl_easy_setopt(curl_handle, CURLOPT_URL, redir_url);
            curl_easy_perform(curl_handle);
        }
    }

    curl_easy_cleanup(curl_handle);

    std::string contentType = response_header["Content-Type"];

    response_header.clear();
    free(headerdata);

    return contentType;
}

void HbbtvCurl::LoadUrl(std::string url, std::map<std::string, std::string>* header) {
    struct MemoryStruct contentdata {
            (char*)malloc(1), 0
    };

    char *headerdata = (char*)malloc(HEADERSIZE);
    memset(headerdata, 0, HEADERSIZE);

    CURL *curl_handle;
    CURLcode res;

    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, USER_AGENT);
    // curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteContentCallback);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteHeaderCallback);

    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, headerdata);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &contentdata);

    res = curl_easy_perform(curl_handle);

    if (res == CURLE_OK) {
        char *redir_url = nullptr;
        curl_easy_getinfo(curl_handle, CURLINFO_REDIRECT_URL, &redir_url);
        if (redir_url) {
            // clear buffer
            memset(headerdata, 0, HEADERSIZE);

            free (contentdata.memory);
            contentdata.size = 0;
            contentdata.memory = (char*)malloc(1);

            response_header.clear();

            // reload url
            redirect_url = redir_url;
            curl_easy_setopt(curl_handle, CURLOPT_URL, redir_url);
            curl_easy_perform(curl_handle);
        }

        DBG("Size %ld\n", contentdata.size);

        response_content =  std::string(contentdata.memory, contentdata.size);
    }

    // response_content =  std::string(contentdata.memory, contentdata.size);
    // DBG("Read content:\n %s\n", response_content.c_str());

    curl_easy_cleanup(curl_handle);

    free(headerdata);
    free(contentdata.memory);
}

size_t HbbtvCurl::WriteContentCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    auto *mem = (struct MemoryStruct*)userp;

    char *ptr = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == nullptr) {
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

size_t HbbtvCurl::WriteHeaderCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;

    memset(userp, 0, HEADERSIZE);
    memcpy(userp, contents, realsize-2);

    std::string h((char*)userp);

    std::string key, val;
    std::istringstream iss(h);
    std::getline(std::getline(iss, key, ':') >> std::ws, val);

    if (key.length() > 0 && val.length() > 0) {
        response_header.insert(std::pair<std::string, std::string>(key, val));
    }

    return realsize;
}

JavascriptHandler::JavascriptHandler() {
    auto streamUrl = std::string(VDR_STATUS_CHANNEL);

    // bind socket
    if ((socketId = nn_socket(AF_SP, NN_PUSH)) < 0) {
        fprintf(stderr, "unable to create nanomsg socket\n");
    }

    if ((endpointId = nn_bind(socketId, streamUrl.c_str())) < 0) {
        fprintf(stderr, "unable to bind nanomsg socket to %s\n", streamUrl.c_str());
    }
}

JavascriptHandler::~JavascriptHandler() {
    nn_close(socketId);
}

bool JavascriptHandler::OnQuery(CefRefPtr<CefBrowser> browser,
             CefRefPtr<CefFrame> frame,
             int64 query_id,
             const CefString& request,
             bool persistent,
             CefRefPtr<Callback> callback) {

    // process the javascript callback
    DBG("Javascript called me: %s\n", request.ToString().c_str());

    if (strncmp(request.ToString().c_str(), "VDR:", 4) == 0) {
        auto bytes = nn_send(socketId, request.ToString().c_str() + 4, strlen(request.ToString().c_str()) - 3, 0);
        return true;
    } else {
        if (request == "HelloCefQuery") {
            callback->Success("HelloCefQuery Ok");
            return true;
        } else if (request == "GiveMeMoney") {
            callback->Failure(404, "There are none thus query!");
            return true;
        }
    }

    return false; // give other handler chance
}

void JavascriptHandler::OnQueryCanceled(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64 query_id) {
    // TODO: cancel our async query task...
}

BrowserClient::BrowserClient(OSRHandler *renderHandler, bool debug) {
    m_renderHandler = renderHandler;
    debugMode = debug;

    mimeTypes.insert(std::pair<std::string, std::string>("hbbtv", "application/vnd.hbbtv.xhtml+xml"));
    mimeTypes.insert(std::pair<std::string, std::string>("cehtml", "application/ce-html+xml"));
    mimeTypes.insert(std::pair<std::string, std::string>("ohtv", "application/vnd.ohtv"));
    mimeTypes.insert(std::pair<std::string, std::string>("bml", "text/X-arib-bml"));
    mimeTypes.insert(std::pair<std::string, std::string>("atsc", "atsc-http-attributes"));
    // mimeTypes.insert(std::pair<std::string, std::string>("mheg", "application/x-mheg-5"));
    // mimeTypes.insert(std::pair<std::string, std::string>("aitx", "application/vnd.dvb.ait"));

    {
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        auto exe = std::string(result, static_cast<unsigned long>((count > 0) ? count : 0));
        exePath = exe.substr(0, exe.find_last_of("/"));
    }
}

BrowserClient::~BrowserClient() {
    browser_side_router->RemoveHandler(handler);
    delete handler;
}

// getter for the different handler
CefRefPtr<CefRenderHandler> BrowserClient::GetRenderHandler() {
    return m_renderHandler;
}

CefRefPtr<CefResourceRequestHandler> BrowserClient::GetResourceRequestHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool is_navigation, bool is_download, const CefString &request_initiator, bool &disable_default_handling) {

    auto url = request->GetURL().ToString();

    // test at first for internal requests
    if (url.find("https://local_js/") != std::string::npos || url.find("https://local_css/") != std::string::npos) {
        return this;
    }

    if (!is_navigation) {
        return CefRequestHandler::GetResourceRequestHandler(browser, frame, request, is_navigation, is_download, request_initiator, disable_default_handling);
    }

    return this;
}

CefRefPtr<CefResourceHandler> BrowserClient::GetResourceHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request) {
    if (mode == 1) {
        // HTML -> Default handler
        return CefResourceRequestHandler::GetResourceHandler(browser, frame, request);
    } else if (mode == 2) {
        // HbbTV -> Possibly special handler otherwise default handler

        // test if the special handler is necesary
        auto url = request->GetURL().ToString();

        // test at first internal requests
        if (url.find("https://local_js/") != std::string::npos || url.find("https://local_css/") != std::string::npos) {
            return this;
        }

        // filter http(s) requests
        if (url.find("http", 0) == std::string::npos) {
            // no http request -> use default handler
            return CefResourceRequestHandler::GetResourceHandler(browser, frame, request);
        }

        // filter known file types, this has to adapted accordingly
        auto handle = (url.find("view-source:'", 0) == std::string::npos) &&
                      (url.find(".json", 0) == std::string::npos) &&
                      (url.find(".js", 0) == std::string::npos) &&
                      (url.find(".css", 0) == std::string::npos) &&
                      (url.find(".ico", 0) == std::string::npos) &&
                      (url.find(".jpg", 0) == std::string::npos) &&
                      (url.find(".png", 0) == std::string::npos) &&
                      (url.find(".gif", 0) == std::string::npos) &&
                      (url.find(".webp", 0) == std::string::npos) &&
                      (url.find(".m3u8", 0) == std::string::npos) &&
                      (url.find(".mpd", 0) == std::string::npos) &&
                      (url.find(".ts", 0) == std::string::npos) &&
                      (url.find(".mpg", 0) == std::string::npos) &&
                      (url.find(".mp3", 0) == std::string::npos) &&
                      (url.find(".mp4", 0) == std::string::npos) &&
                      (url.find(".mov", 0) == std::string::npos) &&
                      (url.find(".avi", 0) == std::string::npos) &&
                      (url.find(".pdf", 0) == std::string::npos) &&
                      (url.find(".ppt", 0) == std::string::npos) &&
                      (url.find(".pptx", 0) == std::string::npos) &&
                      (url.find(".xls", 0) == std::string::npos) &&
                      (url.find(".xlsx", 0) == std::string::npos) &&
                      (url.find(".doc", 0) == std::string::npos) &&
                      (url.find(".docx", 0) == std::string::npos) &&
                      (url.find(".zip", 0) == std::string::npos) &&
                      (url.find(".rar", 0) == std::string::npos) &&
                      (url.find(".woff2", 0) == std::string::npos) &&
                      (url.find(".svg", 0) == std::string::npos) &&
                      (url.find(".ogg", 0) == std::string::npos) &&
                      (url.find(".ogm", 0) == std::string::npos) &&
                      (url.find(".ttf", 0) == std::string::npos) &&
                      (url.find(".7z", 0) == std::string::npos);

        if (!handle) {
            // use default handler
            return CefResourceRequestHandler::GetResourceHandler(browser, frame, request);
        } else {
            // read the content type
            std::string ct = hbbtvCurl.ReadContentType(url);

            bool isHbbtvHtml = false;
            for (auto const &item : mimeTypes) {
                if (ct.find(item.second) != std::string::npos) {
                    isHbbtvHtml = true;
                    break;
                }
            }

            if (isHbbtvHtml) {
                // use special handler
                return this;
            } else {
                // use default handler
                return CefResourceRequestHandler::GetResourceHandler(browser, frame, request);
            }
        }
    } else {
        // unknown mode
        return CefResourceRequestHandler::GetResourceHandler(browser, frame, request);
    }
}


// CefLoadHandler
void BrowserClient::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::TransitionType transition_type) {
    CefLoadHandler::OnLoadStart(browser, frame, transition_type);
}

void BrowserClient::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) {
    fprintf(stderr, "ON LOAD END\n");

    CEF_REQUIRE_UI_THREAD();
    loadingStart = false;

    if (mode == 2) {
        // inject Javascript
        if (debugMode) {
            injectJs(browser, "https://local_js/hbbtv_polyfill_debug", true, false);
        } else {
            injectJs(browser, "https://local_js/hbbtv_polyfill", true, false);
        }

        // set zoom level: 150% means Full HD 1920 x 1080 px
        auto frame = browser->GetMainFrame();
        frame->ExecuteJavaScript("document.body.style.setProperty('zoom', '150%');", frame->GetURL(), 0);

        // TEST
        // frame->ExecuteJavaScript("var request_id = window.cefQuery({ request: 'HelloCefQuery', persistent: false, onSuccess: function(response) {}, onFailure: function(error_code, error_message) {} }); window.cefQueryCancel(request_id);", frame->GetURL(), 0);
        // frame->ExecuteJavaScript("var request_id = window.cefQuery({ request: 'GiveMeMoney', persistent: false, onSuccess: function(response) {}, onFailure: function(error_code, error_message) {} }); window.cefQueryCancel(request_id);", frame->GetURL(), 0);
        // TEST
    }
}

void BrowserClient::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl) {
    CEF_REQUIRE_UI_THREAD();

    if (errorCode == ERR_ABORTED)
        return;
}

// CefResourceHandler
bool BrowserClient::ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    fprintf(stderr, "PROCESS REQUEST %s\n", request->GetURL().ToString().c_str());

    {
        auto js = std::string("https://local_js/");
        auto css = std::string("https://local_css/");

        // distinguish between local files and remote files using cUrl
        auto url = request->GetURL().ToString();
        bool localJs = url.find(js) != std::string::npos;
        bool localCss = url.find(css) != std::string::npos;

        if (localJs || localCss) {
            char result[ PATH_MAX ];
            ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
            auto exe = std::string(result, static_cast<unsigned long>((count > 0) ? count : 0));

        }

        if (localJs) {
            auto file = url.substr(js.length());
            auto _tmp = exePath;
            auto url = _tmp.append("/js/").append(file).append(".js");
            responseContent = readFile(url);

            responseHeader.clear();
            responseHeader.insert(std::pair<std::string, std::string>("Content-Type", "application/javascript"));
            responseHeader.insert(std::pair<std::string, std::string>("Status", "200 OK"));
        } else if (localCss) {
            auto file = url.substr(css.length(), url.length());
            auto _tmp = exePath;
            url = _tmp.append("/css/").append(file).append(".css");

            responseContent = readFile(url);

            responseHeader.clear();
            responseHeader.insert(std::pair<std::string, std::string>("Content-Type", "text/css"));
            responseHeader.insert(std::pair<std::string, std::string>("Status", "200 OK"));
        } else {
            hbbtvCurl.LoadUrl(url, nullptr);

            responseHeader = hbbtvCurl.GetResponseHeader();
            responseContent = hbbtvCurl.GetResponseContent();
            redirectUrl = hbbtvCurl.GetRedirectUrl();
        }
    }

    callback->Continue();
    return true;
}

void BrowserClient::GetResponseHeaders(CefRefPtr<CefResponse> response, int64 &response_length, CefString &_redirectUrl) {
    // reset offset
    offset = 0;

    if (redirectUrl.length() > 0) {
        _redirectUrl.FromString(redirectUrl);
    }

    std::string contentType = responseHeader["Content-Type"];

    // 'fix' the mime type
    // 'application/xhtml+xml'
    for(auto const &item : mimeTypes) {
        if (contentType.find(item.second) != std::string::npos) {
            contentType.replace(contentType.find(item.second), item.second.length(), "application/xhtml+xml");
        }
    }

    std::size_t found = contentType.find(';');
    if (found != std::string::npos) {
        response->SetMimeType(contentType.substr(0, found));
    } else {
        response->SetMimeType(contentType);
    }

    response->SetStatus(200);

    // add all other headers
    CefResponse::HeaderMap map;
    response->GetHeaderMap(map);

    for (auto const& x : responseHeader) {
        map.insert(std::make_pair(x.first, x.second));
    }

    response_length = responseContent.length();
}

bool BrowserClient::ReadResponse(void *data_out, int bytes_to_read, int &bytes_read, CefRefPtr<CefCallback> callback) {
    CEF_REQUIRE_IO_THREAD();

    bool has_data = false;
    bytes_read = 0;

    if (offset < responseContent.length()) {
        int transfer_size = std::min(bytes_to_read, static_cast<int>(responseContent.length() - offset));
        memcpy(data_out, responseContent.data() + offset, static_cast<size_t>(transfer_size));
        offset += transfer_size;

        bytes_read = transfer_size;
        has_data = true;
    }

    return has_data;
}

BrowserClient::ReturnValue BrowserClient::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefRequestCallback> callback) {
    // Customize the request header
    CefRequest::HeaderMap hdrMap;
    request->GetHeaderMap(hdrMap);

    // User-Agent
    std::string newUserAgent(USER_AGENT);
    hdrMap.erase("User-Agent");
    hdrMap.insert(std::make_pair("User-Agent", newUserAgent.c_str()));
    request->SetHeaderMap(hdrMap);

    return RV_CONTINUE;
}

bool BrowserClient::OnBeforeBrowse( CefRefPtr< CefBrowser > browser, CefRefPtr< CefFrame > frame, CefRefPtr< CefRequest > request, bool user_gesture, bool is_redirect ) {
    browser_side_router->OnBeforeBrowse(browser, frame);
    return false;
}

void BrowserClient::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, CefRequestHandler::TerminationStatus status) {
    browser_side_router->OnRenderProcessTerminated(browser);
}

void BrowserClient::OnBeforeClose(CefRefPtr< CefBrowser > browser) {
    CEF_REQUIRE_UI_THREAD();
    browser_side_router->OnBeforeClose(browser);
}

bool BrowserClient::OnProcessMessageReceived(CefRefPtr< CefBrowser > browser, CefRefPtr< CefFrame > frame, CefProcessId source_process, CefRefPtr< CefProcessMessage > message) {
    return browser_side_router->OnProcessMessageReceived(browser, frame, source_process, message);
}

void BrowserClient::setLoadingStart(bool loading) {
    loadingStart = loading;
}

void BrowserClient::injectJs(CefRefPtr<CefBrowser> browser, std::string url, bool defer, bool headerStart) {
    std::ostringstream stringStream;

    stringStream <<  "(function(d){var e=d.createElement('script');";

    if (defer) {
        stringStream << "e.setAttribute('defer','defer');";
    }

    stringStream << "e.setAttribute('type','text/javascript');e.setAttribute('src','" << url << "');";

    if (headerStart) {
        stringStream << "d.head.insertBefore(e, d.head.firstChild)";
    } else {
        stringStream << "d.head.appendChild(e)";
    }

    stringStream << "}(document));";

    auto script = stringStream.str();
    auto frame = browser->GetMainFrame();
    frame->ExecuteJavaScript(script, frame->GetURL(), 0);
}

void BrowserClient::initJavascriptCallback() {
    // register javascript callback
    CefMessageRouterConfig config;
    config.js_query_function = "cefQuery";
    config.js_cancel_function = "cefQueryCancel";

    handler = new JavascriptHandler();

    browser_side_router = CefMessageRouterBrowserSide::Create(config);
    browser_side_router->AddHandler(handler, true);
}

