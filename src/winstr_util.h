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

std::tstring to_tstring(std::string_view string);
std::tstring to_tstring(const std::string& string);
std::tstring to_tstring(const pfc::string8& string);

std::string from_tstring(std::tstring_view string);
std::string from_tstring(const std::tstring& string);

std::string tchar_to_string(const TCHAR* buffer);

// Returns the number of bytes written (not the length of the string, which should be 1 less)
size_t string_to_tchar(const std::string& string, TCHAR*& out_buffer);
