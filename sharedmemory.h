#ifndef VDR_OSR_BROWSER_SHAREDMEMORY_H
#define VDR_OSR_BROWSER_SHAREDMEMORY_H

#include <cstdint>

class Segment;

// all available segments
enum AvailableSegments { VdrCommand = 0, BrowserCommand = 1, Data = 2 };

// different modes
const uint16_t shmpWriteMode = 0;
const uint16_t shmpReadMode  = 1;
const uint16_t shmpCurrentlyReading  = 2;

class SharedMemory {
    friend Segment;

    private:
        int shmid;
        uint8_t *shmp;

        Segment *segments;

    public:
        explicit SharedMemory(bool isBrowserMode);
        ~SharedMemory();

        int waitForRead(AvailableSegments segment, int timeoutMillis = 500);
        int waitForWrite(AvailableSegments segment, int timeoutMillis = 500);

        uint8_t* write(uint8_t* data, int size, AvailableSegments segment);
        uint8_t* write(char* data, AvailableSegments segment);
        uint8_t* read(uint8_t** data, int* size, AvailableSegments segment);
        char* readString(AvailableSegments segment);
        void finishedReading(AvailableSegments segment);

        // use with care
        uint8_t* getMemoryPtr(AvailableSegments segment);
        void setMode(int mode, AvailableSegments segment);
        int getMode(AvailableSegments segment);
};

extern SharedMemory sharedMemory;

class Segment {
    private:
        int16_t dataOffset = -1;
        int16_t statusOffset = -1;

        int waitUsec = 100;
        std::chrono::duration<int64_t, std::micro> pollingInterval = std::chrono::microseconds(waitUsec);

    public:
        Segment() = default;

        Segment(int16_t dataOffset, int16_t statusOffset) {
            this->dataOffset = dataOffset;
            this->statusOffset = statusOffset;
        };

        ~Segment() = default;

        bool canWrite();
        bool canRead();

        int waitForRead(int timeoutMillis);
        int waitForWrite(int timeoutMillis);
        uint8_t* write(uint8_t* data, int size);
        uint8_t* write(char* data);
        uint8_t* read(uint8_t** data, int* size);
        char* readString();
        void finishedReading();

        uint8_t* getMemoryPtr();
        void setMode(int mode);
        uint16_t getMode();
};

#endif //VDR_OSR_BROWSER_SHAREDMEMORY_H
