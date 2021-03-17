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

const int typeVdrCommand = 1;
const int typeBrowserCommand = 2;
const int typeData = 3;

SharedMemory::SharedMemory(bool initBrowserMode) {
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

    segments = new Segment[3];

    segments[0] = Segment(vdrCommandOffset, vdrCommandStatusOffset);
    segments[1] = Segment(browserCommandOffset, browserCommandStatusOffset);
    segments[2] = Segment(dataOffset, dataStatusOffset);

    if (initBrowserMode) {
        setMode(shmpWriteMode, Data);
        setMode(shmpWriteMode, BrowserCommand);
    } else {
        setMode(shmpWriteMode,  VdrCommand);
    }
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

int SharedMemory::waitForRead(AvailableSegments segment, int timeoutMillis) {
    return segments[segment].waitForRead(timeoutMillis);
}

int SharedMemory::waitForWrite(AvailableSegments segment, int timeoutMillis) {
    return segments[segment].waitForWrite(timeoutMillis);
}

uint8_t* SharedMemory::write(uint8_t* data, int size, AvailableSegments segment) {
    return segments[segment].write(data, size);
}

uint8_t* SharedMemory::write(char* data, AvailableSegments segment) {
    return segments[segment].write(data);
}

uint8_t* SharedMemory::read(uint8_t** data, int* size, AvailableSegments segment) {
    return segments[segment].read(data, size);
}

char* SharedMemory::readString(AvailableSegments segment) {
    return segments[segment].readString();
}

void SharedMemory::finishedReading(AvailableSegments segment) {
    segments[segment].finishedReading();
}

uint8_t* SharedMemory::getMemoryPtr(AvailableSegments segment) {
    return segments[segment].getMemoryPtr();
}

void SharedMemory::setMode(int mode, AvailableSegments segment) {
    segments[segment].setMode(mode);
}

int SharedMemory::getMode(AvailableSegments segment) {
    return segments[segment].getMode();
}

bool Segment::canWrite() {
    return getMode() == shmpWriteMode;
}

bool Segment::canRead() {
    bool cr = getMode() == shmpReadMode;

    if (cr) {
        setMode(shmpCurrentlyReading);
    }

    return cr;
}

int Segment::waitForRead(int timeoutMillis) {
    int i = 0;
    int maxIteration = (timeoutMillis * 1000) / waitUsec;

    while (!canRead() && i < maxIteration) {
        std::this_thread::sleep_for(std::chrono::microseconds(pollingInterval));
        i++;
    }

    if (i < maxIteration) {
        return 1;
    } else {
        return -1;
    }
}

int Segment::waitForWrite(int timeoutMillis) {
    int i = 0;
    int maxIteration = (timeoutMillis * 1000) / waitUsec;

    while (!canWrite() && i < maxIteration) {
        std::this_thread::sleep_for(std::chrono::microseconds(pollingInterval));
        ++i;
    }

    if (i < maxIteration) {
        return 1;
    } else {
        return -1;
    }
}

uint8_t* Segment::write(uint8_t* data, int size) {
    memcpy(sharedMemory.shmp + dataOffset, &size, sizeof(int));
    memcpy(sharedMemory.shmp + sizeof(int) + dataOffset, data, size);

    setMode(shmpReadMode);

    return sharedMemory.shmp + dataOffset;
}

uint8_t* Segment::write(char* data) {
    strcpy(reinterpret_cast<char *>(sharedMemory.shmp + dataOffset), data);

    setMode(shmpReadMode);

    return sharedMemory.shmp + dataOffset;
}

uint8_t* Segment::read(uint8_t** data, int* size) {
    memcpy(size, sharedMemory.shmp + dataOffset, sizeof(int));
    *data = sharedMemory.shmp + dataOffset + sizeof(int);

    return *data;
}

char* Segment::readString() {
    return reinterpret_cast<char *>(sharedMemory.shmp + dataOffset);
}

void Segment::finishedReading() {
    setMode(shmpWriteMode);
}

uint8_t* Segment::getMemoryPtr() {
    return sharedMemory.shmp + dataOffset;
}

void Segment::setMode(int mode) {
    volatile auto p = sharedMemory.shmp + statusOffset;
    *((uint16_t*)p) = mode;
}

uint16_t Segment::getMode() {
    volatile auto p = sharedMemory.shmp + statusOffset;
    return *(uint16_t*)p;
}

SharedMemory sharedMemory(true);