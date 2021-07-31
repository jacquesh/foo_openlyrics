#pragma once

#include "stdafx.h"

#include "lyric_io.h"
#include "preferences.h"

namespace auto_edit
{
    std::optional<LyricData> RunAutoEdit(AutoEditType type, const LyricData& lyrics);

    std::optional<LyricData> CreateInstrumental(const LyricData& lyrics);
    std::optional<LyricData> ReplaceHtmlEscapedChars(const LyricData& lyrics);
    std::optional<LyricData> RemoveRepeatedSpaces(const LyricData& lyrics);
    std::optional<LyricData> RemoveRepeatedBlankLines(const LyricData& lyrics);
    std::optional<LyricData> RemoveAllBlankLines(const LyricData& lyrics);
    std::optional<LyricData> ResetCapitalisation(const LyricData& lyrics);
    std::optional<LyricData> FixMalformedTimestamps(const LyricData& lyrics);
}
