#include <map>
#include "include/cef_resource_handler.h"
#include "include/cef_parser.h"
#include "include/wrapper/cef_stream_resource_handler.h"
#include "schemehandler.h"

CefRefPtr<CefResourceHandler> ClientSchemeHandlerFactory::Create(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& scheme_name, CefRefPtr<CefRequest> request) {
    CEF_REQUIRE_IO_THREAD();

    return new ClientSchemeHandler();
}

ClientSchemeHandler:: ClientSchemeHandler() : offset(0), binary_offset(0) {
}

bool ClientSchemeHandler::Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback) {
    std::string url = request->GetURL();

    if ((strstr(url.c_str(), "client://js/") != NULL) || (strstr(url.c_str(), "client://css/") != NULL)) {
        if (GetResourceString(url.substr(9), data)) {
            handle_request = true;

            if (strstr(url.c_str(), ".js.map")) {
                mime_type = "application/json";
            } else {
                mime_type = GetMimeType(url);
            }
            response_code = 200;
        }
    } else if (strstr(url.c_str(), "client://movie/fail") != NULL) {
        // return 400
        handle_request = true;
        response_code = 400;
        mime_type = "";
    } else if (strstr(url.c_str(), "client://movie/transparent.webm") != NULL) {
        handle_request = true;
        response_code = 200;
        mime_type = "video/webm";

        ReadFileToData("movie/transparent.webm");
    } else {
        handle_request = true;
        return false;
    }

    return true;
}

void ClientSchemeHandler::GetResponseHeaders(CefRefPtr<CefResponse> response, int64& response_length, CefString& redirectUrl) {
    CEF_REQUIRE_IO_THREAD();

    if (!mime_type.empty()) {
        response->SetMimeType(mime_type);
    }

    if (!data.empty()) {
        response_length = data.length();
    } else if (binary_data_length > 0) {
        response_length = binary_data_length;
    }

    response->SetStatus(response_code);
}

void ClientSchemeHandler::Cancel() {
    CEF_REQUIRE_IO_THREAD();
}

bool ClientSchemeHandler::Skip(int64 bytes_to_skip, int64& bytes_skipped, CefRefPtr<CefResourceSkipCallback> callback) {
    // data was already read into memory
    binary_offset = bytes_skipped = bytes_to_skip;

    return true;
}

bool ClientSchemeHandler::Read(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefResourceReadCallback> callback) {
    bool has_data = false;
    bytes_read = 0;

    if (data.length() > 0) {
        if (offset < data.length()) {
            int transfer_size = std::min(bytes_to_read, static_cast<int>(data.length() - offset));

            memcpy(data_out, data.c_str() + offset, transfer_size);
            offset += transfer_size;

            bytes_read = transfer_size;
            has_data = true;
        }
    } else if (binary_data_length > 0) {
        if (binary_offset < binary_data_length) {
            int transfer_size = std::min(bytes_to_read, static_cast<int>(binary_data_length - binary_offset));

            memcpy(data_out, binary_data + binary_offset, transfer_size);
            binary_offset += transfer_size;

            bytes_read = transfer_size;
            has_data = true;
        }
    }

    return has_data;
}

bool ClientSchemeHandler::GetResourceString(const std::string& resource_path, std::string& out_data) {
    std::string path;
    if (!GetResourceDir(path))
        return false;

    path.append("/");
    path.append(resource_path);

    return ReadFileToString(path.c_str(), out_data);
}

CefRefPtr<CefStreamReader> ClientSchemeHandler::GetResourceReader(const std::string& resource_path) {
    std::string path;
    if (!GetResourceDir(path))
        return NULL;

    path.append("/");
    path.append(resource_path);

    if (!FileExists(path.c_str()))
        return NULL;

    return CefStreamReader::CreateForFile(path);
}

CefRefPtr<CefResourceHandler> ClientSchemeHandler::GetResourceHandler(const std::string& resource_path) {
    CefRefPtr<CefStreamReader> reader = GetResourceReader(resource_path);
    if (!reader)
        return NULL;

    return new CefStreamResourceHandler(GetMimeType(resource_path), reader);
}

bool ClientSchemeHandler::GetResourceDir(std::string& dir) {
    char buff[1024];

    ssize_t len = readlink("/proc/self/exe", buff, sizeof(buff) - 1);
    if (len == -1)
        return false;

    buff[len] = 0;
    dir = std::string(buff);
    dir = dir.substr(0, dir.find_last_of("/"));

    return true;
}

bool ClientSchemeHandler::FileExists(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

bool ClientSchemeHandler::ReadFileToString(const char* path, std::string& data) {
    FILE* file = fopen(path, "rb");
    if (!file)
        return false;

    char buf[1 << 16];
    size_t len;
    while ((len = fread(buf, 1, sizeof(buf), file)) > 0)
        data.append(buf, len);
    fclose(file);

    return true;
}

bool ClientSchemeHandler::ReadFileToData(const char* path) {
    if (binary_data != nullptr) {
        free(binary_data);
        binary_data = nullptr;
    }
    binary_data_length = 0;

    if (!FileExists(path)) {
        return false;
    }

    FILE* file;
    file = fopen(path, "rb");

    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    binary_data = new uint8_t[size];

    fread(binary_data, size, 1, file);
    fclose(file);

    binary_data_length = size;

    return true;
}


std::string ClientSchemeHandler::GetMimeType(const std::string& resource_path) {
    std::string mime_type;
    size_t sep = resource_path.find_last_of(".");

    if (sep != std::string::npos) {
        mime_type = CefGetMimeType(resource_path.substr(sep + 1));
        if (!mime_type.empty())
            return mime_type;
    }

    return "text/html";
}