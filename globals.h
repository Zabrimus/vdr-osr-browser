#ifndef OSR_BROWSER_GLOBALS_H
#define OSR_BROWSER_GLOBALS_H

#define USER_AGENT "HbbTV/1.4.1 (+DRM;Samsung;SmartTV2015;T-HKM6DEUC-1490.3;;) OsrTvViewer"
// #define USER_AGENT "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/77.0.3865.90 Safari/537.36"
// #define USER_AGENT "Mozilla/5.0 (X11; U; Linux i686; en-US) AppleWebKit/533.4 (KHTML, like Gecko) Chrome/5.0.375.127 Large"

class Globals {
private:
    static int toVdrSocketId;
    static int toVdrEndpointId;

    static int fromVdrSocketId;
    static int fromVdrEndpointId;

public:
    Globals();
    ~Globals();

    static int GetToVdrSocket() { return toVdrSocketId; };
    static int GetFromVdrSocket() { return fromVdrSocketId; };
};

#endif //OSR_BROWSER_GLOBALS_H
