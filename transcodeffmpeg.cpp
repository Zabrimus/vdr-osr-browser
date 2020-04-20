/**
 * Based on the work, source code and tutorial of Leandro Moreira (https://github.com/leandromoreira/ffmpeg-libav-tutorial)
 * Especially on https://github.com/leandromoreira/ffmpeg-libav-tutorial/blob/master/3_transcoding.c
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <inttypes.h>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <nanomsg/nn.h>

#include "transcodeffmpeg.h"

#define DBG_MEASURE_TIME 1

// Fix some compile problems
#undef av_err2str
#define av_err2str(errnum) av_make_error_string((char*)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)

#undef av_ts2str
#define av_ts2str(ts) av_ts_make_string((char*)__builtin_alloca(AV_TS_MAX_STRING_SIZE), ts)

#undef av_ts2timestr
#define av_ts2timestr(ts, tb) av_ts_make_time_string((char*)__builtin_alloca(AV_TS_MAX_STRING_SIZE), ts, tb)

#define FFMPEG_FIFO "ffmpeg_io.mkv"

FILE *fp_output = NULL;

int write_buffer_to_file(void *opaque, uint8_t *buf, int buf_size) {
    if (!feof(fp_output)) {
        int true_size = fwrite(buf,1,buf_size, fp_output);
        return true_size;
    } else {
        return -1;
    }
}

TranscodeFFmpeg::TranscodeFFmpeg(const char* ffmpeg, const char* ffprobe, const char* in, const char* out, bool write2File) {
    // av_log_set_level(AV_LOG_TRACE);

    decoder = (StreamingContext *) calloc(1, sizeof(StreamingContext));
    if (in != NULL) {
        decoder->filename = strdup(in);
    }

    encoder = (StreamingContext *) calloc(1, sizeof(StreamingContext));
    asprintf(&encoder->filename, "%s.ts", out);

    if (write2File) {
        fp_output = fopen(encoder->filename, "wb+");
    }

    if (avfilter_version() < AV_VERSION_INT(7, 57, 100)) {
        fprintf(stderr, "Warning: Found FFmpeg version < 4.2. Some functionality is not available!\n");
    }

    decoder->video_index = -1;
    decoder->video_index = -1;
    encoder->video_index = -1;
    encoder->video_index = -1;

    ffmpeg_executable = strdup(ffmpeg);
    ffprobe_executable = strdup(ffprobe);

    swsCtx = nullptr;
}

TranscodeFFmpeg::~TranscodeFFmpeg() {
    avformat_close_input(&decoder->avfc);

    avformat_free_context(decoder->avfc);
    avformat_free_context(encoder->avfc);
    avcodec_free_context(&decoder->video_avcc);
    avcodec_free_context(&decoder->audio_avcc);

    sws_freeContext(swsCtx);
    swsCtx = NULL;

    free(decoder);
    decoder = NULL;

    free(encoder);
    encoder = NULL;

    if (fp_output != NULL) {
        fclose(fp_output);
    }

    if (ffmpeg_executable) {
        free(ffmpeg_executable);
    }

    if (ffprobe_executable) {
        free(ffprobe_executable);
    }
}

bool TranscodeFFmpeg::set_input_file(const char* input) {
    // create pipe
    bool createPipe = false;

    struct stat sb{};
    if(stat(FFMPEG_FIFO, &sb) != -1) {
        if (!S_ISFIFO( sb.st_mode ) != 0) {
            if(remove( FFMPEG_FIFO ) != 0) {
                fprintf(stderr, "File %s exists and is not a pipe. Delete failed. Aborting...\n", FFMPEG_FIFO);
                return false;
            } else {
                createPipe = true;
            }
        }
    } else {
        createPipe = true;
    }

    if (createPipe) {
        if (mkfifo(FFMPEG_FIFO, 0666) < 0) {
            fprintf(stderr, "Unable to create pipe %s. Aborting...\n", FFMPEG_FIFO);
            return false;
        }
    }

    // get duration
    char *ffprobe;
    asprintf(&ffprobe, "%s -i %s -show_format -v quiet | sed -n 's/duration=//p'", ffprobe_executable, input);

    FILE *durationFile = popen(ffprobe, "r");

    long duration = 0;

    if (durationFile) {
        char buffer[1024];
        char *line = fgets(buffer, sizeof(buffer), durationFile);
        pclose(durationFile);

        if (line != nullptr) {
            duration = strtol(line, (char **) NULL, 10);
        } else {
            duration = 0;
        }
    }

    if (duration == 0) {
        fprintf(stderr, "Unable to determine duration. ffprobe failed.\n");
    }

    // check if full transparent video exists, otherwise create one (shall not happen)
    if (access("movie/transparent-full.webm", R_OK) == -1 ) {
        fprintf(stderr, "Warning: the file movie/transparent-full.webm does not exist!");
        fprintf(stderr, "         Creation of a new one will be started, but it takes time.");

        char *createvideo;
        asprintf(&createvideo, "%s -y -loop 1 -i movie/transparent-16x16.png -t 21600 -r 1 -c:v libvpx -auto-alt-ref 0 movie/transparent-full.webm", ffmpeg_executable);
        int result = system(createvideo);
        free(createvideo);

        if (result == -1) {
            fprintf(stderr, "Error: Unable to create the file movie/transparent-full.webm!");
            return false;
        }
    }

    // create transparent video with desired duration
    char *cvd;
    asprintf(&cvd, "%s -y -i movie/transparent-full.webm -t %ld -codec copy movie/transparent.webm", ffmpeg_executable, duration+1);
    int result = system(cvd);
    free(cvd);

    if (result == -1) {
        fprintf(stderr, "Error: Unable to create the file movie/transparent.webm!");
        return false;
    }

    // fork and start ffmpeg
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Fork failed. Aborting...\n");
        return false;
    } else if (pid == 0) {
        // let ffmpeg do the hard work like fixing dts/pts and more
        fprintf(stderr, "Fork ffmpeg...\n");
        execl(ffmpeg_executable, ffmpeg_executable, "-i", input, "-codec", "copy", "-y", FFMPEG_FIFO, (const char*)NULL);
        exit(0);
    }

    return true;
}

// Logging functions
void TranscodeFFmpeg::logging(const char *fmt, ...)
{
    va_list args;
    fprintf( stderr, "LOG: " );
    va_start( args, fmt );
    vfprintf( stderr, fmt, args );
    va_end( args );
    fprintf( stderr, "\n" );
}

void TranscodeFFmpeg::log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    logging("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d",
            av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
            av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
            av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
            pkt->stream_index);
}

void TranscodeFFmpeg::print_timing(char *name, AVFormatContext *avf, AVCodecContext *avc, AVStream *avs) {
    logging("=================================================");
    logging("%s", name);

    logging("\tAVFormatContext");
    if (avf != NULL) {
        logging("\t\tstart_time=%d duration=%d bit_rate=%d start_time_realtime=%d", avf->start_time, avf->duration, avf->bit_rate, avf->start_time_realtime);
    } else {
        logging("\t\t->NULL");
    }

    logging("\tAVCodecContext");
    if (avc != NULL) {
        logging("\t\tbit_rate=%d ticks_per_frame=%d width=%d height=%d gop_size=%d keyint_min=%d sample_rate=%d profile=%d level=%d ",
                avc->bit_rate, avc->ticks_per_frame, avc->width, avc->height, avc->gop_size, avc->keyint_min, avc->sample_rate, avc->profile, avc->level);
        logging("\t\tavc->time_base=num/den %d/%d", avc->time_base.num, avc->time_base.den);
        logging("\t\tavc->framerate=num/den %d/%d", avc->framerate.num, avc->framerate.den);
        logging("\t\tavc->pkt_timebase=num/den %d/%d", avc->pkt_timebase.num, avc->pkt_timebase.den);
    } else {
        logging("\t\t->NULL");
    }

    logging("\tAVStream");
    if (avs != NULL) {
        logging("\t\tindex=%d start_time=%d duration=%d ", avs->index, avs->start_time, avs->duration);
        logging("\t\tavs->time_base=num/den %d/%d", avs->time_base.num, avs->time_base.den);
        logging("\t\tavs->sample_aspect_ratio=num/den %d/%d", avs->sample_aspect_ratio.num, avs->sample_aspect_ratio.den);
        logging("\t\tavs->avg_frame_rate=num/den %d/%d", avs->avg_frame_rate.num, avs->avg_frame_rate.den);
        logging("\t\tavs->r_frame_rate=num/den %d/%d", avs->r_frame_rate.num, avs->r_frame_rate.den);
    } else {
        logging("\t\t->NULL");
    }

    logging("=================================================");
}

int TranscodeFFmpeg::fill_stream_info(AVStream *avs, AVCodec **avc, AVCodecContext **avcc) {
    *avc = avcodec_find_decoder(avs->codecpar->codec_id);
    if (!*avc) {
        logging("failed to find the codec");
        return -1;
    }

    *avcc = avcodec_alloc_context3(*avc);
    if (!*avcc) {
        logging("failed to alloc memory for codec context");
        return -1;
    }

    if (avcodec_parameters_to_context(*avcc, avs->codecpar) < 0) {
        logging("failed to fill codec context");
        return -1;
    }

    if (avcodec_open2(*avcc, *avc, NULL) < 0) {
        logging("failed to open codec");
        return -1;
    }

    return 0;
}

int TranscodeFFmpeg::open_media(const char *in_filename, AVFormatContext **avfc) {
    *avfc = avformat_alloc_context();
    if (!*avfc) {
        logging("failed to alloc memory for format");
        return -1;
    }

    if (avformat_open_input(avfc, in_filename, NULL, NULL) != 0) {
        logging("failed to open input file %s", in_filename);
        return -1;
    }

    if (avformat_find_stream_info(*avfc, NULL) < 0) {
        logging("failed to get stream info");
        return -1;
    }

    av_dump_format(*avfc, 1, in_filename, 0);

    return 0;
}

int TranscodeFFmpeg::init_audio_filter_graph(StreamingContext *decoder) {
    if (decoder->audio_index < 0) {
        // no audio stream available
        return 0;
    }

    char strbuf[512];
    int err;

    // create new graph
    decoder->audio_fgraph = avfilter_graph_alloc();
    if (!decoder->audio_fgraph) {
        av_log(NULL, AV_LOG_ERROR, "unable to create filter graph: out of memory\n");
        return -1;
    }

    const AVFilter *abuffer = avfilter_get_by_name("abuffer");
    const AVFilter *arealtime = avfilter_get_by_name("arealtime");
    const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");

    // create abuffer filter
    AVCodecContext *avctx = decoder->audio_avcc;
    AVRational time_base = decoder->audio_avs->time_base;

    snprintf(strbuf, sizeof(strbuf),
             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
             time_base.num, time_base.den, avctx->sample_rate,
             av_get_sample_fmt_name(avctx->sample_fmt),
             avctx->channel_layout);

    err = avfilter_graph_create_filter(&decoder->audio_fsrc, abuffer, NULL, strbuf, NULL, decoder->audio_fgraph);
    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "error initializing abuffer filter\n");
        return err;
    }

    // create arealtime filter
    // Test for FFmpeg >= 4.2
    if (avfilter_version() >= AV_VERSION_INT(7, 57, 100)) {
        // speed option is only available for FFmpeg >= 4.2
        snprintf(strbuf, sizeof(strbuf), "speed=%f", 1.0); // TODO: Make this configurable
        err = avfilter_graph_create_filter(&decoder->audio_arealtime, arealtime, NULL, strbuf, NULL, decoder->audio_fgraph);
    } else {
        err = avfilter_graph_create_filter(&decoder->audio_arealtime, arealtime, NULL, NULL, NULL, decoder->audio_fgraph);
    }

    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "error initializing arealtime filter\n");
        return err;
    }

    // create abuffersink filter
    err = avfilter_graph_create_filter(&decoder->audio_fsink, abuffersink, NULL, NULL, NULL, decoder->audio_fgraph);
    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "unable to create aformat filter\n");
        return err;
    }

    // connect inputs and outputs
    if (err >= 0) err = avfilter_link(decoder->audio_fsrc, 0, decoder->audio_arealtime, 0);
    if (err >= 0) err = avfilter_link(decoder->audio_arealtime, 0, decoder->audio_fsink, 0);

    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "error connecting filters\n");
        return err;
    }

    err = avfilter_graph_config(decoder->audio_fgraph, NULL);
    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "error configuring the filter graph\n");
        return err;
    }

    return 0;
}

int TranscodeFFmpeg::create_video_buffersrc_filter(StreamingContext *decoder, AVFilterContext **filt_ctx, AVFilterGraph *graph_ctx) {
    char strbuf[512];
    int err;

    const AVFilter *buffersrc = avfilter_get_by_name("buffer");

    // create buffer filter
    snprintf(strbuf, sizeof(strbuf),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             decoder->video_avcc->width, decoder->video_avcc->height, decoder->video_avcc->pix_fmt,
             decoder->video_avs->time_base.num, decoder->video_avs->time_base.den,
             decoder->video_avcc->sample_aspect_ratio.num, decoder->video_avcc->sample_aspect_ratio.den);

    err = avfilter_graph_create_filter(filt_ctx, buffersrc, NULL, strbuf, NULL, graph_ctx);
    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "error initializing buffersrc filter\n");
        return err;
    }

    return 0;
}

int TranscodeFFmpeg::create_video_realtime_filter(StreamingContext *decoder, AVFilterContext **filt_ctx, AVFilterGraph *graph_ctx) {
    int err;
    char strbuf[512];

    const AVFilter *realtime = avfilter_get_by_name("realtime");

    // Test for FFmpeg >= 4.2
    if (avfilter_version() >= AV_VERSION_INT(7, 57, 100)) {
        // speed option is only available for FFmpeg >= 4.2
        snprintf(strbuf, sizeof(strbuf), "speed=%f", 1.0); // TODO: Make this configurable
        err = avfilter_graph_create_filter(filt_ctx, realtime, NULL, strbuf, NULL, graph_ctx);
    } else {
        err = avfilter_graph_create_filter(filt_ctx, realtime, NULL, NULL, NULL, graph_ctx);
    }

    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "error initializing realtime filter\n");
        return err;
    }

    return 0;
}

int TranscodeFFmpeg::create_video_buffersink_filter(StreamingContext *decoder, AVFilterContext **filt_ctx, AVFilterGraph *graph_ctx) {
    int err;

    const AVFilter *buffersink = avfilter_get_by_name("buffersink");

    err = avfilter_graph_create_filter(filt_ctx, buffersink, NULL, NULL, NULL, graph_ctx);

    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "error initializing buffersink filter\n");
        return err;
    }

    return 0;
}

int TranscodeFFmpeg::init_full_video_filter_graph(StreamingContext *decoder) {
    int err;

    if (decoder->video_index < 0) {
        // no video stream available
        return 0;
    }

    // create new graph
    decoder->video_fgraph = avfilter_graph_alloc();
    if (!decoder->video_fgraph) {
        av_log(NULL, AV_LOG_ERROR, "unable to create filter graph: out of memory\n");
        return -1;
    }

    if ((err = create_video_buffersrc_filter(decoder, &decoder->video_fsrc, decoder->video_fgraph)) < 0)               return err;
    if ((err = create_video_realtime_filter(decoder, &decoder->video_realtime, decoder->video_fgraph)) < 0)            return err;
    if ((err = create_video_buffersink_filter(decoder, &decoder->video_fsink, decoder->video_fgraph)) < 0)             return err;

    // connect everything
    if (err >= 0) err = avfilter_link(decoder->video_fsrc, 0, decoder->video_realtime, 0);
    if (err >= 0) err = avfilter_link(decoder->video_realtime, 0, decoder->video_fsink, 0);

    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "error connecting filters\n");
        return err;
    }

    err = avfilter_graph_config(decoder->video_fgraph, NULL);
    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "error configuring the filter graph\n");
        return err;
    }

    return 0;
}

int TranscodeFFmpeg::prepare_decoder() {
    decoder->audio_index = -1;
    decoder->video_index = -1;

    for (unsigned int i = 0; i < decoder->avfc->nb_streams; i++) {
        if (decoder->avfc->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            decoder->video_avs = decoder->avfc->streams[i];
            decoder->video_index = i;

            if (fill_stream_info(decoder->video_avs, &decoder->video_avc, &decoder->video_avcc)) {
                return -1;
            }
        } else if (decoder->avfc->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            decoder->audio_avs = decoder->avfc->streams[i];
            decoder->audio_index = i;

            if (fill_stream_info(decoder->audio_avs, &decoder->audio_avc, &decoder->audio_avcc)) {
                return -1;
            }
        } else {
            logging("skipping streams other than audio and video");
        }
    }

    // init filters
    int ret;

    ret = init_audio_filter_graph(decoder);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio filter graph\n");
        return ret;
    }

    ret = init_full_video_filter_graph(decoder);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create full video filter graph\n");
        return ret;
    }

    return 0;
}

int TranscodeFFmpeg::prepare_video_encoder(AVCodecContext *decoder_ctx, AVRational input_framerate) {
    int response;

    encoder->video_avs = avformat_new_stream(encoder->avfc, NULL);

    encoder->video_avc = avcodec_find_encoder_by_name("libx264");
    if (!encoder->video_avc) {
        logging("could not find the proper codec");
        return -1;
    }

    encoder->video_avcc = avcodec_alloc_context3(encoder->video_avc);
    if (!encoder->video_avcc) {
        logging("could not allocated memory for codec context");
        return -1;
    }

    av_opt_set(encoder->video_avcc->priv_data, "preset", "veryfast", 0);
    av_opt_set(encoder->video_avcc->priv_data, "x264-params", "keyint=60:min-keyint=60:scenecut=0:force-cfr=1:crf=28", 0);

    encoder->video_avcc->height = decoder_ctx->height;
    encoder->video_avcc->width = decoder_ctx->width;
    encoder->video_avcc->sample_aspect_ratio = decoder_ctx->sample_aspect_ratio;

    if (encoder->video_avc->pix_fmts) {
        encoder->video_avcc->pix_fmt = encoder->video_avc->pix_fmts[0];
    } else {
        encoder->video_avcc->pix_fmt = decoder_ctx->pix_fmt;
    }

    encoder->video_avcc->bit_rate = decoder->video_avcc->bit_rate;
    encoder->video_avcc->rc_buffer_size = decoder->video_avcc->rc_buffer_size;
    encoder->video_avcc->rc_max_rate = decoder->video_avcc->rc_max_rate;
    encoder->video_avcc->rc_min_rate = decoder->video_avcc->rc_min_rate;

    encoder->video_avcc->time_base = av_inv_q(input_framerate);
    encoder->video_avs->time_base = encoder->video_avcc->time_base;

    if (avcodec_open2(encoder->video_avcc, encoder->video_avc, NULL) < 0) {
        logging("could not open the codec");
        return -1;
    }

    response = avcodec_parameters_from_context(encoder->video_avs->codecpar, encoder->video_avcc);
    if (response < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream\n");
        return response;
    }

    return 0;
}

int TranscodeFFmpeg::prepare_audio_encoder() {
    encoder->audio_avs = avformat_new_stream(encoder->avfc, NULL);

    encoder->audio_avc = avcodec_find_encoder_by_name("aac");
    if (!encoder->audio_avc) {
        logging("could not find the proper codec");
        return -1;
    }

    encoder->audio_avcc = avcodec_alloc_context3(encoder->audio_avc);
    if (!encoder->audio_avcc) {
        logging("could not allocated memory for codec context");
        return -1;
    }

    encoder->audio_avcc->channels = decoder->audio_avcc->channels;
    encoder->audio_avcc->channel_layout = av_get_default_channel_layout(decoder->audio_avcc->channels);
    encoder->audio_avcc->sample_rate = decoder->audio_avcc->sample_rate;
    encoder->audio_avcc->sample_fmt = encoder->audio_avc->sample_fmts[0];
    encoder->audio_avcc->bit_rate = decoder->audio_avcc->bit_rate;
    encoder->audio_avcc->time_base = (AVRational) {1, decoder->audio_avcc->sample_rate};

    encoder->audio_avcc->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    encoder->audio_avs->time_base = encoder->audio_avcc->time_base;

    if (avcodec_open2(encoder->audio_avcc, encoder->audio_avc, NULL) < 0) {
        logging("could not open the codec");
        return -1;
    }

    avcodec_parameters_from_context(encoder->audio_avs->codecpar, encoder->audio_avcc);
    return 0;
}

int TranscodeFFmpeg::encode_video(AVFrame *input_frame) {
    if (input_frame) input_frame->pict_type = AV_PICTURE_TYPE_NONE;

    AVPacket *output_packet = av_packet_alloc();
    if (!output_packet) {
        logging("could not allocate memory for output packet");
        return -1;
    }

    output_packet->data = NULL;
    output_packet->size = 0;
    av_init_packet(output_packet);

    int response = avcodec_send_frame(encoder->video_avcc, input_frame);

    while (response >= 0) {
        response = avcodec_receive_packet(encoder->video_avcc, output_packet);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            auto errstr = av_err2str(response);
            logging("Error while receiving packet from encoder: %s", errstr);
            return -1;
        }

        output_packet->duration = encoder->video_avs->time_base.den / encoder->video_avs->time_base.num /
                                  decoder->video_avs->avg_frame_rate.num * decoder->video_avs->avg_frame_rate.den;

        av_packet_rescale_ts(output_packet, decoder->video_avs->time_base, encoder->video_avs->time_base);

        output_packet->stream_index = decoder->video_index;
        response = av_interleaved_write_frame(encoder->avfc, output_packet);
        if (response != 0) {
            logging("Error %d while receiving packet from decoder: %s", response, av_err2str(response));
            return -1;
        }
    }
    av_packet_unref(output_packet);
    av_packet_free(&output_packet);
    return 0;
}

int TranscodeFFmpeg::encode_audio(AVFrame *input_frame) {
    AVPacket *output_packet = av_packet_alloc();
    if (!output_packet) {
        logging("could not allocate memory for output packet");
        return -1;
    }

    int response = avcodec_send_frame(encoder->audio_avcc, input_frame);

    while (response >= 0) {
        response = avcodec_receive_packet(encoder->audio_avcc, output_packet);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            logging("Error while receiving packet from encoder: %s", av_err2str(response));
            return -1;
        }

        av_packet_rescale_ts(output_packet, decoder->audio_avs->time_base, encoder->audio_avs->time_base);

        output_packet->stream_index = decoder->audio_index;
        response = av_interleaved_write_frame(encoder->avfc, output_packet);
        if (response != 0) {
            logging("Error %d while receiving packet from decoder: %s", response, av_err2str(response));
            return -1;
        }
    }
    av_packet_unref(output_packet);
    av_packet_free(&output_packet);
    return 0;
}

int TranscodeFFmpeg::transcode_audio(AVPacket *input_packet, AVFrame *input_frame) {
    int response = avcodec_send_packet(decoder->audio_avcc, input_packet);
    if (response < 0) {
        logging("Error while sending packet to decoder: %s", av_err2str(response));
        return response;
    }

    while (response >= 0) {
        response = avcodec_receive_frame(decoder->audio_avcc, input_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            logging("Error while receiving frame from decoder: %s", av_err2str(response));
            return response;
        }

        // push the video data from decoded frame into the filtergraph
        response = av_buffersrc_write_frame(decoder->audio_fsrc, input_frame);
        if (response < 0) {
            av_log(NULL, AV_LOG_ERROR, "error writing frame to buffersrc\n");
            return -1;
        }

        av_frame_unref(input_frame);

        if (response >= 0) {
            // pull filtered audio from the filtergraph
            response = av_buffersink_get_frame(decoder->audio_fsink, input_frame);
            if (response == AVERROR_EOF || response == AVERROR(EAGAIN)) {
                return response;
            }

            if (response < 0) {
                av_log(NULL, AV_LOG_ERROR, "error reading buffer from buffersink\n");
                return -1;
            }

            if (encode_audio(input_frame)) return -1;
        }
        av_frame_unref(input_frame);
    }
    return 0;
}

int TranscodeFFmpeg::transcode_video(AVPacket *input_packet, AVFrame *input_frame) {
    int response = avcodec_send_packet(decoder->video_avcc, input_packet);
    if (response < 0) {
        logging("Error while sending packet to decoder: %s", av_err2str(response));
        return response;
    }

    while (response >= 0) {
        response = avcodec_receive_frame(decoder->video_avcc, input_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            logging("Error while receiving frame from decoder: %s", av_err2str(response));
            return response;
        }

        // push the video data from decoded frame into the filtergraph
        response = av_buffersrc_write_frame(decoder->video_fsrc, input_frame);
        if (response < 0) {
            av_log(NULL, AV_LOG_ERROR, "error writing frame to buffersrc\n");
            return -1;
        }

        av_frame_unref(input_frame);

        if (response >= 0) {
            // pull filtered video from the filtergraph
            response = av_buffersink_get_frame(decoder->video_fsink, input_frame);

            if (response == AVERROR_EOF || response == AVERROR(EAGAIN)) {
                return response;
            }

            if (response < 0) {
                av_log(NULL, AV_LOG_ERROR, "error reading buffer from buffersink\n");
                return -1;
            }

            if (encode_video(input_frame)) {
                return -1;
            }
        }

        av_frame_unref(input_frame);
    }
    return 0;
}

std::thread TranscodeFFmpeg::transcode(int (*write_packet)(void *opaque, uint8_t *buf, int buf_size)) {
    return std::thread(&TranscodeFFmpeg::transcode_worker, this, write_packet);
}

int TranscodeFFmpeg::transcode_worker(int (*write_packet)(void *opaque, uint8_t *buf, int buf_size)) {

    if (open_media(FFMPEG_FIFO, &decoder->avfc)) {
        return -1;
    }

    if (prepare_decoder()) {
        return -1;
    }

    avformat_alloc_output_context2(&encoder->avfc, NULL, NULL, encoder->filename);
    if (!encoder->avfc) {
        logging("could not allocate memory for output format");
        return -1;
    }

    AVRational input_framerate = av_guess_frame_rate(decoder->avfc, decoder->video_avs, NULL);
    prepare_video_encoder(decoder->video_avcc, input_framerate);

    if (decoder->audio_avc != NULL) {
        if (prepare_audio_encoder()) {
            return -1;
        }
    }

    if (encoder->avfc->oformat->flags & AVFMT_GLOBALHEADER) {
        encoder->avfc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (write_packet == NULL && fp_output != NULL) {
        // use default file writer
        write_packet = write_buffer_to_file;
    }

    unsigned char* outbuffer = (unsigned char*)av_malloc(32712);
    AVIOContext *avio_out = avio_alloc_context(outbuffer, 32712, 1, NULL, NULL, write_packet, NULL);

    if (avio_out == NULL) {
        av_free(outbuffer);
        return -1;
    }

    encoder->avfc->pb = avio_out;
    encoder->avfc->flags |= AVFMT_FLAG_CUSTOM_IO;

    if (avformat_write_header(encoder->avfc, NULL) < 0) {
        logging("an error occurred when opening output file");
        return -1;
    }

    AVFrame *input_frame = av_frame_alloc();
    if (!input_frame) {
        logging("failed to allocated memory for AVFrame");
        return -1;
    }

    AVPacket *input_packet = av_packet_alloc();
    if (!input_packet) {
        logging("failed to allocated memory for AVPacket");
        return -1;
    }

    while (av_read_frame(decoder->avfc, input_packet) >= 0) {
        if (decoder->avfc->streams[input_packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (transcode_video(input_packet, input_frame)) {
                return -1;
            }

            av_packet_unref(input_packet);
        } else if (decoder->avfc->streams[input_packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (transcode_audio(input_packet, input_frame)) {
                return -1;
            }

            av_packet_unref(input_packet);
        } else {
            logging("ignoring all non video or audio packets");
        }
    }

    av_write_trailer(encoder->avfc);

    if (input_frame != NULL) {
        av_frame_free(&input_frame);
        input_frame = NULL;
    }

    if (input_packet != NULL) {
        av_packet_free(&input_packet);
        input_packet = NULL;
    }

    avfilter_graph_free(&decoder->audio_fgraph);
    avfilter_graph_free(&decoder->video_fgraph);
    avformat_close_input(&decoder->avfc);

    av_free(outbuffer);

    return 0;
}

void TranscodeFFmpeg::pause_video() {
    pause_video_flag = true;
}

void TranscodeFFmpeg::resume_video() {
    pause_video_flag = false;
}

void TranscodeFFmpeg::stop_video() {
    stop_video_flag = true;
}

int TranscodeFFmpeg::get_video_width() {
    if (decoder->video_avcc) {
        return decoder->video_avcc->width;
    }

    return 0;
}

int TranscodeFFmpeg::get_video_height() {
    if (decoder->video_avcc) {
        return decoder->video_avcc->height;
    }

    return 0;
}
