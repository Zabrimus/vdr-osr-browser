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
#ifdef __cplusplus
}
#endif

#include "globals.h"

typedef struct StreamingContext {
    AVFormatContext *avfc;
    AVCodec *video_avc;
    AVCodec *audio_avc;
    AVStream *video_avs;
    AVStream *audio_avs;
    AVCodecContext *video_avcc;
    AVCodecContext *audio_avcc;
    int video_index;
    int audio_index;
    char *filename;
} StreamingContext;

class TranscodeFFmpeg {
private:
    StreamingContext *decoder;
    StreamingContext *encoder;

private:
    // Logging functions
    void logging(const char *fmt, ...);
    void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt);
    void print_timing(char *name, AVFormatContext *avf, AVCodecContext *avc, AVStream *avs);

    int fill_stream_info(AVStream *avs, AVCodec **avc, AVCodecContext **avcc);
    int open_media(const char *in_filename, AVFormatContext **avfc);
    int prepare_decoder();
    int prepare_video_encoder(AVCodecContext *decoder_ctx, AVRational input_framerate);
    int prepare_audio_encoder();
    int encode_video(AVFrame *input_frame);
    int encode_audio(AVFrame *input_frame);
    int transcode_audio(AVPacket *input_packet, AVFrame *input_frame);
    int transcode_video(AVPacket *input_packet, AVFrame *input_frame);

    static FILE *fp_output;
    static bool write2File;
    static int write_buffer_to_file(void *opaque, uint8_t *buf, int buf_size);
    static int write_buffer_to_vdr(void *opaque, uint8_t *buf, int buf_size);

public:
    TranscodeFFmpeg(char* input, char* output, bool write2File = false);
    ~TranscodeFFmpeg();

    int transcode();
};


#endif // TRANSCODEFFMPEG_H
