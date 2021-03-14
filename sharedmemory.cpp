#include <cstdio>
#include <thread>
#include <cstring>
#include <sys/shm.h>
#include "sharedmemory.h"

/*
 * Layout of the shared memory
 *
 * uint16_t VDR command status      (2 bytes)
 * uint16_t Browser command status  (2 bytes)
 * uint16_t Data command status     (2 bytes)
 *
 * VDR command                      (1024 bytes)
 * Browser command                  (1024 bytes)
 * OSD/Video data                   (1920 * 1080 * 4 bytes)
 */

const int commandBufferSize    = 1024;
const int dataBufferSize       = 1920 * 1080 * 4;

// 0,1
const int vdrCommandStatusOffset = 0;

// 2,3
const int browserCommandStatusOffset = vdrCommandStatusOffset + sizeof(uint16_t);

// 4,5
const int dataStatusOffset = browserCommandStatusOffset + sizeof(uint16_t);

// 6 - 1029
const int vdrCommandOffset = dataStatusOffset + sizeof(uint16_t);

// 1030 - 2053
const int browserCommandOffset = vdrCommandOffset + commandBufferSize;

// 2054 -
const int dataOffset = browserCommandOffset + commandBufferSize;

const unsigned int sharedMemoryKey = 0xDEADC0DE;
const unsigned int sharedMemorySize = dataOffset + dataBufferSize;

const uint16_t shmpCanWrite = 0;
const uint16_t shmpCanRead  = 1;
const uint16_t shmpCurrentlyReading  = 2;

const int typeVdrCommand = 1;
const int typeBrowserCommand = 2;
const int typeData = 3;

SharedMemory::SharedMemory() {
    // init shared memory
    shmid = -1;
    shmp = nullptr;

    shmid = shmget(sharedMemoryKey, sharedMemorySize, 0666 | IPC_CREAT | IPC_EXCL) ;

    if (errno == EEXIST) {
        shmid = shmget(sharedMemoryKey, sharedMemorySize, 0666);
    }

    if (shmid == -1) {
        perror("Unable to get shared memory");
        return;
    }

    shmp = (uint8_t *) shmat(shmid, nullptr, 0);
    if (shmp == (void *) -1) {
        perror("Unable to attach to shared memory");
        return;
    }

    bufferRead(getOffsetForType(typeBrowserCommand));
    bufferRead(getOffsetForType(typeData));
    bufferRead(getOffsetForType(typeVdrCommand));
}

SharedMemory::~SharedMemory() {
    if (shmdt(shmp) == -1) {
        perror("Unable to detach from shared memory");
        return;
    }

    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        perror("Fehler in After IPC_RMID");
    }
}

int SharedMemory::getOffsetForType(int type) {
    int16_t offset;

    switch (type) {
        case typeVdrCommand:
            offset = vdrCommandStatusOffset;
            break;

        case typeBrowserCommand:
            offset = browserCommandStatusOffset;
            break;

        case typeData:
            offset = dataStatusOffset;
            break;

        default:
            // must not happen
            offset = -1;
            break;
    }

    return offset;
}

bool SharedMemory::canWrite(int offset) {
    uint16_t currentVal;
    memcpy(&currentVal, shmp+offset, sizeof(uint16_t));

    return currentVal == shmpCanWrite;
}

bool SharedMemory::canRead(int offset) {
    uint16_t currentVal;
    memcpy(&currentVal, shmp+offset, sizeof(uint16_t));

    bool cr = currentVal == shmpCanRead;

    if (cr) {
        memcpy(shmp + offset, &shmpCurrentlyReading, sizeof(uint16_t));
    }

    return cr;
}

void SharedMemory::bufferWritten(int offset) {
    memcpy(shmp + offset, &shmpCanRead, sizeof(uint16_t));
}

void SharedMemory::bufferRead(int offset) {
    memcpy(shmp + offset, &shmpCanWrite, sizeof(uint16_t));
}

bool SharedMemory::waitForRead(int type, int waitMillis, int maxIteration) {
    int i = 0;

    int offset = getOffsetForType(type);

    while (!canRead(offset) && i < maxIteration) {
        std::this_thread::sleep_for(std::chrono::milliseconds(waitMillis));
        ++i;
    }

    return i < maxIteration;
}

bool SharedMemory::waitForWrite(int type, int waitMillis, int maxIteration) {
    int i = 0;

    int offset = getOffsetForType(type);

    while (!canWrite(offset) && i < maxIteration) {
        std::this_thread::sleep_for(std::chrono::milliseconds(waitMillis));
        ++i;
    }

    return i < maxIteration;
}

bool SharedMemory::writeBrowserData(uint8_t* data, int size, int waitMillis, int maxIteration) {
    if (waitForWrite(typeData, waitMillis, maxIteration)) {
        memcpy(shmp + dataOffset, &size, sizeof(int));
        memcpy(shmp + sizeof(int) + dataOffset, data, size);

        bufferWritten(dataStatusOffset);
        return true;
    }

    return false;
}

bool SharedMemory::readBrowserData(uint8_t** data, int* size, int waitMillis, int maxIteration) {
    if (waitForRead(typeData, waitMillis, maxIteration)) {
        memcpy(size, shmp + dataOffset, sizeof(int));
        *data = shmp + dataOffset + sizeof(int);
        return true;
    }

    return false;
}

void SharedMemory::finishedReadBrowserData() {
    bufferRead(dataStatusOffset);
}

bool SharedMemory::writeBrowserCommand(char* data, int waitMillis, int maxIteration) {
    if (waitForWrite(typeBrowserCommand, waitMillis, maxIteration)) {
        strcpy(reinterpret_cast<char *>(shmp + browserCommandOffset), data);

        bufferWritten(browserCommandStatusOffset);
        return true;
    }

    return false;
}

char* SharedMemory::readBrowserCommand(int waitMillis, int maxIteration) {
    if (waitForRead(typeBrowserCommand, waitMillis, maxIteration)) {
        return reinterpret_cast<char *>(shmp + browserCommandOffset);
    }

    return nullptr;
}

void SharedMemory::finishedReadBrowserCommand() {
    bufferRead(browserCommandStatusOffset);
}

bool SharedMemory::writeVdrCommand(char* data, int waitMillis, int maxIteration) {
    if (waitForWrite(typeVdrCommand, waitMillis, maxIteration)) {
        strcpy(reinterpret_cast<char *>(shmp + vdrCommandOffset), data);

        bufferWritten(vdrCommandStatusOffset);
        return true;
    }

    return false;
}

char* SharedMemory::readVdrCommand(int waitMillis, int maxIteration) {
    if (waitForRead(typeVdrCommand, waitMillis, maxIteration)) {
        return reinterpret_cast<char *>(shmp + vdrCommandOffset);
    }

    return nullptr;
}

void SharedMemory::finishedReadVdrCommand() {
    bufferRead(vdrCommandStatusOffset);
}

uint8_t* SharedMemory::getBrowserData() {
    return shmp + getOffsetForType(typeData);
}

SharedMemory sharedMemory;