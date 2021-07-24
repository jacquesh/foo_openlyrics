#include "stdafx.h"
#include <cctype>

#include "libxml/HTMLparser.h"
#include "libxml/tree.h"
#include "libxml/xpath.h"

#include "logging.h"
#include "lyric_data.h"
#include "lyric_source.h"
#include "tag_util.h"

static const GUID src_guid = { 0xadf3a1ba, 0x7e88, 0x4539, { 0xaf, 0x9e, 0xa8, 0xc4, 0xbc, 0x62, 0x98, 0xf1 } };

class AZLyricsComSource : public LyricSourceRemote
{
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("azlyrics.com"); }

    std::vector<LyricDataRaw> search(std::string_view artist, std::string_view album, std::string_view title, abort_callback& abort) final;
    bool lookup(LyricDataRaw& data, abort_callback& abort) final;
};
static const LyricSourceFactory<AZLyricsComSource> src_factory;

static std::string remove_chars_for_url(const std::string_view input)
{
    std::string output;
    output.reserve(input.length());

    for(size_t i=0; i<input.length(); i++)
    {
        if(pfc::char_is_ascii_alphanumeric(input[i]))
        {
            output += static_cast<char>(std::tolower(static_cast<unsigned char>(input[i])));
        }
    }

    return output;
}

std::vector<LyricDataRaw> AZLyricsComSource::search(std::string_view artist, std::string_view album, std::string_view title, abort_callback& abort)
{
    http_request::ptr request = http_client::get()->create_request("GET");
    request->add_header("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:84.0) Gecko/20100101 Firefox/84.0");

    std::string url_artist = remove_chars_for_url(artist);
    std::string url_title = remove_chars_for_url(title);
    std::string url = "https://www.azlyrics.com/lyrics/" + url_artist + "/" + url_title + ".html";;
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
        LOG_WARN("Failed to download azlyrics.com page %s: %s", url.c_str(), e.what());
        return {};
    }

    htmlDocPtr doc = htmlReadMemory(content.c_str(), content.length(), url.c_str(), nullptr, 0);
    if(doc != nullptr)
    {
        xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(doc);
        if(xpath_ctx == nullptr)
        {
            xmlFreeDoc(doc);
            throw std::runtime_error("Failed to create xpath context");
        }

        xmlXPathObjectPtr xpath_obj = xmlXPathEvalExpression(BAD_CAST "//div[@class='lyricsh']", xpath_ctx);
        if(xpath_obj == nullptr)
        {
            xmlXPathFreeContext(xpath_ctx);
            xmlFreeDoc(doc);
            throw new std::runtime_error("Failed to create new XPath search expression");
        }

        xmlNodePtr target_node = nullptr;
        if((xpath_obj->nodesetval != nullptr) && (xpath_obj->nodesetval->nodeNr > 0))
        {
            int node_count = xpath_obj->nodesetval->nodeNr;
            for(int i=0; i<node_count; i++)
            {
                xmlNodePtr node = xpath_obj->nodesetval->nodeTab[i];
                if(node == nullptr)
                {
                    continue;
                }

                if(node->type == XML_ELEMENT_NODE)
                {
                    target_node = node;
                }
            }
        }

        while(target_node != nullptr)
        {
            bool is_div = (xmlStrEqual(target_node->name, BAD_CAST "div") != 0);
            bool has_class = xmlHasProp(target_node, BAD_CAST "class");
            if(is_div && !has_class)
            {
                break;
            }

            target_node = xmlNextElementSibling(target_node);
        }

        std::string lyric_text;
        if(target_node != nullptr)
        {
            xmlNode* child = target_node->children;
            while(child != nullptr)
            {
                if(child->type == XML_TEXT_NODE)
                {
                    // NOTE: libxml2 stores strings as UTF8 internally, so we don't need to do any conversion here
                    std::string_view line_text = trim_surrounding_whitespace((char*)child->content);
                    lyric_text += line_text;
                    lyric_text += "\r\n";
                }

                child = child->next;
            }
        }

        xmlXPathFreeObject(xpath_obj);
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(doc);

        if(lyric_text.empty())
        {
            throw new std::runtime_error("Failed to parse lyrics, the page format may have changed");
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

    return {};
}

bool AZLyricsComSource::lookup(LyricDataRaw& /*data*/, abort_callback& /*abort*/)
{
    LOG_ERROR("We should never need to do a lookup of the %s source", friendly_name().data());
    assert(false);
    return false;
}
