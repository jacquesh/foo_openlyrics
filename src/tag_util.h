#pragma once

#include "stdafx.h"

std::string_view trim_surrounding_whitespace(std::string_view str);
std::string_view trim_trailing_text_in_brackets(std::string_view str);

std::string track_metadata(metadb_handle_ptr track, std::string_view key);
std::string track_metadata(const file_info& track_info, std::string_view key);
bool tag_values_match(std::string_view tagA, std::string_view tagB);


// Convert the given tag string from UTF-8 (which fb2k specifies all tag strings to be)
// to ASCII, transliterating characters where possible such that, for example:
// "ā" is converted to "a".
// Characters with no reasonable ascii approximation are dropped.
std::string transliterate_to_ascii(std::string_view tag_str);
