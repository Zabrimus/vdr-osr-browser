#ifndef VDR_OSR_BROWSER_OSRHANDLER_H
#define VDR_OSR_BROWSER_OSRHANDLER_H

#include "include/cef_render_handler.h"
#include "include/cef_audio_handler.h"

class BrowserClient;
class Encoder;

class OSRHandler : public CefRenderHandler,
                   public CefAudioHandler {
private:
    int renderWidth;
    int renderHeight;

    int shmid;
    uint8_t *shmp;
    std::mutex shm_mutex;

    Encoder* encoder = nullptr;
    std::thread* encoderThread;

    bool isVideoStarted = false;
    bool encoderStopped = false;

    static BrowserClient *browserClient;

    int clearX, clearY, clearWidth, clearHeight;

public:
    OSRHandler(BrowserClient *bc, int width, int height);
    ~OSRHandler() override;

    // enable or disable the encoder
    bool enableEncoder();
    void disableEncoder();

    // clear parts of the OSD
    void osdClearVideo(int x, int y, int width, int height);

    // video handler
    void setRenderSize(int width, int height);
    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) override;
    void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height) override;
    void osdProcessed() { shm_mutex.unlock(); };

    // audio handler
    bool GetAudioParameters(CefRefPtr<CefBrowser> browser, CefAudioParameters &params) override;
    void OnAudioStreamStarted(CefRefPtr<CefBrowser> browser, const CefAudioParameters &params, int channels) override;
    void OnAudioStreamPacket(CefRefPtr<CefBrowser> browser, const float **data, int frames, int64 pts) override;
    void OnAudioStreamStopped(CefRefPtr<CefBrowser> browser) override;
    void OnAudioStreamError(CefRefPtr<CefBrowser> browser, const CefString &message) override;

    // video to VDR
    int writeVideoToShm(uint8_t *buf, int buf_size);

    IMPLEMENT_REFCOUNTING(OSRHandler);
};

#endif //VDR_OSR_BROWSER_OSRHANDLER_H
