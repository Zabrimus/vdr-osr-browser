#ifndef VDR_OSR_BROWSER_OSRHANDLER_H
#define VDR_OSR_BROWSER_OSRHANDLER_H

#include "include/cef_render_handler.h"
#include "include/cef_audio_handler.h"
#include "videoplayer.h"

class BrowserClient;
class Encoder;

class OSRHandler : public CefRenderHandler,
                   public CefAudioHandler {
private:
    int renderWidth;
    int renderHeight;

    bool showPlayer;
    bool fullscreen;

    Encoder* encoder = nullptr;
    VideoPlayer *videoPlayer = nullptr;

    bool isVideoStarted = false;
    bool encoderStopped = false;

    static BrowserClient *browserClient;

    int clearX, clearY, clearWidth, clearHeight;

public:
    OSRHandler(BrowserClient *bc, int width, int height, bool showPlayer, bool fullscreen);
    ~OSRHandler() override;

    // enable or disable the encoder
    bool enableEncoder();
    void disableEncoder();
    void flushEncoder();

    // clear parts of the OSD
    void osdClearVideo(int x, int y, int width, int height);

    // video handler
    void setRenderSize(int width, int height);
    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) override;
    void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height) override;

    // audio handler
    bool GetAudioParameters(CefRefPtr<CefBrowser> browser, CefAudioParameters &params) override;
    void OnAudioStreamStarted(CefRefPtr<CefBrowser> browser, const CefAudioParameters &params, int channels) override;
    void OnAudioStreamPacket(CefRefPtr<CefBrowser> browser, const float **data, int frames, int64 pts) override;
    void OnAudioStreamStopped(CefRefPtr<CefBrowser> browser) override;
    void OnAudioStreamError(CefRefPtr<CefBrowser> browser, const CefString &message) override;

    IMPLEMENT_REFCOUNTING(OSRHandler);
};

#endif //VDR_OSR_BROWSER_OSRHANDLER_H
