#include "transcodeffmpeg.h"
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string.h>
#include <vector>
#include <sstream>
#include <signal.h>
#include <fcntl.h>
#include <limits.h>

#include "logger.h"

static pid_t ffmpeg_pid;
bool stop_worker = false;
const int buffer_size = 32712 + 1; // 188 * 174 + 1 (first byte is reserved)
const std::string output_file = "ffmpeg_putput.ts";

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


TranscodeFFmpeg::TranscodeFFmpeg() {
    read_configuration();
}

TranscodeFFmpeg::~TranscodeFFmpeg() {
}

void TranscodeFFmpeg::read_configuration() {
    CONSOLE_TRACE("====== Loading configuration... ======");

    std::string exepath = getbrowserexepath();

    std::ifstream infile(exepath.substr(0, exepath.find_last_of("/")) + "/vdr-osr-ffmpeg.config");
    if (infile.is_open()) {
        std::string key;
        std::string value;
        char c;
        while (infile >> key >> c >> value && c == '=') {
            if (key.at(0) != '#') {
                if (key == "encode_video") {
                    encode_video_param = value;
                } else if (key == "encode_audio") {
                    encode_audio_param = value;
                } else if (key == "ffmpeg_executable") {
                    ffmpeg_executable = std::string(value);
                } else if (key == "ffprobe_executable") {
                    ffprobe_executable = std::string(value);
                }
            }
        }
    }

    infile.close();

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

bool TranscodeFFmpeg::set_input(const char* input, bool verbose) {
    input_file = std::string(input);

    verbose_ffmpeg = verbose;

    // create pipe
    bool createPipe = false;

    struct stat sb{};
    if(stat(output_file.c_str(), &sb) != -1) {
        if (!S_ISFIFO(sb.st_mode) != 0) {
            if(remove(output_file.c_str()) != 0) {
                fprintf(stderr, "File %s exists and is not a pipe. Delete failed. Aborting...\n", output_file.c_str());
                return false;
            } else {
                createPipe = true;
            }
        }
    } else {
        createPipe = true;
    }

    if (createPipe) {
        if (mkfifo(output_file.c_str(), 0666) < 0) {
            fprintf(stderr, "Unable to create pipe %s. Aborting...\n", output_file.c_str());
            return false;
        }
    }

    char *ffprobe;
    asprintf(&ffprobe, "%s -v error -show_entries stream=codec_name,duration -of default=noprint_wrappers=1:nokey=1 -i %s", ffprobe_executable.c_str(), input);

    FILE *infoFile = popen(ffprobe, "r");

    long duration = 0;
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
                duration = std::max(strtol(line, (char **) NULL, 10), duration);
            }

            idx++;
        }

        pclose(infoFile);
    }

    if (duration == 0) {
        fprintf(stderr, "Unable to determine duration. ffprobe failed.\n");
    }

    // check if full transparent video exists, otherwise create one (shall not happen)
    if (access("movie/transparent-full.webm", R_OK) == -1 ) {
        fprintf(stderr, "Warning: the file movie/transparent-full.webm does not exist!");
        fprintf(stderr, "         Creation of a new one will be started, but it takes time.");

        char *createvideo;
        if (verbose_ffmpeg) {
            asprintf(&createvideo, "%s -y -loop 1 -i movie/transparent-16x16.png -t 21600 -r 1 -c:v libvpx -auto-alt-ref 0 movie/transparent-full.webm", ffmpeg_executable.c_str());
        } else {
            asprintf(&createvideo, "%s -y -hide_banner -loglevel warning -loop 1 -i movie/transparent-16x16.png -t 21600 -r 1 -c:v libvpx -auto-alt-ref 0 movie/transparent-full.webm", ffmpeg_executable.c_str());
        }

        int result = system(createvideo);
        free(createvideo);

        if (result == -1) {
            fprintf(stderr, "Error: Unable to create the file movie/transparent-full.webm!");
            return false;
        }
    }

    // create transparent video with desired duration
    char *cvd;

    if (verbose_ffmpeg) {
        asprintf(&cvd,"%s -y -i movie/transparent-full.webm -t %ld -codec copy movie/transparent.webm", ffmpeg_executable.c_str(), duration + 1);
    } else {
        asprintf(&cvd,"%s -y -hide_banner -loglevel warning -i movie/transparent-full.webm -t %ld -codec copy movie/transparent.webm", ffmpeg_executable.c_str(), duration + 1);
    }

    int result = system(cvd);
    free(cvd);

    if (result == -1) {
        fprintf(stderr, "Error: Unable to create the file movie/transparent.webm!");
        return false;
    }

    fork_ffmpeg(0);

    return true;
}

bool TranscodeFFmpeg::fork_ffmpeg(long start_at_ms) {
    // fork and start ffmpeg
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Fork failed. Aborting...\n");
        return false;
    } else if (pid == 0) {
        // build the commandline
        std::string cmdline = "";

        if (verbose_ffmpeg) {
            cmdline += "";
        } else {
            cmdline += "-hide_banner -loglevel warning ";
        }

        if (realtime) {
            cmdline += "-re ";
        }

        cmdline += "-ss " + std::string(ms_to_ffmpeg_time(start_at_ms)) + " ";
        cmdline += "-i " + input_file + " ";

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

        cmdline += "-y " + output_file;

        // create the final commandline parameter for execv
        std::vector <char*> cmd_params;
        std::stringstream cmd(cmdline);
        std::string inter;

        cmd_params.push_back(strdup(ffmpeg_executable.c_str()));

        while(getline(cmd, inter, ' ')) {
            cmd_params.push_back(strdup(inter.c_str()));
        }

        cmd_params.push_back((char*)NULL);

        // let ffmpeg do the hard work like fixing dts/pts, transcoding, copying streams and all this stuff
        char **command = cmd_params.data();
        execv(command[0], &command[0]);
        exit(0);
    }

    ffmpeg_pid = pid;

    return true;
}

std::thread TranscodeFFmpeg::transcode(int (*write_packet)(uint8_t *buf, int buf_size), bool realtime) {
    this->realtime = realtime;
    return std::thread(&TranscodeFFmpeg::transcode_worker, this, write_packet);
}

int TranscodeFFmpeg::transcode_worker(int (*write_packet)(uint8_t *buf, int buf_size)) {
    uint8_t buffer[buffer_size];
    stop_worker = false;

    int fd = open("ffmpeg_putput.ts", O_RDONLY);
    fcntl(fd, F_SETFL, O_NONBLOCK);

    while (!stop_worker) {
        // first byte is reserved
        int bytes = read(fd, &buffer[1], buffer_size - 1);

        if (bytes == -1) {
            if (errno == EAGAIN) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            } else {
                stop_worker = true;
            }
        } else if (bytes == 0) {
            stop_worker = true;
        } else {
            write_packet(&buffer[0], bytes + 1);
        }
    }

    close(fd);

    return 0;
}

void TranscodeFFmpeg::pause_video() {
    kill(ffmpeg_pid, SIGSTOP);
}

void TranscodeFFmpeg::resume_video() {
    kill(ffmpeg_pid, SIGCONT);
}

void TranscodeFFmpeg::stop_video() {
    stop_worker = true;

    if (ffmpeg_pid > 0) {
        kill(ffmpeg_pid, SIGKILL);
    }

    ffmpeg_pid = 0;
}

void TranscodeFFmpeg::seek_video(const char* ms, int (*write_packet)(uint8_t *buf, int buf_size)) {

}

void TranscodeFFmpeg::speed_video(const char* speed) {

}
