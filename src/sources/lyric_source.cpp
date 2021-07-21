#include "stdafx.h"

#include "logging.h"
#include "lyric_source.h"

static std::vector<LyricSourceBase*> g_lyric_sources;

LyricSourceBase* LyricSourceBase::get(GUID id)
{
    for(LyricSourceBase* src : g_lyric_sources)
    {
        if(src->id() == id)
        {
            return src;
        }
    }

    return nullptr;
}

std::vector<GUID> LyricSourceBase::get_all_ids()
{
    std::vector<GUID> result;
    result.reserve(g_lyric_sources.size());
    for(LyricSourceBase* src : g_lyric_sources)
    {
        result.push_back(src->id());
    }
    return result;
}

void LyricSourceBase::on_init()
{
    g_lyric_sources.push_back(this);
}

std::string LyricSourceBase::urlencode(std::string_view input)
{
    size_t inlen = input.length();
    std::string result;
    result.reserve(inlen * 3);

    for(size_t i=0; i<inlen; i++)
    {
        if(pfc::char_is_ascii_alphanumeric(input[i]) ||
            (input[i] == '-') ||
            (input[i] == '_') ||
            (input[i] == '.') ||
            (input[i] == '~'))
        {
            result += input[i];
        }
        else if(input[i] == ' ')
        {
            result += "%20";
        }
        else
        {
            const auto nibble_to_hex = [](char c)
            {
                static char hex[] = "0123456789ABCDEF";
                return hex[c & 0xF];
            };

            char hi_nibble = ((input[i] >> 4) & 0xF);
            char lo_nibble = ((input[i] >> 0) & 0xF);

            result += '%';
            result += nibble_to_hex(hi_nibble);
            result += nibble_to_hex(lo_nibble);
        }
    }

    return result;
}

std::string_view LyricSourceBase::trim_surrounding_whitespace(std::string_view str)
{
    size_t first_non_whitespace = str.find_first_not_of("\r\n ");
    size_t last_non_whitespace = str.find_last_not_of("\r\n ");

    if(first_non_whitespace == std::string_view::npos)
    {
        return "";
    }
    size_t len = (last_non_whitespace+1) - first_non_whitespace;
    return str.substr(first_non_whitespace, len);
}

std::string_view LyricSourceBase::trim_trailing_text_in_brackets(std::string_view str)
{
    std::string_view result = str;
    while(true)
    {
        size_t open_index = result.find_last_of("([{");
        if(open_index == std::string_view::npos)
        {
            break; // Nothing to trim
        }

        if(open_index == 0)
        {
            break; // Don't trim the entire string!
        }

        char opener = result[open_index];
        char closer = '\0';
        switch(opener)
        {
            case '[': closer = ']'; break;
            case '(': closer = ')'; break;
            case '{': closer = '}'; break;
        }
        assert(closer != '\0');

        size_t close_index = result.find_first_of(closer, open_index);
        if(close_index == std::string_view::npos)
        {
            break; // Unmatched open-bracket
        }

        result = result.substr(0, open_index);
    }

    return result;
}

static int compute_edit_distance(const std::string_view strA, const std::string_view strB)
{
    // 2-row levenshtein distance
    int row_count = static_cast<int>(strA.length());
    int row_len = static_cast<int>(strB.length());

    int* prev_row = new int[row_len+1];
    int* cur_row = new int[row_len+1];
    for(int i=0; i<row_len+1; i++)
    {
        prev_row[i] = i;
    }

    for(int row=0; row<row_count; row++)
    {
        cur_row[0] = row + 1;
        for(int i=0; i<row_len; i++)
        {
            int delete_cost = prev_row[i+1] + 1;
            int insert_cost = cur_row[i] + 1;
            int subst_cost;
            if(strA[row] == strB[i]) // TODO: Make this comparison case-insensitive
            {
                subst_cost = prev_row[i];
            }
            else
            {
                subst_cost = prev_row[i] + 1;
            }

            cur_row[i+1] = min(min(delete_cost, insert_cost), subst_cost);
        }

        int* tmp = cur_row;
        cur_row = prev_row;
        prev_row = tmp;
    }

    int result = prev_row[row_len];
    delete[] prev_row;
    delete[] cur_row;
    return result;
}

bool LyricSourceBase::tag_values_match(std::string_view tagA, std::string_view tagB)
{
    if(preferences::searching::exclude_trailing_brackets())
    {
        tagA = trim_surrounding_whitespace(trim_trailing_text_in_brackets(tagA));
        tagB = trim_surrounding_whitespace(trim_trailing_text_in_brackets(tagB));
    }

    const int MAX_TAG_EDIT_DISTANCE = 3; // Arbitrarily selected
    return (compute_edit_distance(tagA, tagB) <= MAX_TAG_EDIT_DISTANCE);
}

bool LyricSourceRemote::is_local() const
{
    return false;
}


LyricDataRaw LyricSourceRemote::query(metadb_handle_ptr track, abort_callback& abort)
{
    const auto get_metadata = [](metadb_handle_ptr track, const char* tag)
    {
        const metadb_info_container::ptr& track_info_container = track->get_info_ref();
        const file_info& track_info = track_info_container->info();

        size_t meta_index = track_info.meta_find(tag);
        if((meta_index != pfc::infinite_size) && (track_info.meta_enum_value_count(meta_index) > 0))
        {
            std::string_view result = track_info.meta_enum_value(meta_index, 0);
            return result;
        }

        return std::string_view{};
    };

    std::string_view artist = get_metadata(track, "artist");
    std::string_view album = get_metadata(track, "album");
    std::string_view title = get_metadata(track, "title");
    return query(artist, album, title, abort);
}

std::string LyricSourceRemote::save(metadb_handle_ptr /*track*/, bool /*is_timestamped*/, std::string_view /*lyrics*/, bool /*allow_ovewrite*/, abort_callback& /*abort*/)
{
    LOG_WARN("Cannot save lyrics to a remote source");
    assert(false);
    return "";
}
