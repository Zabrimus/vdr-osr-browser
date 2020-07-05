#include "nativejshandler.h"

NativeJSHandler::NativeJSHandler() {
}

NativeJSHandler::~NativeJSHandler() {
}

bool NativeJSHandler::Execute(const CefString& name,
                                CefRefPtr<CefV8Value> object,
                                const CefV8ValueList& arguments,
                                CefRefPtr<CefV8Value>& retval,
                                CefString& exception) {

    /*
    if (name == "getAppUrl") {
        if (arguments.size() == 1) {
            CefV8Value *val = arguments.at(0);
            auto str = val->GetStringValue().ToString();
            auto url = this->appUrls->find(str);
        }

        return true;
    }
    */

    // Function does not exist.
    return false;
}
