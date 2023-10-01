#pragma once

#include "lyric_io.h"
#include "win32_util.h"

HWND SpawnLyricEditor(HWND parent_window, const LyricData& lyrics, LyricUpdateHandle& update);
HWND SpawnManualLyricSearch(HWND parent_window, LyricUpdateHandle& update);
HWND SpawnBulkLyricSearch(std::vector<metadb_handle_ptr> tracks_to_search);

size_t num_lyric_panels();
void register_update_handle_with_lyric_panels(std::unique_ptr<LyricUpdateHandle>&& handle);
void repaint_all_lyric_panels();
void recompute_lyric_panel_backgrounds();

class LyricPanel;
bool should_panel_search(const LyricPanel* panel);

void show_external_lyric_window();
LyricPanel* get_external_lyric_window();

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
