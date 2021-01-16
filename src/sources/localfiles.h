#pragma once

#include "stdafx.h"

#include "lyric_data.h"

namespace sources::localfiles
{
    void RegisterLyricPanel(HWND panel_handle);
    void DeregisterLyricPanel(HWND panel_handle);

    pfc::string8 GetLyricsDir();

    LyricDataRaw Query(metadb_handle_ptr track);

    void SaveLyrics(metadb_handle_ptr track, LyricFormat format, const pfc::string8& lyrics);
}