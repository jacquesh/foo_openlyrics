#pragma once

#include "lyric_io.h"
#include "win32_util.h"

void SpawnLyricEditor(const LyricData& lyrics, LyricUpdateHandle& update);
void SpawnManualLyricSearch(LyricUpdateHandle& update);
void SpawnBulkLyricSearch(std::vector<metadb_handle_ptr> tracks_to_search);

void repaint_all_lyric_panels();
