#include "stdafx.h"

#include "winstr_util.h"

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

static std::string tchar_to_string(const TCHAR* buffer, size_t buffer_len)
{
#ifdef UNICODE
    size_t narrow_len = pfc::stringcvt::estimate_wide_to_utf8(buffer, buffer_len);
    std::string result(narrow_len, '\0');
    size_t chars_converted = pfc::stringcvt::convert_wide_to_utf8(result.data(), narrow_len, buffer, buffer_len);
    result.resize(chars_converted);
    return result;
#else // UNICODE
    static_assert(sizeof(TCHAR) == sizeof(char), "UNICODE is defined but TCHAR is not a char");
    return std::string(buffer, buffer_len);
#endif // UNICODE
}

std::string tchar_to_string(const TCHAR* buffer)
{
    return tchar_to_string(buffer, _tcslen(buffer));
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

static size_t string_to_tchar(const char* cstr, size_t start_index, size_t length, TCHAR*& out_buffer)
{
#ifdef UNICODE
    size_t wide_len = pfc::stringcvt::estimate_utf8_to_wide(cstr + start_index, length);
    wchar_t* result = new wchar_t[wide_len];
    assert(result != nullptr);
    pfc::stringcvt::convert_utf8_to_wide(result, wide_len, cstr + start_index, length);
    out_buffer = result;
    return wide_len;
#else // UNICODE
    TCHAR* result = malloc((length+1)*sizeof(TCHAR));
    memcpy(result, cstr + start_index, (length+1)*sizeof(TCHAR));
    out_buffer = result;
    return length+1;
#endif // UNICODE
}

size_t string_to_tchar(const std::string& string, TCHAR*& out_buffer)
{
    return string_to_tchar(string.c_str(), 0, string.length(), out_buffer);
}

