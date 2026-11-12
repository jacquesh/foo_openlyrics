#include "stdafx.h"

#include "pugixml.hpp"
#include "tidy.h"
#include "tidybuffio.h"

#include "logging.h"
#include "lyric_source.h"
#include "tag_util.h"

LyricSearchParams::LyricSearchParams(const metadb_v2_rec_t& track_info)
{
    artist = track_metadata(track_info, "artist");
    album = track_metadata(track_info, "album");
    title = track_metadata(track_info, "title");
    duration_sec = track_duration_in_seconds(track_info);

    if(preferences::searching::exclude_trailing_brackets())
    {
        artist = trim_surrounding_whitespace(trim_trailing_text_in_brackets(artist));
        album = trim_surrounding_whitespace(trim_trailing_text_in_brackets(album));
        title = trim_surrounding_whitespace(trim_trailing_text_in_brackets(title));
    }
}

LyricSearchParams::LyricSearchParams(std::string in_artist,
                                     std::string in_album,
                                     std::string in_title,
                                     std::optional<int> in_duration_sec)
    : artist(std::move(in_artist))
    , album(std::move(in_album))
    , title(std::move(in_title))
    , duration_sec(in_duration_sec)
{
}

static const GUID dummy_src_guid = { 0x2c219517, 0x2c9, 0x494e, { 0xa5, 0x6f, 0x21, 0x65, 0x5e, 0xf2, 0x35, 0x64 } };
class DummyLyricSource : public LyricSourceBase
{
public:
    const GUID& id() const final
    {
        return dummy_src_guid;
    }

    std::tstring_view friendly_name() const final
    {
        return _T("<UNKNOWN SOURCE>");
    }

    bool is_local() const final
    {
        return true;
    }
    bool is_dummy() const final
    {
        return true;
    }

    std::vector<LyricDataRaw> search(metadb_handle_ptr track,
                                     const metadb_v2_rec_t& track_info,
                                     abort_callback& abort) final
    {
        return {};
    }

    bool lookup(LyricDataRaw& data, abort_callback& abort) final
    {
        return false;
    }

    std::string save(metadb_handle_ptr track,
                     const metadb_v2_rec_t& track_info,
                     bool is_timestamped,
                     std::string_view lyrics,
                     bool allow_overwrite,
                     abort_callback& abort) final
    {
        return "";
    }
    bool delete_persisted(metadb_handle_ptr track, const std::string& path) final
    {
        return false;
    }

    std::tstring get_file_path(metadb_handle_ptr track, const LyricData& lyrics) final
    {
        return _T("");
    }
};
static LyricSourceFactory<DummyLyricSource> dummy_src_factory;

static std::vector<LyricSourceBase*> g_lyric_sources;

LyricSourceBase& LyricSourceBase::get(GUID id)
{
    for(LyricSourceBase* src : g_lyric_sources)
    {
        if(src->id() == id)
        {
            return *src;
        }
    }

    return dummy_src_factory.get_static_instance();
}

std::vector<GUID> LyricSourceBase::get_all_ids()
{
    std::vector<GUID> result;
    result.reserve(g_lyric_sources.size());
    for(LyricSourceBase* src : g_lyric_sources)
    {
        if(src->id() == dummy_src_guid)
        {
            continue; // We don't ever want to advertise the dummy source
        }
        result.push_back(src->id());
    }
    return result;
}

void LyricSourceBase::on_init()
{
    g_lyric_sources.push_back(this);
}

std::string LyricSourceRemote::urlencode(std::string_view input)
{
    size_t inlen = input.length();
    std::string result;
    result.reserve(inlen * 3);

    for(size_t i = 0; i < inlen; i++)
    {
        if(pfc::char_is_ascii_alphanumeric(input[i]) || (input[i] == '-') || (input[i] == '_') || (input[i] == '.')
           || (input[i] == '~'))
        {
            result += input[i];
        }
        else if(input[i] == ' ')
        {
            result += "%20";
        }
        else
        {
            const auto nibble_to_hex = [](char c)
            {
                static char hex[] = "0123456789ABCDEF";
                return hex[c & 0xF];
            };

            char hi_nibble = ((input[i] >> 4) & 0xF);
            char lo_nibble = ((input[i] >> 0) & 0xF);

            result += '%';
            result += nibble_to_hex(hi_nibble);
            result += nibble_to_hex(lo_nibble);
        }
    }

    return result;
}

std::vector<uint8_t> LyricSourceBase::string_to_raw_bytes(std::string_view str)
{
    return std::vector((uint8_t*)str.data(), (uint8_t*)str.data() + str.length());
}

bool LyricSourceRemote::is_local() const
{
    return false;
}

bool LyricSourceRemote::is_dummy() const
{
    return false;
}

std::vector<LyricDataRaw> LyricSourceRemote::search(metadb_handle_ptr /*track*/,
                                                    const metadb_v2_rec_t& track_info,
                                                    abort_callback& abort)
{
    const LyricSearchParams params(track_info);
    return search(params, abort);
}

std::string LyricSourceRemote::save(metadb_handle_ptr /*track*/,
                                    const metadb_v2_rec_t& /*track_info*/,
                                    bool /*is_timestamped*/,
                                    std::string_view /*lyrics*/,
                                    bool /*allow_ovewrite*/,
                                    abort_callback& /*abort*/)
{
    LOG_WARN("Cannot save lyrics to a remote source");
    assert(false);
    return "";
}

bool LyricSourceRemote::delete_persisted(metadb_handle_ptr /*track*/, const std::string& /*path*/)
{
    LOG_WARN("Cannot delete lyrics from a remote source");
    assert(false);
    return false;
}

std::tstring LyricSourceRemote::get_file_path(metadb_handle_ptr /*track*/, const LyricData& /*lyrics*/)
{
    LOG_WARN("Cannot get file path for lyrics on a remote source");
    assert(false);
    return _T("");
}

bool LyricSourceRemote::supports_upload() const
{
    return false;
}

void LyricSourceRemote::upload(LyricData /*lyrics*/, abort_callback& /*abort*/)
{
    LOG_WARN("Cannot upload to a generic remote source (that doesn't support upload)");
    assert(false);
}

void LyricSourceRemote::load_html_document(const char* html, pugi::xml_document& doc) const
{
    std::string lyric_text;
    TidyBuffer tidy_output = {};
    TidyBuffer tidy_error = {};

    // NOTE: TidyHtml will output debug messages to stderr by default on Windows, unless we define
    //       `DISABLE_DEBUG_LOG` when compiling the TidyHtml project.
    TidyDoc tidy_doc = tidyCreate();
    tidySetErrorBuffer(tidy_doc, &tidy_error);
    tidyOptSetBool(tidy_doc, TidyXhtmlOut, yes);
    tidyOptSetBool(tidy_doc, TidyForceOutput, yes);
    tidyParseString(tidy_doc, html);
    tidyCleanAndRepair(tidy_doc);
    tidyRunDiagnostics(tidy_doc);
    tidySaveBuffer(tidy_doc, &tidy_output);

    if(tidyErrorCount(tidy_doc) != 0)
    {
        tidyErrorSummary(tidy_doc); // Write more complete error info to the error_buffer
        const std::string source_name = from_tstring(friendly_name());
        LOG_INFO("Failed to cleanly convert retrieved HTML from %s to XHTML:\n%s", source_name.c_str(), tidy_error.bp);
    }

    doc.load_buffer(tidy_output.bp, tidy_output.size);

    tidyBufFree(&tidy_output);
    tidyBufFree(&tidy_error);
    tidyRelease(tidy_doc);
}

void LyricSourceRemote::add_all_text_to_string(std::string& output, const pugi::xml_node& node)
{
    // add_all_text_to_string_internal(output, pugi::xml_node(node));
    switch(node.type())
    {
        case pugi::node_null:
        case pugi::node_comment:
        case pugi::node_pi:
        case pugi::node_declaration:
        case pugi::node_doctype: return;

        case pugi::node_document:
        case pugi::node_element:
        {
            if(std::string_view(node.name()) == "br")
            {
                assert(node.children().empty());
                output += "\r\n";
                break;
            }

            for(pugi::xml_node child : node.children())
            {
                add_all_text_to_string(output, child);
            }
        }
        break;

        case pugi::node_pcdata:
        case pugi::node_cdata:
        {
            // We assume the text is already UTF-8

            // Trim surrounding line-endings to get rid of the newlines in the HTML that don't affect rendering
            std::string node_text(trim_surrounding_line_endings(node.value()));

            // Sometimes tidyHtml inserts newlines in the middle of a line where there should just be a space.
            // Get rid of any carriage returns (in case they were added) and then replace
            // newlines in the middle of the text with spaces.
            node_text.erase(std::remove(node_text.begin(), node_text.end(), '\r'));
            std::replace(node_text.begin(), node_text.end(), '\n', ' ');

            output += node_text;
        }
        break;
    }
}
