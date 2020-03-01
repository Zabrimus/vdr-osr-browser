/**
 *  CEF OSR implementation used für vdr-plugin-htmlskin
 *
 *  osrhandler.h
 *
 *  (c) 2019 Robert Hannebauer
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#ifndef OSRHANDLER_H
#define OSRHANDLER_H

#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_render_handler.h"

class OSRHandler : public CefRenderHandler {
private:
    int renderWidth;
    int renderHeight;

    int socketId;
    int endpointId;

public:
    OSRHandler(int width, int height);
    ~OSRHandler();

    void setRenderSize(int width, int height);
    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) override;
    void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height) override;

    IMPLEMENT_REFCOUNTING(OSRHandler);
};

#endif // OSRHANDLER_H
