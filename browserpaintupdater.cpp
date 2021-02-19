#include <chrono>
#include "browser.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/time.h>
#ifdef __cplusplus
}
#endif


inline uint64_t getCurrentTime() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

inline uint64_t getSleepTime(uint64_t lastTime, uint64_t currentTime, int fps) {
    int64_t fraction = 1000 / fps;
    int64_t next = fraction - (currentTime - lastTime);

    return next;
}

BrowserPaintUpdater::BrowserPaintUpdater(CefRefPtr<CefBrowser> _browser) {
    browser = _browser;
    isRunning = false;
}

BrowserPaintUpdater::~BrowserPaintUpdater() {
}

void BrowserPaintUpdater::Start() {
    isRunning = true;

    uint64_t timeWindow = 1000 * 1000 / 25;

    while (isRunning) {
        int64_t start = av_gettime();

        // FIXME: Siehe Kommentar in main.cpp
        // force call of OnPaint() in OSRHandler
        browser->GetHost()->Invalidate(PET_VIEW);

        // FIXME: Richtige Version
        // browser->GetHost()->SendExternalBeginFrame();

        int64_t end = av_gettime();
        int64_t elapsed = end - start;
        int64_t sleepTime = timeWindow - elapsed;

        std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
    }
}

void BrowserPaintUpdater::Stop() {
    isRunning = false;
}
