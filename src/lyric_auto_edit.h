#pragma once

#include <optional>

#include "lyric_data.h"
#include "preferences.h"

namespace auto_edit
{
    std::optional<LyricData> RunAutoEdit(AutoEditType type, const LyricData& lyrics, metadb_handle_ptr track);
}
