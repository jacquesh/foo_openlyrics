#pragma once

#include "stdafx.h"

std::string_view trim_surrounding_whitespace(std::string_view str);
std::string_view trim_trailing_text_in_brackets(std::string_view str);

std::string track_metadata(metadb_handle_ptr track, std::string_view key);
std::string track_metadata(const file_info& track_info, std::string_view key);
bool tag_values_match(std::string_view tagA, std::string_view tagB);

