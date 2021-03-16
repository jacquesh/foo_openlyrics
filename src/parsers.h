#pragma once

#include "stdafx.h"

#include "lyric_data.h"

namespace parsers
{

namespace plaintext { LyricData parse(const LyricDataRaw& input); }
namespace lrc
{
    LyricData parse(const LyricDataRaw& input);

    std::string expand_text(const LyricData& data);
    std::string shrink_text(const LyricData& data);
} // namespace lrc

} // namespace parsers

