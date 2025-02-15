#include "stdafx.h"
#include <cctype>
#include <charconv>

#include "cJSON.h"
#include "pugixml.hpp"

#include "logging.h"
#include "lyric_source.h"
#include "mvtf/mvtf.h"

static const GUID src_guid = { 0xa7735112, 0x5ef6, 0x4f53, { 0xa7, 0xe5, 0x5b, 0xa5, 0xb5, 0x60, 0xa7, 0xa6 } };

class LyricfindSource : public LyricSourceRemote
{
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("LyricFind.com"); }

    std::optional<LyricDataRaw> extract_lyrics_from_page(std::string_view url, pfc::string8 page_content) const;

    std::vector<LyricDataRaw> search(const LyricSearchParams& params, abort_callback& abort) final;
    bool lookup(LyricDataRaw& data, abort_callback& abort) final;
};
static const LyricSourceFactory<LyricfindSource> src_factory;

static std::string transform_chars_for_url(const std::string_view input)
{
    std::string transliterated = from_tstring(normalise_utf8(to_tstring(input)));

    std::string output;
    output.reserve(transliterated.length() + 3); // We add a bit to allow for one or two & or @ replacements without re-allocation
    for(char c : transliterated)
    {
        if(pfc::char_is_ascii_alphanumeric(c))
        {
            output += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        else
        {
            output += '-';
        }
    }

    while(!output.empty() && (output.back() == '-'))
    {
        output.pop_back();
    }

    return output;
}

std::optional<int> try_parse_duration_seconds(std::string_view tag)
{
    size_t minsec_separator = tag.find(':');
    if(minsec_separator == std::string_view::npos)
    {
        return {};
    }

    std::string_view sec_str = tag.substr(minsec_separator + 1);
    std::string_view min_str = tag.substr(0, minsec_separator);

    int sec = 0;
    int min = 0;
    std::from_chars_result sec_result = std::from_chars(sec_str.data(), sec_str.data() + sec_str.length(), sec);
    std::from_chars_result min_result = std::from_chars(min_str.data(), min_str.data() + min_str.length(), min);
    if((sec_result.ec != std::errc{}) || (min_result.ec != std::errc{}))
    {
        return {};
    }

    const int result = min*60 + sec;
    return {result};
}

std::optional<LyricDataRaw> LyricfindSource::extract_lyrics_from_page(std::string_view url, pfc::string8 page_content) const
{
    std::string json_str;
    pugi::xml_document doc;
    load_html_document(page_content.c_str(), doc);
    const pugi::xpath_query query_lyricdivs("//script[@id='__NEXT_DATA__']");
    const pugi::xpath_node_set lyricdivs = query_lyricdivs.evaluate_node_set(doc);
    if(!lyricdivs.empty())
    {
        add_all_text_to_string(json_str, lyricdivs.first().node());
    }

    cJSON* json = cJSON_ParseWithLength(json_str.c_str(), json_str.length());
    if((json == nullptr) || (json->type != cJSON_Object))
    {
        LOG_WARN("Received lyricfind result but the root json object was malformed: %s", json_str.c_str());
        cJSON_Delete(json);
        return {};
    }

    cJSON* json_props = cJSON_GetObjectItem(json, "props");
    if((json_props == nullptr) || (json_props->type != cJSON_Object))
    {
        LOG_WARN("Received lyricfind result but props was malformed: %s", json_str.c_str());
        cJSON_Delete(json);
        return {};
    }

    cJSON* json_pageprops = cJSON_GetObjectItem(json_props, "pageProps");
    if((json_pageprops == nullptr) || (json_pageprops->type != cJSON_Object))
    {
        LOG_WARN("Received lyricfind result but pageProps was malformed: %s", json_str.c_str());
        cJSON_Delete(json);
        return {};
    }
    // TODO: Error handling

    cJSON* json_songdata = cJSON_GetObjectItem(json_pageprops, "songData");
    if((json_songdata == nullptr) || (json_songdata->type != cJSON_Object))
    {
        LOG_WARN("Received lyricfind result but songData was malformed: %s", json_str.c_str());
        cJSON_Delete(json);
        return {};
    }

    cJSON* json_track = cJSON_GetObjectItem(json_songdata, "track");
    if((json_track == nullptr) || (json_track->type != cJSON_Object))
    {
        LOG_WARN("Received lyricfind result but track was malformed: %s", json_str.c_str());
        cJSON_Delete(json);
        return {};
    }

    cJSON* json_artist = cJSON_GetObjectItem(json_track, "artist");
    if((json_artist == nullptr) || (json_artist->type != cJSON_Object))
    {
        LOG_WARN("Received lyricfind result but artist was malformed: %s", json_str.c_str());
        cJSON_Delete(json);
        return {};
    }

    cJSON* json_artistname = cJSON_GetObjectItem(json_artist, "name");
    if((json_artistname == nullptr) || (json_artistname->type != cJSON_String))
    {
        LOG_WARN("Received lyricfind result but artistname was malformed: %s", json_str.c_str());
        cJSON_Delete(json);
        return {};
    }

    cJSON* json_title = cJSON_GetObjectItem(json_track, "title");
    if((json_title == nullptr) || (json_title->type != cJSON_String))
    {
        LOG_WARN("Received lyricfind result but title was malformed: %s", json_str.c_str());
        cJSON_Delete(json);
        return {};
    }

    cJSON* json_duration = cJSON_GetObjectItem(json_track, "duration");
    if((json_duration == nullptr) || (json_duration->type != cJSON_String))
    {
        LOG_WARN("Received lyricfind result but duration was malformed: %s", json_str.c_str());
        cJSON_Delete(json);
        return {};
    }

    cJSON* json_lyrics = cJSON_GetObjectItem(json_track, "lyrics");
    if((json_lyrics == nullptr) || (json_lyrics->type != cJSON_String))
    {
        LOG_WARN("Received lyricfind result but lyrics was malformed: %s", json_str.c_str());
        cJSON_Delete(json);
        return {};
    }

    LyricDataRaw result = {};
    result.source_id = id();
    result.source_path = url;
    result.artist = json_artistname->valuestring;
    result.title = json_title->valuestring;
    result.type = LyricType::Unsynced;
    result.duration_sec = try_parse_duration_seconds(json_duration->valuestring);
    result.text_bytes = string_to_raw_bytes(json_lyrics->valuestring);
    return {result};
}

std::vector<LyricDataRaw> LyricfindSource::search(const LyricSearchParams& params, abort_callback& abort)
{
    std::string url = "https://lyrics.lyricfind.com/lyrics/";
    url += transform_chars_for_url(params.artist);
    url += '-';
    url += transform_chars_for_url(params.title);

    pfc::string8 content;
    try
    {
        auto request = http_client::get()->create_request("GET");

        // Without these we get 403s back
        request->add_header("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:128.0) Gecko/20100101 Firefox/128.0");

        file_ptr response_file = request->run(url.c_str(), abort);
        response_file->read_string_raw(content, abort);
        // NOTE: We're assuming here that the response is encoded in UTF-8 
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to download lyricfind.com page %s: %s", url.c_str(), e.what());
        return {};
    }

    LOG_INFO("Page %s retrieved", url.c_str());
    std::optional<LyricDataRaw> result = extract_lyrics_from_page(url, content);
    if(!result.has_value())
    {
        throw new std::runtime_error("Failed to parse lyrics, the page format may have changed");
    }

    LOG_INFO("Successfully retrieved lyrics from %s", url.c_str());
    return {std::move(result.value())};
}

bool LyricfindSource::lookup(LyricDataRaw& /*data*/, abort_callback& /*abort*/)
{
    LOG_ERROR("We should never need to do a lookup of the %s source", friendly_name().data());
    assert(false);
    return false;
}

// ============
// Tests
// ============
#if MVTF_TESTS_ENABLED
MVTF_TEST(lyricfind_urltransform_replaces_spaces_with_dash)
{
    const std::string_view input = "The Offspring";
    const std::string output = transform_chars_for_url(input);
    ASSERT(output == "the-offspring");
}

MVTF_TEST(lyricfind_urltransform_replaces_intermediate_special_characters_with_dash)
{
    const std::string_view input = "11:11 P.M.";
    const std::string output = transform_chars_for_url(input);
    ASSERT(output == "11-11-p-m");
}

MVTF_TEST(lyricfind_durationparse_parses_valid_strings)
{
    const std::string_view input = "3:56";
    const std::optional<int> output = try_parse_duration_seconds(input);
    ASSERT(output.has_value());
    ASSERT(output.value() == 236);
}
#endif
