#pragma once

#include "stdafx.h"

#include "lyric_data.h"

namespace sources::localfiles
{
    const extern GUID src_guid;

    void RegisterLyricPanel(HWND panel_handle);
    void DeregisterLyricPanel(HWND panel_handle);

    pfc::string8 GetLyricsDir();

    void SaveLyrics(metadb_handle_ptr track, LyricFormat format, const pfc::string8& lyrics, abort_callback& abort);
}