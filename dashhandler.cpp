#include <stdio.h>
#include <unistd.h>
#include "dashhandler.h"
#include "logger.h"

DashHandler videoDashHandler;
DashHandler audioDashHandler;

DashHandler::DashHandler() {
}

DashHandler::~DashHandler() {
    Clear();

    if (curl) {
        delete curl;
    }
}

void DashHandler::setFilename(std::string filename) {
    this->filename = filename;
    unlink(filename.c_str());
    curl = new DashCurl(filename);
}

void DashHandler::InitLoadThread(int streamidx, int startSegment) {
    DashStream *stream = GetStream(streamidx);

    if (stream == nullptr) {
        return;
    }

    // delete first entries
    stream->DeleteFirst(startSegment - 1);

    // save base segment
    curl->LoadUrl(stream->GetBaseSegment());

    // save at least 60 seconds
    int count = (60 / stream->GetSegmentDuration()) + 1;

    for (int i = 0; i < count; ++i) {
        std::string nextSegment = stream->GetNextSegment();

        if (nextSegment.length() > 0) {
            curl->LoadUrl(nextSegment.c_str());
        }
    }

    CONSOLE_TRACE("preload {} segments", count);
}

void DashHandler::load_dash(int streamidx) {
    DashStream *stream = GetStream(streamidx);
    double duration = (double)stream->GetSegmentDuration();

    while (load_dash_running) {
        std::string nextSegment = stream->GetNextSegment();

        if (nextSegment.length() > 0) {
            auto start = std::chrono::steady_clock::now();

            if (load_dash_running) {
                curl->LoadUrl(nextSegment);
            }

            // calculate wait time for next segment
            if (load_dash_running) {
                auto end = std::chrono::steady_clock::now();
                std::chrono::duration<double> elapsed_seconds = end - start;
                long waittime = (long) ((duration - elapsed_seconds.count()) * 1000) - 500;
                std::this_thread::sleep_for(std::chrono::milliseconds(waittime));

                CONSOLE_TRACE("DashCurl, Waited {} millis", waittime);
            }
        } else {
            load_dash_running = false;
        }
    }
}

void DashHandler::StartLoadThread(int streamidx) {
    // start heartbeat_thread
    load_dash_running = true;
    load_dash_thread = std::thread(&DashHandler::load_dash, this, streamidx);
    load_dash_thread.detach();
}

void DashHandler::StopLoadThread() {
    if (load_dash_running) {
        load_dash_running = false;
        if (load_dash_thread.joinable()) {
            load_dash_thread.join();
        }
    }
}

// ------------------- dash curl

DashCurl::DashCurl(std::string filename) {
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    this->filename = filename;
}

DashCurl::~DashCurl() {
    if (curl_handle) {
        curl_easy_cleanup(curl_handle);
    }

    curl_global_cleanup();
}

void DashCurl::LoadUrl(std::string url) {
    CONSOLE_TRACE("DashCurl::LoadUrl {}", url);

    FILE *out = fopen(filename.c_str(), "ab");

    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

    res = curl_easy_perform(curl_handle);

    if (res == CURLE_OK) {
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
    }

    if (out) {
        fflush(out);
        fclose(out);
    }
}