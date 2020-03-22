#include "transcodeffmpeg.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage %s <input> <output>\n", argv[0]);
        exit(1);
    }

    TranscodeFFmpeg *transcoder = new TranscodeFFmpeg(argv[1], argv[2], true);
    transcoder->transcode();
    delete transcoder;
}