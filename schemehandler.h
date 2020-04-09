#ifndef SCHEMEHANDLER_H
#define SCHEMEHANDLER_H

#include "cef/cef_scheme.h"
#include "include/wrapper/cef_helpers.h"

class ClientSchemeHandlerFactory : public CefSchemeHandlerFactory {
public:
    ClientSchemeHandlerFactory() = default;

    CefRefPtr<CefResourceHandler> Create(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& scheme_name, CefRefPtr<CefRequest> request) override;

private:
    IMPLEMENT_REFCOUNTING(ClientSchemeHandlerFactory);
    DISALLOW_COPY_AND_ASSIGN(ClientSchemeHandlerFactory);
};

class ClientSchemeHandler : public CefResourceHandler {
public:
    ClientSchemeHandler();

    bool ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) override;
    void GetResponseHeaders(CefRefPtr<CefResponse> response, int64& response_length, CefString& redirectUrl) override;
    void Cancel() override;
    bool ReadResponse(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefCallback> callback) override;

private:
    std::string data;
    std::string mime_type;
    int response_code;
    size_t offset;

    bool FileExists(const char* path);
    bool ReadFileToString(const char* path, std::string& data);
    bool GetResourceDir(std::string& dir);
    std::string GetMimeType(const std::string& resource_path);

    bool GetResourceString(const std::string& resource_path, std::string& out_data);
    CefRefPtr<CefStreamReader> GetResourceReader(const std::string& resource_path);
    CefRefPtr<CefResourceHandler> GetResourceHandler(const std::string& resource_path);

    IMPLEMENT_REFCOUNTING(ClientSchemeHandler);
    DISALLOW_COPY_AND_ASSIGN(ClientSchemeHandler);
};

#endif // SCHEMEHANDLER_H
