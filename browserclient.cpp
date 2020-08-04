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
#include <fstream>
#include <string>
#include <map>
#include <list>
#include <regex>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <curl/curl.h>
#include "include/wrapper/cef_helpers.h"
#include "include/cef_cookie.h"
#include "globaldefs.h"
#include "browser.h"
#include "javascriptlogging.h"
#include "dashhandler.h"
//#include <backward.hpp>

#define HEADERSIZE (4 * 1024)

BrowserClient *browserClient;
CefRefPtr<CefCookieManager> cookieManager;

std::map<std::string, std::string> cacheContentType;
std::list<std::string> urlBlockList;

// optimistic regex to determine a hbbtv page
std::regex hbbtv_regex_1("type\\s*=\\s*\"application/oipfApplicationManager\"", std::regex::optimize);
std::regex hbbtv_regex_2("type\\s*=\\s*\"application/oipfConfiguration\"", std::regex::optimize);
std::regex hbbtv_regex_3("type\\s*=\\s*\"oipfCapabilities\"", std::regex::optimize);

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

std::mutex cookieMutex;
std::string singleLineCookies;
std::string ffmpegCookies;

/*
void stacker() {
    using namespace backward;

    StackTrace st;
    st.load_here(99);
    st.skip_n_firsts(3);

    Printer p;
    p.snippet = true;
    p.object = true;
    //p.color = true;
    p.address = true;
    p.print(st, stderr);
}
*/

/*
 * very primitive try to fix some common errors
 * If this is not enough, think about using libtidy
 */
void tryToFixPage(std::string &source) {
    // remove all CDATA declarations.
    replaceAll(source, "/* <![CDATA[ */", "\n");
    replaceAll(source, "/* ]]> */", "\n");
    replaceAll(source, "// <![CDATA[", "\n");
    replaceAll(source, "//<![CDATA[", "\n");
    replaceAll(source, "// ]]>", "\n");
    replaceAll(source, "//]]>", "\n");

    // change quotation marks
    replaceAll(source, "<script type='text/javascript'>", "<script type=\"text/javascript\">");
    replaceAll(source, "<style type='text/css'>", "<style type=\"text/css\">");

    // fix inline style and script elements
    replaceAll(source, "<style type=\"text/css\">", "</style>", "<style type=\"text/css\">\n/* <![CDATA[ */\n", "\n/* ]]> */\n</style>");
    replaceAll(source, "<script type=\"text/javascript\">", "</script>", "<script type=\"text/javascript\">\n/* <![CDATA[ */\n", "\n/* ]]> */\n</script>");
    replaceAll(source, "<script>", "</script>", "<script>\n/* <![CDATA[ */\n", "\n/* ]]> */\n</script>");

    // fix wrong meta-tag in header
    // printf("VOR FIX-META:\n%s\n\n", source.c_str());

    size_t pos = source.find("<meta");
    while (pos != std::string::npos) {
        size_t pos2 = source.find(">", pos);
        if (source[pos2-1] != '/') {
            // check if the following string is </meta>
            size_t pos3 = source.find("</meta>", pos2+1);
            if (pos3 == std::string::npos || pos3 != pos2+1) {
                source.replace(pos2, 1, "/>");
            }
        }

        pos = source.find("<meta", pos2);
    }

    // printf("NACH FIX-META:\n%s\n\n", source.c_str());
}

class BrowserCookieVisitor : public CefCookieVisitor {
private:
    bool log;
    std::map<std::string, std::string> cookies;

public:
    BrowserCookieVisitor(bool log = true) {
        this->log = log;
    }

    ~BrowserCookieVisitor() = default;

    bool Visit(const CefCookie& cookie, int count, int total, bool& deleteCookie) override {
        std::string cname = CefString(cookie.name.str).ToString();
        std::string cvalue = CefString(cookie.value.str).ToString();

        if (log) {
            std::string cpath = CefString(cookie.path.str).ToString();
            std::string cdomain = CefString(cookie.domain.str).ToString();

            CONSOLE_TRACE("Cookie {}/{}: Path {}, Domain {}: '{}' = '{}'", count, total, cpath, cdomain, cname, cvalue);
        }

        cookies.insert(std::pair<std::string, std::string>(cname, cvalue));

        return true;
    }

    void SetCookieStrings() {
        std::string single = "";
        std::string ffmpeg = "";

        for (auto const &item : cookies) {
            single = single + item.first + "=" + item.second + "; ";
            ffmpeg = ffmpeg + item.first + ": " + item.second + "\\r\\n";
        }

        singleLineCookies = single;
        ffmpegCookies = ffmpeg;
    }

    IMPLEMENT_REFCOUNTING(BrowserCookieVisitor);
};

class FrameContentLogger : public CefStringVisitor {
private:
    static int fileno;

public:
    void Visit(const CefString& content) {
        std::string filename = std::string("content_trace_injected_") + std::to_string(fileno) + std::string(".html");

        std::ofstream contentfile;
        contentfile.open(filename);
        contentfile << content << "\n";
        contentfile.close();

        fileno++;
    }

    IMPLEMENT_REFCOUNTING(FrameContentLogger);
};

int FrameContentLogger::fileno = 0;

HbbtvCurl::HbbtvCurl() {
    curl_global_init(CURL_GLOBAL_ALL);
}

HbbtvCurl::~HbbtvCurl() {
    curl_global_cleanup();
}

std::string HbbtvCurl::ReadContentType(std::string url, CefRequest::HeaderMap headers) {
    cookieMutex.lock();
    std::string c = singleLineCookies;
    cookieMutex.unlock();

    CONSOLE_DEBUG("ReadContentType {}, Cookies {}", url, c);

    // load the whole page
    LoadUrl(url, headers, 1L);

    redirect_url = "";

    // Test content
    if (std::regex_search(response_content, hbbtv_regex_1) ||
        std::regex_search(response_content, hbbtv_regex_2) ||
        std::regex_search(response_content, hbbtv_regex_3)) {

        CONSOLE_TRACE("HbbtvCurl::ReadContentType, Found HbbTV Objects, return Content-Type 'application/vnd.hbbtv.xhtml+xml'");
        return "application/vnd.hbbtv.xhtml+xml";
    } else {
        std::string ct = "text/html";
        if (url.find("pro7.gofresh.tv") != std::string::npos) {
            ct = response_header["Content-Type"];
        }

        CONSOLE_TRACE("HbbtvCurl::ReadContentType, No HbbTV Objects found, return Content-Type {}", ct);

        return ct;
    }
}

void HbbtvCurl::LoadUrl(std::string url, CefRequest::HeaderMap headers, long followLocation) {
    cookieMutex.lock();
    std::string c = singleLineCookies;
    cookieMutex.unlock();

    CONSOLE_DEBUG("HbbtvCurl::LoadUrl {}, Cookies {}", url, c);

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

    if (followLocation) {
        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, followLocation);
    }

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteContentCallback);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteHeaderCallback);

    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, headerdata);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &contentdata);

    curl_easy_setopt(curl_handle, CURLOPT_COOKIE, c.c_str());

    res = curl_easy_perform(curl_handle);

    if (res == CURLE_OK) {
        char *redir_url = nullptr;
        curl_easy_getinfo(curl_handle, CURLINFO_REDIRECT_URL, &redir_url);
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);

        CONSOLE_TRACE("HbbtvCurl::LoadUrl, Response code {}, Redir_Url {}", response_code, redir_url != nullptr ? redir_url : "<none>");

        if (redir_url) {
            redirect_url = redir_url;
        } else {
            redirect_url = "";
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
    if (strncmp(request.ToString().c_str(), "DASH_PL", 7) != 0) {
        CONSOLE_TRACE("Javascript called me: {}", request.ToString());
    }

    if (strncmp(request.ToString().c_str(), "VDR:", 4) == 0) {
        browserClient->SendToVdrString(CMD_STATUS, request.ToString().c_str() + 4);
        return true;
    } else {
        if (strncmp(request.ToString().c_str(), "VIDEO_URL:", 10) == 0) {
            CONSOLE_DEBUG("Video URL: {}", request.ToString().c_str() + 10);

            auto video = std::string(request.ToString().c_str() + 10);
            auto delimiterPos = video.find(":");
            auto time = video.substr(0, delimiterPos);
            auto url = video.substr(delimiterPos + 1);

            unlink(VIDEO_UNIX);

            browserClient->SendToVdrString(CMD_STATUS, "PLAY_VIDEO:");
            if (!browserClient->set_input_file(time.c_str(), url.c_str())) {
                browserClient->SendToVdrString(CMD_STATUS, "VIDEO_FAILED");
                return true;
            }

            // get Video size (callback)
            frame->ExecuteJavaScript("window.cefVideoSize()", frame->GetURL(), 0);

            return true;
        } else if (strncmp(request.ToString().c_str(), "PAUSE_VIDEO: ", 13) == 0) {
            CONSOLE_DEBUG("Video streaming pause");

            browserClient->pause_video();
            return true;
        } else if (strncmp(request.ToString().c_str(), "RESUME_VIDEO: ", 14) == 0) {
            CONSOLE_DEBUG("Video streaming resume");

            browserClient->resume_video(request.ToString().c_str() + 14);
            return true;
        } else if (strncmp(request.ToString().c_str(), "END_VIDEO", 9) == 0) {
            CONSOLE_DEBUG("Video streaming ended");

            browserClient->SendToVdrString(CMD_STATUS, "STOP_VIDEO");
            browserClient->stop_video();
            return true;
        } else if (strncmp(request.ToString().c_str(), "STOP_VIDEO", 10) == 0) {
            CONSOLE_DEBUG("Video streaming stopped");

            browserClient->SendToVdrString(CMD_STATUS, "STOP_VIDEO");
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
        } else if (strncmp(request.ToString().c_str(), "CHANGE_VIDEO_URL:", 17) == 0) {
            CONSOLE_DEBUG("Video URL: {}", request.ToString().c_str() + 17);

            auto video = std::string(request.ToString().c_str() + 17);
            auto delimiterPos = video.find(":");
            auto time = video.substr(0, delimiterPos);
            auto url = video.substr(delimiterPos + 1);

            unlink(VIDEO_UNIX);

            browserClient->SendToVdrString(CMD_STATUS, "PLAY_VIDEO:");
            if (!browserClient->set_input_file(time.c_str(), url.c_str())) {
                browserClient->SendToVdrString(CMD_STATUS, "VIDEO_FAILED");
                return true;
            }

            // get Video size (callback)
            frame->ExecuteJavaScript("window.cefVideoSize()", frame->GetURL(), 0);

            return true;
        } else if (strncmp(request.ToString().c_str(), "VIDEO_SIZE: ", 12) == 0) {
            browserClient->SendToVdrString(CMD_STATUS, request.ToString().c_str());

            // send also an OSD update
            browser->GetHost()->Invalidate(PET_VIEW);

            return true;
        } else if (strncmp(request.ToString().c_str(), "CHANGE_URL: ", 12) == 0) {
            CefString url(request.ToString().substr(12));
            browserControl->LoadURL(url);

            // Save the last URL on a stack
            // applicationStack.push(url);

            return true;
        } else if (strncmp(request.ToString().c_str(), "CREATE_APP: ", 12) == 0) {
            CefString url(request.ToString().substr(12));

            CONSOLE_TRACE("Create application {}", url.ToString());

            // Save the last URL on a stack
            // applicationStack.push(url);

            return true;
        } else if (strncmp(request.ToString().c_str(), "DESTROY_APP", 11) == 0) {
            CONSOLE_TRACE("Destroy application");

            // load the last URL of the stack
            browserClient->SendToVdrString(CMD_STATUS, "STOP_VIDEO");
            browserClient->stop_video();

            // std::string lastUrl = applicationStack.top();
            // applicationStack.pop();
            // browserControl->LoadURL(lastUrl);

            // CONSOLE_TRACE("Destroy application: Load new URL {}", lastUrl);
            return true;
        } else if (strncmp(request.ToString().c_str(), "CLEAR_DASH", 10) == 0) {
            audioDashHandler.ClearAll();
            videoDashHandler.ClearAll();
            return true;
        } else if (strncmp(request.ToString().c_str(), "DASH:", 5) == 0) {
            char stype = request.ToString().at(5);
            char type = request.ToString().at(6);
            std::string info = request.ToString().substr(8);

            uint streamIdx = -1;
            auto pos = info.find(':');
            streamIdx = std::stoi(info.substr(0, pos));
            info = info.substr(pos + 1);

            std::istringstream f(info);
            std::string s;
            int idx = 0;

            DashStream& stream = (stype == 'V') ? videoDashHandler.GetStream(streamIdx) : audioDashHandler.GetStream(streamIdx);

            if (stype == 'V') {
                stream.type = Video;
            } else {
                stream.type = Audio;
            }

            if (stype == 'B' && type == 'A') {
                audioDashHandler.SetBaseUrl(info);
                videoDashHandler.SetBaseUrl(info);
            } else {
                switch (type) {
                    case 'C':
                        idx = 0;
                        while (getline(f, s, ':')) {
                            switch (idx) {
                                case 0: stream.duration = std::stoul(s); break;
                                case 1: stream.firstSegment = std::stoul(s); break;
                                case 2: stream.lastSegment = std::stoul(s); break;
                                case 3: stream.startSegment = std::stoul(s); break;
                            }
                            idx++;
                        }
                        break;

                    case 'D':
                        idx = 0;
                        while (getline(f, s, ':')) {
                            switch (idx) {
                                case 0: stream.width = std::stoi(s); break;
                                case 1: stream.height = std::stoi(s); break;
                                case 2: stream.bandwidth = std::stoul(s); break;
                            }
                            idx++;
                        }

                        break;

                    case 'I':
                        stream.initUrl = info;
                        break;

                    case 'M':
                        stream.segmentUrl = info;
                        break;

                    default:
                        // ignore
                        break;
                }
            }

            return true;
        }
    }

    return false; // give other handler chance
}

void JavascriptHandler::OnQueryCanceled(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64 query_id) {
    // TODO: cancel our async query task...
}

BrowserClient::BrowserClient(spdlog::level::level_enum log_level, std::string vproto) {
    logger.set_level(log_level);
    // stacker();

    // bind socket
    if ((toVdrSocketId = nn_socket(AF_SP, NN_PUSH)) < 0) {
        fprintf(stderr, "BrowserClient: unable to create nanomsg socket\n");
    }

    if (nn_bind(toVdrSocketId, TO_VDR_CHANNEL) < 0) {
        fprintf(stderr, "BrowserClient: unable to bind nanomsg socket to %s\n", TO_VDR_CHANNEL);
    }

    browserClient = this;

    if (vproto == "UDP") {
        this->vproto = UDP;
    } else if (vproto == "TCP") {
        this->vproto = TCP;
    } else if (vproto == "UNIX") {
        this->vproto = UNIX;
    } else {
        fprintf(stderr, "BrowserClient: invalid video protocol %s\n", vproto.c_str());
    }

    // start heartbeat_thread
    heartbeat_running = true;
    heartbeat_thread = std::thread(&BrowserClient::heartbeat, this);
    heartbeat_thread.detach();

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

    transcoder = nullptr;

    cookieManager = CefCookieManager::GetGlobalManager(nullptr);

    // read url block pattern
    std::ifstream infile(exePath + "/block_url.config");
    if (infile) {
        std::string str;
        while (std::getline(infile, str)) {
            if (str.size() > 0 && str.at(0) != '#') {
                str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
                if (str.size() > 0) {
                    urlBlockList.push_back(str);
                }
            }
        }
        infile.close();
    }
}

BrowserClient::~BrowserClient() {
    heartbeat_running = false;

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
    CONSOLE_TRACE("BrowserClient::GetResourceRequestHandler for {}, is_navigation {}, request_initiator {}", request->GetURL().ToString(), is_navigation, request_initiator.ToString());

    // check if URL is blocked by pattern list
    auto url = request->GetURL().ToString();
    for (auto sub : urlBlockList) {
        if (url.find(sub) != std::string::npos) {
            // Return this. The request will be canceled later
            return this;
        }
    }

    cookieMutex.lock();
    CefRefPtr<BrowserCookieVisitor> visitor = new BrowserCookieVisitor(logger.isTraceEnabled());
    cookieManager->VisitUrlCookies(request->GetURL(), false, visitor);
    visitor->SetCookieStrings();
    cookieMutex.unlock();

    if (is_navigation) {
        injectJavascript = true;
    } else {
        CONSOLE_TRACE("BrowserClient::GetResourceRequestHandler, use default ResourceRequestHandler");

        // special case: if the requested URL is client://movie/transparent-full.webm then the live TV shall be shown with different size
        if (request->GetURL() == "client://movie/transparent-full.webm") {
            // get Video size (callback)
            frame->ExecuteJavaScript("window.cefVideoSize()", frame->GetURL(), 0);
        }

        return CefRequestHandler::GetResourceRequestHandler(browser, frame, request, is_navigation, is_download,
                                                            request_initiator, disable_default_handling);
    }

    return this;
}

CefRefPtr<CefResourceHandler> BrowserClient::GetResourceHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request) {
    CONSOLE_TRACE("GetResourceHandler: {}", request->GetURL().ToString());

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
        auto handle = endsWith(url,"view-source:'") ||
                      endsWith(url, ".json") ||
                      endsWith(url, ".js") ||
                      endsWith(url, ".css") ||
                      endsWith(url, ".ico") ||
                      endsWith(url, ".jpg") ||
                      endsWith(url, ".png") ||
                      endsWith(url, ".gif") ||
                      endsWith(url, ".webp") ||
                      endsWith(url, ".m3u8") ||
                      endsWith(url, ".mpd") ||
                      endsWith(url, ".ts") ||
                      endsWith(url, ".mpg") ||
                      endsWith(url, ".mp3") ||
                      endsWith(url, ".mp4") ||
                      endsWith(url, ".mov") ||
                      endsWith(url, ".avi") ||
                      endsWith(url, ".pdf") ||
                      endsWith(url, ".ppt") ||
                      endsWith(url, ".pptx") ||
                      endsWith(url, ".xls") ||
                      endsWith(url, ".xlsx") ||
                      endsWith(url, ".doc") ||
                      endsWith(url, ".docx") ||
                      endsWith(url, ".zip") ||
                      endsWith(url, ".rar") ||
                      endsWith(url, ".woff2") ||
                      endsWith(url, ".svg") ||
                      endsWith(url, ".ogg") ||
                      endsWith(url, ".ogm") ||
                      endsWith(url, ".ttf") ||
                      endsWith(url, ".7z");

        if (handle) {
            // use default handler
            CONSOLE_TRACE("GetResourceHandler, URL {} -> use default Handler", url);
            return CefResourceRequestHandler::GetResourceHandler(browser, frame, request);
        } else {
            CefRequest::HeaderMap headers;
            request->GetHeaderMap(headers);

            // read/find the content type
            std::string ct;
            auto ctsearch = cacheContentType.find(url);
            if(ctsearch != cacheContentType.end()) {
                ct = ctsearch->second;
            } else {
                ct = hbbtvCurl.ReadContentType(url, headers);
            }

            // non-cache Version
            // std::string ct = hbbtvCurl.ReadContentType(url, headers);

            bool isHbbtvHtml = false;
            for (auto const &item : mimeTypes) {
                if (ct.find(item.second) != std::string::npos) {
                    isHbbtvHtml = true;
                    cacheContentType[url] = ct;
                    break;
                }
            }

            CONSOLE_TRACE("GetResourceHandler, URL {} -> IsHbbtv: {}", url, isHbbtvHtml);

            if (isHbbtvHtml) {
                CONSOLE_TRACE("GetResourceHandler, URL {} -> use special Handler");
                // use special handler
                return this;
            } else {
                CONSOLE_TRACE("GetResourceHandler, URL {} -> use default Handler2");
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
    CONSOLE_TRACE("ON LOAD END mode={}, injectJavascript={}, responseCode={}", mode, injectJavascript ? "ja" : "nein", responseCode);

    if (responseCode >= 300 && responseCode <= 399) {
        // redirect
        return;
    }

    CEF_REQUIRE_UI_THREAD();

    if (mode == 2 && injectJavascript) {
        // inject Javascript
        injectJs(browser, "js/font.js", true, false, "hbbtvfont", false);
        injectJavascript = false;
    }

    // set zoom level: 150% means Full HD 1920 x 1080 px
    // frame->ExecuteJavaScript("document.body.style.setProperty('zoom', '150%');", frame->GetURL(), 0);

    // disable scrollbars
    frame->ExecuteJavaScript("document.body.style.setProperty('overflow', 'hidden');", frame->GetURL(), 0);

    // Fix for Arte "Plugin in not found" while video playing
    frame->ExecuteJavaScript("if (document.getElementById('appmgr'))document.getElementById('appmgr').style.setProperty('visibility', 'hidden');", frame->GetURL(), 0);
    frame->ExecuteJavaScript("if (document.getElementById('oipfcfg'))document.getElementById('oipfcfg').style.setProperty('visibility', 'hidden');", frame->GetURL(), 0);
    frame->ExecuteJavaScript("if (document.getElementById('oipfCap'))document.getElementById('oipfCap').style.setProperty('visibility', 'hidden');", frame->GetURL(), 0);
    frame->ExecuteJavaScript("if (document.getElementById('oipfDrm'))document.getElementById('oipfDrm').style.setProperty('visibility', 'hidden');", frame->GetURL(), 0);

    // inject all currently existing app/url
    for (auto const &item : appUrls) {
        {
            // construct the javascript command
            std::ostringstream stringStream;
            stringStream << "if (window._HBBTV_APPURL_) window._HBBTV_APPURL_.set('" << item.first << "','" << item.second << "');";
            auto script = stringStream.str();
            auto frame = browser->GetMainFrame();
            frame->ExecuteJavaScript(script, frame->GetURL(), 0);
        }
    }

    if (logger.isTraceEnabled()) {
        frame->GetSource(new FrameContentLogger());
    }
}

void BrowserClient::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl) {
    CEF_REQUIRE_UI_THREAD();

    if (errorCode == ERR_ABORTED)
        return;
}

void BrowserClient::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) {
    CONSOLE_TRACE("BrowserClient::OnLoadingStateChange: URL={}, isLoading={}, canGoBack={}, canGoForward={}", browser->GetMainFrame()->GetURL().ToString(), isLoading, canGoBack, canGoForward);
}

// CefResourceHandler
bool BrowserClient::ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    CONSOLE_DEBUG("BrowserClient::ProcessRequest: {}, Method: {}", request->GetURL().ToString(), request->GetMethod().ToString());

    {
        // distinguish between local files and remote files using cUrl
        auto url = request->GetURL().ToString();

        CefRequest::HeaderMap headers;
        request->GetHeaderMap(headers);

        CONSOLE_TRACE("BrowserClient::ProcessRequest, Request Headers");
        for (auto itra = headers.begin(); itra != headers.end(); itra++) {
            CONSOLE_TRACE("   {}: {}", itra->first.ToString().c_str(), itra->second.ToString().c_str());
        }

        hbbtvCurl.LoadUrl(url, headers, 0L);

        responseHeader = hbbtvCurl.GetResponseHeader();
        responseContent = hbbtvCurl.GetResponseContent();
        responseCode = hbbtvCurl.GetResponseCode();
        redirectUrl = hbbtvCurl.GetRedirectUrl();

        responseHeader.insert(std::pair<std::string, std::string>("Access-Control-Allow-Origin", "*"));

        // Fix some Pages
        tryToFixPage(responseContent);

        /**
         * inject hbbtv javascript (start)
         */
        char *inject;

        // find header tag (normally it is <head>, but i also found <head id="xxx">)
        size_t headStart = responseContent.find("<head");

        if (headStart != std::string::npos) {
            size_t headEnd = responseContent.find(">", headStart);
            std::string head = responseContent.substr(headStart, headEnd - headStart + 1);

            /***
             The order of injected scripts is inverted! The first one will be the last one in the page.
             ***/

            // Enable/disable debug messages
            asprintf(&inject, "%s\n<script type=\"text/javascript\">window._HBBTV_DEBUG_ = %s;</script>\n",
                     head.c_str(),
                     (logger.isTraceEnabled() || logger.isDebugEnabled()) ? "true" : "false");
            replaceAll(responseContent, head, inject);
            free(inject);

            // create global map for application ids/urls
            asprintf(&inject, "%s\n<script type=\"text/javascript\">window._HBBTV_APPURL_ = new Map();</script>\n",
                     head.c_str());
            replaceAll(responseContent, head, inject);
            free(inject);

            // set current channel
            asprintf(&inject,
                     "%s\n<script type=\"text/javascript\">window.HBBTV_POLYFILL_NS = window.HBBTV_POLYFILL_NS || {}; window.HBBTV_POLYFILL_NS.currentChannel = %s;</script>\n",
                     head.c_str(),
                     currentChannel.c_str());
            replaceAll(responseContent, head, inject);
            free(inject);


            // inject xmlhttprequest_quirks.js
            asprintf(&inject,
                     "%s\n<script id=\"xmlhttprequestquirk\" type=\"text/javascript\" src=\"client://js/xmlhttprequest_quirks.js\"/>\n",
                     head.c_str());
            replaceAll(responseContent, head, inject);
            free(inject);


            /*
            // inject xhook.js
            asprintf(&inject, "%s\n<script id=\"hbbtvxhook\" type=\"text/javascript\" src=\"client://js/xhook.js\"/>\n",
                     head.c_str());
            replaceAll(responseContent, head, inject);
            free(inject);
            */

            /*
            // inject ajaxhook.js
            asprintf(&inject, "%s\n<script id=\"hbbtvajaxhook\" type=\"text/javascript\" src=\"client://js/ajaxhook.js\"/>\n",
                     head.c_str());
            replaceAll(responseContent, head, inject);
            free(inject);
            */

            // inject hbbtv_polyfill.js
            asprintf(&inject,
                     "%s\n<script id=\"hbbtvpolyfill\" type=\"text/javascript\" src=\"client://js/hbbtv_polyfill.js\"/>\n",
                     head.c_str());
            replaceAll(responseContent, head, inject);
            free(inject);

            // inject video_quirks.js
            asprintf(&inject,
                     "%s\n<script id=\"videoquirk\" type=\"text/javascript\" src=\"client://js/video_quirks.js\"/>\n",
                     head.c_str());
            replaceAll(responseContent, head, inject);
            free(inject);

            // inject focus.js
            asprintf(&inject,
                     "%s\n<script id=\"hbbtvfocus\" type=\"text/javascript\" src=\"client://js/focus.js\"/>\n",
                     head.c_str());
            replaceAll(responseContent, head, inject);
            free(inject);

            // inject beforeend.js at the end of the document
            replaceAll(responseContent, "</body>",
                       "\n<script type=\"text/javascript\" src=\"client://js/beforeend.js\"></script>\n</body>");

            /**
             * inject hbbtv javascript (end)
             */

            CONSOLE_TRACE("BrowserClient::ProcessRequest, Response Headers");
            for (auto itrb = responseHeader.begin(); itrb != responseHeader.end(); itrb++) {
                CONSOLE_TRACE("   {}: {}", itrb->first, itrb->second);
            }
        }
    }

    callback->Continue();
    return true;
}

void BrowserClient::GetResponseHeaders(CefRefPtr<CefResponse> response, int64 &response_length, CefString &_redirectUrl) {
    // reset offset
    offset = 0;

    CONSOLE_TRACE("BrowserClient::GetResponseHeaders redirectUrl={}, responseCode={}", redirectUrl, responseCode);

    if (redirectUrl.length() > 0) {
        _redirectUrl.FromString(redirectUrl);
    } else {
        std::string contentType = responseHeader["Content-Type"];

        // 'fix' the mime type
        // 'application/xhtml+xml'
        for (auto const &item : mimeTypes) {
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
    }

    response->SetStatus(responseCode);

    // add all other headers
    CefResponse::HeaderMap map;
    response->GetHeaderMap(map);

    for (auto const& x : responseHeader) {
        map.insert(std::make_pair(x.first, x.second));
    }

    response_length = responseContent.length();
}

bool BrowserClient::ReadResponse(void *data_out, int bytes_to_read, int &bytes_read, CefRefPtr<CefCallback> callback) {
    CONSOLE_DEBUG("BrowserClient::ReadResponse, bytes_to_read {}", bytes_to_read);

    CEF_REQUIRE_IO_THREAD();

    static int fileno = 0;

    bool has_data = false;
    bytes_read = 0;

    if (offset < responseContent.length()) {
        if (logger.isTraceEnabled()) {
            std::string filename = std::string("content_trace_") + std::to_string(fileno) + std::string(".html");
            std::ofstream contentfile;
            contentfile.open(filename);
            contentfile << responseContent << "\n";
            contentfile.close();

            fileno++;
        }

        int transfer_size = std::min(bytes_to_read, static_cast<int>(responseContent.length() - offset));
        memcpy(data_out, responseContent.data() + offset, static_cast<size_t>(transfer_size));
        offset += transfer_size;

        bytes_read = transfer_size;
        has_data = true;
    }

    return has_data;
}

BrowserClient::ReturnValue BrowserClient::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefRequestCallback> callback) {
    auto url = request->GetURL().ToString();
    CONSOLE_TRACE("BrowserClient::OnBeforeResourceLoad: Current {}, New {}", browser->GetMainFrame()->GetURL().ToString(), url);

    // check if URL is blocked by pattern list
    for (auto sub : urlBlockList) {
        if (url.find(sub) != std::string::npos) {
            return RV_CANCEL;
        }
    }

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

CefRefPtr<CefDisplayHandler> BrowserClient::GetDisplayHandler() {
    // if the file logger has been enabled...
    if (logger.switchedToFile()) {
        return new JavascriptLogging(this);
    }

    return nullptr;
}

void BrowserClient::injectJs(CefRefPtr<CefBrowser> browser, std::string url, bool defer, bool headerStart, std::string htmlid, bool insert) {
    if (insert) {
        // inject whole javascript file into page
        std::ostringstream stringStream;

        stringStream << "(function(d){if (document.getElementById('";
        stringStream << htmlid << "') == null) { var e=d.createElement('script');";

        if (defer) {
            stringStream << "e.setAttribute('defer','defer');";
        }

        stringStream << "e.setAttribute('id','" << htmlid << "');";
        stringStream << "e.setAttribute('type','text/javascript');";

        // read whole javascript file into buffer
        FILE* f = fopen(url.c_str(), "r");

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        char* buffer = new char[size];
        rewind(f);
        fread(buffer, sizeof(char), size, f);

        // fprintf(stderr, "Buffer: %s\n", buffer);

        stringStream << "e.textContent=`//<![CDATA[\n" << buffer << "\n//]]>\n`;";

        delete[] buffer;

        if (headerStart) {
            stringStream << "d.head.insertBefore(e, d.head.firstChild)";
        } else {
            stringStream << "d.head.appendChild(e)";
        }

        stringStream << "}}(document));";

        auto script = stringStream.str();

        // fprintf(stderr, "SCRIPT\n%s\n", script.c_str());

        auto frame = browser->GetMainFrame();
        frame->ExecuteJavaScript(script, frame->GetURL(), 0);
    } else {
        // inject link to javascript file
        std::ostringstream stringStream;

        stringStream << "(function(d){if (document.getElementById('";
        stringStream << htmlid << "') == null) { var e=d.createElement('script');";

        if (defer) {
            stringStream << "e.setAttribute('defer','defer');";
        }

        stringStream << "e.setAttribute('id','" << htmlid << "');";
        stringStream << "e.setAttribute('type','text/javascript');e.setAttribute('src','client://" << url << "');";

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

void BrowserClient::SendToVdrString(uint8_t messageType, const char* message) {
    if (messageType != 5) {
        CONSOLE_TRACE("Send String to VDR, Message {}", message);
    }

    char* buffer = (char*)malloc(2 + strlen(message));

    buffer[0] = messageType;
    strcpy(buffer + 1, message);
    nn_send(toVdrSocketId, buffer, strlen(message) + 2, 0);

    free(buffer);
}

void BrowserClient::SendToVdrOsd(const char* message, int width, int height) {
    // CONSOLE_TRACE("Send OSD update to VDR, Message {}, Width {}, Height {}", message, width, height);

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

void BrowserClient::SendToVdrPing() {
    SendToVdrString(CMD_PING, "B");
}

bool BrowserClient::set_input_file(const char* time, const char* input) {
    CONSOLE_DEBUG("Set Input video, time {}, url {}", time, input);

    cookieMutex.lock();
    std::string cookies = ffmpegCookies;
    cookieMutex.unlock();

    if (transcoder != nullptr) {
        stop_video();

        delete transcoder;
        transcoder = nullptr;
    }

    transcoder = new TranscodeFFmpeg(vproto);
    transcoder->set_event_callback(eventCallback);
    transcoder->set_cookies(cookies);
    transcoder->set_user_agent(USER_AGENT);
    return transcoder->set_input(time, input, false);
}

void BrowserClient::pause_video() {
    CONSOLE_DEBUG("Pause video");

    if (transcoder != nullptr) {
        transcoder->pause_video();
    }
}

void BrowserClient::resume_video(const char* position) {
    CONSOLE_DEBUG("Resume video");

    if (transcoder != nullptr) {
        transcoder->resume_video(position);
    }
}

void BrowserClient::stop_video() {
    CONSOLE_DEBUG("Stop video");

    if (transcoder != nullptr) {
        transcoder->stop_video();
    }
}

void BrowserClient::seek_video(const char* ms) {
    CONSOLE_DEBUG("Seek video to {} ms", ms);

    if (transcoder != nullptr) {
        transcoder->seek_video(ms);
    }
}

void BrowserClient::speed_video(const char* speed) {
    CONSOLE_DEBUG("Speed video to speed {}", speed);

    if (transcoder != nullptr) {
        transcoder->speed_video(speed);
    }
}

void BrowserClient::heartbeat() {
    SendToVdrPing();

    // request known app urls and channel information
    SendToVdrString(CMD_STATUS, "SEND_INIT");

    while (heartbeat_running) {
        std::this_thread::sleep_for (std::chrono::seconds(10));
        SendToVdrPing();
    }
}

void BrowserClient::AddAppUrl(std::string id, std::string url) {
    appUrls.emplace(id, url);
}
