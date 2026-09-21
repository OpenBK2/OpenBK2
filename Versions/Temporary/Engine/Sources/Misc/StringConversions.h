#pragma once

#include <string>
#include <cstdint>
#include <iomanip>
#include <sstream>

// utf8_to_wstring and wstring_to_utf8 used to live here, built on
// std::wstring_convert and std::codecvt_utf8. Both were deprecated in C++17 and
// are removed in C++26, so they were a compile error waiting for a toolchain
// update rather than merely a warning.
//
// They also duplicated port/unicode.h, which is where this tree decides what
// the narrow encoding is: UTF-8, everywhere and unconditionally, through the
// process code page on Windows and iconv elsewhere. Their five callers use
// WideToUTF8 and UTF8ToWide from there now.

namespace string_conversion
{

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