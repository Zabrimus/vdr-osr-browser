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
#include <sys/stat.h>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include "main.h"
#include "browser.h"
#include "schemehandler.h"
#include "globaldefs.h"
#include "nativejshandler.h"

const auto VERSION = "0.1.0pre1";

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

    CefRefPtr<CefV8Handler> handler = new NativeJSHandler();
    CefRefPtr<CefV8Value> func = CefV8Value::CreateFunction("getAppUrl", handler);
    object->SetValue("getAppUrl", func, V8_PROPERTY_ATTRIBUTE_NONE);
}

void MainApp::OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) {
    renderer_side_router->OnContextReleased(browser, frame, context);
}

bool MainApp::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) {
    bool messageProcessed = renderer_side_router->OnProcessMessageReceived(browser, frame, source_process, message);

    if (messageProcessed) {
        return true;
    }

    /* remove, if found a solution for std::map to be global
    const std::string &message_name = message->GetName();

    if (message_name == "addAppUrl") {
        CefRefPtr<CefListValue> args = message->GetArgumentList();
        auto str = args->GetString(0).ToString();

        auto delimiterPos = str.find(":");
        auto id = str.substr(0, delimiterPos);
        auto u = str.substr(delimiterPos + 1);
        trim(id);
        trim(u);
        appUrls->emplace(id, u);

        return true;
    } else if (message_name == "clearAppUrl") {
        appUrls->clear();
    }
    */

    return false;
}

void MainApp::OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) {
    // registrar->AddCustomScheme("client", CEF_SCHEME_OPTION_STANDARD);
    registrar->AddCustomScheme("client", CEF_SCHEME_OPTION_STANDARD | CEF_SCHEME_OPTION_SECURE | CEF_SCHEME_OPTION_CORS_ENABLED);
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

void startBrowserPaintUpdater(BrowserPaintUpdater paintUpdater) {
    paintUpdater.Start();
    kill(getpid(), SIGINT);
}

/*
void quit_handler(int sig) {
    *
    CefQuitMessageLoop();

    if (browser != nullptr) {
        delete browser;
        browser = nullptr;
    }
    *

    *
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);

    raise(sig);
    *
}
*/

// do some very first checks
inline bool file_exists(const char* name) {
    std::ifstream f(name);
    return f.good();
}

void checkInstallation() {
    std::string exepath = getexepath();

    // check if configuration files exists
    std::ifstream infile1(exepath.substr(0, exepath.find_last_of('/')) + "/vdr-osr-browser.config");
    if (!infile1.is_open()) {
        fprintf(stderr, "Unable to open configuration file 'vdr-osr-browser.config'. Aborting...");
        exit(1);
    }
    infile1.close();

    // check if sockets can be opened
    int socketId;

    if ((socketId = nn_socket(AF_SP, NN_PUSH)) < 0) {
        fprintf(stderr, "Unable to create nanomsg NN_PUSH socket. Aborting...\n");
        exit(1);
    }

    if ((nn_bind(socketId, TO_VDR_CHANNEL)) < 0) {
        exit(1);
    }

    nn_close(socketId);

    if ((socketId = nn_socket(AF_SP, NN_PULL)) < 0) {
        fprintf(stderr, "Unable to create nanomsg NN_PULL socket. Aborting...\n");
        exit(1);
    }

    if ((nn_connect(socketId, FROM_VDR_CHANNEL)) < 0) {
        exit(1);
    }

    nn_close(socketId);
}

std::string *initUrl = nullptr;
std::string *logFile = nullptr;

// Entry point function for all processes.
int main(int argc, char *argv[]) {
    spdlog::level::level_enum log_level = spdlog::level::info;

    // FIXME: Dies ist nur ein Test!
    signal(SIGPIPE, SIG_IGN);

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
        } else if (strncmp(argv[i], "--logfile=", 10) == 0) {
            logFile = new std::string(argv[i] + 10);
        }
    }

    if (logFile != nullptr) {
        logger.switchToFileLogger(*logFile);
        delete logFile;
    }

    logger.set_level(log_level);

    CONSOLE_INFO("vdrosrbrowser version {} started", VERSION);

    CONSOLE_INFO("In Main, argc={}, Parameter:", argc);
    for (int i = 0; i < argc; ++i) {
        CONSOLE_INFO("   {}", argv[i]);
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
    // settings.multi_threaded_message_loop = true;

    checkInstallation();

    // read configuration and set CEF global settings
    std::string exepath = getexepath();
    std::string path = exepath.substr(0, exepath.find_last_of('/'));
    std::string localespath = exepath.substr(0, exepath.find_last_of('/')) + "/locales";
    std::string cache_path = exepath.substr(0, exepath.find_last_of('/')) + "/cache";
    std::string profile_path = exepath.substr(0, exepath.find_last_of('/')) + "/profile";

    CefString(&settings.cache_path).FromASCII(cache_path.c_str());
    CefString(&settings.user_data_path).FromASCII(profile_path.c_str());
    CefString(&settings.user_agent).FromASCII(USER_AGENT);

    std::ifstream infile(path + "/vdr-osr-browser.config");
    if (infile.is_open()) {
        std::string line;
        while (getline(infile, line)) {
            if (line[0] == '#' || line.empty()) {
                continue;
            }

            auto delimiterPos = line.find("=");
            auto key = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);
            trim(key);
            trim(value);

            if (key.at(0) != '#') {
                if (key == "resourcepath") {
                    if (value == ".") {
                        CefString(&settings.resources_dir_path).FromASCII(path.c_str());
                    } else {
                        CefString(&settings.resources_dir_path).FromASCII(value.c_str());
                    }
                } else if (key == "localespath") {
                    if (value == ".") {
                        CefString(&settings.locales_dir_path).FromASCII(localespath.c_str());
                    } else {
                        CefString(&settings.locales_dir_path).FromASCII(value.c_str());
                    }
                } else if (key == "frameworkpath") {
                    if (value == ".") {
                        CefString(&settings.framework_dir_path).FromASCII(path.c_str());
                    } else {
                        CefString(&settings.framework_dir_path).FromASCII(value.c_str());
                    }
                }
            }
        }
    }
    infile.close();

    CefInitialize(main_args, settings, app.get(), nullptr);

    CefBrowserSettings browserSettings;
    browserSettings.windowless_frame_rate = 25;

    CefWindowInfo window_info;
    window_info.SetAsWindowless(0);
    window_info.shared_texture_enabled = true;

    // TODO:
    //  Das funktioniert leider nicht. Es gibt dazu ein Ticket #2800
    //  https://bitbucket.org/chromiumembedded/cef/issues/2800/osr-sendexternalbeginframe-does-not-call
    //
    //  Siehe auch den Forenbeitrag
    //  https://magpcss.org/ceforum/viewtopic.php?f=6&t=17985
    //
    //  Der Thread, der regelmäßig SendExternalBeginFrame aufrufen soll, befindet sich im BrowserPaintUpdater.
    //  window_info.external_begin_frame_enabled = true;

    CefRefPtr<BrowserClient> browserClient = new BrowserClient(log_level);
    browser = CefBrowserHost::CreateBrowserSync(window_info, browserClient.get(), initUrl ? initUrl->c_str() : "", browserSettings, nullptr, nullptr);

    browser->GetHost()->WasHidden(true);
    browser->GetHost()->SetAudioMuted(false);

    if (initUrl != nullptr) {
        delete initUrl;
    }

    browserClient->initJavascriptCallback();
    BrowserControl browserControl(browser, browserClient);
    // BrowserPaintUpdater paintUpdater(browser);

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
        controlThread.detach();

        // std::thread paintUpdateThread(startBrowserPaintUpdater, paintUpdater);
        // paintUpdateThread.detach();

        CefRunMessageLoop();
    }

    printf("Shutdown...");

    CefShutdown();

    return 0;
}
