#include "stdafx.h"
#include <cctype>

#include "cJSON.h"
#include "pugixml.hpp"
#include "tidy.h"
#include "tidybuffio.h"

#include "logging.h"
#include "lyric_source.h"
#include "tag_util.h"

static const GUID src_guid = { 0xa7ac869e, 0xa867, 0x49e6, { 0x97, 0x9e, 0x7b, 0x61, 0x58, 0x84, 0x21, 0x17 } };

class MetalArchivesSource : public LyricSourceRemote
{
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("Metal-Archives.com"); }

    std::vector<LyricDataRaw> search(const LyricSearchParams& params, abort_callback& abort) final;
    bool lookup(LyricDataRaw& data, abort_callback& abort) final;

private:
    std::vector<LyricDataRaw> parse_song_ids(cJSON* json) const;
};
static const LyricSourceFactory<MetalArchivesSource> src_factory;

static void add_all_text_to_string(std::string& output, pugi::xml_node node)
{
    if(node.type() == pugi::node_null)
    {
        return;
    }

    pugi::xml_node current = node;
    while(current != nullptr)
    {
        if(current.type() == pugi::node_pcdata)
        {
            // We assume the text is already UTF-8
            std::string_view node_text = trim_surrounding_whitespace(current.value());
            output += node_text;
        }
        else if((current.type() == pugi::node_element) || (current.type() == pugi::node_document))
        {
            const std::string_view current_name = current.name();
            if(current_name == "br")
            {
                output += "\r\n";
            }
            else if((current_name == "h3") || (current_name == "div"))
            {
                break;
            }
            else
            {
                add_all_text_to_string(output, current.first_child());
            }
        }

        current = current.next_sibling();
    }
}

static std::string collect_all_text_to_string(pugi::xml_node node)
{
    std::string result;
    add_all_text_to_string(result, node);
    return result;
}

static pugi::xml_document load_html(const char* src)
{
    TidyBuffer tidy_output = {};
    TidyBuffer tidy_error = {};

    TidyDoc tidy_doc = tidyCreate();
    tidySetErrorBuffer(tidy_doc, &tidy_error);
    tidyOptSetBool(tidy_doc, TidyXhtmlOut, yes);
    tidyOptSetBool(tidy_doc, TidyForceOutput, yes);
    tidyParseString(tidy_doc, src);
    tidyCleanAndRepair(tidy_doc);
    tidyRunDiagnostics(tidy_doc);
    tidySaveBuffer(tidy_doc, &tidy_output);

    if(tidyErrorCount(tidy_doc) != 0)
    {
        tidyErrorSummary(tidy_doc); // Write more complete error info to the error_buffer
        LOG_INFO("Failed to convert retrieved HTML to XHTML:\n%s", tidy_error.bp);
        return pugi::xml_document{};
    }

    pugi::xml_document doc;
    doc.load_buffer(tidy_output.bp, tidy_output.size);

    tidyBufFree(&tidy_output);
    tidyBufFree(&tidy_error);
    tidyRelease(tidy_doc);

    return doc;
}

std::vector<LyricDataRaw> MetalArchivesSource::parse_song_ids(cJSON* json) const
{
    if((json == nullptr) || (json->type != cJSON_Object))
    {
        LOG_INFO("Root object is null or not an object");
        return {};
    }

    cJSON* result_arr = cJSON_GetObjectItem(json, "aaData");
    if((result_arr == nullptr) || (result_arr->type != cJSON_Array))
    {
        LOG_INFO("No valid 'aaData' property available");
        return {};
    }

    std::vector<LyricDataRaw> output;
    const int result_arr_len = cJSON_GetArraySize(result_arr);
    for(int song_index=0; song_index<result_arr_len; song_index++)
    {
        const cJSON* song_arr = cJSON_GetArrayItem(result_arr, song_index);
        if((song_arr == nullptr) || (song_arr->type != cJSON_Array))
        {
            LOG_INFO("Song array entry %d not available or invalid", song_index);
            continue;
        }

        const int song_arr_len = cJSON_GetArraySize(song_arr);
        if(song_arr_len != 5)
        {
            throw new std::runtime_error("Unexpected number of fields, the page format may have changed");
        }

        const cJSON* artist_item = cJSON_GetArrayItem(song_arr, 0);
        const cJSON* album_item = cJSON_GetArrayItem(song_arr, 1);
        const cJSON* title_item = cJSON_GetArrayItem(song_arr, 3);
        const cJSON* lyrics_item = cJSON_GetArrayItem(song_arr, 4);
        if((artist_item == nullptr) || (artist_item->type != cJSON_String)
            || (album_item == nullptr) || (album_item->type != cJSON_String)
            || (title_item == nullptr) || (title_item->type != cJSON_String)
            || (lyrics_item == nullptr) || (lyrics_item->type != cJSON_String))
        {
            throw new std::runtime_error("Unexpected data-field format, the page format may have changed");
        }

        const pugi::xml_document artist_doc = load_html(artist_item->valuestring);
        const pugi::xml_document album_doc = load_html(album_item->valuestring);

        const std::string result_artist = collect_all_text_to_string(artist_doc);
        const std::string result_album = collect_all_text_to_string(album_doc);
        const char* result_title = title_item->valuestring;

        // NOTE: We can't use load_html here because that inserts all sorts of other
        //       HTML elements (<head>, <body>, <html> etc) to make it a valid webpage
        //       which then significantly complicates the process of pulling out the
        //       id attribute on the root element of the input document.
        const char* const id_prefix = "lyricsLink_";
        pugi::xml_document id_doc;
        id_doc.load_buffer(lyrics_item->valuestring, strlen(lyrics_item->valuestring));
        std::string_view result_id = id_doc.first_child().attribute("id").value();
        if(!result_id.empty() && !result_id.starts_with(id_prefix))
        {
            throw new std::runtime_error("Unrecognised lyric ID format, the page format may have changed");
        }
        result_id = result_id.substr(strlen(id_prefix));

        if(result_artist.empty() || result_album.empty())
        {
            throw new std::runtime_error("Failed to parse metadata component XML, the page format may have changed");
        }

        LyricDataRaw data = {};
        data.source_id = src_guid;
        data.artist = std::move(result_artist);
        data.album = std::move(result_album);
        if(result_title != nullptr) data.title = result_title;
        data.lookup_id = result_id;
        data.type = LyricType::Unsynced;
        output.push_back(std::move(data));
    }

    return output;
}

std::vector<LyricDataRaw> MetalArchivesSource::search(const LyricSearchParams& params, abort_callback& abort)
{
    http_request::ptr request = http_client::get()->create_request("GET");

    const std::string url_artist = urlencode(params.artist);
    const std::string url_album = urlencode(params.album);
    const std::string url_title = urlencode(params.title);
    std::string url = "https://www.metal-archives.com/search/ajax-advanced/searching/songs";
    url += "?bandName=" + url_artist;
    url += "&releaseTitle=" + url_album;
    url += "&songTitle=" + url_title;
    LOG_INFO("Querying for lyrics from %s...", url.c_str());

    pfc::string8 content;
    try
    {
        file_ptr response_file = request->run(url.c_str(), abort);
        response_file->read_string_raw(content, abort);
        // NOTE: We're assuming here that the response is encoded in UTF-8 
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to download metal-archives.com page %s: %s", url.c_str(), e.what());
        return {};
    }

    cJSON* json = cJSON_ParseWithLength(content.c_str(), content.get_length());
    std::vector<LyricDataRaw> song_ids = parse_song_ids(json);
    cJSON_Delete(json);
    LOG_INFO("Retrieved %d tracks from %s", int(song_ids.size()), url.c_str());

    return song_ids;
}

bool MetalArchivesSource::lookup(LyricDataRaw& data, abort_callback& abort)
{
    assert(data.source_id == id());
    if(data.lookup_id.empty())
    {
        return false;
    }

    http_request::ptr request = http_client::get()->create_request("GET");
    std::string url = "https://www.metal-archives.com/release/ajax-view-lyrics/id/" + data.lookup_id;
    LOG_INFO("Looking up lyrics at %s...", url.c_str());

    pfc::string8 content;
    try
    {
        file_ptr response_file = request->run(url.c_str(), abort);
        response_file->read_string_raw(content, abort);
        // NOTE: We're assuming here that the response is encoded in UTF-8 
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to download metal-archives.com page %s: %s", url.c_str(), e.what());
        return false;
    }

    const pugi::xml_document lyric_doc = load_html(content.c_str());
    const std::string lyric_text = collect_all_text_to_string(lyric_doc);

    if(lyric_text == "(lyrics not available)") {
        return false;
    }

    data.text_bytes = string_to_raw_bytes(lyric_text);
    data.source_path = std::move(url);
    return !data.text_bytes.empty();
}
