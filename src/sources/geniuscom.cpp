#include "stdafx.h"

#include "libxml/HTMLparser.h"
#include "libxml/tree.h"
#include "libxml/xpath.h"

#include "logging.h"
#include "lyric_source.h"

static const GUID src_guid = { 0xb4cf497f, 0xd2c, 0x45ff, { 0xaa, 0x46, 0xf1, 0x45, 0xa7, 0xf, 0x90, 0x14 } };

class GeniusComSource : public LyricSourceBase
{
    const GUID& id() const final { return src_guid; }
    const TCHAR* friendly_name() const final { return _T("genius.com"); }
    bool is_local() const final { return false; }

    void add_all_text_to_string(pfc::string8& output, xmlNodePtr node) const;
    LyricDataRaw query(metadb_handle_ptr track, abort_callback& abort) final;
};
static const LyricSourceFactory<GeniusComSource> src_factory;

static pfc::string8 remove_chars_for_url(const pfc::string8& input)
{
    pfc::string8 output;
    output.prealloc(input.length() + 3); // We add a bit to allow for one or two & or @ replacements without re-allocation
    for(size_t i=0; i<input.length(); i++)
    {
        if(pfc::char_is_ascii_alphanumeric(input[i]))
        {
            pfc::stringToLowerAppend(output, &input[i], 1);
        }
        else if((input[i] == ' ') || (input[i] == '-'))
        {
            output.add_char('-');
        }
        else if(input[i] == '&')
        {
            output.add_string("and");
        }
        else if(input[i] == '@')
        {
            output.add_string("at");
        }
    }

    return output;
}

void GeniusComSource::add_all_text_to_string(pfc::string8& output, xmlNodePtr node) const
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
            pfc::string8 node_text = trim_surrounding_whitespace((char*)child->content);
            output.add_string(node_text);
            output.add_string("\r\n");
        }
        else if(child->type == XML_ELEMENT_NODE)
        {
            add_all_text_to_string(output, child);
            xmlNode* grandchild = child->children;
        }

        child = child->next;
    }
}

LyricDataRaw GeniusComSource::query(metadb_handle_ptr track, abort_callback& abort)
{
    abort_callback_dummy noAbort;
    auto request = http_client::get()->create_request("GET");

    pfc::string8 url = "https://genius.com/";
    url.add_string(remove_chars_for_url(get_artist(track)));
    url.add_char('-');
    url.add_string(remove_chars_for_url(get_title(track)));
    url.add_string("-lyrics");

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

        pfc::string8 lyric_text;
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

        if(lyric_text.is_empty())
        {
            throw std::runtime_error("Failed to parse lyrics, the page format may have changed");
        }
        else
        {
            LOG_INFO("Successfully retrieved lyrics from %s", url.c_str());
            LyricDataRaw result = {};
            result.source_id = id();
            result.format = LyricFormat::Plaintext;
            result.text = std::move(lyric_text);
            return result;
        }
    }
    else
    {
        LOG_WARN("Failed to parse HTML response from %s", url.c_str());
    }

    return {};
}
