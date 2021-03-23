/**
 * Based on the work, source code and tutorial of Leandro Moreira (https://github.com/leandromoreira/ffmpeg-libav-tutorial)
 * Especially on https://github.com/leandromoreira/ffmpeg-libav-tutorial/blob/master/3_transcoding.c
 */

#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <unistd.h>
#include <thread>
#include <fstream>
#include "logger.h"
#include "encoder.h"
#include "globaldefs.h"
#include "sharedmemory.h"

#define DEBUG_WRITE_TS_TO_FILE 0

// Fix some compile problems
#undef av_err2str
#define av_err2str(errnum) av_make_error_string((char*)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)

#undef av_ts2str
#define av_ts2str(ts) av_ts_make_string((char*)__builtin_alloca(AV_TS_MAX_STRING_SIZE), ts)

#undef av_ts2timestr
#define av_ts2timestr(ts, tb) av_ts_make_time_string((char*)__builtin_alloca(AV_TS_MAX_STRING_SIZE), ts, tb)

FILE *fp_output = nullptr;

bool isVideoStopping;
bool isAudioFinished;
bool isVideoFinished;

int write_buffer_to_shm(void *opaque, uint8_t *buf, int buf_size) {
    if (DEBUG_WRITE_TS_TO_FILE) {
        if (fp_output) {
            fwrite(buf, 1, buf_size, fp_output);
            fflush(fp_output);
        }
    }

    if (sharedMemory.waitForWrite(Data) != -1) {
        sharedMemory.write(buf, buf_size, Data);
    } else {
        /*
        std::string modeString;
        switch (sharedMemory.getMode(Data)) {
            case shmpWriteMode: modeString = "shmpWriteMode"; break;
            case shmpReadMode: modeString = "shmpReadMode"; break;
            case shmpCurrentlyReading: modeString = "shmpCurrentlyReading"; break;
            default: modeString = "<unknown>";
        }
        CONSOLE_ERROR("Unable to write video data to shared memory: Current Status {}, {}", sharedMemory.getMode(Data), modeString);

        // reset mode
        sharedMemory.setMode(shmpWriteMode, Data);
        */

        // stop the encoder. VDR is not able to read the data. Possibly stopped.
        isVideoStopping = true;
        sharedMemory.setMode(shmpWriteMode, Data);
    }

    return buf_size;
}

std::string getbrowserexepath() {
    char result[ PATH_MAX ];
    ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
    return std::string(result, static_cast<unsigned long>((count > 0) ? count : 0));
}

Encoder::Encoder() {
    encoder = (StreamingContext *) calloc(1, sizeof(StreamingContext));

    isVideoStopping = false;

    swsCtx = sws_getContext(1280,
                            720,
                            AV_PIX_FMT_BGRA,
                            1280, 720,
                            AV_PIX_FMT_YUVA420P,
                            SWS_BICUBIC, nullptr, nullptr, nullptr);

    asprintf(&encoder->filename, "%s.ts", "movie/streaming");

    if (DEBUG_WRITE_TS_TO_FILE) {
        fp_output = fopen(encoder->filename, "wb+");
        if (!fp_output) {
            CONSOLE_ERROR("Cannot open file {}", encoder->filename);
        } else {
            CONSOLE_INFO("Write to file {}", encoder->filename);
        }
    }

    // TODO: use hardcoded values. More than 2 needs some justifications
    channelCount = 2;
    sampleRate = 48000;

    startEncoder();
}

Encoder::~Encoder() {
    isVideoStopping = true;

    stopEncoder();

    if (video_output_packet != nullptr) {
        av_packet_unref(video_output_packet);
        av_packet_free(&video_output_packet);
    }

    if (audio_output_packet != nullptr) {
        av_packet_unref(audio_output_packet);
        av_packet_free(&audio_output_packet);
    }

    if (audioFrame != nullptr) {
        av_frame_free(&audioFrame);
    }

    if (videoFrame != nullptr) {
        av_freep(&videoFrame->data[0]);
        av_frame_free(&videoFrame);
    }

    if (encoder->avfc != nullptr) {
        av_write_trailer(encoder->avfc);
        av_free(outbuffer);

        avcodec_free_context(&encoder->video_avcc);
        avcodec_free_context(&encoder->audio_avcc);
        avformat_free_context(encoder->avfc);
    }

    if (swsCtx != nullptr) {
        sws_freeContext(swsCtx);
    }
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

void Encoder::loadConfiguration(void *avcc_priv_data) {
    CONSOLE_DEBUG("Load x264 configuration");

    std::string exepath = getbrowserexepath();

    std::ifstream infile(exepath.substr(0, exepath.find_last_of("/")) + "/x264_encoding.settings");
    if (infile.is_open()) {
        std::string line;
        while (getline(infile, line)) {
            if (line[0] == '#' || line.empty()) {
                continue;
            }

            auto delimiterPos = line.find("=");
            auto key = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);
            trim(key);
            trim(value);

            av_opt_set(avcc_priv_data, key.c_str(), value.c_str(), 0);

            CONSOLE_DEBUG("   {} = {}", key, value);
        }
    } else {
        // fallback
        av_opt_set(avcc_priv_data, "preset", "superfast", 0);
        av_opt_set(avcc_priv_data, "tune", "zerolatency", 0);
        av_opt_set(avcc_priv_data, "keyint", "10", 0);
        av_opt_set(avcc_priv_data, "min-keyint", "10", 0);
        av_opt_set(avcc_priv_data, "scenecut", "0", 0);
        av_opt_set(avcc_priv_data, "force-cfr", "1", 0);
        av_opt_set(avcc_priv_data, "cfr", "28", 0);

        CONSOLE_DEBUG("   use default values");
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

    loadConfiguration(encoder->video_avcc->priv_data);

    encoder->video_avcc->height = height;
    encoder->video_avcc->width = width;

    if (encoder->video_avc->pix_fmts) {
        encoder->video_avcc->pix_fmt = encoder->video_avc->pix_fmts[0];
    } else {
        encoder->video_avcc->pix_fmt = AV_PIX_FMT_YUV420P;
    }

    encoder->video_avcc->time_base = input_framerate;
    encoder->video_avs->time_base = AVRational {1, 1000000};

    if (avcodec_open2(encoder->video_avcc, encoder->video_avc, nullptr) < 0) {
        CONSOLE_ERROR("could not open the video codec");
        return -1;
    }
    avcodec_parameters_from_context(encoder->video_avs->codecpar, encoder->video_avcc);

    // prepare video frame
    videoFrame = av_frame_alloc();

    if (!videoFrame) {
        av_log(nullptr, AV_LOG_ERROR, "error creating video frame\n");
        isVideoFinished = true;

        return -1;
    }

    videoFrame->width = 1280;
    videoFrame->height = 720;
    videoFrame->format = AV_PIX_FMT_YUVA420P;

    int ret = av_image_alloc(videoFrame->data, videoFrame->linesize, videoFrame->width, videoFrame->height, AV_PIX_FMT_YUVA420P, 24);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Could not allocate raw picture buffer: %d, %s\n", ret, av_err2str(ret));
        isVideoFinished = true;

        return -1;
    }

    video_output_packet = av_packet_alloc();

    if (!video_output_packet) {
        CONSOLE_ERROR("could not allocate memory for output packet");
        return -1;
    }

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

    encoder->video_avs->time_base = AVRational {1, 1000000};

    if (avcodec_open2(encoder->audio_avcc, encoder->audio_avc, nullptr) < 0) {
        CONSOLE_ERROR("could not open the audio codec");
        return -1;
    }
    avcodec_parameters_from_context(encoder->audio_avs->codecpar, encoder->audio_avcc);

    // prepare the audio frame
    audioFrame = av_frame_alloc();

    if (!audioFrame) {
        av_log(nullptr, AV_LOG_ERROR, "error creating audio frame\n");
        isAudioFinished = true;

        return -1;
    }

    audioFrame->nb_samples     = encoder->audio_avcc->frame_size;
    audioFrame->format         = AV_SAMPLE_FMT_FLTP;
    audioFrame->channel_layout = AV_CH_LAYOUT_STEREO;

    audio_output_packet = av_packet_alloc();
    if (!audio_output_packet) {
        CONSOLE_ERROR("could not allocate memory for output packet");
        return -1;
    }

    return 0;
}

int Encoder::encode_video(AVFrame *input_frame) {
    if (input_frame) input_frame->pict_type = AV_PICTURE_TYPE_NONE;

    int response = avcodec_send_frame(encoder->video_avcc, input_frame);

    if (response < 0) {
        CONSOLE_ERROR("Error sending packet to encoder: {} -> {}", response, av_err2str(response));
    }

    while (response >= 0) {
        response = avcodec_receive_packet(encoder->video_avcc, video_output_packet);

        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            auto errstr = av_err2str(response);
            CONSOLE_ERROR("Error while receiving packet from encoder: {} -> {}", response, errstr);
            return -1;
        }

        video_output_packet->stream_index = 0;
        video_output_packet->pts = av_rescale_q(video_output_packet->pts, (AVRational){1, 1000000}, encoder->video_avs->time_base);
        video_output_packet->dts = av_rescale_q(video_output_packet->dts, (AVRational){1, 1000000}, encoder->video_avs->time_base);

        av_packet_rescale_ts(video_output_packet, (AVRational){1, 90000}, encoder->video_avs->time_base);

        // sanity test
        if (video_output_packet->dts != AV_NOPTS_VALUE || video_output_packet->pts != AV_NOPTS_VALUE) {
            response = av_interleaved_write_frame(encoder->avfc, video_output_packet);
        }

        if (response != 0) {
            CONSOLE_ERROR("Error {} while receiving packet from decoder: {}", response, av_err2str(response));
            return -1;
        }
    }
    av_packet_unref(video_output_packet);
    return 0;
}

int Encoder::encode_audio(AVFrame *input_frame) {
    int response = avcodec_send_frame(encoder->audio_avcc, input_frame);

    while (response >= 0) {
        response = avcodec_receive_packet(encoder->audio_avcc, audio_output_packet);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            CONSOLE_ERROR("Error while receiving packet from encoder: {}", av_err2str(response));
            return -1;
        }

        audio_output_packet->stream_index = 1;
        audio_output_packet->pts = av_rescale_q(audio_output_packet->pts, (AVRational){1, 1000000}, encoder->audio_avs->time_base);
        audio_output_packet->dts = av_rescale_q(audio_output_packet->dts, (AVRational){1, 1000000}, encoder->audio_avs->time_base);

        av_packet_rescale_ts(audio_output_packet, (AVRational){1, 90000}, encoder->audio_avs->time_base);

        // sanity test
        if (audio_output_packet->dts != AV_NOPTS_VALUE || audio_output_packet->pts != AV_NOPTS_VALUE) {
            response = av_interleaved_write_frame(encoder->avfc, audio_output_packet);
        }

        if (response != 0) {
            CONSOLE_ERROR("Error {} while receiving packet from decoder: {}", response, av_err2str(response));
            return -1;
        }
    }

    av_packet_unref(audio_output_packet);
    return 0;
}

int Encoder::startEncoder() {
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

    int bufferSize = 32712;
    // int bufferSize = 4 * 188;

    outbuffer = (unsigned char *) av_malloc(bufferSize);
    AVIOContext *avio_out = avio_alloc_context(outbuffer, bufferSize, 1, nullptr, nullptr, write_buffer_to_shm, nullptr);

    if (avio_out == nullptr) {
        av_free(outbuffer);
        return -1;
    }

    encoder->avfc->pb = avio_out;
    encoder->avfc->flags = AVFMT_FLAG_CUSTOM_IO;
    encoder->avfc->flags |= AVFMT_FLAG_NOBUFFER;
    encoder->avfc->flags |= AVFMT_FLAG_FLUSH_PACKETS;

    if (avformat_write_header(encoder->avfc, nullptr) < 0) {
        CONSOLE_ERROR("an error occurred when opening output file");
        return -1;
    }

    return 0;
}

void Encoder::stopEncoder() {
    isVideoStopping = true;

    flush();

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

void Encoder::flush() {
    /* flush the encoder */
    encode_audio(nullptr);
    encode_video(nullptr);
}

void Encoder::setAudioParameters(int channels, int sample_rate) {
    // TODO: at this moment, ignore the channels and sample_rate parameter
    //  and use instead hardcoded values

    // channelCount = channels;
    // sampleRate = sample_rate;

    channelCount = 2;
    sampleRate = 48000;
}

// add an bgra image
bool Encoder::addVideoFrame(int width, int height, uint8_t *image, uint64_t pts) {
    if (isVideoStopping) {
        isVideoFinished = true;
        return false;
    }

    isVideoFinished = false;

    uint8_t *inData[1] = { image };
    int inLinesize[1] = { 4 * width };

    // scale and convert to yuv
    sws_scale(swsCtx, inData, inLinesize, 0, height, videoFrame->data, videoFrame->linesize);

    videoFrame->pts = pts;
    encode_video(videoFrame);

    isVideoFinished = true;

    return true;
}

bool Encoder::addAudioFrame(const float **data, int frames, uint64_t pts) {
    if (isVideoStopping) {
        isAudioFinished = true;
        return false;
    }

    isAudioFinished = false;

    for (int i = 0; i < channelCount; i++) {
        audioFrame->data[i] = (uint8_t *) data[i];
    }

    audioFrame->pts = pts;
    encode_audio(audioFrame);

    isAudioFinished = true;

    return true;
}

void Encoder::disable() {
    isVideoStopping = true;
}

void Encoder::enable() {
    isVideoStopping = false;
}
