#pragma once

#include "lyric_io.h"
#include "win32_util.h"

HWND SpawnLyricEditor(HWND parent_window, const LyricData& lyrics, LyricUpdateHandle& update);
HWND SpawnManualLyricSearch(HWND parent_window, LyricUpdateHandle& update);
HWND SpawnBulkLyricSearch(std::vector<metadb_handle_ptr> tracks_to_search);

bool are_there_any_lyric_panels();
void register_update_handle_with_lyric_panels(std::unique_ptr<LyricUpdateHandle>&& handle);
void repaint_all_lyric_panels();
