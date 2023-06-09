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

enum class AutoSaveStrategy : int
{
    Never        = 0,
    Always       = 1,
    OnlySynced   = 2,
    OnlyUnsynced = 3,
};

enum class LineScrollDirection : int
{
    Vertical   = 0,
    Horizontal = 1
};

enum class LineScrollType : int
{
    Automatic = 0,
    Manual    = 1,
};

enum class AutoEditType : int
{
    Unknown                  = 0,
    // CreateInstrumental       = 1, // Deprecated
    ReplaceHtmlEscapedChars  = 2,
    RemoveRepeatedSpaces     = 3,
    RemoveRepeatedBlankLines = 4,
    RemoveAllBlankLines      = 5,
    ResetCapitalisation      = 6,
    FixMalformedTimestamps   = 7,
    RemoveTimestamps         = 8,
};

enum class BackgroundFillType : int
{
    Default     = 0,
    SolidColour = 1,
    Gradient    = 2,
};

enum class BackgroundImageType : int
{
    None        = 0,
    AlbumArt    = 1,
    CustomImage = 2,
};

namespace preferences
{
    namespace searching
    {
        uint64_t source_config_generation();
        std::vector<GUID> active_sources();
        std::vector<std::string> tags();
        bool exclude_trailing_brackets();

        std::string musixmatch_api_key();

        namespace raw
        {
            std::vector<GUID> active_sources_configured();
        }
    }

    namespace editing
    {
        std::vector<AutoEditType> automated_auto_edits();
    }

    namespace saving
    {
        AutoSaveStrategy autosave_strategy();
        GUID save_source();

        std::string filename(metadb_handle_ptr track, const metadb_v2_rec_t& track_info);

        std::string_view untimed_tag();
        std::string_view timestamped_tag();

        bool merge_equivalent_lrc_lines();

        namespace raw
        {
            SaveDirectoryClass directory_class();
        }
    }

    namespace display
    {
        t_ui_font font();
        std::optional<t_ui_color> foreground_colour();
        std::optional<t_ui_color> highlight_colour();

        LineScrollDirection scroll_direction();
        LineScrollType scroll_type();
        double scroll_time_seconds();

        double highlight_fade_seconds();
        int linegap();

        bool debug_logs_enabled();

        namespace raw
        {
            bool font_is_custom();
        }
    }

    namespace background
    {
        BackgroundFillType fill_type();
        BackgroundImageType image_type();

        t_ui_color colour();
        t_ui_color gradient_tl();
        t_ui_color gradient_tr();
        t_ui_color gradient_bl();
        t_ui_color gradient_br();

        bool maintain_img_aspect_ratio();
        double image_opacity();
        int blur_radius();
        std::string custom_image_path();
    }
}
