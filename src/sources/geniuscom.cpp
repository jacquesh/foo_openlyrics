#include "stdafx.h"
#include <cctype>

#include "pugixml.hpp"

#include "logging.h"
#include "lyric_source.h"
#include "tag_util.h"

static const GUID src_guid = { 0xb4cf497f, 0xd2c, 0x45ff, { 0xaa, 0x46, 0xf1, 0x45, 0xa7, 0xf, 0x90, 0x14 } };

class GeniusComSource : public LyricSourceRemote
{
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("Genius.com"); }

    std::vector<LyricDataRaw> search(const LyricSearchParams& params, abort_callback& abort) final;
    bool lookup(LyricDataRaw& data, abort_callback& abort) final;
};
static const LyricSourceFactory<GeniusComSource> src_factory;

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

std::vector<LyricDataRaw> GeniusComSource::search(const LyricSearchParams& params, abort_callback& abort)
{
    auto request = http_client::get()->create_request("GET");

    std::string url = "https://genius.com/";
    url += remove_chars_for_url(params.artist);
    url += '-';
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

    const char* xpath_queries[] = { "//div[@class='lyrics']", "//div[contains(@class, 'Lyrics__Container')]" };
    for(const char* query_str : xpath_queries)
    {
        pugi::xpath_query query_lyricdivs(query_str);
        pugi::xpath_node_set lyricdivs = query_lyricdivs.evaluate_node_set(doc);

        if(!lyricdivs.empty())
        {
            for(const pugi::xpath_node& node : lyricdivs)
            {
                add_all_text_to_string(lyric_text, node.node());

                // A div is a block element, which means that by definition
                // it effectively includes a trailing line-break.
                // We won't get that line-break by parsing the HTML text content,
                // so add it here manually.
                lyric_text += "\r\n";
            }
            break;
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

bool GeniusComSource::lookup(LyricDataRaw& /*data*/, abort_callback& /*abort*/)
{
    LOG_ERROR("We should never need to do a lookup of the %s source", friendly_name().data());
    assert(false);
    return false;
}
