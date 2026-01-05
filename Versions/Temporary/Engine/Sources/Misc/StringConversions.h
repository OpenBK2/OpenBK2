#pragma once

#include <codecvt>
#include <string>
#include <cstdint>
#include <iomanip>
#include <sstream>

// There are already some conversion functions like this in StrProc.h, but they're rather "archaic", this is the newer, more modern and standard C++ way of doing it

namespace string_conversion
{

    // convert UTF-8 string to wstring
    static std::wstring utf8_to_wstring (const std::string& str)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
        return myconv.from_bytes(str);
    }

    // convert wstring to UTF-8 string
    static std::string wstring_to_utf8 (const std::wstring& str)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
        return myconv.to_bytes(str);
    }

    static std::string RGBA_to_hex(uint32_t bgra, bool includeAlpha = true, bool includeHashtag = false)
    {
        uint8_t b = (bgra >> 8) & 0xFF;
        uint8_t g = (bgra >> 16) & 0xFF;
        uint8_t r = (bgra >> 24)  & 0xFF;
        uint8_t a =  bgra        & 0xFF;

        std::ostringstream oss;
        if (includeHashtag)
            oss << "#";
        oss
            << std::uppercase << std::hex << std::setfill('0')
            << std::setw(2) << (int)r
            << std::setw(2) << (int)g
            << std::setw(2) << (int)b;

        if (includeAlpha)
            oss << std::setw(2) << (int)a;

        return oss.str();
    }

}