#include "stdafx.h"

#include <algorithm>

#include "logging.h"
#include "lyric_data.h"
#include "winstr_util.h"

namespace parsers::lrc
{

static bool is_digit(char c)
{
    return ((c >= '0') && (c <= '9'));
}

static int char_pair_to_int(const char* first_char)
{
    assert(is_digit(first_char[0]) && is_digit(first_char[1]));
    int char1_val = (int)first_char[0] - (int)'0';
    int char2_val = (int)first_char[1] - (int)'0';
    return char1_val*10 + char2_val;
}

static std::vector<double> parse_line_times(const pfc::string8& line, size_t start_index, size_t length)
{
    const size_t timestamp_length = 10;
    std::vector<double> result;
    if(length < timestamp_length)
    {
        return result;
    }

    size_t index = 0;
    // Well-formed timestamps have a fixed length, so we don't need to iterate right to the end of the string
    while(index <= length - timestamp_length)
    {
        if(line[start_index + index] == '[')
        {
            size_t close_index = index;
            while((close_index < length) && (line[start_index + close_index] != ']')) close_index++;
            if(close_index+1 - index != timestamp_length) // We +1 because we stop counting when we find the ], not after it
            {
                // There is a pair of square-brackets but they are not the correct distance apart to bound a well-formed timestamp
                break;
            }

            if((line[start_index + index + 0] != '[') ||
               !is_digit(line[start_index + index + 1]) ||
               !is_digit(line[start_index + index + 2]) ||
               (line[start_index + index + 3] != ':') ||
               !is_digit(line[start_index + index + 4]) ||
               !is_digit(line[start_index + index + 5]) ||
               (line[start_index + index + 6] != '.') ||
               !is_digit(line[start_index + index + 7]) ||
               !is_digit(line[start_index + index + 8]) ||
               (line[start_index + index + 9] != ']'))
            {
                // We do not have a well-formed timestamp
                break;
            }

            int minute = char_pair_to_int(&line[start_index + index + 1]);
            int second = char_pair_to_int(&line[start_index + index + 4]);
            int centisec = char_pair_to_int(&line[start_index + index + 7]);
            double timestamp = (double)(minute*60) + (double)second + ((double)centisec * 0.01);
            result.push_back(timestamp);

            index += timestamp_length;
        }
        else
        {
            index++;
        }
    }

    return result;
}

LyricData parse(const LyricDataRaw& input)
{
    if((input.format != LyricFormat::Timestamped) || input.text.is_empty())
    {
        LOG_WARN("Cannot parse given raw lyrics as LRC");
        return {};
    }

    struct LineData
    {
        TCHAR* text;
        size_t length;
        double timestamp;
    };
    std::vector<LineData> lines;
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

        if(line_bytes > 0)
        {
            const size_t timestamp_text_length = strlen("[00.00.00]");
            std::vector<double> line_times = parse_line_times(input.text, line_start_index, line_bytes);

            const size_t line_timestamp_bytes = timestamp_text_length*line_times.size();
            assert(line_bytes >= line_timestamp_bytes);
            size_t lyric_line_bytes = line_bytes - line_timestamp_bytes;
            for(double timestamp : line_times)
            {
                TCHAR* line_text = nullptr;
                size_t line_length = string_to_tchar(input.text, line_start_index + line_timestamp_bytes, lyric_line_bytes, line_text);
                lines.push_back({line_text, line_length, timestamp});
            }
        }

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

    std::sort(lines.begin(), lines.end(), [](const LineData& a, const LineData& b)
    {
        return a.timestamp < b.timestamp;
    });

    LyricData result = {};
    result.source_id = input.source_id;
    result.format = input.format;
    result.text = input.text;
    result.line_count = lines.size();
    result.lines = new TCHAR*[result.line_count];
    result.line_lengths = new size_t[result.line_count];
    result.timestamps = new double[result.line_count];
    for(size_t i=0; i<result.line_count; i++)
    {
        result.lines[i] = lines[i].text;
        result.line_lengths[i] = lines[i].length;
        result.timestamps[i] = lines[i].timestamp;
    }
    return result;
}

} // namespace parsers::lrc
