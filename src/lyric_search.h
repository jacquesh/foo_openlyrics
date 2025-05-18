#pragma once

#include <optional>
#include <string>

#include "foobar2000/SDK/metadb_handle.h"

// All of these functions must be called from the main thread
void initiate_lyrics_autosearch(metadb_handle_ptr track, metadb_v2_rec_t track_info, bool ignore_search_avoidance);
std::optional<std::string> get_autosearch_progress_message();
