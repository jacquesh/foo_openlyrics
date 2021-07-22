#include "stdafx.h"
#include <cctype>

#include "libxml/HTMLparser.h"
#include "libxml/tree.h"
#include "libxml/xpath.h"

#include "logging.h"
#include "lyric_source.h"

static const GUID src_guid = { 0xb4cf497f, 0xd2c, 0x45ff, { 0xaa, 0x46, 0xf1, 0x45, 0xa7, 0xf, 0x90, 0x14 } };

class GeniusComSource : public LyricSourceRemote
{
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("genius.com"); }

    void add_all_text_to_string(std::string& output, xmlNodePtr node) const;
    std::vector<LyricDataRaw> search(std::string_view artist, std::string_view album, std::string_view title, abort_callback& abort) final;
    LyricDataRaw lookup(std::string_view lookup_id, abort_callback& abort) final;
};
static const LyricSourceFactory<GeniusComSource> src_factory;

static std::string remove_chars_for_url(const std::string_view input)
{
    std::string output;
    output.reserve(input.length() + 3); // We add a bit to allow for one or two & or @ replacements without re-allocation
    for(size_t i=0; i<input.length(); i++)
    {
        if(pfc::char_is_ascii_alphanumeric(input[i]))
        {
            output += static_cast<char>(std::tolower(static_cast<unsigned char>(input[i])));
        }
        else if((input[i] == ' ') || (input[i] == '-'))
        {
            output += '-';
        }
        else if(input[i] == '&')
        {
            output += "and";
        }
        else if(input[i] == '@')
        {
            output += "at";
        }
    }

    return output;
}

void GeniusComSource::add_all_text_to_string(std::string& output, xmlNodePtr node) const
{
    if((node == nullptr) || (node->type != XML_ELEMENT_NODE))
    {
        return;
    }

    xmlNode* child = node->children;
    while(child != nullptr)
    {
        if(child->type == XML_TEXT_NODE)
        {
            // NOTE: libxml2 stores strings as UTF8 internally, so we don't need to do any conversion here
            std::string_view node_text = trim_surrounding_whitespace((char*)child->content);
            output += node_text;
            output += "\r\n";
        }
        else if(child->type == XML_ELEMENT_NODE)
        {
            add_all_text_to_string(output, child);
        }

        child = child->next;
    }
}

std::vector<LyricDataRaw> GeniusComSource::search(std::string_view artist, std::string_view album, std::string_view title, abort_callback& abort)
{
    abort_callback_dummy noAbort;
    auto request = http_client::get()->create_request("GET");

    std::string url = "https://genius.com/";
    url += remove_chars_for_url(artist);
    url += '-';
    url += remove_chars_for_url(title);
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
    htmlDocPtr doc = htmlReadMemory(content.c_str(), content.length(), url.c_str(), nullptr, 0);
    if(doc != nullptr)
    {
        xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(doc);
        if(xpath_ctx == nullptr)
        {
            xmlFreeDoc(doc);
            throw std::runtime_error("Failed to create xpath context");
        }

        xmlXPathObjectPtr xpath_obj = xmlXPathEvalExpression(BAD_CAST "//div[@class='lyrics']", xpath_ctx);
        if(xpath_obj == nullptr)
        {
            xmlXPathFreeContext(xpath_ctx);
            xmlFreeDoc(doc);
            throw new std::runtime_error("Failed to create new XPath search expression");
        }

        const char* xpath_queries[] = { "//div[@class='lyrics']", "//div[contains(@class, 'Lyrics__Container')]" };
        for(const char* query_str : xpath_queries)
        {
            xpath_obj = xmlXPathEvalExpression(BAD_CAST query_str, xpath_ctx);
            if(xpath_obj == nullptr)
            {
                xmlXPathFreeContext(xpath_ctx);
                xmlFreeDoc(doc);
                throw new std::runtime_error("Failed to create new XPath search expression");
            }

            if((xpath_obj->nodesetval != nullptr) && (xpath_obj->nodesetval->nodeNr > 0))
            {
                LOG_INFO("Succeeded with xpath query: %s", query_str);
                break;
            }
        }

        std::string lyric_text;
        if((xpath_obj->nodesetval != nullptr) && (xpath_obj->nodesetval->nodeNr > 0))
        {
            int node_count = xpath_obj->nodesetval->nodeNr;
            for(int i=0; i<node_count; i++)
            {
                xmlNodePtr node = xpath_obj->nodesetval->nodeTab[i];
                add_all_text_to_string(lyric_text, node);
            }
        }

        xmlXPathFreeObject(xpath_obj);
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(doc);

        if(lyric_text.empty())
        {
            throw std::runtime_error("Failed to parse lyrics, the page format may have changed");
        }
        else
        {
            LOG_INFO("Successfully retrieved lyrics from %s", url.c_str());
            LyricDataRaw result = {};
            result.source_id = id();
            result.persistent_storage_path = url;
            result.artist = artist;
            result.title = title;
            result.text = trim_surrounding_whitespace(lyric_text);
            return {std::move(result)};
        }
    }
    else
    {
        LOG_WARN("Failed to parse HTML response from %s", url.c_str());
    }

    return {};
}

LyricDataRaw GeniusComSource::lookup(std::string_view /*lookup_id*/, abort_callback& /*abort*/)
{
    LOG_ERROR("We should never need to do a lookup of the %s source", friendly_name().data());
    assert(false);
    return {};
}
