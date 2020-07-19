#ifndef VDR_OSR_BROWSER_DASHHANDLER_H
#define VDR_OSR_BROWSER_DASHHANDLER_H

#include <string>
#include <list>
#include <thread>
#include <curl/curl.h>
#include "logger.h"

class DashCurl;

class DashStream {
    private:
        int index = -1;

        long bandwidth = -1;
        int width = -1;
        int height = -1;

        std::string baseSegment;
        std::list<std::string> segments;
        int segmentDuration = -1;

    public:
        DashStream(int index, long bandwidth, int width, int height, int segmentDuration) {
            this->index = index;
            this->bandwidth = bandwidth;
            this->width = width;
            this->height = height;
            this->segmentDuration = segmentDuration;
        }

        void SetBaseSegment(std::string base) {
            this->baseSegment = base;
        }

        void AddSegment(std::string segment) {
            segments.push_back(segment);
        }

        std::string GetBaseSegment() {
            return baseSegment;
        }

        std::string GetNextSegment() {
            if (segments.size() > 1) {
                std::string front = segments.front();
                segments.pop_front();

                return front;
            } else {
                return "";
            }
        }

        int GetSegmentDuration() {
            return segmentDuration;
        }

        long GetBandwidth() {
            return bandwidth;
        }

        int GetHeight() {
            return height;
        }

        int GetWidth() {
            return width;
        }

        int GetIndex() {
            return index;
        }

        void PrintTrace() {
            CONSOLE_TRACE("Stream: idx={}, bandwidth={}, segmentDuration={}, width={}, height={}", index, bandwidth, segmentDuration, width, height);
            CONSOLE_TRACE("   Base segment: {}", baseSegment);
            CONSOLE_TRACE("   Segment count: {}", segments.size());
            for (auto s : segments) {
                CONSOLE_TRACE("   Segment: {}", s);
            }
        }
};

class DashHandler {
    private:
        std::list<DashStream*> streams;

        std::thread load_dash_thread;
        bool load_dash_running;
        void load_dash(int streamidx);

        FILE *output;
        std::string filename;
        DashCurl *curl;

    public:
        DashHandler();
        ~DashHandler();

        void setFilename(std::string filename);

        int Size() {
            return streams.size();
        }

        void Clear() {
            for (auto s : streams) {
                delete s;
            }

            streams.clear();
        }

        void AddStream(int index, long bandwidth, int width, int height, int segmentDuration) {
            DashStream *stream = new DashStream(index, bandwidth, width, height, segmentDuration);
            streams.push_back(stream);
        }

        void SetBaseSegment(int index, std::string segmentUri) {
            auto st = GetStream(index);
            if (st != nullptr) {
                st->SetBaseSegment(segmentUri);
            }
        }

        void AddSegment(int index, std::string segmentUri) {
            auto st = GetStream(index);
            if (st != nullptr) {
                st->AddSegment(segmentUri);
            }
        }

        DashStream* GetStream(int idx) {
            for (auto stream : streams) {
                if (stream->GetIndex() == idx) {
                    return stream;
                }
            }

            return nullptr;
        }

        void PrintTrace() {
            CONSOLE_TRACE("Stream count: {}", Size());

            for (auto s : streams) {
                s->PrintTrace();
            }
        }

        void InitLoadThread(int streamidx);
        void StartLoadThread(int streamidx);
        void StopLoadThread();
};

extern DashHandler videoDashHandler;
extern DashHandler audioDashHandler;

struct SegmentStruct {
    char *memory;
    size_t size;
};

class DashCurl {
public:
    DashCurl(FILE *output, std::string filename);
    ~DashCurl();

    void LoadUrl(std::string url);

private:
    FILE *output;
    std::string filename;

    CURL *curl_handle;
    CURLcode res;
    long response_code;
};

#endif //VDR_OSR_BROWSER_DASHHANDLER_H
