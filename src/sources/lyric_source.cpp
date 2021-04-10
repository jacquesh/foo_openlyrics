#include "stdafx.h"

#include "logging.h"
#include "lyric_source.h"
#include "parsers.h"
#include "winstr_util.h" // Included (temporarily?) for tchar_to_string used in the save() call

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

bool LyricSourceRemote::can_save() const
{
    return false;
}

void LyricSourceRemote::save(metadb_handle_ptr /*track*/, bool /*is_timestamped*/, std::string_view /*lyrics*/, abort_callback& abort)
{
    LOG_WARN("Cannot save lyrics to a remote source");
    assert(false);
}

// TODO: This really should be somewhere else
void sources::SaveLyrics(metadb_handle_ptr track, const LyricData& lyrics, abort_callback& abort)
{
    // NOTE: We require that saving happens on the main thread because the ID3 tag updates can
    //       only happen on the main thread.
    core_api::ensure_main_thread();

    // TODO: These are copied from their definitions in the source file. Find another way.
    const GUID localfiles_guid = { 0x76d90970, 0x1c98, 0x4fe2, { 0x94, 0x4e, 0xac, 0xe4, 0x93, 0xf3, 0x8e, 0x85 } };
    const GUID id3tag_guid = { 0x3fb0f715, 0xa097, 0x493a, { 0x94, 0x4e, 0xdb, 0x48, 0x66, 0x8, 0x86, 0x78 } };

    LyricSourceBase* source = nullptr;
    SaveMethod method = preferences::saving::save_method();
    if(method == SaveMethod::ConfigDirectory)
    {
        // TODO: This save triggers an immediate reload from the directory watcher.
        //       This is not *necessarily* a problem, but it is some unnecessary work
        //       and it means that we immediately lose the source information for
        //       downloaded lyrics.
        source = LyricSourceBase::get(localfiles_guid);
    }
    else if(method == SaveMethod::Id3Tag)
    {
        source = LyricSourceBase::get(id3tag_guid);
    }
    else
    {
        // Not configured to save at all
    }

    if(source != nullptr)
    {
        std::string text;
        if(lyrics.IsTimestamped())
        {
            text = parsers::lrc::shrink_text(lyrics);
        }
        else
        {
            text = lyrics.text;
        }

        try
        {
            source->save(track, lyrics.IsTimestamped(), text, abort);
        }
        catch(const std::exception& e)
        {
            std::string source_name = tchar_to_string(source->friendly_name());
            LOG_ERROR("Failed to save lyrics to %s: %s", source_name.c_str(), e.what());
        }
    }
}

