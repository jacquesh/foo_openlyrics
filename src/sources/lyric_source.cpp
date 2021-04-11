#include "stdafx.h"

#include "logging.h"
#include "lyric_source.h"

static std::vector<LyricSourceBase*> g_lyric_sources;

LyricSourceBase* LyricSourceBase::get(GUID id)
{
    for(LyricSourceBase* src : g_lyric_sources)
    {
        if(src->id() == id)
        {
            return src;
        }
    }

    return nullptr;
}

std::vector<GUID> LyricSourceBase::get_all_ids()
{
    std::vector<GUID> result;
    result.reserve(g_lyric_sources.size());
    for(LyricSourceBase* src : g_lyric_sources)
    {
        result.push_back(src->id());
    }
    return result;
}

void LyricSourceBase::on_init()
{
    g_lyric_sources.push_back(this);
}

const char* LyricSourceBase::get_artist(metadb_handle_ptr track) const
{
    const metadb_info_container::ptr& track_info_container = track->get_info_ref();
    const file_info& track_info = track_info_container->info();

    size_t meta_index = track_info.meta_find("artist");
    if((meta_index != pfc::infinite_size) && (track_info.meta_enum_value_count(meta_index) > 0))
    {
        return track_info.meta_enum_value(meta_index, 0);
    }

    return "";
}

const char* LyricSourceBase::get_album(metadb_handle_ptr track) const
{
    const metadb_info_container::ptr& track_info_container = track->get_info_ref();
    const file_info& track_info = track_info_container->info();

    size_t meta_index = track_info.meta_find("album");
    if((meta_index != pfc::infinite_size) && (track_info.meta_enum_value_count(meta_index) > 0))
    {
        return track_info.meta_enum_value(meta_index, 0);
    }

    return "";
}

const char* LyricSourceBase::get_title(metadb_handle_ptr track) const
{
    const metadb_info_container::ptr& track_info_container = track->get_info_ref();
    const file_info& track_info = track_info_container->info();

    size_t meta_index = track_info.meta_find("title");
    if((meta_index != pfc::infinite_size) && (track_info.meta_enum_value_count(meta_index) > 0))
    {
        return track_info.meta_enum_value(meta_index, 0);
    }

    return "";
}

std::string LyricSourceBase::trim_surrounding_whitespace(const char* str) const
{
    size_t len = strlen(str);
    size_t leading_chars_to_remove = 0;
    while((leading_chars_to_remove < len) &&
            ((str[leading_chars_to_remove] == '\r') ||
             (str[leading_chars_to_remove] == '\n') ||
             (str[leading_chars_to_remove] == ' ')))
    {
        leading_chars_to_remove++;
    }

    size_t trailing_chars_to_remove = 0;
    while((leading_chars_to_remove + trailing_chars_to_remove < len) &&
            ((str[len - trailing_chars_to_remove] == '\r') ||
             (str[len - trailing_chars_to_remove] == '\n') ||
             (str[len - trailing_chars_to_remove] == ' ')))
    {
        trailing_chars_to_remove++;
    }

    size_t total_to_remove = leading_chars_to_remove + trailing_chars_to_remove;
    assert(total_to_remove <= len);

    size_t result_len = len - total_to_remove;
    std::string result(str + leading_chars_to_remove, result_len);
    return result;
}

bool LyricSourceRemote::can_save() const
{
    return false;
}

std::string LyricSourceRemote::save(metadb_handle_ptr /*track*/, bool /*is_timestamped*/, std::string_view /*lyrics*/, abort_callback& /*abort*/)
{
    LOG_WARN("Cannot save lyrics to a remote source");
    assert(false);
    return "";
}
