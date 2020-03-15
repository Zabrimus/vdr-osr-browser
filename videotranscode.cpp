#include <sstream>
#include <string>
#include <cstring>
#include <stdio.h>
#include <vector>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "videotranscode.h"

#define OSR_FFMPEG_VIDEOIN "/tmp/osr_ffmpeg_videoin"
#define OSR_FFMPEG_AUDIOIN "/tmp/osr_ffmpeg_audioin"
#define OSR_FFMPEG_OUTPUT  "/tmp/osr_ffmpeg_output"

/**
 * Internal helper class to convert a std::string to a NULL-terminated char* array
 * Used for exec*
 */
class ExecParams {
private:
    std::vector<char*> params;

    std::vector<std::string> split(const std::string& str) {
        std::stringstream stream(str);
        std::string item;
        std::vector<std::string> splittedStrings;

        while (std::getline(stream, item, ' ')) {
            splittedStrings.push_back(item);
        }

        return splittedStrings;
    }

public:
    ExecParams(std::string cmd) {
        std::vector<std::string> splitted = split(cmd);

        for(auto it = splitted.begin(); it != splitted.end(); ++it) {
            const std::string& string = *it;
            char* cstring = new char[string.length() + 1];
            strcpy(cstring, string.c_str());
            params.push_back(cstring);
        }
        params.push_back(nullptr);
    }

    ~ExecParams() {
        for(char* str : params)
            if(str != nullptr) delete[] str;
    }

    char* const * data() const {
        return params.data();
    }
};

VideoTranscode::VideoTranscode() {
}

VideoTranscode::~VideoTranscode() {
    stopStreaming();
}

bool VideoTranscode::startStreaming() {
    fprintf(stderr, "VideoTranscode::Start Streaming\n");

    // delete all existing pipes
    unlink(OSR_FFMPEG_VIDEOIN);
    unlink(OSR_FFMPEG_AUDIOIN);
    unlink(OSR_FFMPEG_OUTPUT);

    // create all pipes
    mkfifo(OSR_FFMPEG_VIDEOIN, 0666);
    mkfifo(OSR_FFMPEG_AUDIOIN, 0666);
    mkfifo(OSR_FFMPEG_OUTPUT, 0666);

    ffmpegVideoIn = open(OSR_FFMPEG_VIDEOIN, O_RDWR); // O_WRONLY blocks
    ffmpegAudioIn = open(OSR_FFMPEG_AUDIOIN, O_RDWR); // O_WRONLY blocks
    ffmpegVideoOut = open(OSR_FFMPEG_OUTPUT, O_RDWR); // O_RDONLY blocks

    // FIXME: Der Aufruf muss konfigurierbar sein.
    std::string ffmpegParams = "/usr/bin/ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt bgra -s 1920x1080 -r 25 -i " OSR_FFMPEG_VIDEOIN " -f mpegts -q:v 10 -an -vcodec mpeg4 " OSR_FFMPEG_OUTPUT;

    ExecParams params(ffmpegParams);

    // start encoder
    pid_t pid = fork();
    if (pid == 0) {
        ffmpegPid = getpid();

        prctl(PR_SET_PDEATHSIG, SIGHUP);
        if (getppid() == 1)
            kill(getpid(), SIGHUP);

        int res = execve("/usr/bin/ffmpeg", params.data(), nullptr);
        fprintf(stderr, "Starting ffmpeg failed: %s\n", strerror(res));
        return false;
    }

    fprintf(stderr, "VideoTranscode::Running\n");
    return true;
}

void VideoTranscode::stopStreaming() {
    if (ffmpegVideoIn > 0) {
        close(ffmpegVideoIn);
        unlink(OSR_FFMPEG_VIDEOIN);
        ffmpegVideoIn = -1;
    }

    if (ffmpegAudioIn > 0) {
        close(ffmpegAudioIn);
        unlink(OSR_FFMPEG_AUDIOIN);
        ffmpegAudioIn = -1;
    }

    if (ffmpegVideoOut > 0) {
        close(ffmpegVideoOut);
        unlink(OSR_FFMPEG_OUTPUT);
        ffmpegVideoOut = -1;
    }
}

void VideoTranscode::writeVideo(const void *buffer, int width, int height) {
    if (toFfmpegVideo) {
        write(ffmpegVideoIn, buffer, width * height * 4);
    } else {
        fprintf(stderr, "Internal error. Pipe to FFmpeg is not available.\n");
    }
}
void VideoTranscode::writeAudio(const void *buffer, int size) {
    if (toFfmpegVideo) {
        write(ffmpegAudioIn, buffer, size);
    } else {
        fprintf(stderr, "Internal error. Pipe to FFmpeg is not available.\n");
    }
}

int VideoTranscode::readEncoded(void* buffer, int size) {
    return read(ffmpegVideoOut, buffer, size);
}