#ifndef BROWSER_MAIN_H
#define BROWSER_MAIN_H

#include "include/cef_app.h"
#include "include/wrapper/cef_message_router.h"
#include "include/cef_scheme.h"
#include "include/wrapper/cef_helpers.h"

class MainApp : public CefApp,
                public CefBrowserProcessHandler,
                public CefRenderProcessHandler {
public:
    MainApp();
    ~MainApp();

    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
        return this;
    }

    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override {
        return this;
    }

    // CefBrowserProcessHandler:
    void OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr< CefCommandLine > command_line) override;
    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) override;
    void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) override;
    void OnContextInitialized() override;

    // CefRenderProcessHandler
    void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override;
    void OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override;

private:
    CefRefPtr<CefMessageRouterRendererSide> renderer_side_router;

    IMPLEMENT_REFCOUNTING(MainApp);
};

#endif // BROWSER_MAIN_H
