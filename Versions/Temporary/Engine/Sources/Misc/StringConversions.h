#include <codecvt>
#include <string>

// There are already some conversion functions like this in StrProc.h, but they're rather "archaic", this is the newer, more modern and standard C++ way of doing it

namespace string_conversion
{

    // convert UTF-8 string to wstring
    std::wstring utf8_to_wstring (const std::string& str)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
        return myconv.from_bytes(str);
    }

    // convert wstring to UTF-8 string
    std::string wstring_to_utf8 (const std::wstring& str)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
        return myconv.to_bytes(str);
    }

}