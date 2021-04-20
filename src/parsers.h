#pragma once

#include "stdafx.h"

#include "lyric_data.h"

namespace parsers
{

namespace lrc
{
    bool is_tag_line(std::string_view line);
    std::string print_6digit_timestamp(double timestamp);
    bool try_parse_6digit_timestamp(std::string_view tag, double& out_timestamp);
    bool try_parse_7digit_timestamp(std::string_view tag, double& out_timestamp);

    LyricData parse(const LyricDataRaw& input);

    std::tstring expand_text(const LyricData& data);
    std::string shrink_text(const LyricData& data);
} // namespace lrc

} // namespace parsers

