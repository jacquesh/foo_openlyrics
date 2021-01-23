#include "stdafx.h"

#include "lyric_source.h"

static const size_t MAX_SOURCE_COUNT = 64;
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
    // t_filetimestamp track_timestamp = track_info_container->stats().m_timestamp; // TODO: This could be useful for setting a cached timestamp to not reload lyrics all the time? Oh but we need to get this for the lyrics file, not the track itself... although I guess if the lyrics are stored in an id3 tag?

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

pfc::string8 LyricSourceBase::trim_surrounding_whitespace(const char* str) const
{
    int leading_chars_to_remove = 0;
    while((str[leading_chars_to_remove] == '\r') ||
          (str[leading_chars_to_remove] == '\n') ||
          (str[leading_chars_to_remove] == ' '))
    {
        leading_chars_to_remove++;
    }

    pfc::string8 result(str + leading_chars_to_remove);
    result.skip_trailing_chars("\r\n ");
    return result;
}
