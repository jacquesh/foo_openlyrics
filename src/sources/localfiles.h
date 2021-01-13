#pragma once

#include "stdafx.h"

#include "ui_lyric_editor.h" // Included for LyricFormat

namespace sources::localfiles
{
    void RegisterLyricPanel(HWND panel_handle);
    void DeregisterLyricPanel(HWND panel_handle);

    pfc::string8 GetLyricsDir();

    // Returns the path of the file that was loaded, if any. If no file is found, returns empty string.
    pfc::string8 Query(const pfc::string8& title, pfc::string8& out_lyrics);

    void SaveLyrics(const pfc::string& title, LyricFormat format, const pfc::string8& lyrics);
}