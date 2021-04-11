#include "stdafx.h"

#include <algorithm>
#include <string_view>

#include "logging.h"
#include "lyric_data.h"
#include "winstr_util.h"

namespace parsers::lrc
{

static bool is_digit(char c)
{
    return ((c >= '0') && (c <= '9'));
}

static int str_to_int(std::string_view str)
{
    assert(str.length() <= 6); // Just to ensure no overflow, could actually be larger
    int result = 0;
    for(char c : str)
    {
        assert(is_digit(c));
        int char_val = (int)c - (int)'0';
        result = (result*10) + char_val;
    }
    return result;
}

bool is_tag_line(std::string_view line)
{
    if(line.size() <= 0) return false;
    if(line[0] != '[') return false;
    if(line[line.size()-1] != ']') return false;

    size_t colon_index = line.find(':');
    if(colon_index == std::string::npos) return false;
    assert(colon_index != 0); // We've already checked that the first char is '['

    size_t tag_length = colon_index - 1; // Tag lines have the form [tag:value] so we -1 to count from the 't' to the ':'
    if(tag_length == 0) return false;

    return true;
}

struct ParsedLineContents
{
    std::vector<double> timestamps;
    std::string line;
};

struct LineTagParseResult
{
    bool success;
    double timestamp;
    int charsConsumed;
};

void print_6digit_timestamp(double timestamp, char (&out_timestamp)[11])
{
    double total_seconds_flt = std::floor(timestamp);
    int total_seconds = static_cast<int>(total_seconds_flt);
    int time_minutes = total_seconds/60;
    int time_seconds = total_seconds - (time_minutes*60);
    int time_centisec = static_cast<int>((timestamp - total_seconds_flt) * 100.0);

    snprintf(out_timestamp, sizeof(out_timestamp), "[%02d:%02d.%02d]", time_minutes, time_seconds, time_centisec);
}

bool try_parse_6digit_timestamp(std::string_view tag, double& out_timestamp)
{
    if((tag.length() != 10) ||
       (tag[0] != '[') ||
       !is_digit(tag[1]) ||
       !is_digit(tag[2]) ||
       (tag[3] != ':') ||
       !is_digit(tag[4]) ||
       !is_digit(tag[5]) ||
       (tag[6] != '.') ||
       !is_digit(tag[7]) ||
       !is_digit(tag[8]) ||
       (tag[9] != ']'))
    {
        // We do not have a well-formed timestamp
        return false;
    }

    int minute = str_to_int(tag.substr(1, 2));
    int second = str_to_int(tag.substr(4,2));
    int centisec = str_to_int(tag.substr(7,2));
    double timestamp = (double)(minute*60) + (double)second + ((double)centisec * 0.01);
    out_timestamp = timestamp;
    return true;
}

bool try_parse_7digit_timestamp(std::string_view tag, double& out_timestamp)
{
    if((tag.length() != 11) ||
       (tag[0] != '[') ||
       !is_digit(tag[1]) ||
       !is_digit(tag[2]) ||
       (tag[3] != ':') ||
       !is_digit(tag[4]) ||
       !is_digit(tag[5]) ||
       (tag[6] != '.') ||
       !is_digit(tag[7]) ||
       !is_digit(tag[8]) ||
       !is_digit(tag[9]) ||
       (tag[10] != ']'))
    {
        // We do not have a well-formed timestamp
        return false;
    }

    int minute = str_to_int(tag.substr(1, 2));
    int second = str_to_int(tag.substr(4,2));
    int millisec = str_to_int(tag.substr(7,3));
    double timestamp = (double)(minute*60) + (double)second + ((double)millisec * 0.001);
    out_timestamp = timestamp;
    return true;
}

static LineTagParseResult parse_tag_from_line(std::string_view line)
{
    int line_length = static_cast<int>(line.length());
    int index = 0;
    while(index < line_length)
    {
        if(line[index] != '[') break;

        int close_index = index;
        while((close_index < line_length) && (line[close_index] != ']')) close_index++;

        const int min_tag_length = 10; // [00:00.00]
        const int max_tag_length = 11; // [00:00.000]
        int tag_length = close_index - index + 1;

        std::string_view tag = line.substr(index, tag_length);

        double timestamp = -1.0;
        if(try_parse_6digit_timestamp(tag, timestamp) ||
           try_parse_7digit_timestamp(tag, timestamp))
        {
            return {true, timestamp, index + tag_length};
        }
        else
        {
            // If we find something that is not a well-formed timestamp then stop and just 
            break;
        }
    }

    // NOTE: It is important that we return `index` here so that we move forwards correctly
    //       in the calling parser function. In particular when it fails we need to extract
    //       the non-tag string correctly and without `index` here we'll include the last
    //       tag (if any) in that string (which is wrong, since we want to ignore metadata
    //       tags such as title and artist).
    return {false, 0.0, index};
}

static ParsedLineContents parse_line_times(std::string_view line)
{
    std::vector<double> result;
    size_t index = 0;
    while(index <= line.size())
    {
        LineTagParseResult parse_result = parse_tag_from_line(line.substr(index));
        index += parse_result.charsConsumed;

        if(parse_result.success)
        {
            result.push_back(parse_result.timestamp);
        }
        else
        {
            break;
        }
    }

    return {result, std::string(line.substr(index).data(), line.size()-index)};
}

LyricData parse(const LyricDataRaw& input)
{
    LOG_INFO("Parsing LRC lyric text...");
    if(input.text.empty())
    {
        LyricData result;
        result.source_id = input.source_id;
        return result;
    }

    struct LineData
    {
        std::string text;
        double timestamp;
    };
    std::vector<LineData> lines;
    std::vector<std::string> tags;
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

        if(line_bytes >= 3)
        {
            // NOTE: We're consuming UTF-8 text here and sometimes files contain byte-order marks.
            //       We don't want to process them so just skip past them. Ordinarily we'd do this
            //       just once at the start of the file but I've seen files with BOMs at the start
            //       of random lines in the file, so just check every line.
            if((input.text[line_start_index] == '\u00EF') &&
               (input.text[line_start_index+1] == '\u00BB') &&
               (input.text[line_start_index+2] == '\u00BF'))
            {
                line_start_index += 3;
                line_bytes -= 3;
            }
        }

        std::string_view line_view {input.text.c_str() + line_start_index, line_bytes};
        ParsedLineContents parse_output = parse_line_times(line_view);
        if(parse_output.timestamps.size() > 0)
        {
            for(double timestamp : parse_output.timestamps)
            {
                lines.push_back({parse_output.line, timestamp});
            }
        }
        else
        {
            // We don't have a timestamp, but rather than failing to parse the entire file,
            // we just keep the line around as "not having a timestamp". We represent this
            // as a line with a timestamp that is way out of the actual length of the track.
            // That way the line will never be highlighted and it neatly slots into the rest
            // of the system without special handling.
            // NOTE: It is important however, to note that this means we need to stable_sort
            //       below, to preserve the ordering of the "untimed" lines
            if(is_tag_line(line_view))
            {
                tags.emplace_back(line_view);
            }
            else
            {
                lines.push_back({std::string(input.text.c_str() + line_start_index, line_bytes), DBL_MAX});
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

    std::stable_sort(lines.begin(), lines.end(), [](const LineData& a, const LineData& b)
    {
        return a.timestamp < b.timestamp;
    });

    LyricData result = {};
    result.source_id = input.source_id;
    result.persistent_storage_path = input.persistent_storage_path;
    result.text = input.text;
    result.tags = std::move(tags);
    result.lines.reserve(lines.size());
    for(const auto& line : lines)
    {
        TCHAR* line_text = nullptr;
        size_t line_length = string_to_tchar(line.text, line_text);
        assert(line_length > 0); // The given length includes a null-terminator, we don't want to count the null terminator here

        result.lines.push_back({line_text, line_length-1, line.timestamp});
    }
    return result;
}

std::string expand_text(const LyricData& data)
{
    LOG_INFO("Expanding lyric text...");
    std::string expanded_text;
    expanded_text.reserve(data.lines.size() * 64); // NOTE: 64 is an arbitrary "probably longer than most lines" value
    for(const std::string& tag : data.tags)
    {
        expanded_text += tag;
        expanded_text += "\r\n";
    }
    expanded_text += "\r\n";
    for(const LyricDataLine& line : data.lines)
    {
        if(line.timestamp != DBL_MAX)
        {
            char timestamp[11];
            print_6digit_timestamp(line.timestamp, timestamp);
            expanded_text += timestamp;
        }

        if(line.text_length == 0)
        {
            expanded_text += " ";
        }
        else
        {
            expanded_text += tchar_to_string(line.text);
        }
        expanded_text += "\r\n";
    }

    return expanded_text;
}

std::string shrink_text(const LyricData& data)
{
    LOG_INFO("Shrinking lyric text...");
    std::string shrunk_text;
    shrunk_text.reserve(data.lines.size() * 64); // NOTE: 64 is an arbitrary "probably longer than most lines" value

    if(!data.tags.empty())
    {
        for(const std::string& tag : data.tags)
        {
            shrunk_text += tag;
            shrunk_text += "\r\n";
        }
        shrunk_text += "\r\n";
    }

    std::vector<std::pair<std::string, std::vector<double>>> timestamp_map;
    for(const LyricDataLine& line : data.lines)
    {
        if(line.timestamp == DBL_MAX) continue;

        std::string linestr = tchar_to_string(line.text);
        auto iter = std::find_if(timestamp_map.begin(),
                                 timestamp_map.end(),
                                 [&linestr](const auto& entry) { return entry.first == linestr; });
        if(iter == timestamp_map.end())
        {
            std::string_view line_to_insert("");
            if(linestr != " ")
            {
                line_to_insert = std::string_view(linestr);
            }
            timestamp_map.emplace_back(line_to_insert, std::vector<double>{line.timestamp});
        }
        else
        {
            iter->second.push_back(line.timestamp);
        }
    }

    for(const auto& [line, times] : timestamp_map)
    {
        char timestamp_str[11];
        for(double time : times)
        {
            print_6digit_timestamp(time, timestamp_str);
            shrunk_text += timestamp_str;
        }
        shrunk_text += line;
        shrunk_text += "\r\n";
    }
    for(const LyricDataLine& line : data.lines)
    {
        if(line.timestamp != DBL_MAX) continue;

        bool was_expanded = ((line.text[0] == ' ') && (line.text[1] == '\0'));
        if(!was_expanded)
        {
            shrunk_text += tchar_to_string(line.text);
        }
        shrunk_text += "\r\n";
    }

    return shrunk_text;
}

} // namespace parsers::lrc
