#ifndef GLOBALDEFS_H
#define GLOBALDEFS_H

#include <string>

// #define TO_VDR_CHANNEL "ipc:///tmp/vdrosr_tovdr.ipc"
// #define FROM_VDR_CHANNEL "ipc:///tmp/vdrosr_tobrowser.ipc"

#define TO_VDR_CHANNEL "tcp://127.0.0.1:5560"
#define FROM_VDR_CHANNEL "tcp://127.0.0.1:5561"

#define USER_AGENT "HbbTV/1.4.1 (+DRM;Samsung;SmartTV2015;T-HKM6DEUC-1490.3;;) OsrTvViewer"
// #define USER_AGENT "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/77.0.3865.90 Safari/537.36"
// #define USER_AGENT "Mozilla/5.0 (X11; U; Linux i686; en-US) AppleWebKit/533.4 (KHTML, like Gecko) Chrome/5.0.375.127 Large"

void replaceAll(std::string &source, std::string search, std::string replacement);
void replaceAll(std::string &source, std::string search1, std::string search2, std::string repl1, std::string repl2);

#endif // GLOBALDEFS_H
