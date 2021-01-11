#pragma once

#include "stdafx.h"

pfc::string8 tchar_to_string(const TCHAR* buffer, size_t buffer_len);
size_t string_to_tchar(const pfc::string8& string, TCHAR*& out_buffer);
