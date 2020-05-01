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
#include "include/wrapper/cef_helpers.h"
#include "globaldefs.h"
#include "browser.h"
#include "logger.h"

#define HEADERSIZE (4 * 1024)

uint8_t CMD_STATUS = 1;
uint8_t CMD_OSD = 2;
uint8_t CMD_VIDEO = 3;

BrowserClient *browserClient;

struct MemoryStruct {
    char *memory;
    size_t size;
};

struct OsdStruct {
    char    message[20];
    int     width;
    int     height;
};

std::map<std::string, std::string> HbbtvCurl::response_header;
std::string HbbtvCurl::response_content;

int shmid;
uint8_t *shmp;
std::mutex shm_mutex;

HbbtvCurl::HbbtvCurl() {
    curl_global_init(CURL_GLOBAL_ALL);
}

HbbtvCurl::~HbbtvCurl() {
    curl_global_cleanup();
}

std::string HbbtvCurl::ReadContentType(std::string url, CefRequest::HeaderMap headers) {
    CONSOLE_DEBUG("ReadContentType {}", url);

    char *headerdata = (char*)malloc(HEADERSIZE);
    memset(headerdata, 0, HEADERSIZE);

    // set headers as requested
    struct curl_slist *headerChunk = NULL;

    for (auto itr = headers.begin(); itr != headers.end(); itr++) {
        std::string headerLine;
        headerLine.append(itr->first.ToString().c_str());
        headerLine.append(":");
        headerLine.append(itr->second.ToString().c_str());

        headerChunk = curl_slist_append(headerChunk, headerLine.c_str());
    }

    CURL *curl_handle;
    CURLcode res;

    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headerChunk);

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

void HbbtvCurl::LoadUrl(std::string url, CefRequest::HeaderMap headers) {
    CONSOLE_DEBUG("LoadUrl {}", url);

    struct MemoryStruct contentdata {
            (char*)malloc(1), 0
    };

    char *headerdata = (char*)malloc(HEADERSIZE);
    memset(headerdata, 0, HEADERSIZE);

    // set headers as requested
    struct curl_slist *headerChunk = NULL;

    // DBG("Headers:\n");
    for (auto itr = headers.begin(); itr != headers.end(); itr++) {
        std::string headerLine;
        headerLine.append(itr->first.ToString().c_str());
        headerLine.append(":");
        headerLine.append(itr->second.ToString().c_str());

        // DBG("    - %s\n", headerLine.c_str());

        headerChunk = curl_slist_append(headerChunk, headerLine.c_str());
    }

    CURL *curl_handle;
    CURLcode res;

    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headerChunk);
    // curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteContentCallback);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteHeaderCallback);

    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, headerdata);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &contentdata);

    res = curl_easy_perform(curl_handle);

    if (res == CURLE_OK) {
        char *redir_url = nullptr;
        curl_easy_getinfo(curl_handle, CURLINFO_REDIRECT_URL, &redir_url);
        while (res == CURLE_OK && redir_url) {
            // clear buffer
            memset(headerdata, 0, HEADERSIZE);

            free (contentdata.memory);
            contentdata.size = 0;
            contentdata.memory = (char*)malloc(1);

            response_header.clear();

            // reload url
            redirect_url = redir_url;
            curl_easy_setopt(curl_handle, CURLOPT_URL, redir_url);
            res = curl_easy_perform(curl_handle);
            curl_easy_getinfo(curl_handle, CURLINFO_REDIRECT_URL, &redir_url);
        }

        response_content =  std::string(contentdata.memory, contentdata.size);
    }

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
}

JavascriptHandler::~JavascriptHandler() {
}

bool JavascriptHandler::OnQuery(CefRefPtr<CefBrowser> browser,
             CefRefPtr<CefFrame> frame,
             int64 query_id,
             const CefString& request,
             bool persistent,
             CefRefPtr<Callback> callback) {

    // process the javascript callback
    CONSOLE_DEBUG("Javascript called me: {}", request.ToString());

    if (strncmp(request.ToString().c_str(), "VDR:", 4) == 0) {
        browserClient->SendToVdrString(CMD_STATUS, request.ToString().c_str() + 4);
        return true;
    } else {
        if (strncmp(request.ToString().c_str(), "VIDEO_URL:", 10) == 0) {
            CONSOLE_DEBUG("Video URL: {}", request.ToString().c_str() + 10);

            browserClient->SendToVdrString(CMD_STATUS, "PLAY_VIDEO:");
            browserClient->set_input_file(request.ToString().c_str() + 10);
            browserClient->transcode();
            return true;
        } else if (strncmp(request.ToString().c_str(), "PAUSE_VIDEO", 11) == 0) {
            CONSOLE_DEBUG("Video streaming pause");

            browserClient->pause_video();
            return true;
        } else if (strncmp(request.ToString().c_str(), "RESUME_VIDEO", 12) == 0) {
            CONSOLE_DEBUG("Video streaming resume");

            browserClient->resume_video();
            return true;
        } else if (strncmp(request.ToString().c_str(), "END_VIDEO", 9) == 0) {
            CONSOLE_DEBUG("Video streaming ended");

            browserClient->stop_video();
            return true;
        } else if (strncmp(request.ToString().c_str(), "STOP_VIDEO", 10) == 0) {
            CONSOLE_DEBUG("Video streaming stopped");

            browserClient->stop_video();
            return true;
        } else if (strncmp(request.ToString().c_str(), "SEEK_VIDEO ", 11) == 0) {
            CONSOLE_DEBUG("Video streaming seeked to {}", request.ToString().c_str() + 11);

            browserClient->seek_video(request.ToString().c_str() + 11);
            return true;
        } else if (strncmp(request.ToString().c_str(), "SPEED_VIDEO ", 12) == 0) {
            CONSOLE_DEBUG("Video streaming speed change to {}", request.ToString().c_str() + 12);

            browserClient->speed_video(request.ToString().c_str() + 12);
            return true;
        } else if (strncmp(request.ToString().c_str(), "ERROR_VIDEO", 11) == 0) {
            CONSOLE_ERROR("Video playing throws an error");

            // TODO: Gibt es eine bessere Lösung?
            browserClient->stop_video();
            return true;
        }
    }

    return false; // give other handler chance
}

void JavascriptHandler::OnQueryCanceled(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64 query_id) {
    // TODO: cancel our async query task...
}

BrowserClient::BrowserClient(spdlog::level::level_enum log_level) {
    logger.set_level(log_level);

    // bind socket
    if ((toVdrSocketId = nn_socket(AF_SP, NN_PUSH)) < 0) {
        fprintf(stderr, "BrowserClient: unable to create nanomsg socket\n");
    }

    if ((toVdrEndpointId = nn_bind(toVdrSocketId, TO_VDR_CHANNEL)) < 0) {
        fprintf(stderr, "BrowserClient: unable to bind nanomsg socket to %s\n", TO_VDR_CHANNEL);
    }

    browserClient = this;

    osrHandler = new OSRHandler(this,1280, 720);
    renderHandler = osrHandler;

    injectJavascript = true;

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
    if (toVdrSocketId > 0) {
        nn_close(toVdrSocketId);
    }

    if (osrHandler != NULL) {
        delete osrHandler;
        osrHandler = NULL;
        renderHandler = NULL;
    }

    browser_side_router->RemoveHandler(handler);
    delete handler;
}

// getter for the different handler
CefRefPtr<CefRenderHandler> BrowserClient::GetRenderHandler() {
    return renderHandler;
}

CefRefPtr<CefResourceRequestHandler> BrowserClient::GetResourceRequestHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool is_navigation, bool is_download, const CefString &request_initiator, bool &disable_default_handling) {
    auto url = request->GetURL().ToString();

    // disable xiti and ioam
    if (url.find(".xiti.com") != std::string::npos || url.find(".ioam.de") != std::string::npos) {
        return this;
    }

    if (is_navigation) {
        injectJavascript = true;
    } else {
        return CefRequestHandler::GetResourceRequestHandler(browser, frame, request, is_navigation, is_download,
                                                            request_initiator, disable_default_handling);
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
            CefRequest::HeaderMap headers;
            request->GetHeaderMap(headers);

            // read the content type
            std::string ct = hbbtvCurl.ReadContentType(url, headers);

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
    CONSOLE_TRACE("ON LOAD START");

    CefLoadHandler::OnLoadStart(browser, frame, transition_type);
}

void BrowserClient::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) {
    CONSOLE_TRACE("ON LOAD END mode={}, injectJavascript={}", mode, injectJavascript ? "ja" : "nein");

    CEF_REQUIRE_UI_THREAD();
    loadingStart = false;

    if (mode == 2 && injectJavascript) {
        // inject Javascript
        injectJs(browser, "client://js/hbbtv_polyfill.js", false, false, "hbbtvpolyfill");

        injectJavascript = false;
    }

    // set zoom level: 150% means Full HD 1920 x 1080 px
    // frame->ExecuteJavaScript("document.body.style.setProperty('zoom', '150%');", frame->GetURL(), 0);

    // disable scrollbars
    frame->ExecuteJavaScript("document.body.style.setProperty('overflow', 'hidden');", frame->GetURL(), 0);
}

void BrowserClient::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl) {
    CEF_REQUIRE_UI_THREAD();

    if (errorCode == ERR_ABORTED)
        return;
}

// CefResourceHandler
bool BrowserClient::ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    CONSOLE_DEBUG("PROCESS REQUEST {}", request->GetURL().ToString().c_str());

    {
        // distinguish between local files and remote files using cUrl
        auto url = request->GetURL().ToString();

        if (url.find(".xiti.com") != std::string::npos || url.find(".ioam.de") != std::string::npos) {
            return false;
        }

        CefRequest::HeaderMap headers;
        request->GetHeaderMap(headers);

        hbbtvCurl.LoadUrl(url, headers);

        responseHeader = hbbtvCurl.GetResponseHeader();
        responseContent = hbbtvCurl.GetResponseContent();
        redirectUrl = hbbtvCurl.GetRedirectUrl();
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

void BrowserClient::injectJs(CefRefPtr<CefBrowser> browser, std::string url, bool defer, bool headerStart, std::string htmlid) {
    std::ostringstream stringStream;

    stringStream <<  "(function(d){if (document.getElementById('";
    stringStream <<  htmlid << "') == null) { var e=d.createElement('script');";

    if (defer) {
        stringStream << "e.setAttribute('defer','defer');";
    }

    stringStream << "e.setAttribute('id','" << htmlid <<"');";
    stringStream << "e.setAttribute('type','text/javascript');e.setAttribute('src','" << url << "');";

    if (headerStart) {
        stringStream << "d.head.insertBefore(e, d.head.firstChild)";
    } else {
        stringStream << "d.head.appendChild(e)";
    }

    stringStream << "}}(document));";

    auto script = stringStream.str();
    auto frame = browser->GetMainFrame();
    frame->ExecuteJavaScript(script, frame->GetURL(), 0);
}

void BrowserClient::injectJsModule(CefRefPtr<CefBrowser> browser, std::string url, std::string htmlid) {
    std::ostringstream stringStream;

    stringStream << "(function(d){if (document.getElementById('" << htmlid << "') == null) {";
    stringStream << "var e=d.createElement('script');";
    stringStream << "e.setAttribute('type','module');";
    stringStream << "e.setAttribute('src','" << url <<"');";
    stringStream << "e.setAttribute('id','" << htmlid <<"');";
    // stringStream << "d.head.insertBefore(e, d.head.firstChild)";
    stringStream << "d.head.appendChild(e)";
    stringStream << "}}(document));";

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
    handler->SetBrowserClient(this);

    browser_side_router = CefMessageRouterBrowserSide::Create(config);
    browser_side_router->AddHandler(handler, true);
}

void BrowserClient::eventCallback(std::string cmd) {
    browserClient->SendToVdrString(CMD_STATUS, cmd.c_str());
}

int BrowserClient::write_buffer_to_vdr(uint8_t *buf, int buf_size) {
    browserClient->SendToVdrVideoData(buf, buf_size);
    return buf_size;
}

void BrowserClient::SendToVdrString(uint8_t messageType, const char* message) {
    CONSOLE_TRACE("Send String to VDR, Message {}", message);

    char* buffer = (char*)malloc(2 + strlen(message));

    buffer[0] = messageType;
    strcpy(buffer + 1, message);
    nn_send(toVdrSocketId, buffer, strlen(message) + 2, 0);

    printf("Send string %2d %s\n", buffer[0], buffer + 1);

    free(buffer);
}

void BrowserClient::SendToVdrVideoData(uint8_t* message, int size) {
    CONSOLE_TRACE("Send video buffer to VDR, size of buffer {}", size);

    message[0] = CMD_VIDEO;
    nn_send(toVdrSocketId, message, size, 0);
}

void BrowserClient::SendToVdrOsd(const char* message, int width, int height) {
    CONSOLE_TRACE("Send OSD update to VDR, Message {}, Width {}, Height {}", message, width, height);

    struct OsdStruct osdStruct = {};
    auto *buffer = new uint8_t[1 + sizeof(struct OsdStruct)];

    osdStruct.width = width;
    osdStruct.height = height;
    memset(osdStruct.message, 0, 20);
    strncpy(osdStruct.message, message, 19);

    buffer[0] = CMD_OSD;
    memcpy((void*)(buffer + 1), &osdStruct, sizeof(struct OsdStruct));

    nn_send(toVdrSocketId, buffer, sizeof(struct OsdStruct) + 1, 0);

    delete[] buffer;
}

bool BrowserClient::set_input_file(const char* input) {
    CONSOLE_DEBUG("Set Input video {}", input);

    if (transcoder != nullptr) {
        stop_video();

        delete transcoder;
        transcoder = nullptr;
    }

    transcoder = new TranscodeFFmpeg();
    transcoder->set_event_callback(eventCallback);
    return transcoder->set_input(input, false);
}

void BrowserClient::pause_video() {
    CONSOLE_DEBUG("Pause video");

    transcoder->pause_video();
}

void BrowserClient::resume_video() {
    CONSOLE_DEBUG("Resume video");

    transcoder->resume_video();
}

void BrowserClient::stop_video() {
    CONSOLE_DEBUG("Stop video");

    transcoder->stop_video();

    if (transcode_thread.joinable()) {
        transcode_thread.join();
    }
}

void BrowserClient::seek_video(const char* ms) {
    CONSOLE_DEBUG("Seek video to {} ms", ms);

    transcode_thread = transcoder->seek_video(ms, write_buffer_to_vdr);
    transcode_thread.detach();
}

void BrowserClient::speed_video(const char* speed) {
    CONSOLE_DEBUG("Speed video to speed {}", speed);

    transcoder->speed_video(speed);
}

int BrowserClient::transcode() {
    if (transcoder == nullptr) {
        fprintf(stderr, "Internal error: transcoder is null!");
        return -1;
    }

    transcode_thread = transcoder->transcode(write_buffer_to_vdr);
    transcode_thread.detach();

    return 0;
}
