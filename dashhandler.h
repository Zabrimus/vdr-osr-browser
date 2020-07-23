#ifndef VDR_OSR_BROWSER_DASHHANDLER_H
#define VDR_OSR_BROWSER_DASHHANDLER_H

#include <string>
#include <list>
#include <thread>
#include <curl/curl.h>
#include "logger.h"

class DashCurl;

enum StreamType { Video, Audio };

typedef struct DashStream {
    uint idx;

    StreamType type;

    uint  width;
    uint  height;
    ulong bandwidth;

    ulong firstSegment;
    ulong lastSegment;
    ulong startSegment;
    uint  duration;

    std::string initUrl;
    std::string segmentUrl;

    void print() {
        printf("Idx:%d, first:%lu, last=%lu, start=%lu, duration=%d, bandwidth=%lu, initUrl=%s, segmentUrl=%s\n", idx, firstSegment, lastSegment, startSegment, duration, bandwidth, initUrl.c_str(), segmentUrl.c_str());
    }
} DashStream;

class DashHandler {
private:
    ulong segmentIdx;
    std::string baseUrl;
    std::list<DashStream> streams;

    std::thread load_dash_thread;
    bool load_dash_running;

    std::string filename;
    DashCurl *curl;

    void LoadDash(int streamidx);
    std::string GetNextUrl(DashStream& stream);

public:
    DashHandler();
    ~DashHandler();

    void SetBaseUrl(std::string baseUrl);
    void SetFilename(std::string filename);

    void ResetSegmentIndex(ulong newSegmentIdx);

    DashStream& GetStream(uint idx);
    void ClearAll();

    void InitLoadThread(int streamidx, int startSegment);
    void StartLoadThread(int streamidx);
    void StopLoadThread();

    uint GetBestStream();
};

class DashCurl {
private:
    std::string filename;

    CURL *curl_handle;
    CURLcode res;
    long response_code;

public:
    DashCurl(std::string filename);
    ~DashCurl();

    void LoadUrl(std::string url);
};

extern DashHandler audioDashHandler;
extern DashHandler videoDashHandler;

#endif //VDR_OSR_BROWSER_DASHHANDLER_H
