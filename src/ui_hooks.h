#pragma once

#include "lyric_io.h"
#include "win32_util.h"

HWND SpawnLyricEditor(HWND parent_window, const LyricData& lyrics, LyricUpdateHandle& update);
HWND SpawnManualLyricSearch(HWND parent_window, LyricUpdateHandle& update);
HWND SpawnBulkLyricSearch(std::vector<metadb_handle_ptr> tracks_to_search);

void repaint_all_lyric_panels();
