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
    namespace searching
    {
        std::vector<GUID> active_sources();
        std::vector<std::string> tags();
    }

    namespace saving
    {
        bool autosave_enabled();
        SaveMethod save_method();

        const char* filename_format();

        std::string_view untimed_tag();
        std::string_view timestamped_tag();
    }

    int get_render_linegap();
}
