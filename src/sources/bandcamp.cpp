#include "stdafx.h"
#include <cctype>

#include "pugixml.hpp"

#include "logging.h"
#include "lyric_source.h"
#include "mvtf/mvtf.h"
#include "tag_util.h"

static const GUID src_guid = { 0x6f0de3a9, 0xc824, 0x429f, { 0x8e, 0x26, 0x6c, 0xe2, 0x7f, 0xab, 0xd8, 0x7b } };

class BandcampSource : public LyricSourceRemote
{
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("Bandcamp"); }

    std::vector<LyricDataRaw> search(const LyricSearchParams& params, abort_callback& abort) final;
    bool lookup(LyricDataRaw& data, abort_callback& abort) final;
};
static const LyricSourceFactory<BandcampSource> src_factory;

static std::string remove_artist_url_chars(const std::string_view input)
{
    std::string transliterated = from_tstring(normalise_utf8(to_tstring(input)));

    std::string output;
    output.reserve(transliterated.length());

    for(char c : transliterated)
    {
        if(pfc::char_is_ascii_alphanumeric(c))
        {
            output += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }

    return output;
}

static std::string remove_title_url_chars(const std::string_view input)
{
    std::string transliterated = from_tstring(normalise_utf8(to_tstring(input)));

    std::string output;
    output.reserve(transliterated.length());

    for(char c : transliterated)
    {
        if(pfc::char_is_ascii_alphanumeric(c))
        {
            output += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        else if(c == ' ')
        {
            output += '-';
        }
    }
    return output;
}

std::vector<LyricDataRaw> BandcampSource::search(const LyricSearchParams& params, abort_callback& abort)
{
    std::vector<LyricDataRaw> result;
    for(int page_index=1; page_index<8; page_index++)
    {
        http_request::ptr request = http_client::get()->create_request("GET");

        const std::string url_artist = remove_artist_url_chars(params.artist);
        const std::string url_title = remove_title_url_chars(params.title);
        std::string url = "http://" + url_artist + ".bandcamp.com/track/" + url_title;
        if(page_index > 1)
        {
            url += "-" + std::to_string(page_index);
        }
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
            LOG_WARN("Failed to download bandcamp.com page %s: %s", url.c_str(), e.what());
            break;
        }

        std::string lyric_text;
        pugi::xml_document doc;
        load_html_document(content.c_str(), doc);

        pugi::xpath_query query_lyricdivs("//div[contains(@class, 'lyricsText')]");
        pugi::xpath_node_set lyricdivs = query_lyricdivs.evaluate_node_set(doc);
        if(!lyricdivs.empty())
        {
            for(const pugi::xpath_node& node : lyricdivs)
            {
                if(node.node().type() != pugi::node_element) continue;

                add_all_text_to_string(lyric_text, node.node());
                break;
            }
        }

        if(!lyric_text.empty())
        {
            LOG_INFO("Successfully retrieved lyrics from %s", url.c_str());
            const std::string_view trimmed_text = trim_surrounding_whitespace(lyric_text);

            LyricDataRaw lyric = {};
            lyric.source_id = id();
            lyric.source_path = url;
            lyric.artist = params.artist;
            lyric.album = params.album;
            lyric.title = params.title;
            lyric.type = LyricType::Unsynced;
            lyric.text_bytes = string_to_raw_bytes(trimmed_text);
            result.push_back(lyric);
        }
    }
    return result;
}

bool BandcampSource::lookup(LyricDataRaw& /*data*/, abort_callback& /*abort*/)
{
    LOG_ERROR("We should never need to do a lookup of the %s source", friendly_name().data());
    assert(false);
    return false;
}

// ============
// Tests
// ============
#if MVTF_TESTS_ENABLED
MVTF_TEST(bandcamp_artist_url_removes_spaces_and_special_chars)
{
    const std::string result = remove_artist_url_chars("hello world!");
    ASSERT(result == "helloworld");
}

MVTF_TEST(bandcamp_title_url_removes_special_chars_and_replaces_spaces_with_dashes)
{
    const std::string result = remove_title_url_chars("hello world!");
    ASSERT(result == "hello-world");
}
#endif
