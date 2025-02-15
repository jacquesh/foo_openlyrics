#include "stdafx.h"
#include <cctype>

#include "pugixml.hpp"

#include "logging.h"
#include "lyric_source.h"
#include "mvtf/mvtf.h"
#include "tag_util.h"

static const GUID src_guid = { 0x5153d050, 0x387d, 0x47ec, { 0x93, 0x50, 0xd5, 0x3, 0xf2, 0x7b, 0x6c, 0x65 } };

class SonglyricsSource : public LyricSourceRemote
{
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("SongLyrics.com"); }

    std::vector<LyricDataRaw> search(const LyricSearchParams& params, abort_callback& abort) final;
    bool lookup(LyricDataRaw& data, abort_callback& abort) final;
};
static const LyricSourceFactory<SonglyricsSource> src_factory;

static std::string remove_chars_for_url(const std::string_view input)
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
        else if((c == ' ') || (c == '-'))
        {
            output += '-';
        }
        else if(c == '&')
        {
            output += "and";
        }
        else if(c == '@')
        {
            output += "at";
        }
    }

    return output;
}

static bool is_text_placeholder(std::string_view lyrics_text, const LyricSearchParams& params)
{
    const std::string not_found_text = std::format("We do not have the lyrics for {} yet.", params.title);
    const int edit_distance = string_edit_distance(lyrics_text, not_found_text);
    return (edit_distance < 5);
}

std::vector<LyricDataRaw> SonglyricsSource::search(const LyricSearchParams& params, abort_callback& abort)
{
    auto request = http_client::get()->create_request("GET");

    std::string url = "https://songlyrics.com/";
    url += remove_chars_for_url(params.artist);
    url += '/';
    url += remove_chars_for_url(params.title);
    url += "-lyrics";

    pfc::string8 content;
    try
    {
        file_ptr response_file = request->run(url.c_str(), abort);
        response_file->read_string_raw(content, abort);
        // NOTE: We're assuming here that the response is encoded in UTF-8 
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to download genius.com page %s: %s", url.c_str(), e.what());
        return {};
    }

    LOG_INFO("Page %s retrieved", url.c_str());
    std::string lyric_text;
    pugi::xml_document doc;
    load_html_document(content.c_str(), doc);

    const pugi::xpath_query query_lyricdivs("//p[@id='songLyricsDiv']");
    const pugi::xpath_node_set lyricdivs = query_lyricdivs.evaluate_node_set(doc);
    add_all_text_to_string(lyric_text, lyricdivs.first().node());
    if(!lyric_text.empty())
    {
        // A paragraph is a block element, which means that by definition
        // it effectively includes a trailing line-break.
        // We won't get that line-break by parsing the HTML text content,
        // so add it here manually.
        lyric_text += "\r\n";
    }

    if(lyric_text.empty())
    {
        throw new std::runtime_error("Failed to parse lyrics, the page format may have changed");
    }
    else
    {
        LOG_INFO("Successfully retrieved lyrics from %s", url.c_str());
        const std::string_view trimmed_text = trim_surrounding_whitespace(lyric_text);

        const bool is_placeholder = is_text_placeholder(trimmed_text, params);
        std::vector<LyricDataRaw> result_list;
        if(!is_placeholder)
        {
            LyricDataRaw result = {};
            result.source_id = id();
            result.source_path = url;
            result.artist = params.artist;
            result.album = params.album;
            result.title = params.title;
            result.type = LyricType::Unsynced;
            result.text_bytes = string_to_raw_bytes(trimmed_text);
            result_list.push_back(std::move(result));
        }
        return result_list;
    }
}

bool SonglyricsSource::lookup(LyricDataRaw& /*data*/, abort_callback& /*abort*/)
{
    LOG_ERROR("We should never need to do a lookup of the %s source", friendly_name().data());
    assert(false);
    return false;
}

// ============
// Tests
// ============
#if MVTF_TESTS_ENABLED
MVTF_TEST(songlyrics_regular_placeholder_text_is_detected_as_such)
{
    const std::string_view text = "We do not have the lyrics for Wishes ft Sophie Barker yet.";
    LyricSearchParams params("Rena Jones", "Echoes", "Wishes ft Sophie Barker", {});
    ASSERT(is_text_placeholder(text, params));
}

MVTF_TEST(songlyrics_placeholder_is_detected_as_such_when_title_contains_special_characters)
{
    // Note the non-ASCII unicode character used here for the apostrophe in "Ain't True"
    const std::string_view text = "We do not have the lyrics for Ain’t True yet.";
    LyricSearchParams params("All Good Funk Alliance", "Jack Of All Trades (Remixed Volume 3)", "Ain't True", {});
    ASSERT(is_text_placeholder(text, params));
}

MVTF_TEST(songlyrics_the_start_of_regular_lyrics_text_is_not_detected_as_a_placeholder)
{
    const std::string_view text = "Sum 41\nMiscellaneous\nHell Song\n...";
    LyricSearchParams params("Sum 41", "Does This Look Infected?", "Hell Song", {});
    ASSERT(!is_text_placeholder(text, params));
}
#endif
