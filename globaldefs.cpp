#include "globaldefs.h"
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

void replaceAll(std::string &source, std::string search, std::string replacement) {
    size_t pos = source.find(search);

    while (pos != std::string::npos) {
        source.replace(pos, search.size(), replacement);
        pos = source.find(search, pos + replacement.size());
    }
}

void replaceAll(std::string &source, std::string search1, std::string search2, std::string repl1, std::string repl2) {
    size_t pos = source.find(search1);

    while (pos != std::string::npos) {
        source.replace(pos, search1.size(), repl1);
        pos = source.find(search2, pos + repl1.size());
        source.replace(pos, search2.size(), repl2);

        pos = source.find(search1, pos + repl2.size());
    }
}


// trim from start (in place)
void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                    std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

// trim from both ends (in place)
void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}

// trim from end (copying)
std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}

// trim from both ends (copying)
std::string trim_copy(std::string s) {
    trim(s);
    return s;
}