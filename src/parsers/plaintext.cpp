#include "stdafx.h"

#include "lyric_data.h"
#include "winstr_util.h"

namespace parsers::plaintext
{

// TODO: Plaintext lyric file parsing
LyricData parse(const LyricDataRaw& input)
{
    LyricData result = {};
    result.format = input.format;
    result.file_title = input.file_title;
    result.text = input.text;

    if((input.format != LyricFormat::Plaintext) || input.file_title.is_empty() || input.text.is_empty())
    {
        // TODO: Log?
        return result;
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
        size_t line_length = line_end_index - line_start_index;

        TCHAR* line_text = nullptr;
        string_to_tchar(input.text, line_start_index, line_length, line_text);
        lines.push_back(line_text);
        line_lengths.push_back(line_length);

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
    result.line_count = lines.size();
    result.lines = new TCHAR*[result.line_count];
    result.line_lengths = new size_t[result.line_count];
    for(size_t i=0; i<result.line_count; i++)
    {
        result.lines[i] = lines[i];
        result.line_lengths[i] = line_lengths[i];
    }
    return result;
}

} // namespace parsers::plaintext

