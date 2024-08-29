#include "stdafx.h"
#include <cctype>

#include "pugixml.hpp"

#include "logging.h"
#include "lyric_source.h"
#include "mvtf/mvtf.h"
#include "parsers.h"
#include "tag_util.h"

static const GUID src_guid = { 0xe6b4d51b, 0x6016, 0x494f, { 0x80, 0xb2, 0xf5, 0xff, 0xb4, 0x88, 0xdd, 0x91 } };

class LetrasSource : public LyricSourceRemote
{
public:
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("letras.com"); }

    std::string extract_lyrics_from_page(pfc::string8 page_content) const;

    std::vector<LyricDataRaw> search(const LyricSearchParams& params, abort_callback& abort) final;
    bool lookup(LyricDataRaw& data, abort_callback& abort) final;
};
static const LyricSourceFactory<LetrasSource> src_factory;

static std::string transform_tag_for_url(const std::string_view input)
{
    std::string transliterated = from_tstring(normalise_utf8(to_tstring(input)));

    std::string output;
    output.reserve(transliterated.length());

    for(char c : transliterated)
    {
        if((c == ' ') || (c == '-'))
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

static std::string transform_artist_for_url(const std::string_view artist)
{
    if(starts_with_ignore_case(artist, "the "))
    {
        return transform_tag_for_url(artist.substr(4));
    }

    return transform_tag_for_url(artist);
}

std::string LetrasSource::extract_lyrics_from_page(pfc::string8 page_content) const
{
    std::string lyric_text;
    pugi::xml_document doc;
    load_html_document(page_content.c_str(), doc);

    pugi::xpath_query query_lyricdivs("//div[@class='lyric-original']");
    pugi::xpath_node_set lyricdivs = query_lyricdivs.evaluate_node_set(doc);
    if(!lyricdivs.empty())
    {
        add_all_text_to_string(lyric_text, lyricdivs.first().node());
    }

    return lyric_text;
}

std::vector<LyricDataRaw> LetrasSource::search(const LyricSearchParams& params, abort_callback& abort)
{
    http_request::ptr request = http_client::get()->create_request("GET");

    const std::string url_artist = transform_artist_for_url(params.artist);
    const std::string url_title = transform_tag_for_url(params.title);
    const std::string url = "http://www.letras.com/" + url_artist + "/" + url_title;
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
        LOG_WARN("Failed to download letras.com page %s: %s", url.c_str(), e.what());
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

bool LetrasSource::lookup(LyricDataRaw& /*data*/, abort_callback& /*abort*/)
{
    LOG_ERROR("We should never need to do a lookup of the %s source", friendly_name().data());
    assert(false);
    return false;
}

// ============
// Tests
// ============
#ifdef MVTF_TESTS_ENABLED
MVTF_TEST(letras_known_bad_chars_removed_from_url)
{
    const std::string result = transform_tag_for_url("?!@#asd!@#qwe[]'_zxc-+><");
    ASSERT(result == "asdqwezxc-");
}

MVTF_TEST(letras_tag_spaces_replaced_with_dash)
{
    const std::string result = transform_tag_for_url("hello world");
    ASSERT(result == "hello-world");
}

MVTF_TEST(letras_prefix_the_removed_from_artist)
{
    const std::string result = transform_artist_for_url("The All-American Rejects");
    ASSERT(result == "all-american-rejects");
}
#endif
