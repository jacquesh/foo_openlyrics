#pragma once

#include "stdafx.h"

namespace std
{
#ifdef UNICODE
    using tstring = wstring;
#else
    using tstring = string;
#endif
}

pfc::string8 tchar_to_pfcstring(const TCHAR* buffer);
pfc::string8 tchar_to_pfcstring(const TCHAR* buffer, size_t buffer_len);

std::string tchar_to_string(const TCHAR* buffer);
std::string tchar_to_string(const TCHAR* buffer, size_t buffer_len);

// Returns the number of bytes written (not the length of the string, which should be 1 less)
size_t string_to_tchar(const pfc::string8& string, TCHAR*& out_buffer);
size_t string_to_tchar(const std::string& string, TCHAR*& out_buffer);
size_t string_to_tchar(const char* cstr, size_t start_index, size_t length, TCHAR*& out_buffer);
