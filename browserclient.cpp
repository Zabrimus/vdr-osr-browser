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
#include <curl/curl.h>
#include "browserclient.h"
#include "include/wrapper/cef_helpers.h"

#define HEADERSIZE (4 * 1024)

struct MemoryStruct {
    char *memory;
    size_t size;
};

class HbbtvCurl {
public:
    HbbtvCurl();

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

std::map<std::string, std::string> HbbtvCurl::response_header;
std::string HbbtvCurl::response_content;

HbbtvCurl::HbbtvCurl() = default;

void HbbtvCurl::LoadUrl(std::string url, std::map<std::string, std::string>* header) {
    struct MemoryStruct contentdata {
            (char*)malloc(1), 0
    };

    char *headerdata = (char*)malloc(HEADERSIZE);
    memset(headerdata, 0, HEADERSIZE);

    CURL *curl_handle;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
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
    }

    response_content =  std::string(contentdata.memory);

    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

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


BrowserClient::BrowserClient(OSRHandler *renderHandler, bool _hbbtv) {
    m_renderHandler = renderHandler;
    hbbtv = _hbbtv;
}



// getter for the different handler
CefRefPtr<CefRenderHandler> BrowserClient::GetRenderHandler() {
    return m_renderHandler;
}



CefRefPtr<CefResourceHandler> BrowserClient::GetResourceHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request) {
    if (hbbtv) {
        // test if the special handler is necesary
        auto url = request->GetURL().ToString();

        // filter http(s) requests
        if (url.find("http", 0) == std::string::npos) {
            // no http request -> use default handler
            return CefRequestHandler::GetResourceHandler(browser, frame, request);
        }

        // filter known file types, this has to adapted accordingly
        auto handle = (url.find("view-source:'", 0) == std::string::npos) &&
                      (url.find(".json", 0) == std::string::npos) &&
                      (url.find(".js", 0) == std::string::npos) &&
                      (url.find(".css", 0) == std::string::npos) &&
                      (url.find(".ico", 0) == std::string::npos) &&
                      (url.find(".jpg", 0) == std::string::npos) &&
                      (url.find(".png", 0 ) == std::string::npos) &&
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
                      (url.find(".7z", 0) == std::string::npos);

        if (!handle) {
            // use default handler
            return CefRequestHandler::GetResourceHandler(browser, frame, request);
        } else {
            // use special handler
            return this;
        }
    } else {
        // use default handler
        return CefRequestHandler::GetResourceHandler(browser, frame, request);
    }
}

CefRefPtr<CefRequestHandler> BrowserClient::GetRequestHandler() {
    return this;
}



// CefLoadHandler
void BrowserClient::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) {
    CEF_REQUIRE_UI_THREAD();
}

void BrowserClient::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl) {
    CEF_REQUIRE_UI_THREAD();

    if (errorCode == ERR_ABORTED)
        return;
}



// CefResourceHandler
bool BrowserClient::ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    {
        HbbtvCurl curl;
        curl.LoadUrl(request->GetURL().ToString(), nullptr);

        responseHeader = curl.GetResponseHeader();
        responseContent = curl.GetResponseContent();
        redirectUrl = curl.GetRedirectUrl();
    }

    callback->Continue();
    return true;
}

void BrowserClient::GetResponseHeaders(CefRefPtr<CefResponse> response, int64 &response_length, CefString &_redirectUrl) {
    // reset offset
    offset = 0;

    if (redirectUrl.length() > 0) {
        _redirectUrl.FromString(redirectUrl.c_str());
    }

    std::string contentType = responseHeader["Content-Type"];

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
        memcpy(data_out, responseContent.c_str() + offset, static_cast<size_t>(transfer_size));
        offset += transfer_size;

        bytes_read = transfer_size;
        has_data = true;
    }

    return has_data;
}

CefRequestHandler::ReturnValue BrowserClient::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefRequestCallback> callback) {
    if (hbbtv) {
        // Customize the request header
        CefRequest::HeaderMap hdrMap;
        request->GetHeaderMap(hdrMap);

        // User-Agent
        std::string newUserAgent("HbbTV/1.4.1 (+DRM;Samsung;SmartTV2015;T-HKM6DEUC-1490.3;;) OsrTvViewer");
        hdrMap.erase("User-Agent");
        hdrMap.insert(std::make_pair("User-Agent", newUserAgent.c_str()));
        request->SetHeaderMap(hdrMap);
    }

    return RV_CONTINUE;
}

/*

#include <algorithm>
#include "include/wrapper/cef_helpers.h"
#include "browserclient.h"
#include "hbbtv_urlclient.h"

BrowserClient::BrowserClient(OSRHandler *renderHandler, bool hbbtv) {
    m_renderHandler = renderHandler;
    isHbbtvClient = hbbtv;
}

CefRefPtr<CefRenderHandler> BrowserClient::GetRenderHandler() {
    return m_renderHandler;
}

CefRefPtr<CefRequestHandler> BrowserClient::GetRequestHandler() {
    return this;
}

CefRefPtr<CefLoadHandler> BrowserClient::GetLoadHandler() {
    return this;
}

CefRequestHandler::ReturnValue BrowserClient::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefRequestCallback> callback) {
    if (isHbbtvClient) {
        // Customize the request header
        CefRequest::HeaderMap hdrMap;
        request->GetHeaderMap(hdrMap);

        // User-Agent
        std::string newUserAgent("HbbTV/1.4.1 (+DRM;Samsung;SmartTV2015;T-HKM6DEUC-1490.3;;) OsrTvViewer");
        hdrMap.erase("User-Agent");
        hdrMap.insert(std::make_pair("User-Agent", newUserAgent.c_str()));
        request->SetHeaderMap(hdrMap);
    }

    return RV_CONTINUE;
}

bool BrowserClient::OnResourceResponse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response) {
    return false;
}

bool BrowserClient::OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool user_gesture, bool is_redirect) {
    CEF_REQUIRE_UI_THREAD();

    fprintf(stderr, "OnBeforeBrowse\n");

    if (isHbbtvClient) {
        auto url = request->GetURL().ToString();

        // filter http(s) requests
        if (url.find("http", 0) == std::string::npos) {
            // no http request
            return false;
        }

        // filter known file types, this has to adapted accordingly
        auto ignore =
                url.find("view-source:'",0) == std::string::npos ||
                url.find(".json", 0) == std::string::npos ||
                url.find(".js", 0) == std::string::npos ||
                url.find(".css", 0) == std::string::npos ||
                url.find(".ico", 0) == std::string::npos ||
                url.find(".jpg", 0) == std::string::npos ||
                url.find(".png", 0 ) == std::string::npos ||
                url.find(".gif", 0) == std::string::npos ||
                url.find(".webp", 0) == std::string::npos ||
                url.find(".m3u8", 0) == std::string::npos ||
                url.find(".mpd", 0) == std::string::npos ||
                url.find(".ts", 0) == std::string::npos ||
                url.find(".mpg", 0) == std::string::npos ||
                url.find(".mp3", 0) == std::string::npos ||
                url.find(".mp4", 0) == std::string::npos ||
                url.find(".mov", 0) == std::string::npos ||
                url.find(".avi", 0) == std::string::npos ||
                url.find(".pdf", 0) == std::string::npos ||
                url.find(".ppt", 0) == std::string::npos ||
                url.find(".pptx", 0) == std::string::npos ||
                url.find(".xls", 0) == std::string::npos ||
                url.find(".xlsx", 0) == std::string::npos ||
                url.find(".doc", 0) == std::string::npos ||
                url.find(".docx", 0) == std::string::npos ||
                url.find(".zip", 0) == std::string::npos ||
                url.find(".rar", 0) == std::string::npos ||
                url.find(".7z", 0) == std::string::npos;

        printf("Return ignore.....%s\n", url.c_str());

        // cancel request if not found and reload the URL in OnLoadError
        return !ignore;
    }

    return CefRequestHandler::OnBeforeBrowse(browser, frame, request, user_gesture, is_redirect);
}

void BrowserClient::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) {
    CEF_REQUIRE_UI_THREAD();
    fprintf(stderr, "Finished loading with code %d\n", httpStatusCode);
}

void BrowserClient::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl) {
    CEF_REQUIRE_UI_THREAD();

    printf("In OnLoadError\n");

    if (errorCode != ERR_ABORTED) {
        fprintf(stderr, "Error loading URL '%s' with (errorCode %d) %s\n", std::string(failedUrl).c_str(), errorCode,
                std::string(errorText).c_str());
        return;
    }

    printf("Reload URL\n");

    // reload URL
    CefRefPtr<CefRequest> request = CefRequest::Create();
    CefRefPtr<HbbtvUrlClient> client = new HbbtvUrlClient();
    CefRefPtr<CefURLRequest> url_request = CefURLRequest::Create(request, client, nullptr);

    printf("URL: %s\n", url_request->GetResponse()->GetURL().ToString().c_str());
    printf("MIME: %s\n", url_request->GetResponse()->GetMimeType().ToString().c_str());
    printf("Status: %d\n", url_request->GetResponse()->GetStatus());
    printf("StatusText: %s\n", url_request->GetResponse()->GetStatusText().ToString().c_str());
    printf("DOWNLOADED DATA: %s\n", client->GetDownloadedData().c_str());

    // frame->LoadString(client->GetDownloadedData(), request->GetURL());
}

void BrowserClient::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) {
    CefLoadHandler::OnLoadingStateChange(browser, isLoading, canGoBack, canGoForward);
}

void BrowserClient::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::TransitionType transition_type) {
    CefLoadHandler::OnLoadStart(browser, frame, transition_type);
}



CefRefPtr<CefResourceHandler> BrowserClient::GetResourceHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request) {
    return this;
}

void BrowserClient::GetResponseHeaders(CefRefPtr<CefResponse> response, int64 &response_length, CefString &redirectUrl) {
    printf("GetResponseHeaders...");
}

bool BrowserClient::ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    return true;
}

bool BrowserClient::ReadResponse(void *data_out, int bytes_to_read, int &bytes_read, CefRefPtr<CefCallback> callback) {
    return true;
}

bool BrowserClient::CanGetCookie(const CefCookie &cookie) {
    return CefResourceHandler::CanGetCookie(cookie);
}

bool BrowserClient::CanSetCookie(const CefCookie &cookie) {
    return CefResourceHandler::CanSetCookie(cookie);
}

void BrowserClient::Cancel() {
}
*/