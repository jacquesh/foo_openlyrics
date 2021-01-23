#include "stdafx.h"

#include "logging.h"
#include "lyric_data.h"
#include "lyric_search.h"
#include "sources/lyric_source.h"
#include "sources/localfiles.h"
#include "winstr_util.h"

namespace parsers
{
namespace plaintext { LyricData parse(const LyricDataRaw& input); } // TODO: From parsers/plaintext.cpp
namespace lrc { LyricData parse(const LyricDataRaw& input); } // TODO: From parsers/lrc.cpp
}

LyricSearch::LyricSearch(metadb_handle_ptr track) :
    m_track(track),
    m_mutex({}),
    m_lyrics(nullptr),
    m_abort(),
    m_complete(nullptr)
{
    InitializeCriticalSection(&m_mutex);
    m_complete = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    assert(m_complete != nullptr);

    fb2k::splitTask([this](){
        run_async();
    });
}

LyricSearch::~LyricSearch()
{
    if(!m_abort.is_aborting())
    {
        m_abort.abort();
    }

    DWORD wait_result = WaitForSingleObject(m_complete, 30'000);
    if(wait_result != WAIT_OBJECT_0)
    {
        LOG_ERROR("Lyric search did not complete successfully during cleanup: %d", wait_result);
    }
    CloseHandle(m_complete);
    DeleteCriticalSection(&m_mutex);

    if(m_lyrics != nullptr)
    {
        delete m_lyrics;
    }
}

LyricData* LyricSearch::get_result()
{
    EnterCriticalSection(&m_mutex);
    LyricData* result = m_lyrics;
    LeaveCriticalSection(&m_mutex);
    return result;
}

static void ensure_windows_newlines(pfc::string8& str)
{
    int replace_count = 0;
    size_t prev_index = 0;
    while(true)
    {
        size_t next_index = str.find_first('\n', prev_index);
        if(next_index == pfc::infinite_size)
        {
            break;
        }

        if((next_index == 0) || (str[next_index-1] != '\r'))
        {
            char cr = '\r';
            str.insert_chars(next_index, &cr, 1);
            replace_count++;
        }

        prev_index = next_index+1;
    }
}

void LyricSearch::run_async()
{
    LyricData* lyric_data = new LyricData();

    // TODO: Return a progress percentage while searching, and show "Searching: 63%" along with a visual progress bar
    LyricDataRaw lyric_data_raw = {};
    for(GUID source_id : preferences::get_active_sources())
    {
        LyricSourceBase* source = LyricSourceBase::get(source_id);
        assert(source != nullptr);

        // TODO: Only load files if the file that gets loaded has a newer timestamp than the existing one
        try
        {
            m_abort.check();

            lyric_data_raw = source->query(m_track, m_abort);
            if(lyric_data_raw.format != LyricFormat::Unknown)
            {
                LOG_INFO("Successfully retrieved lyrics from source: %s", tchar_to_string(source->friendly_name()).c_str());
                break;
            }
        }
        catch(const std::exception& e)
        {
            LOG_ERROR("Error while searching %s: %s", tchar_to_string(source->friendly_name()).c_str(), e.what());
        }
        catch(...)
        {
            LOG_ERROR("Error of unrecognised type while searching %s", tchar_to_string(source->friendly_name()));
        }

        LOG_INFO("Failed to retrieve lyrics from source: %s", tchar_to_string(source->friendly_name()).c_str());
    }
    ensure_windows_newlines(lyric_data_raw.text);

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
            break; // No lyrics to parse
        }
    }

    try
    {
        // TODO: If we load from tags, should we save to file (or vice-versa)?
        if((lyric_data->format != LyricFormat::Unknown) && preferences::get_autosave_enabled())
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
                    if(lyric_data->source_id != sources::localfiles::src_guid)
                    {
                        sources::localfiles::SaveLyrics(m_track, lyric_data->format, lyric_data->text, m_abort);
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
    }
    catch(const std::exception& e)
    {
        LOG_ERROR("Failed to download lyrics: %s", e.what());
    }

    EnterCriticalSection(&m_mutex);
    m_lyrics = lyric_data;
    LeaveCriticalSection(&m_mutex);

    BOOL complete_success = SetEvent(m_complete);
    assert(complete_success);
    LOG_INFO("Lyric loading complete");
}
