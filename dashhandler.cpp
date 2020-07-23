#include <stdio.h>
#include <unistd.h>
#include "dashhandler.h"
#include "logger.h"

// set to true, if all segment URLs shall be printed to stdout
// Use only for debugging purposes
const bool show_all_segments = false;

DashHandler videoDashHandler;
DashHandler audioDashHandler;

DashHandler::DashHandler() {
    segmentIdx = 0;
    baseUrl = "Does not exists";
    streams.clear();
}

DashHandler::~DashHandler() {
    ClearAll();
}

void DashHandler::SetBaseUrl(std::string baseUrl) {
    this->baseUrl = baseUrl;
}

void DashHandler::SetFilename(std::string filename) {
    this->filename = filename;
    unlink(filename.c_str());
    curl = new DashCurl(filename);
}

void DashHandler::ResetSegmentIndex(ulong newSegmentIdx) {
    segmentIdx = newSegmentIdx;
}

DashStream& DashHandler::GetStream(uint idx) {
    for (auto& stream : streams) {
        if (stream.idx == idx) {
            return stream;
        }
    }

    DashStream stream = {};
    stream.idx = idx;
    streams.push_back(stream);

    return streams.back();
}

void DashHandler::ClearAll() {
    StopLoadThread();

    if (curl) {
        delete curl;
        curl = nullptr;
    }

    streams.clear();
    filename = "";
    baseUrl = "";
}

std::string DashHandler::GetNextUrl(DashStream& stream) {
    std::string segmentUrl = stream.segmentUrl;

    auto index = segmentUrl.find("$Number$", 0);
    segmentUrl.replace(index, 8, std::to_string(segmentIdx));

    segmentIdx++;

    return baseUrl + segmentUrl;
}

void DashHandler::InitLoadThread(int streamidx, int startSegment) {
    DashStream stream = GetStream(streamidx);
    ResetSegmentIndex(startSegment);

    // save base segment
    curl->LoadUrl(baseUrl + stream.initUrl);

    // preload segments
    ulong availableSegments = stream.lastSegment - stream.startSegment;
    ulong desiredSegments = (10 / stream.duration) + 1;

    int count;
    if (availableSegments < desiredSegments) {
        // reset Startindex to have more time to load segments
        ResetSegmentIndex(startSegment - 10);
        count = (int)availableSegments + 5;
    } else {
        count = desiredSegments;
    }

    for (int i = 0; i < count; ++i) {
        std::string nextSegment = GetNextUrl(stream);

        if (nextSegment.length() > 0) {
            curl->LoadUrl(nextSegment.c_str());
        }
    }

    CONSOLE_TRACE("preload {} segments", count);
}

void DashHandler::StartLoadThread(int streamidx) {
    // start heartbeat_thread
    load_dash_running = true;
    load_dash_thread = std::thread(&DashHandler::LoadDash, this, streamidx);
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

void DashHandler::LoadDash(int streamidx) {
    DashStream stream = GetStream(streamidx);
    double duration = (double)stream.duration;

    while (load_dash_running) {
        std::string nextSegment = GetNextUrl(stream);

        if (nextSegment.length() > 0) {
            auto start = std::chrono::steady_clock::now();

            if (load_dash_running) {
                curl->LoadUrl(nextSegment);
            }

            // calculate wait time for next segment
            if (load_dash_running) {
                auto end = std::chrono::steady_clock::now();
                std::chrono::duration<double> elapsed_seconds = end - start;
                // long waittime = (long) ((duration - elapsed_seconds.count()) * 1000) - 500;
                // long waittime = (long) ((duration - elapsed_seconds.count()) * 1000) / 2;
                long waittime = (long) ((duration - elapsed_seconds.count()) * 1000);
                std::this_thread::sleep_for(std::chrono::milliseconds(waittime));

                CONSOLE_TRACE("DashCurl, Waited {} millis", waittime);
            }
        } else {
            load_dash_running = false;
        }
    }
}

uint DashHandler::GetBestStream() {
    int bestIdx = -1;
    ulong bestBandwidth = 0;

    for (auto stream : streams) {
        if (stream.bandwidth > bestBandwidth) {
            bestIdx = stream.idx;
            bestBandwidth = stream.bandwidth;
        }
    }

    return bestIdx;
}

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

    if (show_all_segments) {
        printf("%s\n", url.c_str());
    }

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