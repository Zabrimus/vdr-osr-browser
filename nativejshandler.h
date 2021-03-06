#ifndef VDR_OSR_BROWSER_NATIVEJSHANDLER_H
#define VDR_OSR_BROWSER_NATIVEJSHANDLER_H

#include <map>
#include "include/cef_v8.h"

class NativeJSHandler : public CefV8Handler {

public:
    NativeJSHandler();
    ~NativeJSHandler();

    bool Execute(const CefString& name,
                    CefRefPtr<CefV8Value> object,
                    const CefV8ValueList& arguments,
                    CefRefPtr<CefV8Value>& retval,
                    CefString& exception) override;

    IMPLEMENT_REFCOUNTING(NativeJSHandler);
};

#endif // VDR_OSR_BROWSER_NATIVEJSHANDLER_H
