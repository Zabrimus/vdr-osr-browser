#ifndef TRANSCODEFFMPEG_H
#define TRANSCODEFFMPEG_H

#include <cstdint>
#include <string>
#include <thread>

class TranscodeFFmpeg {
private:
    // input file/url
    std::string input_file;
    std::string transparent_time;

    // verbose ffmpeg.
    bool verbose_ffmpeg;

    // transcode in realtime, must be true if VDR shall play the video
    bool realtime;

    // ffmpeg/ffprobe executables
    std::string ffmpeg_executable;
    std::string ffprobe_executable;

    // transcode or copy audio/video
    bool copy_audio;
    bool copy_video;
    uint8_t tsbuf[188];
    int filled;

    std::string encode_video_param;
    std::string encode_audio_param;

    std::string user_agent;
    std::string cookies;

private:
    // fork ffmpeg and processes the input file
    bool fork_ffmpeg(long start_at_ms);

    // read the configuration file
    void read_configuration();

    // the main procedure which reads and process incoming data from ffmpeg
    int transcode_worker(int (*write_packet)(uint8_t *buf, int buf_size));

public:
    TranscodeFFmpeg();
    ~TranscodeFFmpeg();

    void set_user_agent(std::string ua);
    void set_cookies(std::string co);
    bool set_input(const char* time, const char* input, bool verbose = false);

    std::thread transcode(int (*write_packet)(uint8_t *buf, int buf_size), bool realtime = true);

    void set_event_callback(void (*event_callback_)(std::string cmd));

    void pause_video();
    void resume_video();
    void stop_video();
    std::thread seek_video(const char* ms, int (*write_packet)(uint8_t *buf, int buf_size));
    void speed_video(const char* speed);
};

#endif // TRANSCODEFFMPEG_H
