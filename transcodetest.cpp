#include <string>
#include "transcodeffmpeg.h"

#define FFMPEG "/usr/bin/ffmpeg"
#define FFPROBE "/usr/bin/ffprobe"

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage %s <input> <output>\n", argv[0]);
        exit(1);
    }

    TranscodeFFmpeg *transcoder = new TranscodeFFmpeg(FFMPEG, FFPROBE, argv[1], argv[2], true);

    if (!transcoder->set_input_file(argv[1])) {
        fprintf(stderr, "Some error occured. Aborting...\n");
        exit(1);
    }

    // TEST 1
    // push transparent image into  overlay input buffer
    /*
    uint8_t transparent[4096];
    memset(&transparent, 0, 4096);
    transcoder->add_overlay_frame(16, 16, &transparent[0]);
    */
    // TEST 1

    // TEST 2
    // push existing bgra image into overlay input buffer
    /*
    uint8_t transparent[1280 * 720 * 4];
    FILE * filp = fopen("../../test_image.bgra", "rb");
    int bytes_read = fread(transparent, sizeof(uint8_t), 1280 * 720 * 4, filp);
    printf("Read Buffer: %d\n", bytes_read);
    fclose(filp);
    transcoder->add_overlay_frame(1280,720, &transparent[0]);
    */
    // TEST 2

    transcoder->transcode(NULL);
    delete transcoder;
}