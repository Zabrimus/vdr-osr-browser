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
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include "logger.h"
#include "encoder.h"

#define DEBUG_WRITE_TS_TO_FILE 0

// Fix some compile problems
#undef av_err2str
#define av_err2str(errnum) av_make_error_string((char*)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)

#undef av_ts2str
#define av_ts2str(ts) av_ts_make_string((char*)__builtin_alloca(AV_TS_MAX_STRING_SIZE), ts)

#undef av_ts2timestr
#define av_ts2timestr(ts, tb) av_ts_make_time_string((char*)__builtin_alloca(AV_TS_MAX_STRING_SIZE), ts, tb)

FILE *fp_output = nullptr;
OSRHandler *osrHandler = nullptr;

bool isVideoStopping;
bool isAudioFinished;
bool isVideoFinished;

typedef struct RawVideoPicture {
    int status;

    int width;
    int height;
    uint8_t* image;
    uint64_t pts;
} RawVideoPicture;

typedef struct RawPCMAudio {
    int status;

    int audioBufferSize;
    uint64_t pts;
    uint8_t **pcm;
} RawPCMAudio;


RawVideoPicture decodedPicture;
RawPCMAudio decodedAudio;

int write_buffer_to_file(void *opaque, uint8_t *buf, int buf_size) {
    if (!feof(fp_output)) {
        int true_size = fwrite(buf,1,buf_size, fp_output);
        return true_size;
    } else {
        return -1;
    }
}

int write_buffer_to_shm(void *opaque, uint8_t *buf, int buf_size) {
    if (DEBUG_WRITE_TS_TO_FILE) {
        write_buffer_to_file(opaque, buf, buf_size);
    }

    if (osrHandler != nullptr) {
        return osrHandler->writeVideoToShm(buf, buf_size);
    }

    // discard buffer
    return buf_size;
}

Encoder::Encoder(OSRHandler *osrHndl, const char* out, bool writeToFile) {
    osrHandler = osrHndl;
    encoder = (StreamingContext *) calloc(1, sizeof(StreamingContext));

    swsCtx = nullptr;
    isVideoStopping = false;

    asprintf(&encoder->filename, "%s.ts", out);

    if (writeToFile || DEBUG_WRITE_TS_TO_FILE) {
        fp_output = fopen(encoder->filename, "wb+");
    }

    encoder->writeToFile = writeToFile;

    // TODO: The image size is predefined as 1280 x 720.
    //  If images with another size shall be supported, then
    //  these values must be adjusted.
    decodedPicture.image = (uint8_t *) calloc(1, 1280 * 720 * 4);
    decodedPicture.width = 0;
    decodedPicture.height = 0;
    decodedPicture.pts = 0;
    decodedPicture.status = 0;

    // TODO: Currently max. 8 channels are supported. If more are
    //  necessary, then this needs to be adapted
    decodedAudio.status = 0;
    decodedAudio.pts = 0;
    decodedAudio.pcm = (uint8_t **) calloc(1, 8 * sizeof(uint8_t*));
    decodedAudio.audioBufferSize = av_samples_get_buffer_size(NULL, 1,1024, AV_SAMPLE_FMT_FLTP, 0);
    for (int i = 0; i < 8; ++i) {
        decodedAudio.pcm[i] = (uint8_t*) calloc(1, decodedAudio.audioBufferSize);
    }
}

Encoder::~Encoder() {
    isVideoStopping = true;

    free(decodedPicture.image);

    for (int i = 0; i < 8; ++i) {
        free(decodedAudio.pcm[i]);
    }
    free(decodedAudio.pcm);

    av_write_trailer(encoder->avfc);
    av_free(outbuffer);

    avformat_free_context(encoder->avfc);

    sws_freeContext(swsCtx);
    swsCtx = nullptr;

    if (encoder->filename) {
        free(encoder->filename);
    }

    free(encoder);
    encoder = nullptr;

    if (fp_output != nullptr) {
        fclose(fp_output);
    }
}

int Encoder::prepare_video_encoder(int width, int height, AVRational input_framerate) {
    encoder->video_avs = avformat_new_stream(encoder->avfc, nullptr);
    encoder->video_avc = avcodec_find_encoder_by_name("libx264");

    if (!encoder->video_avc) {
        CONSOLE_ERROR("could not find the proper codec");
        return -1;
    }

    encoder->video_avcc = avcodec_alloc_context3(encoder->video_avc);
    if (!encoder->video_avcc) {
        CONSOLE_ERROR("could not allocated memory for codec context");
        return -1;
    }

    // Einstellungen 1: Die Latency ist noch viel zu hoch für meinen Geschmack
    // av_opt_set(encoder->video_avcc->priv_data, "preset", "ultrafast", 0);
    // av_opt_set(encoder->video_avcc->priv_data, "tune", "zerolatency", 0);
    // av_opt_set(encoder->video_avcc->priv_data, "x264-params", "keyint=10:min-keyint=10:scenecut=0:force-cfr=1:crf=28", 0);

    // Einstellungen 2: https://obsproject.com/forum/resources/low-latency-high-performance-x264-options-for-for-most-streaming-services-youtube-facebook.726/
    // common: profile=main:keyint=2:nal-hrd=cbr:vbv-bufsize=2000:bf=2:b-adapt=0:partitions=none:scenecut=0:no-weightb:weightp=0:sliced-threads
    av_opt_set(encoder->video_avcc->priv_data, "preset", "superfast", 0);
    av_opt_set(encoder->video_avcc->priv_data, "profile", "main", 0);
    av_opt_set(encoder->video_avcc->priv_data, "bf", "2", 0);
    av_opt_set(encoder->video_avcc->priv_data, "keyint", "2", 0);
    // av_opt_set(encoder->video_avcc->priv_data, "nal-hrd", "cbr", 0);
    // av_opt_set(encoder->video_avcc->priv_data, "vbv-bufsize", "2000", 0);
    // av_opt_set(encoder->video_avcc->priv_data, "vbv-maxrate", "768", 0);
    av_opt_set(encoder->video_avcc->priv_data, "b-adapt", "0", 0);
    av_opt_set(encoder->video_avcc->priv_data, "partitions", "none", 0);
    av_opt_set(encoder->video_avcc->priv_data, "scenecut", "0", 0);
    // av_opt_set(encoder->video_avcc->priv_data, "no-weightb", "1", 0);
    // av_opt_set(encoder->video_avcc->priv_data, "weightp", "0", 0);
    av_opt_set(encoder->video_avcc->priv_data, "sliced-threads", "1", 0);
    av_opt_set(encoder->video_avcc->priv_data, "threads", "4", 0);


    // Set #1: Low Latency, High Performance
    // Set #1.1:  film/animation/grain/stillimage: aq-mode=0 subme=0 no-deblock
    /* */
    av_opt_set(encoder->video_avcc->priv_data, "aq-mode", "0", 0);
    av_opt_set(encoder->video_avcc->priv_data, "subme", "0", 0);
    av_opt_set(encoder->video_avcc->priv_data, "no-deblock", "1", 0);
    /* */

    // Set #1.2:  zerolatency: aq-mode=0 subme=0 no-deblock sync-lookahead=3
    /*
    av_opt_set(encoder->video_avcc->priv_data, "aq-mode", "0", 0);
    av_opt_set(encoder->video_avcc->priv_data, "subme", "0", 0);
    av_opt_set(encoder->video_avcc->priv_data, "no-deblock", "1", 0);
    av_opt_set(encoder->video_avcc->priv_data, "sync-lookahead", "3", 0);
    */

    // Set #2: Low Latency
    // Set #2.1: film/animation/grain/stillimage/zerolatency: force-cfr aq-mode=0 subme=0 no-deblock

    // Set #3: Balanced
    // Set #3.1: film: aq-mode=0 subme=0
    // Set #3.2: animation/grain/stillimage: subme=0
    // Set #3.3: zerolatency: aq-mode=0 subme=0 no-deblock sync-lookahead=3

    // Set #4: Quality (maintains the settings of each tune)
    // Set #4.1: film: aq-mode=0 subme=0 trellis=1
    // Set #4.2: animation: subme=6
    // Set #4.3: grain: subme=0 trellis=1
    // Set #4.4: stillimage: subme=6 trellis=1
    // Set #4.5: zerolatency: force-cfr aq-mode=0 subme=0 no-deblock



    encoder->video_avcc->height = height;
    encoder->video_avcc->width = width;
    // encoder->video_avcc->sample_aspect_ratio = decoder_ctx->sample_aspect_ratio;

    if (encoder->video_avc->pix_fmts) {
        encoder->video_avcc->pix_fmt = encoder->video_avc->pix_fmts[0];
    } else {
        encoder->video_avcc->pix_fmt = AV_PIX_FMT_YUV420P;
    }

    encoder->video_avcc->time_base = input_framerate;
    encoder->video_avs->time_base = AVRational {1, 1000000};

    // encoder->video_avcc->time_base = AVRational {1, 1000000};
    // encoder->video_avcc->sample_rate = 1000;
    // encoder->video_avs->time_base = input_framerate;

    if (avcodec_open2(encoder->video_avcc, encoder->video_avc, nullptr) < 0) {
        CONSOLE_ERROR("could not open the video codec");
        return -1;
    }
    avcodec_parameters_from_context(encoder->video_avs->codecpar, encoder->video_avcc);
    return 0;
}

int Encoder::prepare_audio_encoder(int channels, int sample_rate, AVRational input_framerate) {
    encoder->audio_avs = avformat_new_stream(encoder->avfc, nullptr);
    encoder->audio_avc = avcodec_find_encoder_by_name("aac");

    if (!encoder->audio_avc) {
        CONSOLE_ERROR("could not find the proper audio codec");
        return -1;
    }

    encoder->audio_avcc = avcodec_alloc_context3(encoder->audio_avc);
    if (!encoder->audio_avcc) {
        CONSOLE_ERROR("could not allocated memory for audio codec context");
        return -1;
    }

    encoder->audio_avcc->channels = channels;
    encoder->audio_avcc->channel_layout = av_get_default_channel_layout(channels);
    encoder->audio_avcc->sample_rate = sample_rate;
    encoder->audio_avcc->sample_fmt = AV_SAMPLE_FMT_FLTP;
    // encoder->audio_avcc->bit_rate = decoder->audio_avcc->bit_rate;
    encoder->audio_avcc->time_base = input_framerate;

    encoder->audio_avcc->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    // encoder->audio_avs->time_base = input_framerate;
    encoder->video_avs->time_base = AVRational {1, 1000000};

    if (avcodec_open2(encoder->audio_avcc, encoder->audio_avc, nullptr) < 0) {
        CONSOLE_ERROR("could not open the audio codec");
        return -1;
    }
    avcodec_parameters_from_context(encoder->audio_avs->codecpar, encoder->audio_avcc);

    return 0;
}

int Encoder::encode_video(uint64_t pts, AVFrame *input_frame) {
    if (input_frame) input_frame->pict_type = AV_PICTURE_TYPE_NONE;

    AVPacket *output_packet = av_packet_alloc();
    if (!output_packet) {
        CONSOLE_ERROR("could not allocate memory for output packet");
        return -1;
    }

    input_frame->pts = pts;

    int response = avcodec_send_frame(encoder->video_avcc, input_frame);

    while (response >= 0) {
        response = avcodec_receive_packet(encoder->video_avcc, output_packet);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            auto errstr = av_err2str(response);
            CONSOLE_ERROR("Error while receiving packet from encoder: {} -> {}", response, errstr);
            return -1;
        }

        output_packet->stream_index = 0;
        output_packet->pts = av_rescale_q(output_packet->pts, (AVRational){1, 1000000}, encoder->video_avs->time_base);
        output_packet->dts = av_rescale_q(output_packet->dts, (AVRational){1, 1000000}, encoder->video_avs->time_base);

        // av_packet_rescale_ts(output_packet, encoder->video_avcc->time_base, encoder->video_avs->time_base);
        av_packet_rescale_ts(output_packet, (AVRational){1, 90000}, encoder->video_avs->time_base);

        // log_packet(encoder->avfc, output_packet);

        // sanity test
        if (output_packet->dts != AV_NOPTS_VALUE || output_packet->pts != AV_NOPTS_VALUE) {
            // response = av_interleaved_write_frame(encoder->avfc, output_packet);
            response = av_write_frame(encoder->avfc, output_packet);
        }

        if (response != 0) {
            CONSOLE_ERROR("Error {} while receiving packet from decoder: {}", response, av_err2str(response));
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
        CONSOLE_ERROR("could not allocate memory for output packet");
        return -1;
    }

    input_frame->pts = pts;

    int response = avcodec_send_frame(encoder->audio_avcc, input_frame);

    while (response >= 0) {
        response = avcodec_receive_packet(encoder->audio_avcc, output_packet);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            CONSOLE_ERROR("Error while receiving packet from encoder: {}", av_err2str(response));
            return -1;
        }

        output_packet->stream_index = 1;
        output_packet->pts = av_rescale_q(output_packet->pts, (AVRational){1, 1000000}, encoder->audio_avs->time_base);
        output_packet->dts = av_rescale_q(output_packet->dts, (AVRational){1, 1000000}, encoder->audio_avs->time_base);

        // av_packet_rescale_ts(output_packet, encoder->video_avcc->time_base, encoder->video_avs->time_base);
        av_packet_rescale_ts(output_packet, (AVRational){1, 90000}, encoder->audio_avs->time_base);


        // output_packet->pts = av_rescale_q(pts, (AVRational){1, 1000000}, encoder->video_avs->time_base);
        // output_packet->dts = AV_NOPTS_VALUE;
        // output_packet->dts = output_packet->pts;

        // av_packet_rescale_ts(output_packet, encoder->audio_avcc->time_base, encoder->audio_avs->time_base);
        // av_packet_rescale_ts(output_packet, (AVRational){1, 1000000}, encoder->audio_avs->time_base);

        // sanity test
        if (output_packet->dts != AV_NOPTS_VALUE || output_packet->pts != AV_NOPTS_VALUE) {
            // response = av_interleaved_write_frame(encoder->avfc, output_packet);
            response = av_write_frame(encoder->avfc, output_packet);
        }

        if (response != 0) {
            CONSOLE_ERROR("Error {} while receiving packet from decoder: {}", response, av_err2str(response));
            return -1;
        }
    }
    av_packet_unref(output_packet);
    av_packet_free(&output_packet);
    return 0;
}

void Encoder::Start() {
    // main wait loop
    while (!isVideoStopping) {
        std::this_thread::sleep_for(std::chrono::microseconds(500));

        if (decodedPicture.status == 1) {
            processVideoFrame();
        }

        if (decodedAudio.status == 1) {
            processAudioFrame();
        }
    }

    // wait for the encoder
    bool isRunning = true;
    while (isRunning) {
        if (isVideoFinished && isAudioFinished) {
            isRunning = false;
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
}

int Encoder::startEncoder(int (*write_packet)(void *opaque, uint8_t *buf, int buf_size)) {

    if (encoder->writeToFile) {
        // use default file writer
        write_packet = write_buffer_to_file;
    } else {
        // socket writer
        write_packet = write_buffer_to_shm;
    }

    int response = avformat_alloc_output_context2(&encoder->avfc, nullptr, nullptr, encoder->filename);
    if (!encoder->avfc) {
        CONSOLE_ERROR("Error {}: could not allocate memory for output format: {}", response, av_err2str(response));
        return -1;
    }

    if (prepare_video_encoder(1280, 720, (AVRational) {1, 25}) != 0) {
        CONSOLE_CRITICAL("Prepare video encoder failed.");
        return -1;
    }

    if (prepare_audio_encoder(channelCount, sampleRate, (AVRational) {1, 25}) != 0) {
        CONSOLE_CRITICAL("Prepare audio encoder failed.");
        return -1;
    }

    if (encoder->avfc->oformat->flags & AVFMT_GLOBALHEADER)
        encoder->avfc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // int bufferSize = 32712;
    int bufferSize = 1000 * 188;
    outbuffer = (unsigned char *) av_malloc(bufferSize);
    AVIOContext *avio_out = avio_alloc_context(outbuffer, bufferSize, 1, nullptr, nullptr, write_packet, nullptr);

    if (avio_out == nullptr) {
        av_free(outbuffer);
        return -1;
    }

    encoder->avfc->pb = avio_out;
    encoder->avfc->flags = AVFMT_FLAG_CUSTOM_IO;

    if (avformat_write_header(encoder->avfc, nullptr) < 0) {
        CONSOLE_ERROR("an error occurred when opening output file");
        return -1;
    }

    return 0;
}

void Encoder::stopEncoder() {
    isVideoStopping = true;
}

void Encoder::setAudioParameters(int channels, int sample_rate) {
    channelCount = channels;
    sampleRate = sample_rate;
}

// add an bgra image
void Encoder::addVideoFrame(int width, int height, uint8_t *image, uint64_t pts) {
    while (decodedPicture.status != 0 && !isVideoStopping) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    decodedPicture.height = height;
    decodedPicture.width = width;
    decodedPicture.pts = pts;

    memcpy(decodedPicture.image, image, width * height * 4);
    decodedPicture.status = 1;
}

void Encoder::processVideoFrame() {
    if (isVideoStopping) {
        isVideoFinished = true;
        return;
    }

    if (decodedPicture.status == 0) {
        return;
    }

    decodedPicture.status = 2;
    isVideoFinished = false;

    AVFrame *videoFrame;

    // get sws context
    if (swsCtx == nullptr) {
        swsCtx = sws_getContext(decodedPicture.width,
                                decodedPicture.height,
                                AV_PIX_FMT_BGRA,
                                1280, 720,
                                AV_PIX_FMT_YUVA420P,
                                SWS_BICUBIC, nullptr, nullptr, nullptr);
    } else {
        swsCtx = sws_getCachedContext(swsCtx,
                                      decodedPicture.width,
                                      decodedPicture.height,
                                      AV_PIX_FMT_BGRA,
                                     1280, 720,
                                     AV_PIX_FMT_YUVA420P,
                                      SWS_BICUBIC, nullptr, nullptr, nullptr);
    }

    videoFrame = av_frame_alloc();

    if (!videoFrame) {
        av_log(nullptr, AV_LOG_ERROR, "error creating video frame\n");
        isVideoFinished = true;

        decodedPicture.status = 0;
        return;
    }

    videoFrame->width = 1280;
    videoFrame->height = 720;
    videoFrame->format = AV_PIX_FMT_YUVA420P;

    int ret = av_image_alloc(videoFrame->data, videoFrame->linesize, videoFrame->width, videoFrame->height, AV_PIX_FMT_YUVA420P, 24);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Could not allocate raw picture buffer: %d, %s\n", ret, av_err2str(ret));
        isVideoFinished = true;

        decodedPicture.status = 0;

        return;
    }

    uint8_t *inData[1] = { decodedPicture.image };
    int inLinesize[1] = { 4 * decodedPicture.width };

    // scale and convert to yuv
    sws_scale(swsCtx, inData, inLinesize, 0, decodedPicture.height, videoFrame->data, videoFrame->linesize);

    encode_video(decodedPicture.pts, videoFrame);

    av_freep(&videoFrame->data[0]);
    av_frame_free(&videoFrame);

    isVideoFinished = true;
    decodedPicture.status = 0;
}

void Encoder::addAudioFrame(const float **data, int frames, uint64_t pts) {
     while (decodedAudio.status != 0 && !isVideoStopping) {
         std::this_thread::sleep_for(std::chrono::microseconds(100));
     }

     uint8_t **pcm = (uint8_t **)data;
     for (int i = 0; i < std::min(8, channelCount); i++) {
        memcpy(decodedAudio.pcm[i], pcm[i], decodedAudio.audioBufferSize);
     }

     decodedAudio.pts = pts;
     decodedAudio.status = 1;
}

void Encoder::processAudioFrame() {
    if (isVideoStopping) {
        isAudioFinished = true;
        return;
    }

    if (decodedAudio.status == 0) {
        return;
    }

    decodedAudio.status = 2;
    isAudioFinished = false;

    AVFrame *audioFrame;

    audioFrame = av_frame_alloc();

    if (!audioFrame) {
        av_log(nullptr, AV_LOG_ERROR, "error creating audio frame\n");
        isAudioFinished = true;
        decodedAudio.status = 0;
        return;
    }

    audioFrame->nb_samples     = encoder->audio_avcc->frame_size;
    audioFrame->format         = AV_SAMPLE_FMT_FLTP;
    audioFrame->channel_layout = AV_CH_LAYOUT_STEREO;

    for (int i = 0; i < 2; i++)
        audioFrame->data[i] = decodedAudio.pcm[i];

    encode_audio(decodedAudio.pts, audioFrame);

    av_frame_free(&audioFrame);

    isAudioFinished = true;
    decodedAudio.status = 0;
}