#include <chrono>
#include <thread>
#include <iostream>
#include <cstring>
#include "sharedmemory.h"

const int sleepTime = 10;
const int sleepOffset = 10;
const int loops = 500000;

static int type1 = 0;
static int type2 = 0;
static int type3 = 0;
static int type4 = 0;
static int type5 = 0;
static int type6 = 0;

static int timeout1 = 0;
static int timeout2 = 0;
static int timeout3 = 0;
static int timeout4 = 0;
static int timeout5 = 0;
static int timeout6 = 0;

class TestThread {
private:
    bool isRunning;
    int type = 0;
    int loop = 0;
    char *cmd;
    uint8_t *buf = nullptr;
    int size;
    char* result;
    int timeout;

public:
    TestThread(int type, int timeout) {
        this->type = type;
        this->timeout = timeout;
    }

    void Start() {
        isRunning = true;

        while (isRunning) {
            if (loop > loops) {
                isRunning = false;
            }

            switch(type) {
                case 1: // writeBrowserCommand
                    asprintf(&cmd, "writeBrowserCommand %d", loop);

                    if (sharedMemory.waitForWrite(BrowserCommand, timeout) != -1) {
                        sharedMemory.write(cmd, BrowserCommand);

                        loop++;
                        type1++;
                    } else {
                        // fprintf(stderr, "Timeout in writeBrowserCommand\n");
                        timeout1++;

                        isRunning = timeout1 < 40;
                    }

                    free(cmd);

                    // sleep a random time
                    if (sleepTime > 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(rand() % sleepTime + sleepOffset));
                    }

                    // sharedMemory.unlock(BrowserCommand);

                    break;

                case 2: // readBrowserCommand
                    asprintf(&cmd, "writeBrowserCommand %d", loop);

                    if (sharedMemory.waitForRead(BrowserCommand, timeout) != -1) {
                        result = sharedMemory.readString(BrowserCommand);

                        if (strcmp(result, cmd) != 0) {
                            fprintf(stderr, "readBrowserCommand failed\n");
                            isRunning = false;
                        } else {
                            loop++;
                            type2++;
                        }
                    } else {
                        // fprintf(stderr, "Timeout in readBrowserCommand\n");
                        timeout2++;

                        isRunning = timeout2 < 40;
                    }

                    sharedMemory.finishedReading(BrowserCommand);

                    free(cmd);

                    // sleep a random time
                    if (sleepTime > 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(rand() % sleepTime + sleepOffset));
                    }

                    break;

                case 3: // writeVdrCommand
                    asprintf(&cmd, "writeVdrCommand %d", loop);

                    if (sharedMemory.waitForWrite(VdrCommand, timeout) != -1) {
                        sharedMemory.write(cmd, VdrCommand);
                        loop++;
                        type3++;
                    } else {
                        // fprintf(stderr, "Timeout in writeVdrCommand\n");
                        timeout3++;

                        isRunning = timeout3 < 40;
                    }

                    free(cmd);

                    // sleep a random time
                    if (sleepTime > 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(rand() % sleepTime + sleepOffset));
                    }

                    break;

                case 4: // readVdrCommand
                    asprintf(&cmd, "writeVdrCommand %d", loop);

                    if (sharedMemory.waitForRead(VdrCommand, timeout) != -1) {
                        result = sharedMemory.readString(VdrCommand);

                        if (strcmp(result, cmd) != 0) {
                            fprintf(stderr, "readBrowserCommand failed\n");
                            isRunning = false;
                        } else {
                            loop++;
                            type4++;
                        }
                    } else {
                        // fprintf(stderr, "Timeout in readBrowserCommand\n");
                        timeout4++;

                        isRunning = timeout4 < 40;
                    }

                    sharedMemory.finishedReading(VdrCommand);
                    free(cmd);

                    // sleep a random time
                    if (sleepTime > 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(rand() % sleepTime + sleepOffset));
                    }

                    break;

                case 5: // writeBrowserData
                    asprintf(&cmd, "writeBrowserData %d", loop);

                    if (sharedMemory.waitForWrite(Data, timeout) != -1) {
                        sharedMemory.write(reinterpret_cast<uint8_t *>(cmd), strlen(cmd) + 1, Data);
                        loop++;
                        type5++;
                    } else {
                        // fprintf(stderr, "Timeout in writeBrowserdata\n");
                        timeout5++;

                        isRunning = timeout5 < 40;
                    }

                    free(cmd);

                    // sleep a random time
                    if (sleepTime > 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(rand() % sleepTime + sleepOffset));
                    }

                    break;

                case 6: // readBrowserData
                    asprintf(&cmd, "writeBrowserData %d", loop);

                    if (sharedMemory.waitForRead(Data, timeout) != -1) {

                        sharedMemory.read(&buf, &size, Data);

                        if ((buf == nullptr) || ((size - 1 != strlen(reinterpret_cast<const char *>(buf))) || (strcmp(reinterpret_cast<const char *>(buf), cmd) != 0))) {
                            fprintf(stderr, "readBrowserData failed\n");
                            isRunning = false;
                        } else {
                            loop++;
                            type6++;
                        }
                    } else {
                        // fprintf(stderr, "Timeout in readBrowserCommand\n");
                        timeout6++;

                        isRunning = timeout6 < 40;
                    }

                    free(cmd);
                    sharedMemory.finishedReading(Data);

                    // sleep a random time
                    if (sleepTime > 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(rand() % sleepTime + sleepOffset));
                    }

                    break;
            }

            fprintf(stderr, "Run: %04d, %04d, %04d, %04d, %04d, %04d\r", type1, type2, type3, type4, type5, type6);
        }
    }

    void Stop() {
        isRunning = false;
    }
};

void startTestThread(TestThread test) {
    test.Start();
}

int main(int argc, char *argv[]) {

    int timeout = 500;

    TestThread testThread1(1, timeout);
    TestThread testThread2(2, timeout);
    TestThread testThread3(3, timeout);
    TestThread testThread4(4, timeout);
    TestThread testThread5(5, timeout);
    TestThread testThread6(6, timeout);

    std::thread test1(startTestThread, testThread1);
    std::thread test2(startTestThread, testThread2);
    std::thread test3(startTestThread, testThread3);
    std::thread test4(startTestThread, testThread4);
    std::thread test5(startTestThread, testThread5);
    std::thread test6(startTestThread, testThread6);

    test1.join();
    test2.join();
    test3.join();
    test4.join();
    test5.join();
    test6.join();


    fprintf(stderr, "\nFinished: ");

    int result = (type1-type2) + (type3-type4) + (type5-type6);
    if (result == 0) {
        fprintf(stderr, "\nOk...\n");
    } else {
        fprintf(stderr, "\nSomething is wrong...\n");
    }

    fprintf(stderr, "Timeout: %d %d %d %d %d %d\n", timeout1, timeout2, timeout3, timeout4, timeout5, timeout6);
}