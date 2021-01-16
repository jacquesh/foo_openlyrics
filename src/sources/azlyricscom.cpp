#include "stdafx.h"

#include "libxml/HTMLparser.h"
#include "libxml/tree.h"
#include "libxml/xpath.h"

#include "logging.h"
#include "lyric_data.h"

namespace sources::azlyricscom
{
    LyricDataRaw Query(const pfc::string8& artist, const pfc::string8& album, const pfc::string8& title);
}

static pfc::string8 remove_chars_for_url(const pfc::string8& input)
{
    // TODO: This almost certainly does not work for non-ASCII characters
    pfc::string8 output;
    output.prealloc(input.length());

    for(size_t i=0; i<input.length(); i++)
    {
        if(pfc::char_is_ascii_alphanumeric(input[i]))
        {
            pfc::stringToLowerAppend(output, &input[i], 1);
        }
    }

    return output;
}

LyricDataRaw sources::azlyricscom::Query(const pfc::string8& artist, const pfc::string8& album, const pfc::string8& title)
{
    http_request::ptr request = http_client::get()->create_request("GET");
    request->add_header("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:84.0) Gecko/20100101 Firefox/84.0");

    pfc::string8 url_artist = std::move(remove_chars_for_url(artist));
    pfc::string8 url_title = std::move(remove_chars_for_url(title));
    pfc::string8 url = "https://www.azlyrics.com/lyrics/";
    url.add_string(url_artist);
    url.add_string("/");
    url.add_string(url_title);
    url.add_string(".html");
    LOG_INFO("Querying for lyrics from %s...", url.c_str());

    try
    {
        abort_callback_dummy noAbort; // TODO: What should this be instead?
        file_ptr response_file = request->run(url.c_str(), noAbort);

        pfc::string8 content;
        response_file->read_string_raw(content, noAbort);

        htmlDocPtr doc = htmlReadMemory(content.c_str(), content.length(), url.c_str(), nullptr, 0);
        if(doc != nullptr)
        {
            // TODO: Check for UTF-8 encoding and if its not, then re-encode it.
            //       Read: http://xmlsoft.org/tutorial/ar01s09.html
            //LOG_INFO("XML document encoding = %s", doc->encoding);
            //xmlCharEncodingHandlerPtr encoding_handler = xmlGetCharEncodingHandler(encoding);

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

            pfc::string8 lyric_text;
            if(target_node != nullptr)
            {
                xmlNode* child = target_node->children;
                while(child != nullptr)
                {
                    if(child->type == XML_TEXT_NODE)
                    {
                        int leading_chars_to_remove = 0;
                        while((child->content[leading_chars_to_remove] == '\r') ||
                              (child->content[leading_chars_to_remove] == '\n') ||
                              (child->content[leading_chars_to_remove] == ' '))
                        {
                            leading_chars_to_remove++;
                        }

                        if(child->content[leading_chars_to_remove] != '\0')
                        {
                            // TODO: Encoding? This cast should probably tell us something...
                            pfc::string8 line_text((char*)child->content + leading_chars_to_remove);
                            line_text.skip_trailing_chars("\r\n ");
                            lyric_text.add_string(line_text);
                        }
                        lyric_text.add_string("\r\n");
                    }

                    child = child->next;
                }
            }

            xmlXPathFreeObject(xpath_obj);
            xmlXPathFreeContext(xpath_ctx);
            xmlFreeDoc(doc);

            if(lyric_text.is_empty())
            {
                throw new std::runtime_error("Failed to parse lyrics, the page format may have changed");
            }
            else
            {
                LOG_INFO("Successfully retrieved lyrics from %s", url.c_str());
                LyricDataRaw result = {};
                result.format = LyricFormat::Plaintext;
                result.text = std::move(lyric_text);
                return result;
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to download azlyrics.com page %s: %s", url.c_str(), e.what());
    }

    return {};
}
