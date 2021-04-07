#include "sendvdr.h"
#include "logger.h"
#include "sharedmemory.h"

bool SendToVdrString(uint8_t messageType, const char* message) {
    if (messageType != CMD_PING && messageType != CMD_VIDEO) {
        CONSOLE_TRACE("Send String to VDR, Message {}", message);
    }

    char* buffer = (char*)malloc(2 + strlen(message));

    buffer[0] = messageType;
    strcpy(buffer + 1, message);

    bool sent = false;

    if (sharedMemory.waitForWrite(BrowserCommand) != -1) {
        sharedMemory.write((uint8_t *) buffer, strlen(message) + 2, BrowserCommand);
        sent = true;
    }

    free(buffer);

    return sent;
}

void SendToVdrOsd(const char* message, int width, int height) {
    // CONSOLE_TRACE("Send OSD update to VDR, Message {}, Width {}, Height {}", message, width, height);

    struct OsdStruct osdStruct = {};
    auto *buffer = new uint8_t[1 + sizeof(struct OsdStruct)];

    osdStruct.width = width;
    osdStruct.height = height;
    memset(osdStruct.message, 0, 20);
    strncpy(osdStruct.message, message, 19);

    buffer[0] = CMD_OSD;
    memcpy((void*)(buffer + 1), &osdStruct, sizeof(struct OsdStruct));

    if (sharedMemory.waitForWrite(BrowserCommand) != -1) {
        sharedMemory.write((uint8_t *) buffer, sizeof(struct OsdStruct) + 1, BrowserCommand);
    }

    delete[] buffer;
}

bool SendToVdrPing() {
    return SendToVdrString(CMD_PING, "B");
}
