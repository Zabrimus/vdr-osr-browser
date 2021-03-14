#ifndef VDR_OSR_BROWSER_SHAREDMEMORY_H
#define VDR_OSR_BROWSER_SHAREDMEMORY_H

#include <cstdint>

class SharedMemory {
    private:
        int shmid;
        uint8_t *shmp;

        int getOffsetForType(int type);

        bool canWrite(int offset);
        bool canRead(int offset);

        void bufferWritten(int offset);
        void bufferRead(int offset);

        bool waitForRead(int type, int waitMillis, int maxIteration);
        bool waitForWrite(int type, int waitMillis, int maxIteration);

public:
        SharedMemory();
        ~SharedMemory();

        bool writeBrowserData(uint8_t* data, int size, int waitMillis = 10, int maxIteration = 50);
        bool readBrowserData(uint8_t** data, int* size, int waitMillis = 10, int maxIteration = 50);
        void finishedReadBrowserData();

        bool writeBrowserCommand(char* data, int waitMillis = 10, int maxIteration = 50);
        char* readBrowserCommand(int waitMillis = 10, int maxIteration = 50);
        void finishedReadBrowserCommand();

        bool writeVdrCommand(char* data, int waitMillis = 10, int maxIteration = 50);
        char* readVdrCommand(int waitMillis = 10, int maxIteration = 50);
        void finishedReadVdrCommand();

        // use with care
        uint8_t* getBrowserData();
};

extern SharedMemory sharedMemory;

#endif //VDR_OSR_BROWSER_SHAREDMEMORY_H
