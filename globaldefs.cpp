#include "globaldefs.h"

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

