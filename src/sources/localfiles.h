#pragma once

#include "stdafx.h"

#include "lyric_data.h"

namespace sources::localfiles
{
    void RegisterLyricPanel(HWND panel_handle);
    void DeregisterLyricPanel(HWND panel_handle);

    pfc::string8 GetLyricsDir();

    // Returns the path of the file that was loaded, if any. If no file is found, returns empty string.
    LyricDataRaw Query(const pfc::string8& title, pfc::string8& out_loaded_file_path);

    void SaveLyrics(const pfc::string& title, LyricFormat format, const pfc::string8& lyrics);
}