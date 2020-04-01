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
#include <libavutil/opt.h>
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

    // Audio stream
    AVFilterContext *audio_fsrc;
    AVFilterContext *audio_fsink;
    AVFilterContext *audio_arealtime;
    AVFilterGraph   *audio_fgraph;

    // Video stream
    AVFilterContext *video_fsrc;
    AVFilterContext *video_fsrc_short;
    AVFilterContext *video_fsink;
    AVFilterContext *video_fsink_short;
    AVFilterContext *video_realtime;
    AVFilterContext *video_realtime_short;
    AVFilterContext *video_overlay;
    AVFilterContext *video_overlay_fsrc;
    AVFilterContext *video_format;
    AVFilterGraph   *video_fgraph;
    AVFilterGraph   *video_fgraph_short;

    int video_index;
    int audio_index;
    char *filename;
} StreamingContext;

class TranscodeFFmpeg {
private:
    StreamingContext *decoder;
    StreamingContext *encoder;
    SwsContext* swsCtx;
    AVFrame *video_overlay_frame;

    int srcWidth;
    int srcHeight;

    bool use_short_filter = false;

    // tempory saved overlay image
    int tmpOverlayWidth;
    int tmpOverlayHeight;
    uint8_t* tmpOverlayImage;

private:
    // Logging functions
    void logging(const char *fmt, ...);
    void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt);
    void print_timing(char *name, AVFormatContext *avf, AVCodecContext *avc, AVStream *avs);

    // create all filters
    int create_video_buffersrc_filter(StreamingContext *decoder, AVFilterContext **filt_ctx, AVFilterGraph *graph_ctx);
    int create_video_image_buffersrc_filter(StreamingContext *decoder, AVFilterContext **filt_ctx, AVFilterGraph *graph_ctx);
    int create_video_realtime_filter(StreamingContext *decoder, AVFilterContext **filt_ctx, AVFilterGraph *graph_ctx);
    int create_video_format_filter(StreamingContext *decoder, AVFilterContext **filt_ctx, AVFilterGraph *graph_ctx);
    int create_video_overlay_filter(StreamingContext *decoder, AVFilterContext **filt_ctx, AVFilterGraph *graph_ctx);
    int create_video_buffersink_filter(StreamingContext *decoder, AVFilterContext **filt_ctx, AVFilterGraph *graph_ctx);

    // build filter graphs
    int init_audio_filter_graph(StreamingContext *decoder);
    int init_full_video_filter_graph(StreamingContext *decoder);
    int init_short_video_filter_graph(StreamingContext *decoder);

    // decode/encode/transcode
    int fill_stream_info(AVStream *avs, AVCodec **avc, AVCodecContext **avcc);
    int open_media(const char *in_filename, AVFormatContext **avfc);
    int prepare_decoder();
    int prepare_video_encoder(AVCodecContext *decoder_ctx, AVRational input_framerate);
    int prepare_audio_encoder();
    int encode_video(AVFrame *input_frame);
    int encode_audio(AVFrame *input_frame);
    int transcode_audio(AVPacket *input_packet, AVFrame *input_frame);
    int transcode_video(AVPacket *input_packet, AVFrame *input_frame);

    // helper function
    inline int is_buffer_not_empty(uint8_t *buff, size_t size) {
        return *buff || memcmp(buff, buff+1, size-1);
    }

public:
    // Transcode video stream
    TranscodeFFmpeg(const char* input, const char* output, bool write2File = true);
    ~TranscodeFFmpeg();

    void set_input_file(const char* input);
    int transcode(int (*write_packet)(void *opaque, uint8_t *buf, int buf_size));

    int add_overlay_frame(int width, int height, uint8_t* image);
};


#endif // TRANSCODEFFMPEG_H
