#include "javascriptlogging.h"

void JavascriptLogging::OnAddressChange(CefRefPtr< CefBrowser > browser, CefRefPtr< CefFrame > frame, const CefString& url) {
    CONSOLE_INFO("JS: Change Frame URL to {}", url.ToString());
}

bool JavascriptLogging::OnConsoleMessage(CefRefPtr<CefBrowser> browser, cef_log_severity_t level, const CefString& message, const CefString& source, int line) {
    switch (level) {
        case LOGSEVERITY_DEBUG:
            CONSOLE_DEBUG("JS DEBUG: Source {}:{}, Message: {}", source.ToString(), line, message.ToString());
            break;

        case LOGSEVERITY_INFO:
            CONSOLE_INFO("JS INFO: Source {}:{}, Message: {}", source.ToString(), line, message.ToString());
            break;

        case LOGSEVERITY_WARNING:
            CONSOLE_INFO("JS WARNING: Source {}:{}, Message: {}", source.ToString(), line, message.ToString());
            break;

        case LOGSEVERITY_ERROR:
            CONSOLE_ERROR("JS ERROR: Source {}:{}, Message: {}", source.ToString(), line, message.ToString());
            break;

        case LOGSEVERITY_FATAL:
            CONSOLE_CRITICAL("JS CRITICAL: Source {}:{}, Message: {}", source.ToString(), line, message.ToString());
            break;

        default:
            break;
    }

    return true;
}

void JavascriptLogging::OnTitleChange(CefRefPtr< CefBrowser > browser, const CefString& title) {
    CONSOLE_DEBUG("JS: Title changed to {}", title.ToString());
}

bool JavascriptLogging::OnTooltip(CefRefPtr< CefBrowser > browser, CefString& text) {
    CONSOLE_INFO("JS: Tooltip {}", text.ToString());

    return true;
}