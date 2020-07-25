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

    std::ifstream infile2(exepath.substr(0, exepath.find_last_of("/")) + "/vdr-osr-ffmpeg.config");
    if (infile2.is_open()) {
        std::string line;
        while (getline(infile2, line)) {
            if (line[0] == '#' || line.empty()) {
                continue;
            }

            auto delimiterPos = line.find("=");
            auto key = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);
            trim(key);
            trim(value);

            if (key == "ffmpeg_executable") {
                if (!file_exists(value.c_str())) {
                    fprintf(stderr, "Configured ffmpeg executable '%s' does not exists. Aborting...\n", value.c_str());
                    exit(1);
                }
            } else if (key == "ffprobe_executable") {
                if (!file_exists(value.c_str())) {
                    fprintf(stderr, "Configured ffprobe executable '%s' does not exists. Aborting...\n", value.c_str());
                    exit(1);
                }
            }
        }
    } else {
        fprintf(stderr, "Unable to open configuration file 'vdr-osr-ffmpeg.config'. Aborting...");
        exit(1);
    }
    infile2.close();

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
    spdlog::level::level_enum log_level = spdlog::level::err;

    std::string *vproto = nullptr;

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
        } else if (strncmp(argv[i], "--video=", 8) == 0) {
            vproto = new std::string(argv[i] + 8);
        }
    }

    if (vproto == nullptr) {
        vproto = new std::string("UDP");
    }

    if (logFile != nullptr) {
        logger.switchToFileLogger(*logFile);
        delete logFile;
    }

    logger.set_level(log_level);

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
    CefWindowInfo window_info;
    window_info.SetAsWindowless(0);

    CefRefPtr<BrowserClient> browserClient = new BrowserClient(log_level, *vproto);
    browser = CefBrowserHost::CreateBrowserSync(window_info, browserClient.get(), initUrl ? initUrl->c_str() : "", browserSettings, nullptr, nullptr);

    browser->GetHost()->WasHidden(true);
    browser->GetHost()->SetAudioMuted(true);

    if (initUrl != nullptr) {
        delete initUrl;
    }

    if (vproto != nullptr) {
        delete vproto;
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
