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

private:
    static size_t WriteContentCallback(void *contents, size_t size, size_t nmemb, void *userp);
    static size_t WriteHeaderCallback(void *contents, size_t size, size_t nmemb, void *userp);

    static std::map<std::string, std::string> response_header;
    static std::string response_content;
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
        char *redirect_url = nullptr;
        curl_easy_getinfo(curl_handle, CURLINFO_REDIRECT_URL, &redirect_url);
        if (redirect_url) {
            // clear buffer
            memset(headerdata, 0, HEADERSIZE);

            free (contentdata.memory);
            contentdata.size = 0;
            contentdata.memory = (char*)malloc(1);

            response_header.clear();

            // reload url
            curl_easy_setopt(curl_handle, CURLOPT_URL, redirect_url);
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


BrowserClient::BrowserClient(OSRHandler *renderHandler) {
    m_renderHandler = renderHandler;
}



// getter for the different handler
CefRefPtr<CefRenderHandler> BrowserClient::GetRenderHandler() {
    return m_renderHandler;
}

CefRefPtr<CefResourceHandler> BrowserClient::GetResourceHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request) {
    return this;
}

CefRefPtr<CefRequestHandler> BrowserClient::GetRequestHandler() {
    return this;
}



// CefLoadHandler
void BrowserClient::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) {
    std::cout << "BrowserClient::OnLoadEnd" << std::endl;

    CEF_REQUIRE_UI_THREAD();

    fprintf(stderr, "Finished loading with code %d\n", httpStatusCode);
}

void BrowserClient::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl) {
    std::cout << "BrowserClient::OnLoadError" << std::endl;

    CEF_REQUIRE_UI_THREAD();

    if (errorCode == ERR_ABORTED)
        return;

    fprintf(stderr, "Error loading URL '%s' with (errorCode %d) %s\n", std::string(failedUrl).c_str(), errorCode, std::string(errorText).c_str());
}



// CefResourceHandler
bool BrowserClient::ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    std::cout << "BrowserClient::ProcessRequest" << std::endl;

    {
        HbbtvCurl curl;
        curl.LoadUrl(request->GetURL().ToString(), nullptr);

        responseHeader = curl.GetResponseHeader();
        responseContent = curl.GetResponseContent();
    }

    callback->Continue();
    return true;

    // return false;
}

void BrowserClient::GetResponseHeaders(CefRefPtr<CefResponse> response, int64 &response_length, CefString &redirectUrl) {
    std::cout << "BrowserClient::GetResponseHeaders" << std::endl;

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
    std::cout << "BrowserClient::ReadResponse" << std::endl;

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
