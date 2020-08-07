#include "transcodeffmpeg.h"
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include <string.h>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <limits.h>

#include "logger.h"
#include "globaldefs.h"
#include "dashhandler.h"

static const char* DASH_VIDEO_FILE = "movie/dash_video.mp4";
static const char* DASH_AUDIO_FILE = "movie/dash_audio.mp4";

static pid_t ffmpeg_pid;

void (*event_callback)(std::string cmd)  = nullptr;

inline char* ms_to_ffmpeg_time(long millis) {
    char *result;

    int s = millis / 1000;

    int ms = millis % 1000;
    int hours = s / 3600;
    int minutes = (s - hours * 3600) / 60;
    int seconds = s % 60;

    asprintf(&result, "%02d:%02d:%02d.%03d", hours, minutes, seconds, ms);

    return result;
}

std::string getbrowserexepath() {
    char result[ PATH_MAX ];
    ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
    return std::string(result, static_cast<unsigned long>((count > 0) ? count : 0));
}


TranscodeFFmpeg::TranscodeFFmpeg(Protocol proto) {
    protocol = proto;
    alwaysEncodeVideo = false;
    alwaysEncodeAudio = false;
    read_configuration();
}

TranscodeFFmpeg::~TranscodeFFmpeg() {
}

void TranscodeFFmpeg::set_event_callback(void (*event_callback_)(std::string cmd)) {
    event_callback = event_callback_;
}

void TranscodeFFmpeg::read_configuration() {
    CONSOLE_TRACE("====== Loading configuration... ======");

    udp_packet_size = 1316;
    udp_buffer_size = 31960;

    std::string exepath = getbrowserexepath();

    std::ifstream infile(exepath.substr(0, exepath.find_last_of("/")) + "/vdr-osr-ffmpeg.config");
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

            if (key == "encode_video") {
                encode_video_param = value;
            } else if (key == "encode_audio") {
                encode_audio_param = value;
            } else if (key == "always_encode_video") {
                alwaysEncodeVideo = (value == "true");
            } else if (key == "always_encode_audio") {
                alwaysEncodeAudio = (value == "true");
            } else if (key == "ffmpeg_executable") {
                ffmpeg_executable = std::string(value);
            } else if (key == "ffprobe_executable") {
                ffprobe_executable = std::string(value);
            } else if (key == "udp_packet_size") {
                udp_packet_size = strtoul(value.c_str(), NULL, 0);

                if (udp_packet_size >= 1 && udp_packet_size < 188) {
                    udp_packet_size = 188 * udp_packet_size;
                } else if (udp_packet_size < 0 || udp_packet_size > 65424) {
                    udp_packet_size = 1316;
                }

                if (udp_packet_size % 188 != 0) {
                    // must be a multiple of 188
                    udp_packet_size = 1316;
                }
            } else if (key == "udp_buffer_size") {
                udp_buffer_size = strtoul(value.c_str(), NULL, 0);
                if (udp_buffer_size == ULONG_MAX) {
                    udp_buffer_size = 31960;
                }
            } else if (key == "transport") {
                if (value == "TCP") {
                    protocol = TCP;
                } else if (value == "UDP") {
                    protocol = UDP;
                } else {
                    CONSOLE_INFO("Protocol {} is not defined, fallback to UDP");
                }
            }
        }
    }

    if (protocol == UDP) {
        CONSOLE_INFO("Use UDP packet size {}, buffer size {}", udp_packet_size, udp_buffer_size);
    }

    // check command line and set default values if empty
    if (encode_video_param.empty()) {
        encode_video_param = "-c:v libx264 -preset veryfast -x264-params keyint=60:min-keyint=60:scenecut=0:force-cfr=1:crf=28";
    }

    if (encode_audio_param.empty()) {
        encode_audio_param = "-c:a aac -b:a 192k";
    }

    if (ffprobe_executable.empty()) {
        ffprobe_executable = std::string("/usr/bin/ffprobe");
    }

    if (ffmpeg_executable.empty()) {
        ffmpeg_executable = std::string("/usr/bin/ffmpeg");
    }
}

void TranscodeFFmpeg::set_user_agent(std::string ua) {
    if (ua.length() > 0) {
        user_agent = ua;
    } else {
        user_agent.clear();
    }
}

void TranscodeFFmpeg::set_cookies(std::string co) {
    if (co.length() > 0) {
        cookies = co;
    } else {
        cookies.clear();
    }
}

bool TranscodeFFmpeg::set_input(const char* time, const char* input, bool verbose) {
    CONSOLE_TRACE("TranscodeFFmpeg::set_input, time = {}, input = {}", time, input);

    if (strncmp(input, "client://movie/fail", 19) == 0) {
        // something went wrong in the page. Don't try to play this
        CONSOLE_ERROR("Try to play movie/fail. Investigation needed. Perhaps it's 'pause before play' problem.");
        return false;
    }

    input_file = std::string(input);
    isDash = endsWith(input_file, ".mpd");

    if (isDash) {
        // it's a dash file, start handler
        uint videoIdx = videoDashHandler.GetBestStream();
        uint audioIdx = audioDashHandler.GetBestStream();

        videoDashHandler.SetFilename(DASH_VIDEO_FILE);
        videoDashHandler.InitLoadThread(videoIdx, videoDashHandler.GetStream(videoIdx).startSegment);
        videoDashHandler.StartLoadThread(videoIdx);

        audioDashHandler.SetFilename(DASH_AUDIO_FILE);
        audioDashHandler.InitLoadThread(audioIdx, audioDashHandler.GetStream(audioIdx).startSegment);
        audioDashHandler.StartLoadThread(audioIdx);
    }

    verbose_ffmpeg = verbose;

    long duration = 0;

    // call ffprobe only, if it's not a dash file
    if (!isDash) {
        // construct commandline
        std::vector <char*> cmd_params;
        cmd_params.push_back(strdup(ffprobe_executable.c_str()));
        cmd_params.push_back(strdup("-v"));
        cmd_params.push_back(strdup("error"));
        cmd_params.push_back(strdup("-show_entries"));
        cmd_params.push_back(strdup("stream=codec_name,duration"));
        cmd_params.push_back(strdup("-of"));
        cmd_params.push_back(strdup("default=noprint_wrappers=1:nokey=1"));
        cmd_params.push_back(strdup("-i"));
        cmd_params.push_back(strdup(input));
        cmd_params.push_back((char*)NULL);

        // fork ffprobe and get result from stdout
        pid_t pid = 0;
        int pipefd[2];
        FILE* output;
        char line[1024];
        int status;

        pipe(pipefd);
        pid = fork();
        if (pid == 0) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);

            char **command = cmd_params.data();
            execv(command[0], &command[0]);
        }

        close(pipefd[1]);
        output = fdopen(pipefd[0], "r");

        copy_audio = false;
        copy_video = false;

        int idx = 0;
        while(nullptr != fgets(line, sizeof(line), output)) {
            if (idx % 2 == 0) {
                // codec
                if (strncmp(line, "h264", 4) == 0) {
                    copy_video = true;
                } else if (strncmp(line, "aac", 3) == 0) {
                    copy_audio = true;
                }
            } else {
                // duration
                if (strncmp(line, "N/A", 3) == 0) {
                    // e.g. for mpeg-dash. Set to 2 hours
                    duration = 2 * 60 * 60;
                } else {
                    duration = std::max(strtol(line, (char **) NULL, 10), duration);
                }
            }

            idx++;
        }

        waitpid(pid, &status, 0);

        if (duration == 0) {
            CONSOLE_ERROR("Unable to determine duration. ffprobe failed.\n");
            return false;
        }
    } else {
        // set very optimistic standard values for dash
        duration = 2 * 60 * 60;
    }

    // check if full transparent video exists, otherwise create one (shall not happen)
    if (access("movie/transparent-full.webm", R_OK) == -1 ) {
        CONSOLE_INFO("Warning: the file movie/transparent-full.webm does not exist!");
        CONSOLE_INFO("         Creation of a new one will be started, but it takes time.");

        char *createvideo;
        if (verbose_ffmpeg) {
            asprintf(&createvideo, "%s -y -loop 1 -i movie/transparent-16x16.png -t 21600 -r 1 -c:v libvpx -auto-alt-ref 0 movie/transparent-full.webm", ffmpeg_executable.c_str());
            CONSOLE_TRACE("call ffmpeg (createvideo): {}", createvideo);
        } else {
            asprintf(&createvideo, "%s -y -hide_banner -loglevel warning -loop 1 -i movie/transparent-16x16.png -t 21600 -r 1 -c:v libvpx -auto-alt-ref 0 movie/transparent-full.webm", ffmpeg_executable.c_str());
            CONSOLE_TRACE("call ffmpeg (createvideo): {}", createvideo);
        }

        int result = system(createvideo);
        free(createvideo);

        if (result == -1) {
            CONSOLE_ERROR("Error: Unable to create the file movie/transparent-full.webm!");
            return false;
        }
    }

    // create transparent video with desired duration
    transparent_time = time;

    char *cvd;

    if (verbose_ffmpeg) {
        asprintf(&cvd,"%s -y -i movie/transparent-full.webm -t %ld -codec copy movie/transparent_%s.webm", ffmpeg_executable.c_str(), duration + 1, time);
        CONSOLE_TRACE("call ffmpeg (transparent): {}", cvd);
    } else {
        asprintf(&cvd,"%s -y -hide_banner -loglevel warning -i movie/transparent-full.webm -t %ld -codec copy movie/transparent_%s.webm", ffmpeg_executable.c_str(), duration + 1, time);
        CONSOLE_TRACE("call ffmpeg (transparent): {}", cvd);
    }

    int result = system(cvd);
    free(cvd);

    if (result == -1) {
        CONSOLE_ERROR("Error: Unable to create the file movie/transparent.webm!");
        return false;
    }

    fork_ffmpeg(0);

    return true;
}

bool TranscodeFFmpeg::fork_ffmpeg(long start_at_ms) {
    std::string inter;

    if (alwaysEncodeVideo) {
        copy_video = false;
    }

    if (alwaysEncodeAudio) {
        copy_audio = false;
    }

    // fork and start ffmpeg
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Fork failed. Aborting...\n");
        return false;
    } else if (pid == 0) {
        std::vector <char*> cmd_params;
        cmd_params.push_back(strdup(ffmpeg_executable.c_str()));

        if (!verbose_ffmpeg) {
            cmd_params.push_back(strdup("-hide_banner"));
            cmd_params.push_back(strdup("-loglevel"));
            cmd_params.push_back(strdup("warning"));
        }

        if (!isDash) {
            // add user agent and cookies
            if (user_agent.length() > 0) {
                cmd_params.push_back(strdup("-user_agent"));
                cmd_params.push_back(strdup(user_agent.c_str()));
            }

            if (cookies.length() > 0) {
                cmd_params.push_back(strdup("-headers"));
                std::string co("$'");
                co += std::string(cookies) + std::string("'");
                cmd_params.push_back(strdup(co.c_str()));
            }
        }

        cmd_params.push_back(strdup("-re"));
        cmd_params.push_back(strdup("-ss"));
        cmd_params.push_back(strdup(std::string(ms_to_ffmpeg_time(start_at_ms)).c_str()));

        if (isDash) {
            // Mux Audio/Video
            cmd_params.push_back(strdup("-follow"));
            cmd_params.push_back(strdup("1"));
            cmd_params.push_back(strdup("-dn"));
            cmd_params.push_back(strdup("-i"));
            cmd_params.push_back(strdup((std::string("file:") + std::string(DASH_VIDEO_FILE)).c_str()));
            cmd_params.push_back(strdup("-i"));
            cmd_params.push_back(strdup((std::string("file:") + std::string(DASH_AUDIO_FILE)).c_str()));

            if (copy_video) {
                cmd_params.push_back(strdup("-c:v"));
                cmd_params.push_back(strdup("copy"));
            } else {
                std::stringstream ev(encode_video_param);
                while(getline(ev, inter, ' ')) {
                    cmd_params.push_back(strdup(inter.c_str()));
                }
            }

            // FIXME: always encode audio. Otherwise the stream is not usable
            std::stringstream ea(encode_audio_param);
            while(getline(ea, inter, ' ')) {
                cmd_params.push_back(strdup(inter.c_str()));
            }
        } else {
            cmd_params.push_back(strdup("-dn"));
            cmd_params.push_back(strdup("-i"));
            cmd_params.push_back(strdup(input_file.c_str()));

            if (copy_audio && copy_video) {
                cmd_params.push_back(strdup("-codec"));
                cmd_params.push_back(strdup("copy"));
            } else {
                if (copy_video) {
                    cmd_params.push_back(strdup("-c:v"));
                    cmd_params.push_back(strdup("copy"));
                } else {
                    std::stringstream ev(encode_video_param);
                    while(getline(ev, inter, ' ')) {
                        cmd_params.push_back(strdup(inter.c_str()));
                    }
                }

                if (copy_audio) {
                    cmd_params.push_back(strdup("-c:a"));
                    cmd_params.push_back(strdup("copy"));
                } else {
                    std::stringstream ea(encode_audio_param);
                    while(getline(ea, inter, ' ')) {
                        cmd_params.push_back(strdup(inter.c_str()));
                    }
                }
            }
        }

        cmd_params.push_back(strdup("-write_tmcd"));
        cmd_params.push_back(strdup("0"));
        cmd_params.push_back(strdup("-y"));
        cmd_params.push_back(strdup("-f"));
        cmd_params.push_back(strdup("mpegts"));

        if (protocol == UDP) {
            std::string udp = "udp://127.0.0.1:" + std::to_string(VIDEO_UDP_PORT) + "?pkt_size=" + std::to_string(udp_packet_size) + "&buffer_size=" + std::to_string(udp_buffer_size);
            cmd_params.push_back(strdup(udp.c_str()));
        } else if (protocol == TCP) {
            std::string tcp = "tcp://127.0.0.1:" + std::to_string(VIDEO_TCP_PORT) + "?listen&send_buffer_size=64860";
            cmd_params.push_back(strdup(tcp.c_str()));
        } else if (protocol == UNIX) {
            std::string unix = "unix://" + std::string(VIDEO_UNIX);
            cmd_params.push_back(strdup("-listen"));
            cmd_params.push_back(strdup("1"));
            cmd_params.push_back(strdup(unix.c_str()));
        }

        cmd_params.push_back((char*)NULL);

        /*
        fprintf(stderr, "ffmpeg command line:\n");
        for(auto it = std::begin(cmd_params); it != std::end(cmd_params); ++it) {
            fprintf(stderr, "%s ", (*it));
        }
        */

        // let ffmpeg do the hard work like fixing dts/pts, transcoding, copying streams and all this stuff
        char **command = cmd_params.data();

        execv(command[0], &command[0]);
        exit(0);
    }

    ffmpeg_pid = pid;

    return true;
}

void TranscodeFFmpeg::seek_video(const char* ms) {
    CONSOLE_TRACE("seek_video, stop video");
    stop_video();

    if (event_callback != nullptr) {
        event_callback("SEEK_VIDEO");
    }

    CONSOLE_TRACE("seek_video, fork_ffmpeg");
    fork_ffmpeg(strtol(ms, (char **)nullptr, 10));
}

void TranscodeFFmpeg::pause_video() {
    stop_video(false);

    if (event_callback != nullptr) {
        event_callback("PAUSE_VIDEO");
    }
}

void TranscodeFFmpeg::resume_video(const char* position) {
    // restart ffmpeg
    if (ffmpeg_pid > 0) {
        // called resume without pause. Ignore this request
        return;
    }

    fork_ffmpeg(strtol(position, (char **)nullptr, 10) * 1000);

    if (event_callback != nullptr) {
        event_callback("RESUME_VIDEO");
    }
}

void TranscodeFFmpeg::stop_video(bool cleanup) {

    if (cleanup) {
        videoDashHandler.StopLoadThread();
        audioDashHandler.StopLoadThread();
    }

    if (ffmpeg_pid > 0) {
        CONSOLE_TRACE("stop video, kill ffmpeg with pid {}", ffmpeg_pid);
        kill(ffmpeg_pid, SIGKILL);

        CONSOLE_TRACE("stop video, wait for ffmpeg pid {}", ffmpeg_pid);

        int status;
        waitpid(ffmpeg_pid, &status, 0);

        CONSOLE_TRACE("ffmpeg with pid {} is hopefully not running anymore", ffmpeg_pid);
    }

    ffmpeg_pid = 0;

    if (cleanup) {
        char *transvideo;
        asprintf(&transvideo, "movie/transparent_%s.webm", transparent_time.c_str());
        unlink(transvideo);
        free(transvideo);

        unlink(VIDEO_UNIX);
        unlink(DASH_VIDEO_FILE);
        unlink(DASH_AUDIO_FILE);
    }
}

void TranscodeFFmpeg::speed_video(const char* speed) {

}
