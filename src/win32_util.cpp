#include "stdafx.h"

#include "win32_util.h"

std::tstring to_tstring(std::string_view string)
{
#ifdef UNICODE
    size_t wide_len = pfc::stringcvt::estimate_utf8_to_wide(string.data(), string.length());
    wchar_t* out_buffer = new wchar_t[wide_len];
    pfc::stringcvt::convert_utf8_to_wide(out_buffer, wide_len, string.data(), string.length());
    std::wstring result = std::wstring(out_buffer);
    delete[] out_buffer;
    return result;
#else // UNICODE
    static_assert(sizeof(TCHAR) == sizeof(char), "UNICODE is defined but TCHAR is not a char");
    return string;
#endif // UNICODE
}

std::tstring to_tstring(const std::string& string)
{
    return to_tstring(std::string_view{string});
}

std::tstring to_tstring(const pfc::string8& string)
{
    return to_tstring(std::string_view{string.c_str(), string.length()});
}

std::string from_tstring(std::tstring_view string)
{
#ifdef UNICODE
    size_t narrow_len = pfc::stringcvt::estimate_wide_to_utf8(string.data(), string.length());
    std::string result(narrow_len, '\0');
    size_t chars_converted = pfc::stringcvt::convert_wide_to_utf8(result.data(), narrow_len, string.data(), string.length());
    result.resize(chars_converted);
    return result;
#else // UNICODE
    static_assert(sizeof(TCHAR) == sizeof(char), "UNICODE is defined but TCHAR is not a char");
    return std::string(buffer, buffer_len);
#endif // UNICODE
}

std::string from_tstring(const std::tstring& string)
{
    return from_tstring(std::tstring_view(string));
}

std::optional<SIZE> GetTextExtents(HDC dc, std::tstring_view string)
{
    SIZE output;
    BOOL success = GetTextExtentPoint32(dc, string.data(), string.length(), &output);
    if(success)
    {
        return output;
    }
    else
    {
        return {};
    }
}

BOOL DrawTextOut(HDC dc, int x, int y, std::tstring_view string)
{
    return TextOut(dc, x, y, string.data(), string.length());
}

