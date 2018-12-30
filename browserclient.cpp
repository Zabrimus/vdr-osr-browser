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

#include "browserclient.h"
#include "include/wrapper/cef_helpers.h"

BrowserClient::BrowserClient(OSRHandler *renderHandler) {
    m_renderHandler = renderHandler;
}

CefRefPtr<CefRenderHandler> BrowserClient::GetRenderHandler() {
    return m_renderHandler;
}

void BrowserClient::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) {
    CEF_REQUIRE_UI_THREAD();

    fprintf(stderr, "Finished loading with code %d\n", httpStatusCode);
}

void BrowserClient::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl) {
    CEF_REQUIRE_UI_THREAD();

    if (errorCode == ERR_ABORTED)
        return;

    fprintf(stderr, "Error loading URL '%s' with (errorCode %d) %s\n", std::string(failedUrl).c_str(), errorCode, std::string(errorText).c_str());
}
