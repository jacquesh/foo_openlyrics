#pragma once

#include "foobar2000/SDK/metadb_handle.h"

#include "lyric_data.h"
#include "lyric_io.h"

HWND SpawnLyricEditor(const LyricData& lyrics, metadb_handle_ptr track, metadb_v2_rec_t track_info);
HWND SpawnManualLyricSearch(metadb_handle_ptr track, metadb_v2_rec_t track_info);
HWND SpawnBulkLyricSearch(std::vector<metadb_handle_ptr> tracks_to_search);
void SpawnExternalLyricWindow();

size_t num_visible_lyric_panels();
void repaint_all_lyric_panels();
void recompute_lyric_panel_backgrounds();
void announce_lyric_update(LyricUpdate update);

// Provides fb2k's default UI parameters, usually only available through UI components.
// These should be queried just-in-time as the values they return may change if the user
// changes their preferences (e.g to switch to dark mode).
namespace defaultui
{
    t_ui_font default_font();
    t_ui_font console_font();
    t_ui_color background_colour();
    t_ui_color text_colour();
    t_ui_color highlight_colour();
}
