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
#include <iostream>
#include <thread>
#include <csignal>

#include "main.h"
#include "browser.h"
#include "schemehandler.h"

#include "logger.h"

// I'm not sure, if this is really needed anymore
bool NativeJsHandler::Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception) {
    // TODO: Is this really needed?
    if (name == "registerVideo") {
        if (arguments.size() == 1) {
            CefV8Value *val = arguments.at(0);
            fprintf(stderr, "Called registerVideo: %s\n", val->GetStringValue().ToString().c_str());
        }

        return true;
    }

    // Function does not exist.
    return false;
}

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

    // register native javascript function
    CefRefPtr<CefV8Value> object = context->GetGlobal();

    CefRefPtr<CefV8Handler> handler = new NativeJsHandler();
    CefRefPtr<CefV8Value> func = CefV8Value::CreateFunction("registerVideo", handler);
    object->SetValue("registerVideo", func, V8_PROPERTY_ATTRIBUTE_NONE);
}

void MainApp::OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) {
    renderer_side_router->OnContextReleased(browser, frame, context);
}

bool MainApp::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) {
    return renderer_side_router->OnProcessMessageReceived(browser, frame, source_process, message);
}

void MainApp::OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) {
    registrar->AddCustomScheme("client", CEF_SCHEME_OPTION_STANDARD);
}

void MainApp::OnContextInitialized() {
    CefRegisterSchemeHandlerFactory("client", "js",new ClientSchemeHandlerFactory());
    CefRegisterSchemeHandlerFactory("client", "css",new ClientSchemeHandlerFactory());
    CefRegisterSchemeHandlerFactory("client", "movie",new ClientSchemeHandlerFactory());
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
    /*
    CefQuitMessageLoop();

    if (browser != nullptr) {
        delete browser;
        browser = nullptr;
    }
    */

    /*
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);

    raise(sig);
    */
}

std::string *initUrl = nullptr;

// Entry point function for all processes.
int main(int argc, char *argv[]) {
    CONSOLE_INFO("In Main, argc={}, Parameter:", argc);
    for (int i = 0; i < argc; ++i) {
            CONSOLE_INFO("   {}", argv[i]);
    };

    spdlog::level::level_enum log_level = spdlog::level::err;

    // try to find some parameters
    for (int i = 0; i < argc; ++i) {
        if (strncmp(argv[i], "--skinurl=", 10) == 0) {
            initUrl = new std::string(argv[i] + 10);
        } else if (strncmp(argv[i], "--debug", 7) == 0) {
            log_level = spdlog::level::debug;
        } else if (strncmp(argv[i], "--trace", 7) == 0) {
            log_level = spdlog::level::trace;
        } else if (strncmp(argv[i], "--info", 6) == 0) {
            log_level = spdlog::level::info;
        } else if (strncmp(argv[i], "--error", 7) == 0) {
            log_level = spdlog::level::err;
        } else if (strncmp(argv[i], "--critical", 7) == 0) {
            log_level = spdlog::level::critical;
        }
    }

    logger.set_level(log_level);

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

    std::ifstream infile(exepath.substr(0, exepath.find_last_of('/')) + "/vdr-osr-browser.config");
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

    CefRefPtr<BrowserClient> browserClient = new BrowserClient(log_level);
    browser = CefBrowserHost::CreateBrowserSync(window_info, browserClient.get(), initUrl ? initUrl->c_str() : "", browserSettings, nullptr, nullptr);

    browser->GetHost()->WasHidden(true);
    browser->GetHost()->SetAudioMuted(true);

    if (initUrl != nullptr) {
        delete initUrl;
    }

    browserClient->initJavascriptCallback();
    BrowserControl browserControl(browser, browserClient);

    {
        /*
         * Wird zwar aufgerufen, aber funktioniert nicht, wie gewünscht
         * Eine weitere Analyse wird auf später verschoben.
         */
        /*
        signal(SIGINT, quit_handler);
        signal(SIGQUIT, quit_handler);
        */

        std::thread controlThread(startBrowserControl, browserControl);
        CefRunMessageLoop();
        controlThread.detach();
    }

    printf("Shutdown...");

    CefShutdown();

    return 0;
}
