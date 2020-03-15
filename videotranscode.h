#ifndef BROWSER_VIDEOTRANSCODE_H
#define BROWSER_VIDEOTRANSCODE_H

class VideoTranscode {
private:
    FILE *toFfmpegVideo;
    int ffmpegPid = -1;
    int ffmpegVideoIn = -1;
    int ffmpegAudioIn = -1;
    int ffmpegVideoOut = -1;

public:
    VideoTranscode();
    ~VideoTranscode();

    bool startStreaming();
    void stopStreaming();

    int getVideoOut() { return ffmpegVideoOut; };

    void writeVideo(const void *buffer, int width, int height);
    void writeAudio(const void *buffer, int size);

    int readEncoded(void* buffer, int size);
};

#endif // BROWSER_VIDEOTRANSCODE_H
