#pragma once

#include <string>

#include "lyric_data.h"

void lyric_metadata_log_retrieved(metadb_handle_ptr track, const LyricData& lyrics);
void lyric_metadata_log_edit(metadb_handle_ptr track);

std::string get_lyric_metadata_string(const LyricData& lyrics, metadb_handle_ptr track);
