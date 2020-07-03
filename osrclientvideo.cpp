/**
 *  CEF OSR implementation used für vdr-plugin-htmlskin
 *
 *  osrclient.cpp
 *
 *  (c) 2019 Robert Hannebauer
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#include <getopt.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include "globaldefs.h"

unsigned char CMD_STATUS = 1;
unsigned char CMD_VIDEO = 3;

std::string video;
char command;

void PrintHelp() {
    std::cout <<
              "--video <file>     :load and stream file to VDR\n";
    exit(1);
}

void ProcessArgs(int argc, char** argv)
{
    const char* const short_opts = "v:f:a:";
    const option long_opts[] = {
            {"video", required_argument, nullptr, 'v'},
            {nullptr, no_argument, nullptr, 0}
    };

    while (true)
    {
        const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);

        if (-1 == opt) {
            break;
        }

        switch (opt) {
            case 'v':
                // load video
                command = 'v';
                video = std::string(optarg);
                break;

            case 'h': // -h or --help
            case '?': // Unrecognized option
            default:
                PrintHelp();
                break;
        }
    }
}

int main(int argc, char **argv)
{
    ProcessArgs(argc, argv);

    unsigned char buffer[32712];
    FILE *ptr;
    ptr = fopen(video.c_str(),"rb");

    if (ptr == NULL) {
        fputs ("File error", stderr);
        exit (1);
    }

    // bind socket
    int toVdrSocketId;

    if ((toVdrSocketId = nn_socket(AF_SP, NN_PUSH)) < 0) {
        fprintf(stderr, "unable to create nanomsg socket\n");
        exit(1);
    }

    if (nn_bind(toVdrSocketId, TO_VDR_CHANNEL) < 0) {
        fprintf(stderr, "unable to bind nanomsg socket to %s\n", TO_VDR_CHANNEL);
        exit(1);
    }

    fprintf(stderr, "Send video command to VDR...\n");

    nn_send(toVdrSocketId, &CMD_STATUS, 1, 0);
    nn_send(toVdrSocketId, "PLAY_VIDEO:10", 14 , 0);

    fprintf(stderr, "Start streaming...\n");

    while (fread(buffer, sizeof(buffer),1, ptr) > 0) {
        // write to VDR
        nn_send(toVdrSocketId, &CMD_VIDEO, 1, 0);
        nn_send(toVdrSocketId, &buffer, sizeof(buffer), 0);

        // don't be too fast
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    fprintf(stderr, "Closing...\n");
    nn_close(toVdrSocketId);
    fclose(ptr);

    return 0;
}

