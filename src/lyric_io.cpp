#include "stdafx.h"

#include "logging.h"
#include "lyric_data.h"
#include "lyric_io.h"
#include "parsers.h"
#include "sources/lyric_source.h"
#include "ui_hooks.h"
#include "winstr_util.h"

std::string io::save_lyrics(metadb_handle_ptr track, const LyricData& lyrics, bool allow_overwrite, abort_callback& abort)
{
    // NOTE: We require that saving happens on the main thread because the ID3 tag updates can
    //       only happen on the main thread.
    core_api::ensure_main_thread();

    std::string output_path;

    LyricSourceBase* source = LyricSourceBase::get(preferences::saving::save_source());
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
            output_path = source->save(track, lyrics.IsTimestamped(), text, allow_overwrite, abort);
        }
        catch(const std::exception& e)
        {
            std::string source_name = from_tstring(source->friendly_name());
            LOG_ERROR("Failed to save lyrics to %s: %s", source_name.c_str(), e.what());
        }
    }

    return output_path;
}

static void ensure_windows_newlines(std::string& str)
{
    int replace_count = 0;
    size_t prev_index = 0;
    while(true)
    {
        size_t next_index = str.find('\n', prev_index);
        if(next_index == pfc::infinite_size)
        {
            break;
        }

        if((next_index == 0) || (str[next_index-1] != '\r'))
        {
            char cr = '\r';
            str.insert(next_index, 1, cr);
            replace_count++;
        }

        prev_index = next_index+1;
    }
}

static void internal_search_for_lyrics(LyricUpdateHandle& handle, bool local_only)
{
    LOG_INFO("Searching for lyrics...");
    handle.set_started();

    LyricDataRaw lyric_data_raw = {};
    for(GUID source_id : preferences::searching::active_sources())
    {
        LyricSourceBase* source = LyricSourceBase::get(source_id);
        assert(source != nullptr);

        std::string friendly_name = from_tstring(source->friendly_name());
        if(local_only && !source->is_local())
        {
            LOG_INFO("Current search is only considering local sources and %s is not marked as local, skipping...", friendly_name.c_str());
            continue;
        }
        handle.set_progress("Searching " + friendly_name + "...");

        try
        {
            lyric_data_raw = source->query(handle.get_track(), handle.get_checked_abort());
            assert(lyric_data_raw.source_id == source_id);

            if(!lyric_data_raw.text.empty())
            {
                LOG_INFO("Successfully retrieved lyrics from source: %s", friendly_name.c_str());
                break;
            }
        }
        catch(const std::exception& e)
        {
            LOG_ERROR("Error while searching %s: %s", friendly_name.c_str(), e.what());
        }
        catch(...)
        {
            LOG_ERROR("Error of unrecognised type while searching %s", friendly_name.c_str());
        }

        LOG_INFO("Failed to retrieve lyrics from source: %s", friendly_name.c_str());
    }
    ensure_windows_newlines(lyric_data_raw.text);

    LOG_INFO("Parsing lyrics text...");
    handle.set_progress("Parsing...");
    LyricData lyric_data = parsers::lrc::parse(lyric_data_raw);

    handle.set_result(std::move(lyric_data), true);
    LOG_INFO("Lyric loading complete");
}

void io::search_for_lyrics(LyricUpdateHandle& handle, bool local_only)
{
    fb2k::splitTask([&handle, local_only](){
        internal_search_for_lyrics(handle, local_only);
    });
}

LyricUpdateHandle::LyricUpdateHandle(Type type, metadb_handle_ptr track, abort_callback& abort) :
    m_track(track),
    m_type(type),
    m_mutex({}),
    m_lyrics(),
    m_abort(abort),
    m_complete(nullptr),
    m_status(Status::Created),
    m_progress()
{
    InitializeCriticalSection(&m_mutex);
    m_complete = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    assert(m_complete != nullptr);
}

LyricUpdateHandle::~LyricUpdateHandle()
{
    DWORD wait_result = WaitForSingleObject(m_complete, 30'000);
    while(wait_result != WAIT_OBJECT_0)
    {
        LOG_ERROR("Lyric search did not complete successfully during cleanup: %d", wait_result);
        assert(wait_result == WAIT_OBJECT_0);

        wait_result = WaitForSingleObject(m_complete, 30'000);
    }
    CloseHandle(m_complete);
    DeleteCriticalSection(&m_mutex);
}

LyricUpdateHandle::Type LyricUpdateHandle::get_type()
{
    return m_type;
}

std::string LyricUpdateHandle::get_progress()
{
    EnterCriticalSection(&m_mutex);
    std::string result = m_progress;
    LeaveCriticalSection(&m_mutex);
    return result;
}

bool LyricUpdateHandle::is_complete()
{
    EnterCriticalSection(&m_mutex);
    bool complete = ((m_status == Status::Complete) || (m_status == Status::Closed));
    LeaveCriticalSection(&m_mutex);
    return complete;
}

bool LyricUpdateHandle::wait_for_complete(uint32_t timeout_ms)
{
    DWORD wait_result = WaitForSingleObject(m_complete, timeout_ms);
    return (wait_result == WAIT_OBJECT_0);
}

bool LyricUpdateHandle::has_result()
{
    EnterCriticalSection(&m_mutex);
    bool complete = ((m_status == Status::ResultAvailable) || (m_status == Status::Complete));
    LeaveCriticalSection(&m_mutex);
    return complete;
}

LyricData LyricUpdateHandle::get_result()
{
    assert(has_result());
    EnterCriticalSection(&m_mutex);
    LyricData result = std::move(m_lyrics);
    if(m_status == Status::ResultAvailable)
    {
        m_status = Status::Running;
    }
    else
    {
        m_status = Status::Closed;
    }
    LeaveCriticalSection(&m_mutex);
    return result;
}

abort_callback& LyricUpdateHandle::get_checked_abort()
{
    m_abort.check();
    return m_abort;
}

metadb_handle_ptr LyricUpdateHandle::get_track()
{
    return m_track;
}

void LyricUpdateHandle::set_started()
{
    EnterCriticalSection(&m_mutex);
    assert(m_status == Status::Created);
    m_status = Status::Running;
    LeaveCriticalSection(&m_mutex);
}

void LyricUpdateHandle::set_progress(std::string_view value)
{
    EnterCriticalSection(&m_mutex);
    assert(m_status == Status::Running);
    m_progress = value;
    LeaveCriticalSection(&m_mutex);

    repaint_all_lyric_panels();
}

void LyricUpdateHandle::set_result(LyricData&& data, bool final_result)
{
    EnterCriticalSection(&m_mutex);
    assert(m_status == Status::Running);
    m_lyrics = std::move(data);

    if(final_result)
    {
        m_status = Status::Complete;
        BOOL complete_success = SetEvent(m_complete);
        assert(complete_success);
    }
    else
    {
        m_status = Status::ResultAvailable;
    }
    LeaveCriticalSection(&m_mutex);

    repaint_all_lyric_panels();
}

