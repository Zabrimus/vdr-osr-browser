#ifndef VDR_OSR_BROWSER_ENCODER_H
#define VDR_OSR_BROWSER_ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
}
#endif

#include "osrhandler.h"

typedef struct StreamingContext {
    AVFormatContext *avfc;

    AVCodec *video_avc;
    AVCodec *audio_avc;

    AVStream *video_avs;
    AVStream *audio_avs;

    AVCodecContext *video_avcc;
    AVCodecContext *audio_avcc;

    bool writeToFile;
    char *filename;
} StreamingContext;

class Encoder {
private:
    StreamingContext *encoder;
    SwsContext* swsCtx;

    AVFrame *audioFrame;

    int srcWidth;
    int srcHeight;

    int channelCount;
    int sampleRate;

    unsigned char *outbuffer;

    std::mutex audioEncodeMutex;
    std::mutex videoEncodeMutex;

private:
    // encode
    int prepare_video_encoder(int width, int height, AVRational input_framerate);
    int prepare_audio_encoder(int channels, int sample_rate, AVRational input_framerate);
    int encode_video(uint64_t pts, AVFrame *input_frame);
    int encode_audio(uint64_t pts, AVFrame *input_frame);

public:
    // Transcode video stream
    Encoder(OSRHandler *osrHandler, const char* output, bool writeToFile = false);
    ~Encoder();

    int startEncoder(int (*write_packet)(void *opaque, uint8_t *buf, int buf_size));
    int stopEncoder();

    void setAudioParameters(int channels, int sample_rate);

    int addVideoFrame(int width, int height, uint8_t* image, uint64_t pts);
    int addAudioFrame(const float **data, int frames, uint64_t pts);
};

#endif //VDR_OSR_BROWSER_ENCODER_H
