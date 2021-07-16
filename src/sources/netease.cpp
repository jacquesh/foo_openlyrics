#include "stdafx.h"

#include "cJSON.h"

#include "logging.h"
#include "lyric_data.h"
#include "lyric_source.h"

static const GUID src_guid = { 0xaac13215, 0xe32e, 0x4667, { 0xac, 0xd7, 0x1f, 0xd, 0xbd, 0x84, 0x27, 0xe4 } };

class NetEaseLyricsSource : public LyricSourceRemote
{
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("NetEase Online Music"); }

    LyricDataRaw query(metadb_handle_ptr track, abort_callback& abort) final;

private:
    LyricDataRaw get_song_lyrics(int64_t song_id, abort_callback& abort) const;
    int64_t parse_song_id(cJSON* json, const std::string_view artist, const std::string_view album, const std::string_view title);
    int64_t get_song_id(const std::string_view artist, const std::string_view album, const std::string_view title, abort_callback& abort);
};
static const LyricSourceFactory<NetEaseLyricsSource> src_factory;

static const char* BASE_URL = "https://music.163.com/api";

static http_request::ptr make_post_request()
{
    http_request::ptr request = http_client::get()->create_request("POST");
    request->add_header("Referer", "https://music.163.com");
    request->add_header("Cookie", "appver=2.0.2");
    request->add_header("charset", "utf-8");
    request->add_header("Content-Type", "application/x-www-form-urlencoded");
    return request;
}


int64_t NetEaseLyricsSource::parse_song_id(cJSON* json, const std::string_view artist, const std::string_view album, const std::string_view title)
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
                        if(!tag_values_match(artist_name->valuestring, artist))
                        {
                            LOG_INFO("Rejected NetEase search result %s/%s/%s for artist mismatch: %s",
                                    artist.data(),
                                    album.data(),
                                    title.data(),
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
                if(!tag_values_match(album_title_item->valuestring, album))
                {
                    LOG_INFO("Rejected NetEase search result %s/%s/%s for album mismatch: %s",
                            artist.data(),
                            album.data(),
                            title.data(),
                            album_title_item->valuestring);
                    continue;
                }
            }
        }

        cJSON* title_item = cJSON_GetObjectItem(song_item, "name");
        if((title_item != nullptr) && (title_item->type == cJSON_String))
        {
            if(!tag_values_match(title_item->valuestring, title))
            {
                LOG_INFO("Rejected NetEase search result %s/%s/%s for title mismatch: %s",
                        artist.data(),
                        album.data(),
                        title.data(),
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

int64_t NetEaseLyricsSource::get_song_id(const std::string_view artist, const std::string_view album, const std::string_view title, abort_callback& abort)
{
    std::string url = std::string(BASE_URL) + "/search/get?s=" + urlencode(artist) + '+' + urlencode(title) + "&type=1&offset=0&sub=false&limit=5";
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

LyricDataRaw NetEaseLyricsSource::get_song_lyrics(int64_t song_id, abort_callback& abort) const
{
    LyricDataRaw result = {};
    result.source_id = src_guid;

    if(song_id == 0)
    {
        return result;
    }

    LOG_INFO("Get NetEase lyrics for song id %d", song_id);
    char id_buffer[16];
    snprintf(id_buffer, sizeof(id_buffer), "%lld", song_id);

    std::string url = std::string(BASE_URL) + "/song/lyric?tv=-1&kv=-1&lv=-1&os=pc&id=" + id_buffer;
    result.persistent_storage_path = url;
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
        return result;
    }


    cJSON* json = cJSON_ParseWithLength(content.c_str(), content.get_length());
    if((json != nullptr) && (json->type == cJSON_Object))
    {
        cJSON* lrc_item = cJSON_GetObjectItem(json, "lrc");
        if((lrc_item != nullptr) && (lrc_item->type == cJSON_Object))
        {
            cJSON* lrc_lyric = cJSON_GetObjectItem(lrc_item, "lyric");
            if((lrc_lyric != nullptr) && (lrc_lyric->type == cJSON_String))
            {
                result.text = trim_surrounding_whitespace(lrc_lyric->valuestring);
            }
        }
    }
    cJSON_Delete(json);

    return result;
}

LyricDataRaw NetEaseLyricsSource::query(metadb_handle_ptr track, abort_callback& abort)
{
    std::string_view artist = get_artist(track);
    std::string_view album = get_album(track);
    std::string_view title = get_title(track);
    int64_t song_id = get_song_id(artist, album, title, abort);
    return get_song_lyrics(song_id, abort);
}

