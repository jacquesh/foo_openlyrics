#include "stdafx.h"
#include <cctype>

#include "pugixml.hpp"

#include "logging.h"
#include "lyric_source.h"
#include "mvtf/mvtf.h"
#include "parsers.h"
#include "tag_util.h"

static const GUID src_guid = { 0x79959787, 0xd75e, 0x4643, { 0xa2, 0x43, 0x6a, 0xdd, 0x5d, 0xe7, 0x42, 0x5c } };

class LyricsifySource : public LyricSourceRemote
{
public:
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("Lyricsify.com"); }

    void add_all_text_to_string(std::string& output, pugi::xml_node node) const;
    std::string extract_lyrics_from_page(pfc::string8 page_content) const;

    std::vector<LyricDataRaw> search(const LyricSearchParams& params, abort_callback& abort) final;
    bool lookup(LyricDataRaw& data, abort_callback& abort) final;
};
static const LyricSourceFactory<LyricsifySource> src_factory;

static std::string transform_tag_for_url(const std::string_view input)
{
    std::string transliterated = from_tstring(normalise_utf8(to_tstring(input)));

    std::string output;
    output.reserve(transliterated.length());

    for(char c : transliterated)
    {
        if(c == ' ')
        {
            output += '-';
        }
        if(pfc::char_is_ascii_alphanumeric(c))
        {
            output += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }

    return output;
}

void LyricsifySource::add_all_text_to_string(std::string& output, pugi::xml_node node) const
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
        else if(current.type() == pugi::node_element)
        {
            const std::string_view current_name = current.name();
            if(current_name == "br")
            {
                output += "\r\n";
            }
            else
            {
                add_all_text_to_string(output, current.first_child());
            }
        }

        current = current.next_sibling();
    }
}

std::string LyricsifySource::extract_lyrics_from_page(pfc::string8 page_content) const
{
    std::string lyric_text;
    pugi::xml_document doc;
    load_html_document(page_content.c_str(), doc);

    try
    {
        // Really this should be starts-with() and ends-with(), but pugi::xml doesn't
        // seem to support ends-with, so we settle for contains() instead.
        pugi::xpath_query query_lyricdivs("//div[starts-with(@id, 'lyrics_') and contains(@id, '_details')]");
        pugi::xpath_node_set lyricdivs = query_lyricdivs.evaluate_node_set(doc);
        if(!lyricdivs.empty())
        {
            add_all_text_to_string(lyric_text, lyricdivs.first().node());
        }
    }
    catch(const pugi::xpath_exception& ex)
    {
        const std::string w = ex.what();
        LOG_WARN("Failed to process xpath query: %s", w.c_str());
    }

    return lyric_text;
}

std::vector<LyricDataRaw> LyricsifySource::search(const LyricSearchParams& params, abort_callback& abort)
{
    http_request::ptr request = http_client::get()->create_request("GET");

    const std::string url_artist = transform_tag_for_url(params.artist);
    const std::string url_title = transform_tag_for_url(params.title);
    const std::string url = "http://www.lyricsify.com/lyrics/" + url_artist + "/" + url_title;
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
        LOG_WARN("Failed to download lyricsify.com page %s: %s", url.c_str(), e.what());
        return {};
    }

    const std::string lyric_text = extract_lyrics_from_page(content);
    if(lyric_text.empty())
    {
        throw new std::runtime_error("Failed to parse lyrics, the page format may have changed");
    }
    else
    {
        LOG_INFO("Successfully retrieved lyrics from %s", url.c_str());
        const std::string_view trimmed_text = trim_surrounding_whitespace(lyric_text);

        LyricDataRaw result = {};
        result.source_id = id();
        result.source_path = url;
        result.artist = params.artist;
        result.album = params.album;
        result.title = params.title;
        result.text_bytes = string_to_raw_bytes(trimmed_text);

        const LyricData parsed = parsers::lrc::parse(result, trimmed_text);
        result.type = parsed.IsTimestamped() ? LyricType::Synced : LyricType::Unsynced;
        return {std::move(result)};
    }
}

bool LyricsifySource::lookup(LyricDataRaw& /*data*/, abort_callback& /*abort*/)
{
    LOG_ERROR("We should never need to do a lookup of the %s source", friendly_name().data());
    assert(false);
    return false;
}

// ============
// Tests
// ============
#ifdef MVTF_TESTS_ENABLED
MVTF_TEST(lyricsify_known_bad_chars_removed_from_url)
{
    const std::string result = transform_tag_for_url("?!@#asd!@#qwe[]'_zxc-+><");
    ASSERT(result == "asdqwezxc");
}

MVTF_TEST(lyricsify_tag_spaces_replaced_with_dash)
{
    const std::string result = transform_tag_for_url("hello world");
    ASSERT(result == "hello-world");
}

MVTF_TEST(lyrisify_extracts_lyrics_from_page_content)
{
    const pfc::string8 input = 
        "<body>"
        "<div class=\"main-container\">"
        "<div class=\"main-page\">"
        "<div class=\"lyrics_1234\">"
        "<div id=\"lyrics_1234_details\">"
        "line1<br>line2"
        "</div>"
        "<div id=\"lyrics_1234_lyricsonly\"></div>"
        "</div></div></div></body>";
    LyricSourceFactory<LyricsifySource> factory;
    const std::string output = factory.get_static_instance().extract_lyrics_from_page(input);
    ASSERT(output == "line1\r\nline2");
}
#endif
