#include "stdafx.h"

#include "cJSON.h"

#include "logging.h"
#include "lyric_data.h"
#include "lyric_source.h"

static const GUID src_guid = { 0xf94ba31a, 0x7b33, 0x49e4, { 0x81, 0x9b, 0x0, 0xc, 0x36, 0x44, 0x29, 0xcd } };

struct SongSearchResult
{
    int64_t track_id;
    bool has_synced_lyrics;
    bool has_unsynced_lyrics;
};

class MusixmatchLyricsSource : public LyricSourceRemote
{
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("Musixmatch"); }

    std::vector<LyricDataRaw> search(std::string_view artist, std::string_view album, std::string_view title, abort_callback& abort) final;
    bool lookup(LyricDataRaw& data, abort_callback& abort) final;

private:
    std::vector<LyricDataRaw> get_song_ids(std::string_view artist, std::string_view album, std::string_view title, abort_callback& abort) const;
    bool get_lyrics(LyricDataRaw& data, int64_t track_id, abort_callback& abort, const char* method, const char* body_entry_name, const char* text_entry_name) const;
    bool get_unsynced_lyrics(LyricDataRaw& data, int64_t track_id, abort_callback& abort) const;
    bool get_synced_lyrics(LyricDataRaw& data, int64_t track_id, abort_callback& abort) const;
};
static const LyricSourceFactory<MusixmatchLyricsSource> src_factory;

static const char* g_api_url = "https://apic-desktop.musixmatch.com/ws/1.1/";
static const char* g_common_params = "user_language=en&app_id=web-desktop-app-v1.0";

static std::string EncodeSearchResult(SongSearchResult search_result)
{
    std::string output;
    output += (search_result.has_unsynced_lyrics ? '1' : '0');
    output += (search_result.has_synced_lyrics ? '1' : '0');
    output += std::to_string(search_result.track_id);
    return output;
}

static std::optional<SongSearchResult> DecodeSearchResult(std::string_view str)
{
    if(str.length() < 3)
    {
        return {};
    }
    if((str[0] != '0') && (str[0] != '1'))
    {
        return {};
    }
    if((str[1] != '0') && (str[1] != '1'))
    {
        return {};
    }

    SongSearchResult result = {};
    result.has_unsynced_lyrics = (str[0] == '1');
    result.has_synced_lyrics = (str[1] == '1');
    result.track_id = strtoll(&str[2], nullptr, 10);
    return result;
}

std::vector<LyricDataRaw> MusixmatchLyricsSource::get_song_ids(std::string_view artist, std::string_view album, std::string_view title, abort_callback& abort) const
{
    std::string apikey = preferences::searching::musixmatch_api_key();
    std::string url = std::string(g_api_url) + "track.search?" + g_common_params + "&subtitle_format=lrc&usertoken=" + apikey;
    url += "&q_artist=" + urlencode(artist);
    url += "&q_album=" + urlencode(album);
    url += "&q_track=" + urlencode(title);
    LOG_INFO("Querying for track ID from %s...", url.c_str());

    pfc::string8 content;
    try
    {
        http_request::ptr request = http_client::get()->create_request("GET");

        // NOTE: Without adding the AWSELB and AWSELBCORS headers, we get a 301 (permanent redirect back)
        //       with a header instructing us to set those cookies to the given hash.
        //       The fb2k http API automatically follows the redirect but does not honour the Set-Cookie headers.
        //       The redirect goes to the same URL and the request then fails after a while (presumably because
        //       ELB thinks we're DoS'ing them and kills the connection).
        //       Setting the headers here to just *some* value (even if its not a useful one) seems to make it work.
        //       We may need to upgrade this in future to actually set the cookies that we're asked to set.
        request->add_header("cookie", "AWSELBCORS=0; AWSELB=0");

        file_ptr response_file = request->run(url.c_str(), abort);

        response_file->read_string_raw(content, abort);
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to make Musixmatch search request to %s: %s", url.c_str(), e.what());
        return {};
    }

    cJSON* json = cJSON_ParseWithLength(content.c_str(), content.get_length());
    if((json == nullptr) || (json->type != cJSON_Object))
    {
        LOG_INFO("Received musixmatch search result but root was malformed: %s", content.c_str());
        cJSON_Delete(json);
        return {};
    }

    cJSON* json_message = cJSON_GetObjectItem(json, "message");
    if((json_message == nullptr) || (json_message->type != cJSON_Object))
    {
        LOG_INFO("Received musixmatch search result but message was malformed: %s", content.c_str());
        cJSON_Delete(json);
        return {};
    }

    cJSON* json_body = cJSON_GetObjectItem(json_message, "body");
    if((json_body == nullptr) || (json_body->type != cJSON_Object))
    {
        LOG_INFO("Received musixmatch search result but body was malformed: %s", content.c_str());
        cJSON_Delete(json);
        return {};
    }

    cJSON* json_tracklist = cJSON_GetObjectItem(json_body, "track_list");
    if((json_tracklist == nullptr) || (json_tracklist->type != cJSON_Array))
    {
        LOG_INFO("Received musixmatch search result but track_list was malformed: %s", content.c_str());
        cJSON_Delete(json);
        return {};
    }

    std::vector<LyricDataRaw> results;
    cJSON* json_track = nullptr;
    cJSON_ArrayForEach(json_track, json_tracklist)
    {
        if((json_track == nullptr) || (json_track->type != cJSON_Object))
        {
            LOG_INFO("Received musixmatch search result but track was malformed: %s", content.c_str());
            break;
        }

        cJSON* json_tracktrack = cJSON_GetObjectItem(json_track, "track");
        if((json_tracktrack == nullptr) || (json_tracktrack->type != cJSON_Object))
        {
            LOG_INFO("Received musixmatch search result but tracktrack was malformed: %s", content.c_str());
            break;
        }

        cJSON* json_artist = cJSON_GetObjectItem(json_tracktrack, "artist_name");
        if((json_artist == nullptr) || (json_artist->type != cJSON_String))
        {
            LOG_INFO("Received musixmatch search result but artist_name was malformed");
            break;
        }

        cJSON* json_album = cJSON_GetObjectItem(json_tracktrack, "album_name");
        if((json_album == nullptr) || (json_album->type != cJSON_String))
        {
            LOG_INFO("Received musixmatch search result but album_name was malformed");
            break;
        }

        cJSON* json_title = cJSON_GetObjectItem(json_tracktrack, "track_name");
        if((json_title == nullptr) || (json_title->type != cJSON_String))
        {
            LOG_INFO("Received musixmatch search result but track_name was malformed");
            break;
        }

        cJSON* json_haslyrics = cJSON_GetObjectItem(json_tracktrack, "has_lyrics");
        if((json_haslyrics == nullptr) || (json_haslyrics->type != cJSON_Number))
        {
            LOG_INFO("Received musixmatch search result but has-lyrics was malformed: %s", content.c_str());
            break;
        }

        cJSON* json_hassubtitles = cJSON_GetObjectItem(json_tracktrack, "has_subtitles");
        if((json_hassubtitles == nullptr) || (json_hassubtitles->type != cJSON_Number))
        {
            LOG_INFO("Received musixmatch search result but has-subtitles was malformed: %s", content.c_str());
            break;
        }

        cJSON* json_trackid = cJSON_GetObjectItem(json_tracktrack, "commontrack_id");
        if((json_tracktrack == nullptr) || (json_trackid->type != cJSON_Number))
        {
            LOG_INFO("Received musixmatch search result but track ID was malformed: %s", content.c_str());
            break;
        }

        SongSearchResult search_result = {};
        search_result.track_id = json_trackid->valueint;
        search_result.has_unsynced_lyrics = (json_haslyrics->valueint != 0);
        search_result.has_synced_lyrics = (json_hassubtitles->valueint != 0);

        LyricDataRaw data = {};
        data.source_id = id();
        data.artist = json_artist->valuestring;
        data.album = json_album->valuestring;
        data.title = json_title->valuestring;
        data.lookup_id = EncodeSearchResult(search_result);
        results.push_back(std::move(data));
    }

    cJSON_Delete(json);
    return results;
}

bool MusixmatchLyricsSource::get_lyrics(LyricDataRaw& data, int64_t track_id, abort_callback& abort, const char* method, const char* body_entry_name, const char* text_entry_name) const
{
    assert(data.source_id == id());

    std::string apikey = preferences::searching::musixmatch_api_key();
    std::string url = std::string(g_api_url) + method + "?" + g_common_params + "&usertoken=" + apikey + "&commontrack_id=" + std::to_string(track_id);
    data.persistent_storage_path = url;
    LOG_INFO("Get Musixmatch lyrics lyrics from %s...", url.c_str());

    pfc::string8 content;
    try
    {
        http_request::ptr request = http_client::get()->create_request("GET");
        request->add_header("cookie", "AWSELBCORS=0; AWSELB=0"); // NOTE: See the comment on the cookie in the track ID query

        file_ptr response_file = request->run(url.c_str(), abort);

        response_file->read_string_raw(content, abort);
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to make Musixmatch %s request to %s: %s", method, url.c_str(), e.what());
        return false;
    }

    cJSON* json = cJSON_ParseWithLength(content.c_str(), content.get_length());
    if((json == nullptr) || (json->type != cJSON_Object))
    {
        LOG_INFO("Received musixmatch %s response but root was malformed: %s", method, content.c_str());
        cJSON_Delete(json);
        return false;
    }

    cJSON* json_message = cJSON_GetObjectItem(json, "message");
    if((json_message == nullptr) || (json_message->type != cJSON_Object))
    {
        LOG_INFO("Received musixmatch %s response but message was malformed: %s", method, content.c_str());
        cJSON_Delete(json);
        return false;
    }

    cJSON* json_body = cJSON_GetObjectItem(json_message, "body");
    if((json_body == nullptr) || (json_body->type != cJSON_Object))
    {
        LOG_INFO("Received musixmatch %s response but body was malformed: %s", method, content.c_str());
        cJSON_Delete(json);
        return false;
    }

    cJSON* json_lyrics = cJSON_GetObjectItem(json_body, body_entry_name);
    if((json_lyrics == nullptr) || (json_lyrics->type != cJSON_Object))
    {
        LOG_INFO("Received musixmatch %s response but %s was malformed: %s", method, body_entry_name, content.c_str());
        cJSON_Delete(json);
        return false;
    }

    cJSON* json_lyricstext = cJSON_GetObjectItem(json_lyrics, text_entry_name);
    if((json_lyricstext == nullptr) || (json_lyricstext->type != cJSON_String))
    {
        LOG_INFO("Received musixmatch %s response but %s was malformed: %s", method, text_entry_name, content.c_str());
        cJSON_Delete(json);
        return false;
    }

    data.text = json_lyricstext->valuestring;
    cJSON_Delete(json);
    return true;
}

bool MusixmatchLyricsSource::get_unsynced_lyrics(LyricDataRaw& data, int64_t track_id, abort_callback& abort) const
{
    return get_lyrics(data, track_id, abort, "track.lyrics.get", "lyrics", "lyrics_body");
}

bool MusixmatchLyricsSource::get_synced_lyrics(LyricDataRaw& data, int64_t track_id, abort_callback& abort) const
{
    return get_lyrics(data, track_id, abort, "track.subtitle.get", "subtitle", "subtitle_body");
}

std::vector<LyricDataRaw> MusixmatchLyricsSource::search(std::string_view artist, std::string_view album, std::string_view title, abort_callback& abort)
{
    if(preferences::searching::musixmatch_api_key().empty())
    {
        // An API key is required.
        // Skip the search if we don't have one so we don't accidentally spam their servers with obviously-bad requests.
        LOG_INFO("Skipping request to the Musixmatch source because no API key is available");
        return {};
    }

    return get_song_ids(artist, album, title, abort);
}

bool MusixmatchLyricsSource::lookup(LyricDataRaw& data, abort_callback& abort)
{
    std::optional<SongSearchResult> maybe_search_result = DecodeSearchResult(data.lookup_id);
    if(!maybe_search_result.has_value())
    {
        LOG_WARN("Attempt to lookup musixmatch lyrics with invalid lookup ID: %s", data.lookup_id.c_str());
        return false;
    }

    const SongSearchResult& search_result = maybe_search_result.value();
    if(search_result.track_id == 0)
    {
        LOG_WARN("Attempt to lookup musixmatch lyrics with null lookup track ID: %s", data.lookup_id.c_str());
        return false;
    }

    if(search_result.has_synced_lyrics)
    {
        return get_synced_lyrics(data, search_result.track_id, abort);
    }
    else if(search_result.has_unsynced_lyrics)
    {
        return get_unsynced_lyrics(data, search_result.track_id, abort);
    }
    else
    {
        return false;
    }
}
