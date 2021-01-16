#pragma once

#include "stdafx.h"

pfc::string8 tchar_to_string(const TCHAR* buffer, size_t buffer_len);

// Returns the number of bytes written (not the length of the string, which should be 1 less)
size_t string_to_tchar(const pfc::string8& string, TCHAR*& out_buffer);
size_t string_to_tchar(const pfc::string8& string, size_t start_index, size_t length, TCHAR*& out_buffer);
