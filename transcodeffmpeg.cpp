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
    read_configuration();
    protocol = proto;
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
    protocol = UDP;

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

    input_file = std::string(input);
    isDash = endsWith(input_file, ".mpd");

    if (isDash) {
        // it's a dash file, start handler
        videoDashHandler.setFilename(DASH_VIDEO_FILE);
        videoDashHandler.InitLoadThread(0, videoDashHandler.GetStartSegment());
        videoDashHandler.StartLoadThread(0);

        audioDashHandler.setFilename(DASH_AUDIO_FILE);
        audioDashHandler.InitLoadThread(0, audioDashHandler.GetStartSegment());
        audioDashHandler.StartLoadThread(0);
    }

    verbose_ffmpeg = verbose;

    long duration = 0;

    // call ffprobe only, if it's not a dash file
    if (!isDash) {
        char *ffprobe;
        std::string chinput(input);
        replaceAll(chinput, "&", "\\&");

        asprintf(&ffprobe,
                 "%s -v error -show_entries stream=codec_name,duration -of default=noprint_wrappers=1:nokey=1 -i %s ",
                 ffprobe_executable.c_str(), chinput.c_str());

        CONSOLE_TRACE("call ffprobe: {}", ffprobe);

        FILE *infoFile = popen(ffprobe, "r");

        copy_audio = false;
        copy_video = false;

        if (infoFile) {
            char buffer[1024];
            char *line;
            int idx = 0;
            while (nullptr != (line = fgets(buffer, sizeof(buffer), infoFile))) {
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

            pclose(infoFile);
        }

        if (duration == 0) {
            CONSOLE_ERROR("Unable to determine duration. ffprobe failed.\n");
            return false;
        }
    } else {
        // set very optimistic standard values for dash
        duration = 2 * 60 * 60;
        copy_audio = true;
        copy_video = true;
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
    // videoDashHandler.PrintTrace();
    // audioDashHandler.PrintTrace();

    // fork and start ffmpeg
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Fork failed. Aborting...\n");
        return false;
    } else if (pid == 0) {
        // build the commandline
        std::string cmdline = "";

        if (!verbose_ffmpeg) {
            cmdline += "-hide_banner -loglevel warning ";
        }

        if (!isDash) {
            cmdline += "-re -ss " + std::string(ms_to_ffmpeg_time(start_at_ms)) + " ";
        } else {
            cmdline += "-re ";
        }

        if (isDash) {
            // Mux Audio/Video
            cmdline += "-follow 1 -i file:" + std::string(DASH_VIDEO_FILE) + " -i file:" + std::string(DASH_AUDIO_FILE) + " -c:v copy " + encode_audio_param + " ";
        } else {
            std::string ninput(input_file);
            replaceAll(ninput, "&", "\\&");
            cmdline += "-i " + ninput + " ";

            if (copy_audio && copy_video) {
                cmdline += "-codec copy ";
            } else {
                if (copy_video) {
                    cmdline += "-c:v copy ";
                } else {
                    cmdline += encode_video_param + " ";
                }

                if (copy_audio) {
                    cmdline += "-c:a copy ";
                } else {
                    cmdline += encode_audio_param + " ";
                }
            }
        }

        cmdline += "-y -f mpegts ";

        if (protocol == UDP) {
            cmdline += "udp://127.0.0.1:" + std::to_string(VIDEO_UDP_PORT) + "?pkt_size=" +
                       std::to_string(udp_packet_size) + "&buffer_size=" + std::to_string(udp_buffer_size);
        } else if (protocol == TCP) {
            cmdline += "tcp://127.0.0.1:" + std::to_string(VIDEO_TCP_PORT) + "?listen&send_buffer_size=64860";
        } else if (protocol == UNIX) {
            cmdline += "-listen 1 unix://" + std::string(VIDEO_UNIX);
        }

        // create the final commandline parameter for execv
        std::vector <char*> cmd_params;
        std::stringstream cmd(cmdline);
        std::string inter;

        cmd_params.push_back(strdup(ffmpeg_executable.c_str()));

        if (!isDash) {
            // add user agent and cookies
            if (user_agent.length() > 0) {
                cmd_params.push_back((char *) "-user_agent");
                std::string ua("'");
                ua += user_agent + "'";
                cmd_params.push_back(strdup(ua.c_str()));
            }

            if (cookies.length() > 0) {
                cmd_params.push_back((char *) "-headers");
                std::string co("$'");
                co += std::string(cookies) + std::string("'");
                cmd_params.push_back(strdup(co.c_str()));
            }
        }

        while(getline(cmd, inter, ' ')) {
            cmd_params.push_back(strdup(inter.c_str()));
        }

        cmd_params.push_back((char*)NULL);

        /*
        CONSOLE_TRACE("ffmpeg Commandline:");
        for(auto it = std::begin(cmd_params); it != std::end(cmd_params); ++it) {
            CONSOLE_TRACE("{} ", *it);
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
        event_callback("SEEK");
    }

    CONSOLE_TRACE("seek_video, fork_ffmpeg");
    fork_ffmpeg(strtol(ms, (char **)nullptr, 10));
}

void TranscodeFFmpeg::pause_video() {
    if (ffmpeg_pid > 0) {
        kill(ffmpeg_pid, SIGSTOP);
    }
}

void TranscodeFFmpeg::resume_video() {
    if (ffmpeg_pid > 0) {
        kill(ffmpeg_pid, SIGCONT);
    }
}

void TranscodeFFmpeg::stop_video() {
    videoDashHandler.StopLoadThread();
    audioDashHandler.StopLoadThread();

    if (ffmpeg_pid > 0) {
        CONSOLE_TRACE("stop video, kill ffmpeg with pid {}", ffmpeg_pid);
        // kill(ffmpeg_pid, SIGTERM);
        kill(ffmpeg_pid, SIGKILL);

        CONSOLE_TRACE("stop video, wait for ffmpeg pid {}", ffmpeg_pid);

        int status;
        waitpid(ffmpeg_pid, &status, 0);

        CONSOLE_TRACE("ffmpeg with pid {} is hopefully not running anymore", ffmpeg_pid);
    }

    ffmpeg_pid = 0;

    char *transvideo;
    asprintf(&transvideo, "movie/transparent_%s.webm", transparent_time.c_str());
    unlink(transvideo);
    free(transvideo);

    unlink(VIDEO_UNIX);
    // unlink(DASH_VIDEO_FILE);
    // unlink(DASH_AUDIO_FILE);
}

void TranscodeFFmpeg::speed_video(const char* speed) {

}
