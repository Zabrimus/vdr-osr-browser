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

    char *filename;
} StreamingContext;

class Encoder {
private:
    StreamingContext *encoder;
    SwsContext* swsCtx;

    int srcWidth;
    int srcHeight;

    int channelCount;
    int sampleRate;

    unsigned char *outbuffer;

    AVFrame *videoFrame;
    AVFrame *audioFrame;

    AVPacket *video_output_packet;
    AVPacket *audio_output_packet;

private:
    // load x264 configuration
    void loadConfiguration(void *avcc_priv_data);

    // encode
    int prepare_video_encoder(int width, int height, AVRational input_framerate);
    int prepare_audio_encoder(int channels, int sample_rate, AVRational input_framerate);
    int encode_video(AVFrame *input_frame);
    int encode_audio(AVFrame *input_frame);

public:
    // Transcode video stream
    Encoder();
    ~Encoder();

    void Start();

    int startEncoder();
    void stopEncoder();

    void setAudioParameters(int channels, int sample_rate);

    void addVideoFrame(int width, int height, uint8_t* image, uint64_t pts);
    void processVideoFrame();

    void addAudioFrame(const float **data, int frames, uint64_t pts);
    void processAudioFrame();
};

#endif //VDR_OSR_BROWSER_ENCODER_H
