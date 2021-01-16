#pragma once

#include "stdafx.h"

#include "lyric_data.h"

namespace sources::localfiles
{
    void RegisterLyricPanel(HWND panel_handle);
    void DeregisterLyricPanel(HWND panel_handle);

    pfc::string8 GetLyricsDir();

    LyricDataRaw Query(const pfc::string8& artist, const pfc::string8& album, const pfc::string8& title);

    void SaveLyrics(const pfc::string& title, LyricFormat format, const pfc::string8& lyrics);
}