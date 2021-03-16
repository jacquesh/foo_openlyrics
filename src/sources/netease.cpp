#include "stdafx.h"

#include "cJSON.h"

#include "logging.h"
#include "lyric_data.h"
#include "lyric_source.h"

static const GUID src_guid = { 0xaac13215, 0xe32e, 0x4667, { 0xac, 0xd7, 0x1f, 0xd, 0xbd, 0x84, 0x27, 0xe4 } };

class NetEaseLyricsSource : public LyricSourceBase
{
    const GUID& id() const final { return src_guid; }
    const TCHAR* friendly_name() const final { return _T("NetEase Online Music"); }

    LyricDataRaw query(metadb_handle_ptr track, abort_callback& abort) final;
};
static const LyricSourceFactory<NetEaseLyricsSource> src_factory;

const char* BASE_URL = "https://music.163.com/api";

http_request::ptr make_post_request()
{
    http_request::ptr request = http_client::get()->create_request("POST");
    request->add_header("Referer", "https://music.163.com");
    request->add_header("Cookie", "appver=2.0.2");
    request->add_header("charset", "utf-8");
    request->add_header("Content-Type", "application/x-www-form-urlencoded");
    return request;
}

static pfc::string8 urlencode(const pfc::string8& input)
{
    size_t inlen = input.length();
    pfc::string8 result;
    result.prealloc(inlen * 3);

    for(size_t i=0; i<inlen; i++)
    {
        if(pfc::char_is_ascii_alphanumeric(input[i]) ||
            (input[i] == '-') ||
            (input[i] == '_') ||
            (input[i] == '.') ||
            (input[i] == '~'))
        {
            result.add_char(input[i]);
        }
        else if(input[i] == ' ')
        {
            result.add_char('+');
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

            result.add_char('%');
            result.add_char(nibble_to_hex(hi_nibble));
            result.add_char(nibble_to_hex(lo_nibble));
        }
    }

    return result;
}

const int MAX_TAG_EDIT_DISTANCE = 3; // Arbitrarily selected
int edit_distance(const char* strA, const char* strB)
{
    // 2-row levenshtein distance
    int row_count = static_cast<int>(strlen(strA));
    int row_len = static_cast<int>(strlen(strB));

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

static int64_t parse_song_id(cJSON* json, const pfc::string8& artist, const pfc::string8& album, const pfc::string8& title)
{
    if((json == nullptr) || (json->type != cJSON_Object))
    {
        LOG_INFO("Root object is null or not an object");
        return 0;
    }

    cJSON* result_obj = cJSON_GetObjectItem(json, "result");
    if((result_obj == nullptr) || (result_obj->type != cJSON_Object))
    {
        LOG_INFO("No valid 'result' property available");
        return 0;
    }
    cJSON* song_arr = cJSON_GetObjectItem(result_obj, "songs");
    if((song_arr == nullptr) || (song_arr->type != cJSON_Array))
    {
        LOG_INFO("No valid 'songs' property available");
        return 0;
    }

    int song_arr_len = cJSON_GetArraySize(song_arr);
    if(song_arr_len <= 0)
    {
        LOG_INFO("Songs array has no items available");
        return 0;
    }

    for(int song_index=0; song_index<song_arr_len; song_index++)
    {
        cJSON* song_item = cJSON_GetArrayItem(song_arr, song_index);
        if((song_item == nullptr) || (song_item->type != cJSON_Object))
        {
            LOG_INFO("Song array entry %d not available or invalid", song_index);
            continue;
        }

        cJSON* artist_list_item = cJSON_GetObjectItem(song_item, "artists");
        if((artist_list_item != nullptr) && (artist_list_item->type == cJSON_Array))
        {
            int artist_list_len = cJSON_GetArraySize(artist_list_item);
            if(artist_list_len > 0)
            {
                cJSON* artist_item = cJSON_GetArrayItem(artist_list_item, 0);
                if((artist_item != nullptr) && (artist_item->type == cJSON_Object))
                {
                    cJSON* artist_name = cJSON_GetObjectItem(artist_item, "name");
                    if((artist_name != nullptr) && (artist_name->type == cJSON_String))
                    {
                        int editdist = edit_distance(artist_name->valuestring, artist.c_str());
                        if(editdist > MAX_TAG_EDIT_DISTANCE)
                        {
                            LOG_INFO("Rejected NetEase search result %s/%s/%s for artist mismatch: %s",
                                    artist.c_str(),
                                    album.c_str(),
                                    title.c_str(),
                                    artist_name->valuestring);
                            continue;
                        }
                    }
                }
            }
        }

        cJSON* album_item = cJSON_GetObjectItem(song_item, "album");
        if((album_item != nullptr) && (album_item->type == cJSON_Object))
        {
            cJSON* album_title_item = cJSON_GetObjectItem(album_item, "name");
            if((album_title_item != nullptr) && (album_title_item->type == cJSON_String))
            {
                int editdist = edit_distance(album_title_item->valuestring, album.c_str());
                if(editdist > MAX_TAG_EDIT_DISTANCE)
                {
                    LOG_INFO("Rejected NetEase search result %s/%s/%s for album mismatch: %s",
                            artist.c_str(),
                            album.c_str(),
                            title.c_str(),
                            album_title_item->valuestring);
                    continue;
                }
            }
        }

        cJSON* title_item = cJSON_GetObjectItem(song_item, "name");
        if((title_item != nullptr) && (title_item->type == cJSON_String))
        {
            int editdist = edit_distance(title_item->valuestring, title.c_str());
            if(editdist > MAX_TAG_EDIT_DISTANCE)
            {
                LOG_INFO("Rejected NetEase search result %s/%s/%s for title mismatch: %s",
                        artist.c_str(),
                        album.c_str(),
                        title.c_str(),
                        title_item->valuestring);
                continue;
            }
        }

        cJSON* song_id_item = cJSON_GetObjectItem(song_item, "id");
        if((song_id_item == nullptr) || (song_id_item->type != cJSON_Number))
        {
            LOG_INFO("Song item ID field is not available or invalid");
            return 0;
        }

        return (int64_t)song_id_item->valuedouble;
    }

    return 0;
}

static int64_t get_song_id(const pfc::string8& artist, const pfc::string8& album, const pfc::string8& title, abort_callback& abort)
{
    pfc::string8 url = BASE_URL;
    url.add_string("/search/get?s=");
    url.add_string(urlencode(artist));
    url.add_string("+");
    url.add_string(urlencode(title));
    url.add_string("&type=1&offset=0&sub=false&limit=5");
    LOG_INFO("Querying for song ID from %s...", url.c_str());

    pfc::string8 content;
    try
    {
        http_request::ptr request = make_post_request();
        file_ptr response_file = request->run(url.c_str(), abort);

        response_file->read_string_raw(content, abort);
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to download netease page %s: %s", url.c_str(), e.what());
        return 0;
    }

    cJSON* json = cJSON_ParseWithLength(content.c_str(), content.get_length());
    int64_t song_id = parse_song_id(json, artist, album, title);
    cJSON_Delete(json);

    return song_id;
}

static LyricDataRaw get_song_lyrics(int64_t song_id, abort_callback& abort)
{
    if(song_id == 0)
    {
        return {};
    }

    LOG_INFO("Get NetEase lyrics for song id %d", song_id);
    char id_buffer[16];
    snprintf(id_buffer, sizeof(id_buffer), "%lld", song_id);

    pfc::string8 url = BASE_URL;
    url.add_string("/song/lyric?tv=-1&kv=-1&lv=-1&os=pc&id=");
    url.add_string(id_buffer);
    LOG_INFO("Querying for lyrics from %s...", url.c_str());

    pfc::string8 content;
    try
    {
        http_request::ptr request = make_post_request();
        file_ptr response_file = request->run(url.c_str(), abort);

        response_file->read_string_raw(content, abort);
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to download NetEase page %s: %s", url.c_str(), e.what());
        return {};
    }

    LyricDataRaw result = {};

    cJSON* json = cJSON_ParseWithLength(content.c_str(), content.get_length());
    if((json != nullptr) && (json->type == cJSON_Object))
    {
        cJSON* lrc_item = cJSON_GetObjectItem(json, "lrc");
        if((lrc_item != nullptr) && (lrc_item->type == cJSON_Object))
        {
            cJSON* lrc_lyric = cJSON_GetObjectItem(lrc_item, "lyric");
            if((lrc_lyric != nullptr) && (lrc_lyric->type == cJSON_String))
            {
                result.source_id = src_guid;
                result.format = LyricFormat::Timestamped;
                result.text = lrc_lyric->valuestring;
            }
        }
    }
    cJSON_Delete(json);

    return result;
}

LyricDataRaw NetEaseLyricsSource::query(metadb_handle_ptr track, abort_callback& abort)
{
    const char* artist = get_artist(track);
    const char* album = get_album(track);
    const char* title = get_title(track);
    int64_t song_id = get_song_id(artist, album, title, abort);
    return get_song_lyrics(song_id, abort);
}

