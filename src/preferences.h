#pragma once

#include "stdafx.h"

// NOTE: These enums must change in a backward-compatible manner.
//       This means that values can never be removed or re-used.
//       If we don't do this then updates will cause weird behaviour on users that have
//       saved settings on previous versions.

enum class SaveMethod : int
{
    None            = 0,
    ConfigDirectory = 1,
    Id3Tag          = 2
};

namespace preferences
{
    pfc::list_t<GUID> get_active_sources();

    bool get_autosave_enabled();
    SaveMethod get_save_method();
    const char* get_filename_format();

    int get_render_linegap();
}
