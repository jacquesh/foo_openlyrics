#pragma once

#include <string>

#include "lyric_data.h"

void lyric_metadata_log_retrieved(const metadb_v2_rec_t& track_info, const LyricData& lyrics);
void lyric_metadata_log_edit(const metadb_v2_rec_t& track_info);

std::string get_lyric_metadata_string(const LyricData& lyrics, const metadb_v2_rec_t& track_info);
