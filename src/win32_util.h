#pragma once

#include "stdafx.h"

namespace std
{
#ifdef UNICODE
    using tstring = wstring;
    using tstring_view = wstring_view;
#else
    using tstring = string;
    using tstring_view = string_view;
#endif
}

size_t wide_to_narrow_string(unsigned int codepage,
                             std::wstring_view wide,
                             std::vector<char>& out_buffer); // Returns bytes written to out_buffer
size_t narrow_to_wide_string(unsigned int codepage,
                             std::string_view narrow,
                             std::vector<wchar_t>& out_buffer); // Returns characters written to out_buffer

std::tstring to_tstring(std::string_view string);
std::tstring to_tstring(const std::string& string);
std::tstring to_tstring(const pfc::string8& string);

std::string from_tstring(std::tstring_view string);
std::string from_tstring(const std::tstring& string);

std::tstring normalise_utf8(std::tstring_view input);

size_t find_first_space(const std::tstring_view str, bool positive = true, size_t pos = 0);
size_t find_last_space(const std::tstring_view str, bool positive = true, size_t pos = std::tstring_view::npos);

#define HR_SUCCESS(hr) hr_success(hr, __FILE__, __LINE__)
bool hr_success(HRESULT result, const char* filename, int line_number);
