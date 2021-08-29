#include "stdafx.h"

#include "logging.h"
#include "lyric_data.h"
#include "lyric_io.h"
#include "parsers.h"
#include "sources/lyric_source.h"
#include "ui_hooks.h"
#include "win32_util.h"

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
            std::vector<LyricDataRaw> search_results = source->search(handle.get_track(), handle.get_checked_abort());

            std::string tag_artist = track_metadata(handle.get_track(), "artist");
            std::string tag_album = track_metadata(handle.get_track(), "album");
            std::string tag_title = track_metadata(handle.get_track(), "title");
            for(LyricDataRaw& result : search_results)
            {
                // NOTE: Some sources don't return an album so we ignore album data if the source didn't give us any
                bool tag_match = (result.album.empty() || tag_values_match(tag_album, result.album)) &&
                                 tag_values_match(tag_artist, result.artist) &&
                                 tag_values_match(tag_title, result.title);
                if(!tag_match)
                {
                    LOG_INFO("Rejected %s search result %s/%s/%s due to tag mismatch: %s/%s/%s",
                            friendly_name.c_str(),
                            tag_artist.c_str(),
                            tag_album.c_str(),
                            tag_title.c_str(),
                            result.artist.c_str(),
                            result.album.c_str(),
                            result.title.c_str());
                    continue;
                }

                assert(result.source_id == source_id);
                if(result.lookup_id.empty())
                {
                    assert(!result.text.empty());
                    lyric_data_raw = std::move(result);
                    LOG_INFO("Successfully retrieved lyrics from source: %s", friendly_name.c_str());
                    break;
                }
                else
                {
                    bool lyrics_found = source->lookup(result, handle.get_checked_abort());
                    if(lyrics_found)
                    {
                        if(result.text.empty())
                        {
                            LOG_WARN("Received illegal empty success result from source: %s", friendly_name.c_str());
                            assert(!result.text.empty());
                            continue;
                        }

                        lyric_data_raw = std::move(result);
                        LOG_INFO("Successfully looked-up lyrics from source: %s", friendly_name.c_str());
                        break;
                    }
                    else
                    {
                        LOG_INFO("Look up for lyrics from source %s returned an empty result, ignoring...", friendly_name.c_str());
                    }
                }
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

        if(!lyric_data_raw.text.empty())
        {
            break;
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

static void internal_search_for_all_lyrics_from_source(LyricUpdateHandle& handle, LyricSourceBase* source, std::string artist, std::string album, std::string title)
{
    std::string friendly_name = from_tstring(source->friendly_name());
    handle.set_started();

    try
    {
        std::vector<LyricDataRaw> search_results;
        if(source->is_local())
        {
            search_results = source->search(handle.get_track(), handle.get_checked_abort());
        }
        else
        {
            LyricSourceRemote* remote_source = dynamic_cast<LyricSourceRemote*>(source);
            assert(remote_source != nullptr);
            if(remote_source == nullptr)
            {
                LOG_ERROR("Bad LyricSourceRemote cast for: %s", friendly_name.c_str());
                handle.set_complete();
                return;
            }

            search_results = remote_source->search(artist, album, title, handle.get_checked_abort());
        }

        for(LyricDataRaw& result : search_results)
        {
            assert(result.source_id == source->id());

            std::optional<LyricDataRaw> lyric;
            if(result.lookup_id.empty())
            {
                assert(!result.text.empty());
                lyric = std::move(result);
            }
            else
            {
                bool lyrics_found = source->lookup(result, handle.get_checked_abort());
                if(lyrics_found)
                {
                    assert(!result.text.empty());
                    lyric = std::move(result);
                }
            }

            if(lyric.has_value())
            {
                ensure_windows_newlines(lyric.value().text);

                LyricData parsed_lyrics = parsers::lrc::parse(lyric.value());
                handle.set_result(std::move(parsed_lyrics), false);
            }
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

    handle.set_complete();
}

static void internal_search_for_all_lyrics(LyricUpdateHandle& handle, std::string artist, std::string album, std::string title)
{
    LOG_INFO("Searching for lyrics using custom parameters...");
    handle.set_started();

    // NOTE: It is crucial that this is a std::list so that inserting new items or removing old ones
    //       does not re-allocate the entire list and invalidate earlier pointers. We pass references
    //       to these handles into the search task and so they need to remain valid for the task's
    //       entire lifetime or we'll get weird random-memory bugs.
    std::list<LyricUpdateHandle> source_handles;

    std::vector<GUID> all_source_ids = LyricSourceBase::get_all_ids();
    for(GUID source_id : all_source_ids)
    {
        LyricSourceBase* source = LyricSourceBase::get(source_id);
        assert(source != nullptr);

        source_handles.emplace_back(handle.get_type(), handle.get_track(), handle.get_checked_abort());
        LyricUpdateHandle& src_handle = source_handles.back();

        fb2k::splitTask([&src_handle, source, artist, album, title](){
            internal_search_for_all_lyrics_from_source(src_handle, source, artist, album, title);
        });
    }

    while(!source_handles.empty())
    {
        for(auto iter=source_handles.begin(); iter!=source_handles.end(); /*omitted*/)
        {
            LyricUpdateHandle& src_handle = *iter;
            while(src_handle.has_result())
            {
                handle.set_result(src_handle.get_result(), false);
            }

            bool is_complete = src_handle.wait_for_complete(100);
            if(is_complete)
            {
                iter = source_handles.erase(iter);
            }
            else
            {
                iter++;
            }
        }
    }

    handle.set_complete();
    LOG_INFO("Finished loading lyrics from a custom search");
}

void io::search_for_all_lyrics(LyricUpdateHandle& handle, std::string artist, std::string album, std::string title)
{
    fb2k::splitTask([&handle, artist, album, title](){
        internal_search_for_all_lyrics(handle, artist, album, title);
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

LyricUpdateHandle::LyricUpdateHandle(LyricUpdateHandle&& other) :
    m_track(other.m_track),
    m_type(other.m_type),
    m_mutex(),
    m_lyrics(std::move(other.m_lyrics)),
    m_abort(other.m_abort),
    m_complete(nullptr),
    m_status(other.m_status),
    m_progress(std::move(other.m_progress))
{
    other.m_status = Status::Closed;
    InitializeCriticalSection(&m_mutex);

    BOOL event_set = other.is_complete();
    m_complete = CreateEvent(nullptr, TRUE, event_set, nullptr);
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
    bool output = !m_lyrics.empty();
    LeaveCriticalSection(&m_mutex);
    return output;
}

LyricData LyricUpdateHandle::get_result()
{
    EnterCriticalSection(&m_mutex);
    assert(!m_lyrics.empty());
    LyricData result = std::move(m_lyrics.front());
    m_lyrics.erase(m_lyrics.begin());
    if(m_lyrics.empty() && (m_status == Status::Complete))
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
    m_lyrics.push_back(std::move(data));

    if(final_result)
    {
        m_status = Status::Complete;
        BOOL complete_success = SetEvent(m_complete);
        assert(complete_success);
    }
    LeaveCriticalSection(&m_mutex);

    repaint_all_lyric_panels();
}

void LyricUpdateHandle::set_complete()
{
    EnterCriticalSection(&m_mutex);
    assert(m_status == Status::Running);

    if(m_lyrics.empty())
    {
        m_status = Status::Closed;
    }
    else
    {
        m_status = Status::Complete;
    }
    BOOL complete_success = SetEvent(m_complete);
    assert(complete_success);

    LeaveCriticalSection(&m_mutex);
}

