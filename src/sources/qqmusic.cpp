#include "stdafx.h"

#include "cJSON.h"

#include "logging.h"
#include "lyric_data.h"
#include "lyric_source.h"

static const GUID src_guid = { 0x4b0b5722, 0x3a84, 0x4b8e, { 0x82, 0x7a, 0x26, 0xb9, 0xea, 0xb3, 0xb4, 0xe8 } };

class QQMusicLyricsSource : public LyricSourceRemote
{
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("QQ Music"); }

    LyricDataRaw query(metadb_handle_ptr track, abort_callback& abort) final;

private:
    LyricDataRaw get_song_lyrics(std::string song_id, abort_callback& abort) const;
    std::string parse_song_id(cJSON* json, const std::string_view artist, const std::string_view album, const std::string_view title) const;
    std::string get_song_id(const std::string_view artist, const std::string_view album, const std::string_view title, abort_callback& abort) const;
};
static const LyricSourceFactory<QQMusicLyricsSource> src_factory;

static http_request::ptr make_get_request()
{
    http_request::ptr request = http_client::get()->create_request("GET");
    request->add_header("Referer", "http://y.qq.com/portal/player.html");
    return request;
}

std::string QQMusicLyricsSource::parse_song_id(cJSON* json, const std::string_view artist, const std::string_view album, const std::string_view title) const
{
    const int MAX_TAG_EDIT_DISTANCE = 3; // Arbitrarily selected

    if((json == nullptr) || (json->type != cJSON_Object))
    {
        LOG_INFO("Root object is null or not an object");
        return "";
    }

    cJSON* result_obj = cJSON_GetObjectItem(json, "data");
    if((result_obj == nullptr) || (result_obj->type != cJSON_Object))
    {
        LOG_INFO("No valid 'data' property available");
        return ""; 
    }
    cJSON* song_obj = cJSON_GetObjectItem(result_obj, "song");
    if((song_obj == nullptr) || (song_obj->type != cJSON_Object))
    {
        LOG_INFO("No valid 'song' property available");
        return "";
    }

    cJSON* song_arr = cJSON_GetObjectItem(song_obj, "list");
    if((song_arr == nullptr) || (song_arr->type != cJSON_Array))
    {
        LOG_INFO("No valid 'list' property available");
        return "";
    }

    int song_arr_len = cJSON_GetArraySize(song_arr);
    if(song_arr_len <= 0)
    {
        LOG_INFO("Songs array has no items available");
        return "";
    }

    for(int song_index=0; song_index<song_arr_len; song_index++)
    {
        cJSON* song_item = cJSON_GetArrayItem(song_arr, song_index);
        if((song_item == nullptr) || (song_item->type != cJSON_Object))
        {
            LOG_INFO("Song array entry %d not available or invalid", song_index);
            continue;
        }

        cJSON* artist_list_item = cJSON_GetObjectItem(song_item, "singer");
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
                        int editdist = compute_edit_distance(artist_name->valuestring, artist);
                        if(editdist > MAX_TAG_EDIT_DISTANCE)
                        {
                            LOG_INFO("Rejected QQMusic search result %s/%s/%s for artist mismatch: %s",
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
                int editdist = compute_edit_distance(album_title_item->valuestring, album);
                if(editdist > MAX_TAG_EDIT_DISTANCE)
                {
                    LOG_INFO("Rejected QQMusic search result %s/%s/%s for album mismatch: %s",
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
            int editdist = compute_edit_distance(title_item->valuestring, title);
            if(editdist > MAX_TAG_EDIT_DISTANCE)
            {
                LOG_INFO("Rejected QQMusic search result %s/%s/%s for title mismatch: %s",
                        artist.data(),
                        album.data(),
                        title.data(),
                        title_item->valuestring);
                continue;
            }
        }

        cJSON* song_id_item = cJSON_GetObjectItem(song_item, "mid");
        if((song_id_item == nullptr) || (song_id_item->type != cJSON_String))
        {
            LOG_INFO("Song item ID field is not available or invalid");
            return "";
        }

        return song_id_item->valuestring;
    }

    return "";
}

std::string QQMusicLyricsSource::get_song_id(const std::string_view artist, const std::string_view album, const std::string_view title, abort_callback& abort) const
{
    std::string url = "http://c.y.qq.com/soso/fcgi-bin/client_search_cp?p=1&n=30&new_json=1&cr=1&format=json&inCharset=utf-8&outCharset=utf-8&w=" + urlencode(artist) + '+' + urlencode(album) + '+' + urlencode(title);
    LOG_INFO("Querying for song ID from %s...", url.c_str());

    pfc::string8 content;
    try
    {
        http_request::ptr request = make_get_request();
        file_ptr response_file = request->run(url.c_str(), abort);

        response_file->read_string_raw(content, abort);
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to download QQMusic page %s: %s", url.c_str(), e.what());
        return 0;
    }

    cJSON* json = cJSON_ParseWithLength(content.c_str(), content.get_length());
    std::string song_id = parse_song_id(json, artist, album, title);
    cJSON_Delete(json);

    return song_id;
}

LyricDataRaw QQMusicLyricsSource::get_song_lyrics(std::string song_id, abort_callback& abort) const
{
    LyricDataRaw result = {};
    result.source_id = src_guid;

    if(song_id.empty())
    {
        return result;
    }

    LOG_INFO("Get QQMusic lyrics for song id %d", song_id);
    std::string url = "http://c.y.qq.com/lyric/fcgi-bin/fcg_query_lyric_new.fcg?g_tk=5381&format=json&inCharset=utf-8&outCharset=utf-8&songmid=" + song_id;
    result.persistent_storage_path = url;
    LOG_INFO("Querying for lyrics from %s...", url.c_str());

    pfc::string8 content;
    try
    {
        http_request::ptr request = make_get_request();
        file_ptr response_file = request->run(url.c_str(), abort);

        response_file->read_string_raw(content, abort);
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to download QQMusic page %s: %s", url.c_str(), e.what());
        return result;
    }

    cJSON* json = cJSON_ParseWithLength(content.c_str(), content.get_length());
    if((json != nullptr) && (json->type == cJSON_Object))
    {
        cJSON* lyric_item = cJSON_GetObjectItem(json, "lyric");
        if((lyric_item != nullptr) && (lyric_item->type == cJSON_String))
        {
            pfc::string8 lyric_str;
            pfc::base64_decode_to_string(lyric_str, lyric_item->valuestring);

            result.text = std::string(lyric_str.c_str(), lyric_str.length());
        }
    }
    cJSON_Delete(json);

    return result;
}

LyricDataRaw QQMusicLyricsSource::query(metadb_handle_ptr track, abort_callback& abort)
{
    std::string_view artist = get_artist(track);
    std::string_view album = get_album(track);
    std::string_view title = get_title(track);
    std::string song_id = get_song_id(artist, album, title, abort);
    return get_song_lyrics(std::move(song_id), abort);
}

