#include "stdafx.h"

#include "preferences.h"
#include "tag_util.h"

std::string_view trim_surrounding_whitespace(std::string_view str)
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

std::string_view trim_trailing_text_in_brackets(std::string_view str)
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
            // TODO: Make this comparison case-insensitive
            if(strA[row] == strB[i])
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

bool tag_values_match(std::string_view tagA, std::string_view tagB)
{
    if(preferences::searching::exclude_trailing_brackets())
    {
        tagA = trim_surrounding_whitespace(trim_trailing_text_in_brackets(tagA));
        tagB = trim_surrounding_whitespace(trim_trailing_text_in_brackets(tagB));
    }

    const int MAX_TAG_EDIT_DISTANCE = 3; // Arbitrarily selected
    return (compute_edit_distance(tagA, tagB) <= MAX_TAG_EDIT_DISTANCE);
}

std::string track_metadata(metadb_handle_ptr track, std::string_view key)
{
    const metadb_info_container::ptr& track_info_container = track->get_info_ref();
    const file_info& track_info = track_info_container->info();

    size_t value_index = track_info.meta_find_ex(key.data(), key.length());
    if(value_index == pfc::infinite_size)
    {
        return "";
    }

    std::string result;
    size_t value_count = track_info.meta_enum_value_count(value_index);
    for(size_t i=0; i<value_count; i++)
    {
        const char* value = track_info.meta_enum_value(value_index, i);
        result += value;
    }
    return result;
}

