#pragma once

#include "stdafx.h"

#include "lyric_data.h"

namespace parsers
{

namespace lrc
{
    bool is_tag_line(std::string_view line);
    void set_offset_tag(LyricData& lyrics, double offset_seconds);
    void remove_offset_tag(LyricData& lyrics);

    double get_line_first_timestamp(std::string_view line);
    std::string print_timestamp(double timestamp);
    bool try_parse_timestamp(std::string_view tag, double& out_timestamp);

    LyricData parse(const LyricDataCommon& metadata, std::string_view text_utf8);

    std::tstring expand_text(const LyricData& data, bool merge_equivalent_lrc_lines);
} // namespace lrc

} // namespace parsers

