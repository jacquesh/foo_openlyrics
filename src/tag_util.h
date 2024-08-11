#pragma once

#include <optional>
#include <string>

#include "foobar2000/SDK/metadb_handle.h"

std::string_view trim_surrounding_whitespace(std::string_view str);
std::string_view trim_surrounding_line_endings(std::string_view str);
std::string_view trim_trailing_text_in_brackets(std::string_view str);

metadb_v2_rec_t get_full_metadata(metadb_handle_ptr track);

std::string track_metadata(const metadb_v2_rec_t& track, std::string_view key);
std::string track_metadata(const file_info& track_info, std::string_view key);
bool tag_values_match(std::string_view tagA, std::string_view tagB);

std::optional<int> track_duration_in_seconds(const metadb_v2_rec_t& track);

bool track_is_remote(metadb_handle_ptr track);

