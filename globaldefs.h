#ifndef VDR_OSR_BROWSER_GLOBALDEFS_H
#define VDR_OSR_BROWSER_GLOBALDEFS_H

#include <string>

// #define TO_VDR_CHANNEL "ipc:///tmp/vdrosr_tovdr.ipc"
// #define FROM_VDR_CHANNEL "ipc:///tmp/vdrosr_tobrowser.ipc"

#define TO_VDR_CHANNEL   "tcp://127.0.0.1:5560"
#define FROM_VDR_CHANNEL "tcp://127.0.0.1:5561"
#define VIDEO_UDP_PORT   5560
#define VIDEO_TCP_PORT   5562
#define VIDEO_UNIX       "/tmp/hbbtvvideo.uds"

#define USER_AGENT "HbbTV/1.4.1 (+DRM;Samsung;SmartTV2015;T-HKM6DEUC-1490.3;;) OsrTvViewer"
// #define USER_AGENT "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/77.0.3865.90 Safari/537.36"
// #define USER_AGENT "Mozilla/5.0 (X11; U; Linux i686; en-US) AppleWebKit/533.4 (KHTML, like Gecko) Chrome/5.0.375.127 Large"

void replaceAll(std::string &source, std::string search, std::string replacement);
void replaceAll(std::string &source, std::string search1, std::string search2, std::string repl1, std::string repl2);

// trim from start (in place)
void ltrim(std::string &s);

// trim from end (in place)
void rtrim(std::string &s);

// trim from both ends (in place)
void trim(std::string &s);

// trim from start (copying)
std::string ltrim_copy(std::string s);

// trim from end (copying)
std::string rtrim_copy(std::string s);

// trim from both ends (copying)
std::string trim_copy(std::string s);

// test if string endsWith another string
bool endsWith(const std::string& str, const std::string& suffix);

// test if string startsWith another string
bool startsWith(const std::string& str, const std::string& prefix);

#endif // VDR_OSR_BROWSER_GLOBALDEFS_H
