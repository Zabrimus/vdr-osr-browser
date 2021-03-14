#include <chrono>
#include <thread>
#include <iostream>
#include <cstring>
#include "sharedmemory.h"

const int sleepTime = 0;
const int sleepOffset = 0;
const int loops = 5000;

static int type1 = 0;
static int type2 = 0;
static int type3 = 0;
static int type4 = 0;
static int type5 = 0;
static int type6 = 0;

class TestThread {
private:
    bool isRunning;
    int type = 0;
    int loop = 0;
    char *cmd;
    uint8_t *buf = nullptr;
    int size;
    char* result;

public:
    TestThread(int type) {
        this->type = type;
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
                    if (!sharedMemory.writeBrowserCommand(cmd)) {
                        fprintf(stderr, "writeBrowserCommand failed\n");
                        isRunning = false;
                    }
                    free(cmd);
                    loop++;

                    type1++;

                    // sleep a random time
                    if (sleepTime > 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % sleepTime + sleepOffset));
                    }

                    break;

                case 2: // readBrowserCommand
                    asprintf(&cmd, "writeBrowserCommand %d", loop);
                    result = sharedMemory.readBrowserCommand();

                    if (strcmp(result, cmd) != 0) {
                        fprintf(stderr, "readBrowserCommand failed\n");
                        isRunning = false;
                    }
                    loop++;

                    sharedMemory.finishedReadBrowserCommand();

                    free(cmd);

                    type2++;

                    // sleep a random time
                    if (sleepTime > 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % sleepTime + sleepOffset));
                    }

                    break;

                case 3: // writeVdrCommand
                    asprintf(&cmd, "writeVdrCommand %d", loop);
                    if (!sharedMemory.writeVdrCommand(cmd)) {
                        fprintf(stderr, "writeVdrCommand failed\n");
                        isRunning = false;
                    }
                    free(cmd);
                    loop++;

                    type3++;

                    // sleep a random time
                    if (sleepTime > 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % sleepTime + sleepOffset));
                    }

                    break;

                case 4: // readVdrCommand
                    asprintf(&cmd, "writeVdrCommand %d", loop);
                    result = sharedMemory.readVdrCommand();

                    if (strcmp(result, cmd) != 0) {
                        fprintf(stderr, "readVdrCommand failed\n");
                        isRunning = false;
                    }
                    loop++;

                    sharedMemory.finishedReadVdrCommand();
                    free(cmd);

                    type4++;

                    // sleep a random time
                    if (sleepTime > 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % sleepTime + sleepOffset));
                    }

                    break;

                case 5: // writeBrowserData
                    asprintf(&cmd, "writeBrowserData %d", loop);
                    if (!sharedMemory.writeBrowserData(reinterpret_cast<uint8_t *>(cmd), strlen(cmd) + 1)) {
                        fprintf(stderr, "writeBrowserData failed\n");
                        isRunning = false;
                    }
                    free(cmd);
                    loop++;

                    type5++;

                    // sleep a random time
                    if (sleepTime > 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % sleepTime + sleepOffset));
                    }

                    break;

                case 6: // readBrowserData
                    asprintf(&cmd, "writeBrowserData %d", loop);
                    if (!sharedMemory.readBrowserData(&buf, &size)) {
                        fprintf(stderr, "READ Failed\n");
                        isRunning = false;
                    }

                    if ((buf == nullptr) || ((size - 1 != strlen(reinterpret_cast<const char *>(buf))) || (strcmp(reinterpret_cast<const char *>(buf), cmd) != 0))) {
                        fprintf(stderr, "readBrowserData failed\n");
                        isRunning = false;
                    }
                    free(cmd);
                    sharedMemory.finishedReadBrowserData();

                    loop++;

                    type6++;

                    // sleep a random time
                    if (sleepTime > 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % sleepTime + sleepOffset));
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

    TestThread testThread1(1);
    TestThread testThread2(2);
    TestThread testThread3(3);
    TestThread testThread4(4);
    TestThread testThread5(5);
    TestThread testThread6(6);

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
}