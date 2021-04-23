#pragma once

#include "stdafx.h"

extern const GUID GUID_PREFERENCES_PAGE_ROOT;

// NOTE: These enums must change in a backward-compatible manner.
//       This means that values can never be removed or re-used.
//       If we don't do this then updates will cause weird behaviour on users that have
//       saved settings on previous versions.
enum class SaveMethod : int
{
    DEPRECATED_None = 0,
    LocalFile       = 1,
    Id3Tag          = 2
};

enum class SaveDirectoryClass : int
{
    DEPRECATED_None    = 0,
    ConfigDirectory    = 1,
    TrackFileDirectory = 2,
    Custom             = 3
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

        std::string filename(metadb_handle_ptr track);

        std::string_view untimed_tag();
        std::string_view timestamped_tag();
    }

    namespace display
    {
        t_ui_font font();
        std::optional<t_ui_color> foreground_colour();
        std::optional<t_ui_color> background_colour();
        std::optional<t_ui_color> highlight_colour();

        int render_linegap();
    }
}
