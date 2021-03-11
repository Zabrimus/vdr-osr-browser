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
#include <sys/time.h>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include "globaldefs.h"

std::string url;
std::string call;
std::string mode;
std::string key;

char command;

void PrintHelp() {
    std::cout <<
              "--url <url>:   load URL\n"
              "--pause:       pause rendering\n"
              "--resume:      resume rendering\n"
              "--stop:        shutdown osrcef\n"
              "--stream:      read all dirty recs\n"
              "--js:          executed a javascript procedure\n"
              "--connect:     ping the osr server\n"
              "--key:         send key event\n"
              "--mode:        1 = HTML, 2 = HbbTV\n";
    exit(1);
}

void ProcessArgs(int argc, char** argv)
{
    const char* const short_opts = "u:j:m:prqhc";
    const option long_opts[] = {
            {"url", required_argument, nullptr, 'u'},
            {"pause", no_argument, nullptr, 'p'},
            {"resume", no_argument, nullptr, 'r'},
            {"stop", no_argument, nullptr, 'q'},
            {"connect", no_argument, nullptr, 'c'},
            {"stream", no_argument, nullptr, 's'},
            {"js", required_argument, nullptr, 'j'},
            {"help", no_argument, nullptr, 'h'},
            {"mode", required_argument, nullptr, 'm'},
            {"key", required_argument, nullptr, 'k'},
            {nullptr, no_argument, nullptr, 0}
    };

    while (true)
    {
        const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);

        if (-1 == opt) {
            break;
        }

        switch (opt) {
            case 'u':
                // load url
                command = 'u';
                url = std::string(optarg);
                break;

            case 'p':
                // pause
                command = 'p';
                break;

            case 'r':
                // resume
                command = 'r';
                break;

            case 'q':
                // quit
                command = 'q';
                break;

            case 'c':
                // connect / ping
                command = 'c';
                break;

            case 's':
                // stream
                command = 's';
                break;

            case 'j':
                // execute javascript
                call = std::string(optarg);
                command = 'j';
                break;

            case 'm':
                // switch mode
                mode = std::string(optarg);
                command = 'm';
                break;

            case 'k':
                // send key event
                key = std::string(optarg);
                command = 'k';
                break;

            case 'h': // -h or --help
            case '?': // Unrecognized option
            default:
                PrintHelp();
                break;
        }
    }
}

bool sendCommand(int socketId, const char *buffer) {
    char *response = nullptr;
    bool result;
    int bytes;

    if ((bytes = nn_send(socketId, buffer, strlen(buffer) + 1, 0)) < 0) {
        fprintf(stderr, "Unable to send command '%s'\n", buffer);
    } else {
        fprintf(stderr, "Send command '%s'\n", buffer);
    }

    result = true;

    /*
    if (bytes > 0 && nn_recv(socketId, &response, NN_MSG, 0) < 0) {
        fprintf(stderr, "Unable to read response for '%s'\n", buffer);
        result = false;
    } else {
        fprintf(stderr, "Response read: '%s'\n", response);

        if (strncmp(response, "ok", 2) != 0) {
            result = false;
        }
    }
    */

    if (response) {
        nn_freemsg(response);
    }

    return result;
}

int main(int argc, char **argv)
{
    int to = 2000;

    ProcessArgs(argc, argv);

    // connect to nanomsg socket
    int socketId;
    int endpointId;

    if ((socketId = nn_socket(AF_SP, NN_PULL)) < 0) {
        fprintf(stderr, "Unable to create socket\n");
        exit(1);
    }

    if ((endpointId = nn_connect(socketId, FROM_VDR_CHANNEL)) < 0) {
        fprintf(stderr, "Unable to connect to socket\n");
        exit(2);
    }

    // set timeout
    nn_setsockopt (socketId, NN_SOL_SOCKET, NN_SNDTIMEO, &to, sizeof (to));
    nn_setsockopt (socketId, NN_SOL_SOCKET, NN_RCVTIMEO, &to, sizeof (to));

    printf("Socket connected (%d,%d)\n", socketId, endpointId);

    // connect to stream socket
    int streamSocketId;
    int streamEndpointId;

    if ((streamSocketId = nn_socket(AF_SP, NN_PULL)) < 0) {
        fprintf(stderr, "Unable to create stream socket\n");
        exit(1);
    }

    if ((streamEndpointId = nn_connect(streamSocketId, TO_VDR_CHANNEL)) < 0) {
        fprintf(stderr, "Unable to connect to stream socket\n");
        exit(2);
    }

    printf("Stream socket connected (%d,%d)\n", streamSocketId, streamEndpointId);

    switch (command) {
        case 'u': {
            std::string cmdUrl("URL ");
            cmdUrl.append(url);
            sendCommand(socketId, cmdUrl.c_str());
            break;
        }

        case 'p': {
            std::string cmdUrl("PAUSE");
            cmdUrl.append(url);
            sendCommand(socketId, cmdUrl.c_str());
            break;
        }

        case 'r': {
            std::string cmdUrl("RESUME");
            cmdUrl.append(url);
            sendCommand(socketId, cmdUrl.c_str());
            break;
        }

        case 'q': {
            std::string cmdUrl("STOP");
            cmdUrl.append(url);
            sendCommand(socketId, cmdUrl.c_str());
            break;
        }

        case 'c': {
            std::string cmd("PING");
            sendCommand(socketId, cmd.c_str());
            break;
        }

        case 'j': {
            std::string cmd("JS ");
            cmd.append(call);
            sendCommand(socketId, cmd.c_str());
            break;
        }

        case 'm': {
            std::string modeCmd("MODE ");
            modeCmd.append(mode);
            sendCommand(socketId, modeCmd.c_str());
            break;
        }

        case 'k': {
            std::string keyCmd("KEY ");
            keyCmd.append(key);
            sendCommand(socketId, keyCmd.c_str());
            break;
        }

        case 's': {
            int bytes;
            while (1) {
                // read count of dirty rects
                unsigned long dirtyRecs = 0;
                unsigned char type;

                if (nn_recv(streamSocketId, &type, 1, 0) > 0) {
                    if (type != 2) {
                        continue;
                    }
                }

                if ((bytes = nn_recv(streamSocketId, &dirtyRecs, sizeof(dirtyRecs), 0)) > 0) {
                    printf("Dirty Recs: %lu\n", dirtyRecs);
                } else {
                    printf("No Response %d\n", bytes);
                    dirtyRecs = 0;
                }

                struct timeval t1,t0;
                gettimeofday(&t0, 0);

                for (unsigned long i = 0; i < dirtyRecs; ++i) {
                    printf("Read dirty rec %lu\n", i);

                    // read coordinates and size
                    int x, y, w, h;
                    if (nn_recv(streamSocketId, &x, sizeof(x), 0) > 0) {
                        printf("X: %d\n", x);
                    }

                    if (nn_recv(streamSocketId, &y, sizeof(y), 0) > 0) {
                        printf("Y: %d\n", y);
                    }

                    if (nn_recv(streamSocketId, &w, sizeof(w), 0) > 0) {
                        printf("W: %d\n", w);
                    }

                    if (nn_recv(streamSocketId, &h, sizeof(h), 0) > 0) {
                        printf("H: %d\n", h);
                    }

                    // read buffer
                    unsigned long *buffer = (unsigned long*) malloc(4 * w);
                    for (int j = 0; j < h; ++j) {
                        if (nn_recv(streamSocketId, buffer, 4 * w, 0) > 0) {
                            // printf("Line %u, size %lu\n", j, bytes);
                        }
                    }
                    free(buffer);
                }

                gettimeofday(&t1, 0);
                double dif = double( (t1.tv_usec-t0.tv_usec)*1000);
                printf ("Elasped time is %lf nanoseconds, %lf millis.\n", dif, dif/1000000.0);
            }
        }
    }

    return 0;
}

