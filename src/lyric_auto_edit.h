#pragma once

#include "stdafx.h"

#include "lyric_io.h"

namespace auto_edit
{
    std::optional<LyricData> CreateInstrumental(const LyricData& lyrics);
    std::optional<LyricData> ReplaceHtmlEscapedChars(const LyricData& lyrics);
    std::optional<LyricData> RemoveRepeatedSpaces(const LyricData& lyrics);
    std::optional<LyricData> RemoveRepeatedBlankLines(const LyricData& lyrics);
    std::optional<LyricData> RemoveAllBlankLines(const LyricData& lyrics);
    std::optional<LyricData> ResetCapitalisation(const LyricData& lyrics);
}
