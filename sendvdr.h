#ifndef VDR_OSR_BROWSER_SENDVDR_H
#define VDR_OSR_BROWSER_SENDVDR_H

#include <cstdint>

const uint8_t CMD_STATUS = 1;
const uint8_t CMD_OSD = 2;
const uint8_t CMD_VIDEO = 3;
const uint8_t CMD_PING = 5;

struct OsdStruct {
    char    message[20];
    int     width;
    int     height;
};

bool SendToVdrString(uint8_t messageType, const char* message);
void SendToVdrOsd(const char* message, int width, int height);
bool SendToVdrPing();


#endif //VDR_OSR_BROWSER_SENDVDR_H
