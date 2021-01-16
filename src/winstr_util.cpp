#include "stdafx.h"

#include "winstr_util.h"

pfc::string8 tchar_to_string(const TCHAR* buffer, size_t buffer_len)
{
#ifdef UNICODE
    size_t narrow_len = pfc::stringcvt::estimate_wide_to_utf8(buffer, buffer_len);
    pfc::string8 result;
    result.prealloc(narrow_len);
    char* narrow_buffer = result.lock_buffer(narrow_len);
    size_t chars_converted = pfc::stringcvt::convert_wide_to_utf8(narrow_buffer, narrow_len, buffer, buffer_len);
    result.unlock_buffer();
    return result;
#else // UNICODE
    static_assert(sizeof(TCHAR) == sizeof(char), "UNICODE is defined but TCHAR is not a char");
    return pfc::string8(buffer, buffer_len);
#endif // UNICODE
}

size_t string_to_tchar(const pfc::string8& string, TCHAR*& out_buffer)
{
    return string_to_tchar(string, 0, string.length(), out_buffer);
}

size_t string_to_tchar(const pfc::string8& string, size_t start_index, size_t length, TCHAR*& out_buffer)
{
#ifdef UNICODE
    // TODO: This seems to be doing something wrong. Some lyrics (such as that for Rise Against's Injection)
    //       include non-ASCII characters that display differently on our panel than they do in vim or in
    //       foo_uie_lyrics3
    size_t wide_len = pfc::stringcvt::estimate_utf8_to_wide(string.c_str() + start_index, length);
    wchar_t* result = new wchar_t[wide_len];
    assert(result != nullptr);
    pfc::stringcvt::convert_utf8_to_wide(result, wide_len, string.c_str() + start_index, length);
    out_buffer = result;
    return wide_len;
#else // UNICODE
    TCHAR* result = malloc((length+1)*sizeof(TCHAR));
    memcpy(result, string.c_str() + start_index, (length+1)*sizeof(TCHAR));
    out_buffer = result;
    return length+1;
#endif // UNICODE
}
