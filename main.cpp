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

#include "include/cef_app.h"

#include "osrhandler.h"
#include "browserclient.h"
#include "browsercontrol.h"

CefRefPtr<CefBrowser> browser;

std::string getexepath()
{
    char result[ PATH_MAX ];
    ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
    return std::string(result, static_cast<unsigned long>((count > 0) ? count : 0));
}

void startBrowserControl(BrowserControl ctrl) {
    auto commandurl = std::string("ipc:///tmp/vdrosr_command.ipc");
    ctrl.Start(commandurl);
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

    // try to find --skinurl parameter
    for (int i = 0; i < argc; ++i) {
        if (strncmp(argv[i], "--skinurl=", 10) == 0) {
            initUrl = new std::string(argv[i] + 10);
        }
    }

    CefMainArgs main_args(argc, argv);

    int exit_code = CefExecuteProcess(main_args, nullptr, nullptr);
    if (exit_code >= 0) {
        return exit_code;
    }

    CefSettings settings;

    settings.windowless_rendering_enabled = true;
    settings.no_sandbox = true;

    // read configuration and set CEF global settings
    std::string exepath = getexepath();

    std::ifstream infile(exepath.substr(0, exepath.find_last_of("/")) + "/cef-osr-browser.config");
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

    CefInitialize(main_args, settings, nullptr, nullptr);

    CefBrowserSettings browserSettings;
    CefWindowInfo window_info;
    window_info.SetAsWindowless(0);

    auto osrHandler = new OSRHandler(850, 600);
    CefRefPtr<BrowserClient> browserClient = new BrowserClient(osrHandler, true);

    browser = CefBrowserHost::CreateBrowserSync(window_info, browserClient.get(), initUrl ? initUrl->c_str() : "", browserSettings, nullptr);
    browser->GetHost()->WasHidden(true);

    if (initUrl) {
        delete initUrl;
    }

    BrowserControl browserControl(browser, osrHandler);

    {
        std::thread controlThread(startBrowserControl, browserControl);
        CefRunMessageLoop();
        controlThread.detach();
    }

    CefShutdown();

    return 0;
}
