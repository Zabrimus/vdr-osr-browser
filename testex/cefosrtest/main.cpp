#include "include/cef_app.h"
#include "include/cef_render_handler.h"
#include "include/cef_audio_handler.h"
#include <chrono>
#include <iostream>

std::string getCurrentDateTime() {
    time_t tt;
    struct tm *st;

    tt = time(NULL);
    st = localtime(&tt);
    return asctime(st);
}

class BrowserClient : public CefApp,
                      public CefClient,
                      public CefBrowserProcessHandler,
                      public CefRenderProcessHandler,
                      public CefAudioHandler,
                      public CefRenderHandler
{
    public:
        BrowserClient() { }
        ~BrowserClient() { };

        CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
            return this;
        }

        CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override {
            return this;
        }

        CefRefPtr<CefRenderHandler> GetRenderHandler() override {
            return this;
        }

        CefRefPtr<CefAudioHandler> GetAudioHandler() override {
            return this;
        }

        void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override {
            rect = CefRect(0, 0, 1280, 720);
        }

        void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height) override {
            // printf("OnPaint\n");
        }

        bool GetAudioParameters(CefRefPtr<CefBrowser> browser, CefAudioParameters &params) override {
            return true;
        }

        void OnAudioStreamStarted(CefRefPtr<CefBrowser> browser, const CefAudioParameters &params, int channels) override {
            std::cout << "OnAudioStreamStarted at " << getCurrentDateTime() << '\n';
        }

        void OnAudioStreamPacket(CefRefPtr<CefBrowser> browser, const float **data, int frames, int64 pts) override {
        }

        void OnAudioStreamStopped(CefRefPtr<CefBrowser> browser) override {
            std::cout << "OnAudioStreamStopped at " << getCurrentDateTime() << '\n';
        }

        void OnAudioStreamError(CefRefPtr<CefBrowser> browser, const CefString &message) override {
            std::cout << "OnAudioStreamError at " << getCurrentDateTime() << '\n';
        }

    IMPLEMENT_REFCOUNTING(BrowserClient);
};


CefRefPtr<CefBrowser> browser;
std::string *initUrl = nullptr;

int main(int argc, char *argv[]) {

    for (int i = 0; i < argc; ++i) {
        if (strncmp(argv[i], "--url=", 6) == 0) {
            initUrl = new std::string(argv[i] + 6);
        }
    }

    CefMainArgs main_args(argc, argv);

    CefRefPtr<BrowserClient> browserClient = new BrowserClient();

    int exit_code = CefExecuteProcess(main_args, browserClient, nullptr);
    if (exit_code >= 0) {
        return exit_code;
    }

    CefSettings settings;

    settings.windowless_rendering_enabled = true;
    settings.no_sandbox = true;
    // settings.multi_threaded_message_loop = true;

    CefInitialize(main_args, settings, browserClient, nullptr);

    CefBrowserSettings browserSettings;
    browserSettings.windowless_frame_rate = 25;

    CefWindowInfo window_info;
    window_info.SetAsWindowless(0);
    window_info.shared_texture_enabled = true;

    browser = CefBrowserHost::CreateBrowserSync(window_info, browserClient.get(), initUrl ? initUrl->c_str() : "", browserSettings, nullptr, nullptr);

    browser->GetHost()->WasHidden(false);
    browser->GetHost()->SetAudioMuted(false);

    CefRunMessageLoop();
    CefShutdown();

    return 0;
}
