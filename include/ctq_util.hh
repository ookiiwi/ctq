#ifndef CTQ_UTIL_HH
#define CTQ_UTIL_HH

#include <string>
#include <cctype>
#include <algorithm>

inline std::string ltrim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }));
    return s;
}

inline std::string rtrim(std::string s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), s.end());
    return s;
}

inline std::string trim(std::string s) {
    return rtrim(ltrim(s));
}

#endif