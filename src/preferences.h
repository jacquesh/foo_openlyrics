#pragma once

enum class SaveMethod
{
    None            = 0,
    ConfigDirectory = 1,
    Id3Tag          = 2
};

namespace preferences
{
    bool get_autosave_enabled();
    SaveMethod get_save_method();

    int get_render_linegap();
    const char* get_filename_format();
}
