#ifndef BROWSER_MAIN_H
#define BROWSER_MAIN_H

#include "include/cef_app.h"

class MainApp : public CefApp, public CefBrowserProcessHandler {
 public:
  MainApp() {};

  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
    return this;
  }

  // CefBrowserProcessHandler:
  void OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr< CefCommandLine > command_line ) override;

 private:
  IMPLEMENT_REFCOUNTING(MainApp);
};

#endif // BROWSER_MAIN_H
