#include <string>
#include <thread>
#include "transcodeffmpeg.h"

FILE* out;

int write_buffer(uint8_t *buf, int buf_size) {
    fwrite(buf+1, 1, buf_size-1, out);
    return buf_size;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage %s <input> <output>\n", argv[0]);
        exit(1);
    }

    out = fopen(argv[2], "wb");
    if (out == nullptr) {
        fprintf(stderr, "Unable to open output file %s\n", argv[2]);
        exit(1);
    }

    TranscodeFFmpeg *transcoder = new TranscodeFFmpeg();

    if (!transcoder->set_input(argv[1], true)) {
        fprintf(stderr, "Some error occured. Aborting...\n");
        exit(1);
    }

    std::thread th = transcoder->transcode(write_buffer, false);
    th.join();

    delete transcoder;

    fclose(out);
}