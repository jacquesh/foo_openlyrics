#include "stdafx.h"

#include "cJSON.h"

#include "logging.h"
#include "lyric_data.h"
#include "lyric_source.h"
#include "openlyrics_version.h"

static const GUID src_guid = { 0x9b4be445, 0x9a38, 0x4342, { 0xab, 0x72, 0x6f, 0x55, 0x8c, 0x4, 0x4d, 0xc0 } };

class LrclibLyricsSource : public LyricSourceRemote
{
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("LRCLIB"); }

    std::vector<LyricDataRaw> search(const LyricSearchParams& params, abort_callback& abort) final;
    bool lookup(LyricDataRaw& data, abort_callback& abort) final;
};
static const LyricSourceFactory<LrclibLyricsSource> src_factory;

static const char* g_api_url = "https://lrclib.net/api/";

std::vector<LyricDataRaw> LrclibLyricsSource::search(const LyricSearchParams& params, abort_callback& abort)
{
    std::string url = std::string(g_api_url) + "search";
    url += "?artist_name=" + urlencode(params.artist);
    url += "&album_name=" + urlencode(params.album);
    url += "&track_name=" + urlencode(params.title);
    LOG_INFO("Querying for track ID from %s", url.c_str());

    pfc::string8 content;
    try
    {
        http_request::ptr request = http_client::get()->create_request("GET");
        request->add_header("User-Agent", "foo_openlyrics v" OPENLYRICS_VERSION " (https://github.com/jacquesh/foo_openlyrics)");
        file_ptr response_file = request->run(url.c_str(), abort);
        response_file->read_string_raw(content, abort);
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to make LRCLIB search request to %s: %s", url.c_str(), e.what());
        return {};
    }

    cJSON* json = cJSON_ParseWithLength(content.c_str(), content.get_length());
    if((json == nullptr) || (json->type != cJSON_Array))
    {
        LOG_WARN("Received LRCLIB search result but root was malformed: %s", content.c_str());
        cJSON_Delete(json);
        return {};
    }

    std::vector<LyricDataRaw> results;
    cJSON* json_result = nullptr;
    cJSON_ArrayForEach(json_result, json)
    {
        if((json_result == nullptr) || (json_result->type != cJSON_Object))
        {
            LOG_WARN("Received LRCLIB search result but track was malformed: %s", content.c_str());
            break;
        }

        cJSON* json_title = cJSON_GetObjectItem(json_result, "trackName");
        if((json_title == nullptr) || (json_title->type != cJSON_String))
        {
            LOG_WARN("Received LRCLIB search result but trackName was malformed: %s", content.c_str());
            break;
        }

        cJSON* json_artist = cJSON_GetObjectItem(json_result, "artistName");
        if((json_artist == nullptr) || (json_artist->type != cJSON_String))
        {
            LOG_WARN("Received LRCLIB search result but artistName was malformed: %s", content.c_str());
            break;
        }

        cJSON* json_album = cJSON_GetObjectItem(json_result, "albumName");
        if((json_album == nullptr) || (json_album->type != cJSON_String))
        {
            LOG_WARN("Received LRCLIB search result but albumName was malformed: %s", content.c_str());
            break;
        }

        cJSON* json_plainlyrics = cJSON_GetObjectItem(json_result, "plainLyrics");
        if((json_plainlyrics == nullptr) || (json_plainlyrics->type != cJSON_String))
        {
            LOG_WARN("Received LRCLIB search result but plainLyrics was malformed: %s", content.c_str());
            break;
        }

        cJSON* json_syncedlyrics = cJSON_GetObjectItem(json_result, "syncedLyrics");
        if((json_syncedlyrics == nullptr) || (json_syncedlyrics->type != cJSON_String))
        {
            LOG_WARN("Received LRCLIB search result but syncedLyrics was malformed: %s", content.c_str());
            break;
        }

        cJSON* json_id = cJSON_GetObjectItem(json_result, "id");
        if((json_id == nullptr) || (json_id->type != cJSON_Number))
        {
            LOG_WARN("Received LRCLIB search result but id was malformed: %s", content.c_str());
            break;
        }
        const std::string source_path = std::string(g_api_url) + "get/" + std::to_string(json_id->valueint);

        // Add synced lyrics to the output first so that those will be preferred
        if(strlen(json_syncedlyrics->valuestring) > 0) {
            LOG_INFO("Successfully retrieved synced lyrics from %s", source_path.c_str());
            LyricDataRaw data = {};
            data.source_id = id();
            data.source_path = source_path;
            data.artist = json_artist->valuestring;
            data.album = json_album->valuestring;
            data.title = json_title->valuestring;
            data.text_bytes = string_to_raw_bytes(json_syncedlyrics->valuestring);
            results.push_back(std::move(data));
        }

        if(strlen(json_plainlyrics->valuestring) > 0) {
            LOG_INFO("Successfully retrieved unsynced lyrics from %s", source_path.c_str());
            LyricDataRaw data = {};
            data.source_id = id();
            data.source_path = source_path;
            data.artist = json_artist->valuestring;
            data.album = json_album->valuestring;
            data.title = json_title->valuestring;
            data.text_bytes = string_to_raw_bytes(json_plainlyrics->valuestring);
            results.push_back(std::move(data));
        }
    }

    cJSON_Delete(json);
    return results;
}

bool LrclibLyricsSource::lookup(LyricDataRaw& /*data*/, abort_callback& /*abort*/)
{
    LOG_ERROR("We should never need to do a lookup of the %s source", friendly_name().data());
    assert(false);
    return false;
}
