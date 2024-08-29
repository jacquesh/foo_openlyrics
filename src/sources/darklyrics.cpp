#include "stdafx.h"
#include <cctype>

#include "pugixml.hpp"

#include "logging.h"
#include "lyric_source.h"
#include "mvtf/mvtf.h"
#include "tag_util.h"

static const GUID src_guid = { 0x5901c128, 0xc67f, 0x4eec, { 0x8f, 0x10, 0x47, 0x5d, 0x12, 0x52, 0x89, 0xe9 } };

class DarkLyricsSource : public LyricSourceRemote
{
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("DarkLyrics.com"); }

    void add_subsection_text_to_string(std::string& output, pugi::xml_node node) const;
    std::vector<LyricDataRaw> search(const LyricSearchParams& params, abort_callback& abort) final;
    bool lookup(LyricDataRaw& data, abort_callback& abort) final;
};
static const LyricSourceFactory<DarkLyricsSource> src_factory;

static std::string remove_chars_for_url(const std::string_view input)
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

void DarkLyricsSource::add_subsection_text_to_string(std::string& output, pugi::xml_node node) const
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
            else if((current_name == "h3") || (current_name == "div"))
            {
                break;
            }
            else
            {
                add_subsection_text_to_string(output, current.first_child());
            }
        }

        current = current.next_sibling();
    }
}

std::vector<LyricDataRaw> DarkLyricsSource::search(const LyricSearchParams& params, abort_callback& abort)
{
    http_request::ptr request = http_client::get()->create_request("GET");

    const std::string url_artist = remove_chars_for_url(params.artist);
    const std::string url_album = remove_chars_for_url(params.album);
    const std::string url_title = remove_chars_for_url(params.title);
    const std::string url = "http://darklyrics.com/lyrics/" + url_artist + "/" + url_album + ".html";
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
        LOG_WARN("Failed to download darklyrics.com page %s: %s", url.c_str(), e.what());
        return {};
    }

    std::string lyric_text;
    pugi::xml_document doc;
    load_html_document(content.c_str(), doc);

    pugi::xpath_query query_lyricdivs("//div[@class='lyrics']/h3/a[@name]");
    pugi::xpath_node_set lyricdivs = query_lyricdivs.evaluate_node_set(doc);
    if(!lyricdivs.empty())
    {
        for(const pugi::xpath_node& node : lyricdivs)
        {
            if(node.node().type() != pugi::node_element) continue;
            if(node.node().first_child().type() != pugi::node_pcdata) continue;

            std::string_view title_text = node.node().first_child().value();
            size_t title_dot_index = title_text.find('.');

            if(title_dot_index == std::string_view::npos) continue;

            title_text.remove_prefix(title_dot_index + 1); // +1 to include the '.' that we found
            title_text = trim_surrounding_whitespace(title_text);
            if(!tag_values_match(title_text, params.title))
            {
                continue;
            }

            if(node.parent().type() != pugi::node_null)
            {
                add_subsection_text_to_string(lyric_text, node.parent().next_sibling());
                break;
            }
        }
    }

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
        result.type = LyricType::Unsynced;
        result.text_bytes = string_to_raw_bytes(trimmed_text);
        return {std::move(result)};
    }
}

bool DarkLyricsSource::lookup(LyricDataRaw& /*data*/, abort_callback& /*abort*/)
{
    LOG_ERROR("We should never need to do a lookup of the %s source", friendly_name().data());
    assert(false);
    return false;
}

// ============
// Tests
// ============
#ifdef MVTF_TESTS_ENABLED
MVTF_TEST(darklyrics_known_bad_chars_removed_from_url)
{
    const std::string result = remove_chars_for_url("?!@#asd!@#qwe[]'_zxc-+><");
    ASSERT(result == "asdqwezxc");
}
#endif
