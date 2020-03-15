/**
 *  CEF OSR implementation used für vdr-plugin-htmlskin
 *
 *  main.cpp
 *
 *  (c) 2019 Robert Hannebauer
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#include <string>
#include <climits>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <thread>
#include <csignal>

#include "globals.h"
#include "main.h"
#include "osrhandler.h"
#include "browserclient.h"
#include "browsercontrol.h"
#include "globals.h"

MainApp::MainApp() {
    CefMessageRouterConfig config;
    config.js_query_function = "cefQuery";
    config.js_cancel_function = "cefQueryCancel";

    renderer_side_router = CefMessageRouterRendererSide::Create(config);
}

MainApp::~MainApp() {

}

void MainApp::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) {
    command_line->AppendSwitchWithValue("autoplay-policy", "no-user-gesture-required");
}

void MainApp::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) {
    renderer_side_router->OnContextCreated(browser, frame, context);
}

void MainApp::OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) {
    renderer_side_router->OnContextReleased(browser, frame, context);
}

bool MainApp::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) {
    return renderer_side_router->OnProcessMessageReceived(browser, frame, source_process, message);
}

CefRefPtr<CefBrowser> browser;

std::string getexepath()
{
    char result[ PATH_MAX ];
    ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
    return std::string(result, static_cast<unsigned long>((count > 0) ? count : 0));
}

void startBrowserControl(BrowserControl ctrl) {
    ctrl.Start();
    kill(getpid(), SIGINT);
}

void quit_handler(int sig) {
    CefQuitMessageLoop();
}

std::string *initUrl = nullptr;

// Entry point function for all processes.
int main(int argc, char *argv[]) {
    signal (SIGQUIT, quit_handler);
    signal(SIGINT, quit_handler);

    bool debugmode = false;

    // try to find some parameters
    for (int i = 0; i < argc; ++i) {
        if (strncmp(argv[i], "--skinurl=", 10) == 0) {
            initUrl = new std::string(argv[i] + 10);
        } else if (strncmp(argv[i], "--debug", 7) == 0) {
            debugmode = true;
        }
    }

    CefMainArgs main_args(argc, argv);
    CefRefPtr<MainApp> app(new MainApp);

    int exit_code = CefExecuteProcess(main_args, app.get(), nullptr);
    if (exit_code >= 0) {
        return exit_code;
    }

    CefSettings settings;

    settings.windowless_rendering_enabled = true;
    settings.no_sandbox = true;

    // read configuration and set CEF global settings
    std::string exepath = getexepath();

    std::ifstream infile(exepath.substr(0, exepath.find_last_of("/")) + "/vdr-osr-browser.config");
    if (infile.is_open()) {
        std::string key;
        std::string value;
        char c;
        while (infile >> key >> c >> value && c == '=') {
            if (key.at(0) != '#') {
                if (key == "resourcepath") {
                    CefString(&settings.resources_dir_path).FromASCII(value.c_str());
                } else if (key == "localespath") {
                    CefString(&settings.locales_dir_path).FromASCII(value.c_str());
                } else if (key == "frameworkpath") {
                    CefString(&settings.framework_dir_path).FromASCII(value.c_str());
                }
            }
        }
    }
    infile.close();

    CefInitialize(main_args, settings, app.get(), nullptr);

    CefBrowserSettings browserSettings;
    CefWindowInfo window_info;
    window_info.SetAsWindowless(0);

    auto osrHandler = new OSRHandler(1920, 1080);
    CefRefPtr<BrowserClient> browserClient = new BrowserClient(osrHandler, debugmode);
    browser = CefBrowserHost::CreateBrowserSync(window_info, browserClient.get(), initUrl ? initUrl->c_str() : "", browserSettings, nullptr, nullptr);

    browser->GetHost()->WasHidden(true);
    browser->GetHost()->SetAudioMuted(true);

    if (initUrl) {
        delete initUrl;
    }

    browserClient->initJavascriptCallback();

    BrowserControl browserControl(browser, osrHandler, browserClient);

    Globals *globals = new Globals();

    {
        std::thread controlThread(startBrowserControl, browserControl);
        CefRunMessageLoop();
        controlThread.detach();
    }

    CefShutdown();

    return 0;
}
