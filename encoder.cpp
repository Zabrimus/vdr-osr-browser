/**
 * Based on the work, source code and tutorial of Leandro Moreira (https://github.com/leandromoreira/ffmpeg-libav-tutorial)
 * Especially on https://github.com/leandromoreira/ffmpeg-libav-tutorial/blob/master/3_transcoding.c
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <chrono>

#include "encoder.h"

// Fix some compile problems
#undef av_err2str
#define av_err2str(errnum) av_make_error_string((char*)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)

#undef av_ts2str
#define av_ts2str(ts) av_ts_make_string((char*)__builtin_alloca(AV_TS_MAX_STRING_SIZE), ts)

#undef av_ts2timestr
#define av_ts2timestr(ts, tb) av_ts_make_time_string((char*)__builtin_alloca(AV_TS_MAX_STRING_SIZE), ts, tb)

FILE *fp_output = nullptr;

int write_buffer_to_file(void *opaque, uint8_t *buf, int buf_size) {
    if (!feof(fp_output)) {
        int true_size = fwrite(buf,1,buf_size, fp_output);
        return true_size;
    } else {
        return -1;
    }
}

Encoder::Encoder(const char* out, bool write2File) {
    encoder = (StreamingContext *) calloc(1, sizeof(StreamingContext));
    asprintf(&encoder->filename, "%s.ts", out);

    if (write2File) {
        fp_output = fopen(encoder->filename, "wb+");
    }
}

Encoder::~Encoder() {
    avformat_free_context(encoder->avfc);

    sws_freeContext(swsCtx);
    swsCtx = nullptr;

    free(encoder);
    encoder = nullptr;

    if (fp_output != nullptr) {
        fclose(fp_output);
    }

    if (audioFrame != nullptr) {
        av_free(audioFrame);
    }
}

// Logging functions
void Encoder::logging(const char *fmt, ...)
{
    va_list args;
    fprintf( stderr, "LOG: " );
    va_start( args, fmt );
    vfprintf( stderr, fmt, args );
    va_end( args );
    fprintf( stderr, "\n" );
}

void Encoder::log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    logging("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d",
            av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
            av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
            av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
            pkt->stream_index);
}

void Encoder::print_timing(char *name, AVFormatContext *avf, AVCodecContext *avc, AVStream *avs) {
    logging("=================================================");
    logging("%s", name);

    logging("\tAVFormatContext");
    if (avf != nullptr) {
        logging("\t\tstart_time=%d duration=%d bit_rate=%d start_time_realtime=%d", avf->start_time, avf->duration, avf->bit_rate, avf->start_time_realtime);
    } else {
        logging("\t\t->NULL");
    }

    logging("\tAVCodecContext");
    if (avc != nullptr) {
        logging("\t\tbit_rate=%d ticks_per_frame=%d width=%d height=%d gop_size=%d keyint_min=%d sample_rate=%d profile=%d level=%d ",
                avc->bit_rate, avc->ticks_per_frame, avc->width, avc->height, avc->gop_size, avc->keyint_min, avc->sample_rate, avc->profile, avc->level);
        logging("\t\tavc->time_base=num/den %d/%d", avc->time_base.num, avc->time_base.den);
        logging("\t\tavc->framerate=num/den %d/%d", avc->framerate.num, avc->framerate.den);
        logging("\t\tavc->pkt_timebase=num/den %d/%d", avc->pkt_timebase.num, avc->pkt_timebase.den);
    } else {
        logging("\t\t->NULL");
    }

    logging("\tAVStream");
    if (avs != nullptr) {
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

int Encoder::prepare_video_encoder(int width, int height, AVRational input_framerate) {
    encoder->video_avs = avformat_new_stream(encoder->avfc, nullptr);

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

    encoder->video_avcc->height = height;
    encoder->video_avcc->width = width;
    // encoder->video_avcc->sample_aspect_ratio = decoder_ctx->sample_aspect_ratio;

    if (encoder->video_avc->pix_fmts) {
        encoder->video_avcc->pix_fmt = encoder->video_avc->pix_fmts[0];
    } else {
        encoder->video_avcc->pix_fmt = AV_PIX_FMT_YUV420P;
    }

    encoder->video_avcc->time_base = input_framerate;
    encoder->video_avcc->sample_rate = 1000;
    encoder->video_avs->time_base = input_framerate;

    if (avcodec_open2(encoder->video_avcc, encoder->video_avc, nullptr) < 0) {
        logging("could not open the codec");
        return -1;
    }
    avcodec_parameters_from_context(encoder->video_avs->codecpar, encoder->video_avcc);
    return 0;
}

int Encoder::prepare_audio_encoder(int channels, int sample_rate, AVRational input_framerate) {
    encoder->audio_avs = avformat_new_stream(encoder->avfc, nullptr);

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

    encoder->audio_avcc->channels = channels;
    encoder->audio_avcc->channel_layout = av_get_default_channel_layout(channels);
    encoder->audio_avcc->sample_rate = sample_rate;
    encoder->audio_avcc->sample_fmt = AV_SAMPLE_FMT_FLTP;
    // encoder->audio_avcc->bit_rate = decoder->audio_avcc->bit_rate;
    encoder->audio_avcc->time_base = input_framerate;

    encoder->audio_avcc->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    encoder->audio_avs->time_base = input_framerate;

    if (avcodec_open2(encoder->audio_avcc, encoder->audio_avc, nullptr) < 0) {
        logging("could not open the codec");
        return -1;
    }
    avcodec_parameters_from_context(encoder->audio_avs->codecpar, encoder->audio_avcc);

    // alloc audioFrame
    audioFrame = av_frame_alloc();
    if (!audioFrame) {
        fprintf(stderr, "could not allocate audio frame\n");
        return -1;
    }

    audioFrame->nb_samples     = encoder->audio_avcc->frame_size;
    audioFrame->format         = AV_SAMPLE_FMT_FLTP;

    // FIXME: Das Layout stimmt nicht. Der muss aus der Anzahl der Channels bestimmt werden.
    audioFrame->channel_layout = AV_CH_LAYOUT_STEREO;

    return 0;
}

int Encoder::encode_video(uint64_t pts, AVFrame *input_frame) {
    if (input_frame) input_frame->pict_type = AV_PICTURE_TYPE_NONE;

    AVPacket *output_packet = av_packet_alloc();
    if (!output_packet) {
        logging("could not allocate memory for output packet");
        return -1;
    }

    static int i = 0;

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

        output_packet->stream_index = 0;
        output_packet->pts = av_rescale_q(pts, (AVRational){1, 1000000}, encoder->video_avs->time_base);
        output_packet->dts = AV_NOPTS_VALUE;

        // av_packet_rescale_ts(output_packet, encoder->video_avcc->time_base, encoder->video_avs->time_base);
        // log_packet(encoder->avfc, output_packet);

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

int Encoder::encode_audio(uint64_t pts, AVFrame *input_frame) {
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

        output_packet->stream_index = 1;
        output_packet->pts = av_rescale_q(pts, (AVRational){1, 1000000}, encoder->video_avs->time_base);
        output_packet->dts = AV_NOPTS_VALUE;

        // av_packet_rescale_ts(output_packet, decoder->audio_avs->time_base, encoder->audio_avs->time_base);
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

int Encoder::startEncoder(int (*write_packet)(void *opaque, uint8_t *buf, int buf_size)) {

    if (write_packet == nullptr && fp_output != nullptr) {
        // use default file writer
        write_packet = write_buffer_to_file;
    }

    // avformat_alloc_output_context2(&encoder->avfc, nullptr, nullptr, encoder->filename);
    int response = avformat_alloc_output_context2(&encoder->avfc, nullptr, nullptr, encoder->filename);
    if (!encoder->avfc) {
        logging("Error %d: could not allocate memory for output format: %s", response, av_err2str(response));
        return -1;
    }

    prepare_video_encoder(1280, 720, (AVRational) {1, 25});

    // FIXME: Channels und Sample_Rate werden hier wohl nicht stimmen. Wie komme ich rechtzeitig an die richtigen Werte?
    prepare_audio_encoder(2, 44100, (AVRational) {1, 25});

    if (encoder->avfc->oformat->flags & AVFMT_GLOBALHEADER)
        encoder->avfc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    outbuffer = (unsigned char *) av_malloc(32712);
    AVIOContext *avio_out = avio_alloc_context(outbuffer, 32712, 1, nullptr, nullptr, write_packet, nullptr);

    if (avio_out == nullptr) {
        av_free(outbuffer);
        return -1;
    }

    encoder->avfc->pb = avio_out;
    encoder->avfc->flags = AVFMT_FLAG_CUSTOM_IO;

    if (avformat_write_header(encoder->avfc, nullptr) < 0) {
        logging("an error occurred when opening output file");
        return -1;
    }

    return 0;
}

int Encoder::stopEncoder() {
    av_write_trailer(encoder->avfc);
    av_free(outbuffer);

    return 0;
}

// FIXME: Ist das nicht zu spät? Ich brauche channels und sample_rate früher.
void Encoder::setAudioParameters(int channels, int sample_rate) {
    channelCount = channels;
}

// add an bgra image
int Encoder::addVideoFrame(int width, int height, uint8_t* image, uint64_t pts) {
    AVFrame *videoFrame;

    // get sws context
    swsCtx = sws_getCachedContext(swsCtx, width, height, AV_PIX_FMT_BGRA,
                                  1280, 720, AV_PIX_FMT_YUVA420P,
                                  SWS_BICUBIC, nullptr, nullptr, nullptr);

    videoFrame = av_frame_alloc();

    if (!videoFrame) {
        av_log(nullptr, AV_LOG_ERROR, "error creating video frame\n");
        return -1;
    }

    videoFrame->width = 1280;
    videoFrame->height = 720;
    videoFrame->format = AV_PIX_FMT_YUVA420P;

    int ret = av_image_alloc(videoFrame->data, videoFrame->linesize, videoFrame->width, videoFrame->height, AV_PIX_FMT_YUVA420P, 24);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Could not allocate raw picture buffer: %d, %s\n", ret, av_err2str(ret));
        return -1;
    }

    uint8_t *inData[1] = { image };
    int inLinesize[1] = { 4 * width };

    // scale and convert to yuv
    sws_scale(swsCtx, inData, inLinesize, 0, height, videoFrame->data, videoFrame->linesize);

    encode_video(pts, videoFrame);

    av_frame_free(&videoFrame);
    return 0;
}

int Encoder::addAudioFrame(const float **data, int frames, uint64_t pts) {
    uint8_t **pcm = (uint8_t **)data;
    for (int i = 0; i < 2; i++)
        audioFrame->data[i] = pcm[i];

    encode_audio(pts, audioFrame);

    return 0;
}