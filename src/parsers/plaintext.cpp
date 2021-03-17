#include "stdafx.h"

#include "logging.h"
#include "lyric_data.h"
#include "winstr_util.h"

namespace parsers::plaintext
{

LyricData parse(const LyricDataRaw& input)
{
    if((input.format != LyricFormat::Plaintext) || input.text.empty())
    {
        LOG_WARN("Cannot parse given raw lyrics as plaintext");
        return {};
    }

    std::vector<TCHAR*> lines;
    std::vector<size_t> line_lengths;
    const size_t text_length = input.text.length();
    size_t line_start_index = 0;
    while (line_start_index < text_length)
    {
        size_t line_end_index = line_start_index;
        while ((line_end_index <= text_length) &&
            (input.text[line_end_index] != '\0') &&
            (input.text[line_end_index] != '\n') &&
            (input.text[line_end_index] != '\r'))
        {
            line_end_index++;
        }
        size_t line_bytes = line_end_index - line_start_index;

        TCHAR* line_text = nullptr;
        size_t line_length = string_to_tchar(input.text.c_str(), line_start_index, line_bytes, line_text);
        assert(line_length > 0); // The given length includes a null-terminator
        lines.push_back(line_text);
        line_lengths.push_back(line_length-1); // We don't want to count the null-terminator here

        if ((line_end_index + 1 < input.text.length()) &&
            (input.text[line_end_index] == '\r') &&
            (input.text[line_end_index + 1] == '\n'))
        {
            line_start_index = line_end_index + 2;
        }
        else
        {
            line_start_index = line_end_index + 1;
        }
    }
    assert(lines.size() == line_lengths.size());

    LyricData result = {};
    result.source_id = input.source_id;
    result.format = input.format;
    result.text = input.text;

    result.lines.reserve(lines.size());
    for(size_t i=0; i<lines.size(); i++)
    {
        result.lines.push_back({lines[i], line_lengths[i], DBL_MAX});
    }
    return result;
}

} // namespace parsers::plaintext

