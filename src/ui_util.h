#pragma once

#include "stdafx.h"

#include "lyric_data.h"

std::string get_track_friendly_string(const LyricData& lyrics);
std::string get_track_friendly_string(metadb_handle_ptr track);
