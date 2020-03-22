#include <cstdio>
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/pipeline.h>
#include "globals.h"

#define TO_VDR_CHANNEL "ipc:///tmp/vdrosr_tovdr.ipc"
#define FROM_VDR_CHANNEL "ipc:///tmp/vdrosr_tobrowser.ipc"

int Globals::toVdrSocketId;
int Globals::toVdrEndpointId;

int Globals::fromVdrSocketId;
int Globals::fromVdrEndpointId;

Globals *globals = new Globals();

Globals::Globals() {
    // bind socket
    if ((toVdrSocketId = nn_socket(AF_SP, NN_PUSH)) < 0) {
        fprintf(stderr, "unable to create nanomsg socket\n");
    }

    if ((toVdrEndpointId = nn_bind(toVdrSocketId, TO_VDR_CHANNEL)) < 0) {
        fprintf(stderr, "unable to bind nanomsg socket to %s\n", TO_VDR_CHANNEL);
    }

    // bind socket
    if ((fromVdrSocketId = nn_socket(AF_SP, NN_REP)) < 0) {
        fprintf(stderr, "unable to create nanomsg socket\n");
    }

    if ((fromVdrEndpointId = nn_bind(fromVdrSocketId, FROM_VDR_CHANNEL)) < 0) {
        fprintf(stderr, "unable to bind nanomsg socket to %s\n", FROM_VDR_CHANNEL);
    }
}

Globals::~Globals() {
    nn_close(toVdrSocketId);
    nn_close(fromVdrSocketId);
};
