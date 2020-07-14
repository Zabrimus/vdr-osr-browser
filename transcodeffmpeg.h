#ifndef VDR_OSR_BROWSER_TRANSCODEFFMPEG_H
#define VDR_OSR_BROWSER_TRANSCODEFFMPEG_H

#include <cstdint>
#include <string>
#include <thread>

enum Protocol { UDP, TCP, UNIX };

class TranscodeFFmpeg {
private:
    // input file/url
    std::string input_file;
    std::string transparent_time;
    bool seekPossible;

    // verbose ffmpeg.
    bool verbose_ffmpeg;

    // ffmpeg/ffprobe executables
    std::string ffmpeg_executable;
    std::string ffprobe_executable;

    // Transport protocol
    Protocol protocol;

    // UDP packet size/buffer configuration
    uint32_t udp_packet_size;
    uint32_t udp_buffer_size;

    // transcode or copy audio/video
    bool copy_audio;
    bool copy_video;

    std::string encode_video_param;
    std::string encode_audio_param;

    std::string user_agent;
    std::string cookies;

private:
    // fork ffmpeg and processes the input file
    bool fork_ffmpeg(long start_at_ms);

    // read the configuration file
    void read_configuration();

public:
    TranscodeFFmpeg(Protocol proto);
    ~TranscodeFFmpeg();

    void set_user_agent(std::string ua);
    void set_cookies(std::string co);
    bool set_input(const char* time, const char* input, bool verbose = false);

    void set_event_callback(void (*event_callback_)(std::string cmd));

    void pause_video();
    void resume_video();
    void stop_video();
    void seek_video(const char* ms);
    void speed_video(const char* speed);
};

#endif // VDR_OSR_BROWSER_TRANSCODEFFMPEG_H
