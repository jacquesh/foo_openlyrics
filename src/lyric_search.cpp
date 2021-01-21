#include "stdafx.h"

#include "logging.h"
#include "lyric_data.h"
#include "lyric_search.h"
#include "sources/lyric_source.h"
#include "sources/localfiles.h"

namespace parsers
{
namespace plaintext { LyricData parse(const LyricDataRaw& input); } // TODO: From parsers/plaintext.cpp
namespace lrc { LyricData parse(const LyricDataRaw& input); } // TODO: From parsers/lrc.cpp
}

// TODO: from sources/azlyricscom.cpp
namespace sources::azlyricscom
{
    LyricDataRaw Query(const pfc::string8& artist, const pfc::string8& album, const pfc::string8& title);
}

LyricSearch::LyricSearch(metadb_handle_ptr track) :
    m_track(track),
    m_finished(false),
    m_lyrics(nullptr)
{
    InitializeCriticalSection(&m_mutex);
    fb2k::splitTask([this](){
        run_async();
    });
}

LyricSearch::~LyricSearch()
{
    // TODO: Cancel current task, clean up
    if(m_lyrics != nullptr)
    {
        delete m_lyrics;
    }

    DeleteCriticalSection(&m_mutex);
}

LyricData* LyricSearch::get_result()
{
    EnterCriticalSection(&m_mutex);
    LyricData* result = m_lyrics;
    LeaveCriticalSection(&m_mutex);
    return result;
}

static void GetTrackMetaIdentifiers(metadb_handle_ptr track_handle, pfc::string8& out_artist, pfc::string8& out_album, pfc::string8& out_title)
{
    const metadb_info_container::ptr& track_info_container = track_handle->get_info_ref();
    const file_info& track_info = track_info_container->info();
    // t_filetimestamp track_timestamp = track_info_container->stats().m_timestamp; // TODO: This could be useful for setting a cached timestamp to not reload lyrics all the time? Oh but we need to get this for the lyrics file, not the track itself... although I guess if the lyrics are stored in an id3 tag?

    size_t track_artist_index = track_info.meta_find("artist");
    size_t track_album_index = track_info.meta_find("album");
    size_t track_title_index = track_info.meta_find("title");
    const size_t invalid_index = static_cast<size_t>(pfc_infinite);
    static_assert(invalid_index == pfc_infinite, "These types are different but they should still compare equal");
    if((track_artist_index != invalid_index) && (track_info.meta_enum_value_count(track_artist_index) > 0))
    {
        out_artist = pfc::string8(track_info.meta_enum_value(track_artist_index, 0));
    }
    else
    {
        out_artist.reset();
    }

    if((track_album_index != invalid_index) && (track_info.meta_enum_value_count(track_album_index) > 0))
    {
        out_album = pfc::string8(track_info.meta_enum_value(track_album_index, 0));
    }
    else
    {
        out_album.reset();
    }

    if((track_title_index != invalid_index) && (track_info.meta_enum_value_count(track_title_index) > 0))
    {
        out_title = pfc::string8(track_info.meta_enum_value(track_title_index, 0));
    }
    else
    {
        out_title.reset();
    }
}

void LyricSearch::run_async()
{
    // TODO: Return a progress percentage while searching, and show "Searching: 63%" along with a visual progress bar
    pfc::string8 track_artist;
    pfc::string8 track_album;
    pfc::string8 track_title;
    GetTrackMetaIdentifiers(m_track, track_artist, track_album, track_title);

    const pfc::list_t<GUID> active_source_ids = preferences::get_active_sources();
    LyricDataRaw lyric_data_raw = {};
    for(size_t i=0; i<active_source_ids.get_count(); i++)
    {
        GUID source_id = active_source_ids[i];
        LyricSourceBase* source = LyricSourceBase::get(source_id);
        assert(source != nullptr);

        // TODO: Only load files if the file that gets loaded has a newer timestamp than the existing one
        lyric_data_raw = std::move(source->query(m_track));

        if(lyric_data_raw.format != LyricFormat::Unknown)
        {
            break;
        }
    }

    LyricData* lyric_data = new LyricData();
    switch(lyric_data_raw.format)
    {
        case LyricFormat::Plaintext:
        {
            LOG_INFO("Parsing lyrics as plaintext...");
            *lyric_data = parsers::plaintext::parse(lyric_data_raw);
        } break;

        case LyricFormat::Timestamped:
        {
            LOG_INFO("Parsing lyrics as LRC...");
            *lyric_data = parsers::lrc::parse(lyric_data_raw);
            if(lyric_data->format != LyricFormat::Timestamped)
            {
                LOG_INFO("Failed to parse lyrics as LRC, falling back to plaintext...");
                lyric_data_raw.format = LyricFormat::Plaintext;
                *lyric_data = parsers::plaintext::parse(lyric_data_raw);
            }
        } break;

        case LyricFormat::Unknown:
        default:
        {
            LOG_INFO("Could not find lyrics for %s", track_title.c_str());
        }
    }

    // TODO: If we load from tags, should we save to file (or vice-versa)?
    if((lyric_data->source != LyricSource::None) && preferences::get_autosave_enabled())
    {
        SaveMethod method = preferences::get_save_method();
        switch(method)
        {
            case SaveMethod::ConfigDirectory:
            {
                // TODO: This save triggers an immediate reload from the directory watcher.
                //       This is not *necessarily* a problem, but it is some unnecessary work
                //       and it means that we immediately lose the source information for
                //       downloaded lyrics.
                if(lyric_data->source != LyricSource::LocalFiles)
                {
                    sources::localfiles::SaveLyrics(m_track, lyric_data->format, lyric_data->text);
                }
            } break;

            case SaveMethod::Id3Tag:
            {
                LOG_WARN("Saving lyrics to file tags is not currently supported");
                assert(false);
            } break;

            case SaveMethod::None: break;

            default:
                LOG_WARN("Unrecognised save method: %d", (int)method);
                assert(false);
                break;
        }
    }

    EnterCriticalSection(&m_mutex);
    m_lyrics = lyric_data;
    LeaveCriticalSection(&m_mutex);

    LOG_INFO("Lyric loading complete");
}
