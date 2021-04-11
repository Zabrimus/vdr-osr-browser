#ifndef VDR_OSR_BROWSER_GLOBALDEFS_H
#define VDR_OSR_BROWSER_GLOBALDEFS_H

#include <string>

// #define USER_AGENT "HbbTV/1.4.1 (+DL+PVR+DRM;Samsung;SmartTV2020;T-HKM6DEUC-1490.3;;) OsrTvViewer;Chrome"

// If we use a newer version (2016 and later), then Playready videos marked as playable, but they always fail.
// It's better not to try to play these videos.
#define USER_AGENT "HbbTV/1.4.1 (+DL+PVR+DRM;Samsung;SmartTV2015;T-HKM6DEUC-1490.3;;) OsrTvViewer;Chrome"

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
