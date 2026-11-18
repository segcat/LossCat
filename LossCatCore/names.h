#pragma once
#include <string>
#include <cstring>
#include <cctype>

static std::string cleanName(std::string s, const char* fallback)
{
    for (char& c : s)
    {
        if (std::strchr("<>:\"/\\|?*", c) || (unsigned char)c < 32)
        {
            c = '_';
        }
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == '.'))
    {
        s.pop_back();
    }
    return s.empty() ? fallback : s;
}

inline bool sameName(const std::string& a, const std::string& b)
{
    if (a.size() != b.size()) return false;
    for (size_t k = 0; k < a.size(); ++k)
    {
        if (std::tolower((unsigned char)a[k]) != std::tolower((unsigned char)b[k])) return false;
    }
    return true;
}
