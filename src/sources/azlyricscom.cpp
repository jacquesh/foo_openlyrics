#include "stdafx.h"
#include <cctype>

#include "tidy.h"
#include "tidybuffio.h"
#include "pugixml.hpp"

#include "logging.h"
#include "lyric_data.h"
#include "lyric_source.h"
#include "tag_util.h"

static const GUID src_guid = { 0xadf3a1ba, 0x7e88, 0x4539, { 0xaf, 0x9e, 0xa8, 0xc4, 0xbc, 0x62, 0x98, 0xf1 } };

class AZLyricsComSource : public LyricSourceRemote
{
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("AZLyrics.com"); }

    std::vector<LyricDataRaw> search(const LyricSearchParams& params, abort_callback& abort) final;
    bool lookup(LyricDataRaw& data, abort_callback& abort) final;
};
static const LyricSourceFactory<AZLyricsComSource> src_factory;

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

std::vector<LyricDataRaw> AZLyricsComSource::search(const LyricSearchParams& params, abort_callback& abort)
{
    // NOTE: It seems that if we let the user-agent indicate a browser that is sufficiently far out of date, we get served a captcha.
    //       Firefox has a published release schedule (https://wiki.mozilla.org/Release_Management/Calendar) and there's a new
    //       release roughly once a month, so we can fake this being updated by just bumping our version number once a month.
    // NOTE: Until C++20 there is no concept of a "day", "month" or "year" in std::chrono.
    //       Really what we want is the number of months since 2022-08-30 but instead we
    //       need to approximate that using hours.
    std::tm useragent_epoch = {};
    useragent_epoch.tm_year = 2022 - 1900;
    useragent_epoch.tm_mon = 8 - 1;
    useragent_epoch.tm_mday = 30;
    useragent_epoch.tm_isdst = -1;
    std::time_t useragent_epoch_time = std::mktime(&useragent_epoch);
    const auto useragent_epoch_time_point = std::chrono::system_clock::from_time_t(useragent_epoch_time);
    const auto now = std::chrono::system_clock::now();
    const int64_t hours_since_epoch = std::chrono::duration_cast<std::chrono::hours>(now - useragent_epoch_time_point).count();
    const int64_t hours_per_month = 24*30;
    const int64_t months_since_epoch = hours_since_epoch / hours_per_month;
    const int64_t firefox_version = 103 + months_since_epoch; // 103 is the version of Firefox that was current as of our epoch above (2022-07-30)
    char useragent[128] = {};
    snprintf(useragent, sizeof(useragent), "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:%lld.0) Gecko/20100101 Firefox/%lld.0", firefox_version, firefox_version);

    http_request::ptr request = http_client::get()->create_request("GET");
    request->add_header("User-Agent", useragent);

    std::string url_artist = remove_chars_for_url(params.artist);
    std::string url_title = remove_chars_for_url(params.title);
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

    std::string lyric_text;
    TidyBuffer tidy_output = {};
    TidyBuffer tidy_error = {};

    TidyDoc tidy_doc = tidyCreate();
    tidySetErrorBuffer(tidy_doc, &tidy_error);
    tidyOptSetBool(tidy_doc, TidyXhtmlOut, yes);
    tidyOptSetBool(tidy_doc, TidyForceOutput, yes);
    tidyParseString(tidy_doc, content.c_str());
    tidyCleanAndRepair(tidy_doc);
    tidyRunDiagnostics(tidy_doc);
    tidySaveBuffer(tidy_doc, &tidy_output);

    if(tidyErrorCount(tidy_doc) == 0)
    {
        pugi::xml_document doc;
        doc.load_buffer(tidy_output.bp, tidy_output.size);

        pugi::xpath_query query_lyricdivs("//div[@class='lyricsh']");
        pugi::xpath_node_set lyricdivs = query_lyricdivs.evaluate_node_set(doc);
        if(!lyricdivs.empty())
        {
            pugi::xml_node target_node;
            for(const pugi::xpath_node& node : lyricdivs)
            {
                if(node.node().type() == pugi::node_element)
                {
                    target_node = node.node();
                    break;
                }
            }
            while(target_node.type() != pugi::node_null)
            {
                const auto attr_is_class = [](const pugi::xml_attribute& attr) { return strcmp(attr.name(), "class") == 0; };
                bool is_div = (strcmp(target_node.name(), "div") == 0);
                bool has_class = std::find_if(target_node.attributes_begin(), target_node.attributes_end(), attr_is_class) != target_node.attributes_end();
                if(is_div && !has_class)
                {
                    break;
                }

                target_node = target_node.next_sibling();
            }

            add_all_text_to_string(lyric_text, target_node);
        }
        else
        {
            LOG_INFO("No appropriate lyric header divs found on page: %s", url.c_str());
        }
    }
    else
    {
        tidyErrorSummary(tidy_doc); // Write more complete error info to the error_buffer
        LOG_INFO("Failed to convert retrieved HTML from %s to XHTML:\n%s", url.c_str(), tidy_error.bp);
    }

    tidyBufFree(&tidy_output);
    tidyBufFree(&tidy_error);
    tidyRelease(tidy_doc);

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
        result.title = params.title;
        result.type = LyricType::Unsynced;
        result.text_bytes = string_to_raw_bytes(trimmed_text);
        return {std::move(result)};
    }
}

bool AZLyricsComSource::lookup(LyricDataRaw& /*data*/, abort_callback& /*abort*/)
{
    LOG_ERROR("We should never need to do a lookup of the %s source", friendly_name().data());
    assert(false);
    return false;
}
