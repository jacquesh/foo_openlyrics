#pragma once

#include "stdafx.h"

#include "lyric_data.h"

namespace sources::localfiles
{
    const extern GUID src_guid;

    void RegisterLyricPanel(HWND panel_handle);
    void DeregisterLyricPanel(HWND panel_handle);

    pfc::string8 GetLyricsDir();

    void SaveLyrics(metadb_handle_ptr track, bool is_timestamped, std::string_view lyrics, abort_callback& abort);
}