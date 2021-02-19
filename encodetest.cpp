#include "encoder.h"

Encoder encoder("output.ts", true);

int main(int argc, char* argv[]) {
    char buffer[1280 * 720 * 4];
    uint64_t pts = 0;
    size_t readbytes;

    encoder.startEncoder(nullptr);

    for (int j = 0; j < 10; ++j) {
        for (int i = 0; i < 60; ++i) {
            char *filename = nullptr;
            asprintf(&filename, "videotest/image_%d.rgba", i);

            fprintf(stderr, "File: %s\n", filename);

            FILE *f = fopen(filename, "rb");
            readbytes = fread(buffer, 1, 1280 * 720 * 4, f);

            fclose(f);
            free(filename);

            encoder.addVideoFrame(1280, 720, (uint8_t *) buffer, pts);

            pts += 10000;
        }
    }

    encoder.stopEncoder();

}


