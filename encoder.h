#ifndef VDR_OSR_BROWSER_ENCODER_H
#define VDR_OSR_BROWSER_ENCODER_H

#ifndef TRANSCODEFFMPEG_H
#define TRANSCODEFFMPEG_H

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

    unsigned char *outbuffer;

private:
    // Logging functions
    void logging(const char *fmt, ...);
    void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt);
    void print_timing(char *name, AVFormatContext *avf, AVCodecContext *avc, AVStream *avs);

    // encode
    int prepare_video_encoder(int width, int height, AVRational input_framerate);
    int prepare_audio_encoder(int channels, int sample_rate);
    int encode_video(uint64_t pts, AVFrame *input_frame);
    int encode_audio(AVFrame *input_frame);

public:
    // Transcode video stream
    Encoder(const char* output, bool write2File = true);
    ~Encoder();

    int startEncoder(int (*write_packet)(void *opaque, uint8_t *buf, int buf_size));
    int stopEncoder();

    int addVideoFrame(int width, int height, uint8_t* image, uint64_t pts);
};


#endif // TRANSCODEFFMPEG_H


#endif //VDR_OSR_BROWSER_ENCODER_H
