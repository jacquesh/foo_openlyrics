#include "stdafx.h"

#include "logging.h"
#include "lyric_auto_edit.h"
#include "lyric_metadata.h"
#include "metrics.h"
#include "mvtf/mvtf.h"
#include "parsers.h"

static std::optional<LyricData> ReplaceHtmlEscapedChars(const LyricData& lyrics)
{
    std::string text = from_tstring(parsers::lrc::expand_text(lyrics, false));
    std::pair<std::string_view, char> replacements[] = {
        { "&amp;", '&' }, { "&lt;", '<' }, { "&gt;", '>' }, { "&quot;", '"' }, { "&apos;", '\'' },
    };

    size_t replace_count = 0;
    for(auto [escaped, replacement] : replacements)
    {
        size_t current_index = 0;
        while(current_index < text.length())
        {
            size_t next_index = text.find(escaped, current_index);
            if(next_index == std::string::npos) break;

            text.replace(next_index, escaped.length(), 1, replacement);
            current_index = next_index + 1;
            replace_count++;
        }
    }
    LOG_INFO("Auto-edit replaced %u named HTML-encoded characters", replace_count);

    if(replace_count > 0)
    {
        return { parsers::lrc::parse(lyrics, text) };
    }
    else
    {
        return {};
    }
}

static std::optional<LyricData> RemoveRepeatedSpaces(const LyricData& lyrics)
{
    size_t spaces_erased = 0;
    LyricData new_lyrics = lyrics;
    for(LyricDataLine& line : new_lyrics.lines)
    {
        size_t search_start = 0;
        while(search_start < line.text.length())
        {
            size_t next_space = find_first_whitespace(line.text, search_start);

            // NOTE: If the line was empty we would not enter this loop.
            //       We subtract 1 from the length to avoid overflowing when next_space == npos == (size_t)-1
            assert(line.text.length() > 0);
            if(next_space > line.text.length() - 1)
            {
                break;
            }

            size_t erase_start = next_space + 1;
            size_t erase_end = find_first_nonwhitespace(line.text, erase_start);

            if((erase_end != std::tstring::npos) && (erase_end > erase_start))
            {
                size_t erase_count = erase_end - erase_start;
                line.text.erase(erase_start, erase_count);
                spaces_erased += erase_count;
            }
            search_start = next_space + 1;
        }
    }
    LOG_INFO("Auto-removal removed %u unnecessary spaces", spaces_erased);

    if(spaces_erased > 0)
    {
        return { std::move(new_lyrics) };
    }
    else
    {
        return {};
    }
}

static std::optional<LyricData> RemoveRepeatedBlankLines(const LyricData& lyrics)
{
    size_t lines_removed = 0;
    bool previous_blank = true;
    LyricData new_lyrics = lyrics;
    for(auto iter = new_lyrics.lines.begin(); iter != new_lyrics.lines.end(); /*Omitted*/)
    {
        size_t first_non_space = find_first_nonwhitespace(iter->text);
        bool is_blank = (first_non_space == std::tstring::npos);
        if(is_blank && previous_blank)
        {
            iter = new_lyrics.lines.erase(iter);
            lines_removed++;
        }
        else
        {
            iter++;
        }
        previous_blank = is_blank;
    }
    LOG_INFO("Auto-removal removed %u blank lines", lines_removed);

    if(lines_removed > 0)
    {
        return { std::move(new_lyrics) };
    }
    else
    {
        return {};
    }
}

static std::optional<LyricData> RemoveAllBlankLines(const LyricData& lyrics)
{
    LyricData new_lyrics = lyrics;
    auto line_is_empty = [](const LyricDataLine& line)
    {
        if(line.text.empty())
        {
            return true;
        }

        const auto is_not_whitespace = [](wchar_t c) { return std::iswspace(c) == 0; };
        const auto first_not_whitespace = std::find_if(line.text.begin(), line.text.end(), is_not_whitespace);
        return first_not_whitespace == line.text.end();
    };
    auto new_end = std::remove_if(new_lyrics.lines.begin(), new_lyrics.lines.end(), line_is_empty);
    ptrdiff_t lines_removed = std::distance(new_end, new_lyrics.lines.end());
    new_lyrics.lines.erase(new_end, new_lyrics.lines.end());
    LOG_INFO("Auto-removal removed %d blank lines", lines_removed);

    if(lines_removed > 0)
    {
        return { std::move(new_lyrics) };
    }
    else
    {
        return {};
    }
}

static std::optional<LyricData> ResetCapitalisation(const LyricData& lyrics)
{
    LyricData new_lyrics = lyrics;

    size_t edit_count = 0;
    for(LyricDataLine& line : new_lyrics.lines)
    {
        if(line.text.empty()) continue;

        bool edited = false;
        if(line.text[0] <= 255)
        {
            if(_istlower(line.text[0]))
            {
                line.text[0] = _totupper(static_cast<unsigned char>(line.text[0]));
                edited = true;
            }
        }

        for(size_t i = 1; i < line.text.length(); i++)
        {
            if(line.text[i] <= 255)
            {
                if(_istupper(line.text[i]))
                {
                    line.text[i] = _totlower(static_cast<unsigned char>(line.text[i]));
                    edited = true;
                }
            }
        }

        if(edited)
        {
            edit_count++;
        }
    }
    LOG_INFO("Auto-edit changed the capitalisation of %u lines", edit_count);

    if(edit_count > 0)
    {
        return { std::move(new_lyrics) };
    }
    else
    {
        return {};
    }
}

static std::optional<LyricData> FixMalformedTimestamps(const LyricData& lyrics)
{
    const auto fix_decimal_separator = [](std::string_view tag)
    {
        if((tag.length() < 10) || // Tag isn't long enough to contain a timestamp
           (tag[0] != '[') || // Tag doesn't start with a bracket
           (tag[tag.length() - 1] != ']')) // Tag doesn't end with a bracket
        {
            return false;
        }

        double unused;
        bool was_bad = !parsers::lrc::try_parse_timestamp(tag, unused);
        if(!was_bad)
        {
            return false;
        }

        size_t last_nondecimal_index = tag.find_last_not_of("1234567890.", tag.length() - 2);
        if(last_nondecimal_index == std::string_view::npos)
        {
            return false; // No non-decimal characters, this really shouldn't happen since we've checked tag[0]=='['
                          // above.
        }

        char replaced_char = tag[last_nondecimal_index];
        const_cast<char&>(tag[last_nondecimal_index]) = '.';
        bool is_fixed = parsers::lrc::try_parse_timestamp(tag, unused);
        if(is_fixed)
        {
            return true;
        }
        else
        {
            const_cast<char&>(tag[last_nondecimal_index]) = replaced_char;
            return false;
        }
    };

    std::string text = from_tstring(parsers::lrc::expand_text(lyrics, false));
    int change_count = 0;
    size_t current_index = text.find('[');
    while(current_index != std::string::npos)
    {
        bool changed = false;

        size_t end_index = text.find(']', current_index);
        if(end_index == std::string::npos)
        {
            break;
        }
        std::string_view tag { text.c_str() + current_index, end_index - current_index + 1 };
        changed |= fix_decimal_separator(tag);

        if(changed)
        {
            change_count++;
        }
        current_index = text.find('[', current_index + 1);
    }

    if(change_count > 0)
    {
        return { parsers::lrc::parse(lyrics, text) };
    }
    else
    {
        return {};
    }
}

static std::optional<LyricData> RemoveTimestamps(const LyricData& lyrics)
{
    if(!lyrics.IsTimestamped())
    {
        return {};
    }

    LyricData new_lyrics = lyrics;
    new_lyrics.RemoveTimestamps();
    return { new_lyrics };
}

static std::optional<LyricData> RemoveSurroundingWhitespace(const LyricData& lyrics)
{
    constexpr std::tstring_view trimset = _T("\t ");
    LyricData new_lyrics = lyrics;
    size_t edit_count = 0;
    for(LyricDataLine& line : new_lyrics.lines)
    {
        size_t line_start = 0;
        while(line_start < line.text.length())
        {
            const size_t exclusive_line_end = std::min(line.text.find('\n', line_start), line.text.length());
            if(exclusive_line_end == line_start)
            {
                // If there are 2 consecutive newlines then just skip past them
                line_start++;
                continue;
            }
            const size_t inclusive_line_end = exclusive_line_end - 1;

            const size_t trimmed_start = line.text.find_first_not_of(trimset, line_start);
            const size_t inclusive_trimmed_end = line.text.find_last_not_of(trimset, inclusive_line_end);
            if(inclusive_trimmed_end == std::tstring::npos)
            {
                // The entire line is whitespace
                line.text.erase(line_start);
            }
            else
            {
                line.text.erase(inclusive_trimmed_end + 1, exclusive_line_end - inclusive_trimmed_end - 1);
                if(trimmed_start < exclusive_line_end)
                {
                    line.text.erase(line_start, trimmed_start - line_start);
                }
            }
            if((line_start != trimmed_start) || (inclusive_line_end != inclusive_trimmed_end))
            {
                edit_count++;
            }

            line_start = line.text.find('\n', line_start);
            if(line_start != std::tstring::npos)
            {
                line_start++;
            }
        }
    }
    LOG_INFO("Auto-edit removed surrounding whitespace from %zu lyric lines", edit_count);

    if(edit_count > 0)
    {
        return { new_lyrics };
    }
    else
    {
        return {};
    }
}

std::optional<LyricData> auto_edit::RunAutoEdit(AutoEditType type,
                                                const LyricData& lyrics,
                                                const metadb_v2_rec_t& track_info)
{
    std::optional<LyricData> result;
    switch(type)
    {
        case AutoEditType::ReplaceHtmlEscapedChars: result = ReplaceHtmlEscapedChars(lyrics); break;
        case AutoEditType::RemoveRepeatedSpaces: result = RemoveRepeatedSpaces(lyrics); break;
        case AutoEditType::RemoveRepeatedBlankLines: result = RemoveRepeatedBlankLines(lyrics); break;
        case AutoEditType::RemoveAllBlankLines: result = RemoveAllBlankLines(lyrics); break;
        case AutoEditType::ResetCapitalisation: result = ResetCapitalisation(lyrics); break;
        case AutoEditType::FixMalformedTimestamps: result = FixMalformedTimestamps(lyrics); break;
        case AutoEditType::RemoveTimestamps: result = RemoveTimestamps(lyrics); break;
        case AutoEditType::RemoveSurroundingWhitespace: result = RemoveSurroundingWhitespace(lyrics); break;

        case AutoEditType::Unknown:
        default:
            LOG_ERROR("Unexpected auto-edit type: %d", int(type));
            assert(false);
            break;
    }

    metrics::log_used_auto_edit();
    if(result.has_value())
    {
        lyric_metadata_log_edit(track_info);
    }
    return result;
}

// ============
// Tests
// ============
#if MVTF_TESTS_ENABLED
MVTF_TEST(autoedit_fixmalformedtimestamps_corrects_decimal_separator_from_colon_to_dot)
{
    const std::string input = "[00:15:83]test";
    LyricData parsed = parsers::lrc::parse({}, input);
    std::optional<LyricData> fixed = FixMalformedTimestamps(parsed);
    ASSERT(fixed.has_value());

    std::tstring output = parsers::lrc::expand_text(fixed.value(), false);

    ASSERT(fixed.value().IsTimestamped());
    ASSERT(fixed.value().lines.size() == 1);
    ASSERT(fixed.value().lines[0].timestamp == 15.83);
    ASSERT(fixed.value().lines[0].text == _T("test"));
    ASSERT(output == _T("[00:15.83]test\r\n"));
}

MVTF_TEST(autoedit_removesurroundingwhitespace_leaves_already_trimmed_lyrics_unchanged)
{
    LyricData input = {};
    input.lines.push_back({ _T("This is an already-trimmed lyric line"), DBL_MAX });

    std::optional<LyricData> output = RemoveSurroundingWhitespace(input);

    ASSERT(!output.has_value());
}

MVTF_TEST(autoedit_removesurroundingwhitespace_leaves_already_trimmed_multiline_lyrics_unchanged)
{
    LyricData input = {};
    input.lines.push_back(
        { _T("This is an already-trimmed lyric line\nThat has had concurrent lines collapsed"), 1.0 });

    std::optional<LyricData> output = RemoveSurroundingWhitespace(input);

    ASSERT(!output.has_value());
}

MVTF_TEST(autoedit_removesurroundingwhitespace_trims_surrounding_whitespace)
{
    LyricData input = {};
    input.lines.push_back({ _T("This line has trailing whitespace   "), DBL_MAX });
    input.lines.push_back({ _T(" This line has leading whitespace"), DBL_MAX });
    input.lines.push_back({ _T("This line has no surrounding whitespace"), DBL_MAX });
    input.lines.push_back({ _T("  This line has both surrounding whitespaces "), DBL_MAX });

    std::optional<LyricData> output = RemoveSurroundingWhitespace(input);

    ASSERT(output.has_value());
    ASSERT(output->lines.size() == 4);
    ASSERT(output->lines[0].text == _T("This line has trailing whitespace"));
    ASSERT(output->lines[1].text == _T("This line has leading whitespace"));
    ASSERT(output->lines[2].text == _T("This line has no surrounding whitespace"));
    ASSERT(output->lines[3].text == _T("This line has both surrounding whitespaces"));
}

MVTF_TEST(autoedit_removesurroundingwhitespace_trims_lines_that_are_only_whitespace)
{
    LyricData input = {};
    input.lines.push_back({ _T("  "), DBL_MAX }); // This line is only whitespace
    input.lines.push_back({ _T("Next line is only whitespace\n  \nSee?"), DBL_MAX });

    std::optional<LyricData> output = RemoveSurroundingWhitespace(input);

    ASSERT(output.has_value());
    ASSERT(output->lines.size() == 2);
    ASSERT(output->lines[0].text == _T(""));
    ASSERT(output->lines[1].text == _T("Next line is only whitespace\n\nSee?"));
}

MVTF_TEST(autoedit_removesurroundingwhitespace_trims_surrounding_whitespace_from_collapsed_lines)
{
    LyricData input = {};
    input.lines.push_back({ _T("This line has trailing whitespace   \n This line has leading whitespace"), 1.0 });
    input.lines.push_back({ _T("One line followed by \n\n A whole empty line"), 2.0 });

    std::optional<LyricData> output = RemoveSurroundingWhitespace(input);

    ASSERT(output.has_value());
    ASSERT(output->lines.size() == 2);
    ASSERT(output->lines[0].text == _T("This line has trailing whitespace\nThis line has leading whitespace"));
    ASSERT(output->lines[1].text == _T("One line followed by\n\nA whole empty line"));
}
#endif
