#ifndef BROWSER_VIDEOTRANSCODE_H
#define BROWSER_VIDEOTRANSCODE_H

#include "include/cef_app.h"

class VideoTranscode {
private:
    FILE *toFfmpegVideo;
    int ffmpegPid = -1;
    int ffmpegVideoIn = -1;
    int ffmpegAudioIn = -1;
    int ffmpegVideoOut = -1;

    // will be filled after calling analyzeVideo
    int width;
    int height;
    std::string videoCodec;
    std::string audioCodec;

public:
    VideoTranscode();
    ~VideoTranscode();

    bool startStreaming();
    void stopStreaming();

    int getVideoOut() { return ffmpegVideoOut; };

    void writeVideo(const void *buffer, int width, int height);
    void writeAudio(const void *buffer, int size);

    int readEncoded(void* buffer, int size);
    void analyzeVideo(const CefString& url);
};

extern VideoTranscode *videoTranscode;

#endif // BROWSER_VIDEOTRANSCODE_H
