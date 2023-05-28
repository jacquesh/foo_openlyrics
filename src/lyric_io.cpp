#include "stdafx.h"

#include "logging.h"
#include "lyric_auto_edit.h"
#include "lyric_data.h"
#include "lyric_io.h"
#include "metadb_index_search_avoidance.h"
#include "parsers.h"
#include "sources/lyric_source.h"
#include "ui_hooks.h"
#include "win32_util.h"

bool io::save_lyrics(metadb_handle_ptr track, const metadb_v2_rec_t& track_info, LyricData& lyrics, bool allow_overwrite, abort_callback& abort)
{
    // NOTE: We require that saving happens on the main thread because the ID3 tag updates can
    //       only happen on the main thread.
    core_api::ensure_main_thread();

    LyricSourceBase* source = LyricSourceBase::get(preferences::saving::save_source());
    if(source == nullptr)
    {
        LOG_WARN("Failed to load configured save source");
        return false;
    }

    std::string text;
    if(lyrics.IsTimestamped() && preferences::saving::merge_equivalent_lrc_lines())
    {
        LyricData merged_lyrics = lyrics;
        const auto lexicographic_sort = [](const auto& lhs, const auto& rhs){ return lhs.text < rhs.text; };
        std::stable_sort(merged_lyrics.lines.begin(), merged_lyrics.lines.end(), lexicographic_sort);
        std::vector<LyricDataLine>::iterator equal_begin = merged_lyrics.lines.begin();

        while(equal_begin != merged_lyrics.lines.end())
        {
            std::vector<LyricDataLine>::iterator equal_end = equal_begin + 1;
            while((equal_end != merged_lyrics.lines.end()) && (equal_begin->text == equal_end->text) && (equal_end->timestamp != DBL_MAX))
            {
                equal_end++;
            }

            // NOTE: We don't need to move equal_begin back one because we don't add
            //       the first timestamp to the string. That'll happen as part of the
            //       normal printing below.
            for(auto iter=equal_end-1; iter!=equal_begin; iter--)
            {
                equal_begin->text = to_tstring(parsers::lrc::print_timestamp(iter->timestamp)) + equal_begin->text;
            }
            equal_begin = merged_lyrics.lines.erase(equal_begin+1, equal_end);
        }

        const auto timestamp_sort = [](const auto& lhs, const auto& rhs){ return lhs.timestamp < rhs.timestamp; };
        std::stable_sort(merged_lyrics.lines.begin(), merged_lyrics.lines.end(), timestamp_sort);

        text = from_tstring(parsers::lrc::expand_text(merged_lyrics));
    }
    else
    {
        text = from_tstring(parsers::lrc::expand_text(lyrics));
    }

    try
    {
        std::string output_path = source->save(track, track_info, lyrics.IsTimestamped(), text, allow_overwrite, abort);
        lyrics.save_path = output_path;
        lyrics.save_source = source->id();
        clear_search_avoidance(track); // Clear here so that we will always find saved lyrics
        return true;
    }
    catch(const std::exception& e)
    {
        std::string source_name = from_tstring(source->friendly_name());
        LOG_ERROR("Failed to save lyrics to %s: %s", source_name.c_str(), e.what());
        return false;
    }
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

static std::string decode_to_utf8(const std::vector<uint8_t> text_bytes)
{
    const auto GetLastErrorString = []() -> const char*
    {
        DWORD error = GetLastError();
        static char errorMsgBuffer[4096];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error, 0, errorMsgBuffer, sizeof(errorMsgBuffer), nullptr);
        return errorMsgBuffer;
    };

    assert(text_bytes.size() < INT_MAX);
    int utf8success = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                          (char*)text_bytes.data(), (int)text_bytes.size(),
                                          nullptr, 0);
    if(utf8success > 0)
    {
        // The input bytes are already valid UTF8, so we don't need to do any converting back-and-forth with wide chars
        LOG_INFO("Loaded lyrics already form a valid UTF-8 sequence");
        return std::string((char*)text_bytes.data(), text_bytes.size());
    }

    std::vector<char> narrow_tmp;
    std::vector<WCHAR> wide_tmp;

    constexpr size_t wchar_per_byte = sizeof(wchar_t)/sizeof(uint8_t);
    const std::wstring_view bytes_as_widestr = std::wstring_view((wchar_t*)text_bytes.data(), text_bytes.size()/wchar_per_byte);
    int narrow_bytes = wide_to_narrow_string(CP_UTF8, bytes_as_widestr, narrow_tmp);
    if(narrow_bytes > 0)
    {
        LOG_INFO("Successfully converted %d bytes of UTF-16 into UTF-8", narrow_bytes);
        const std::string result = std::string(narrow_tmp.data(), narrow_bytes);
        return result;
    }

    UINT codepages[] = { CP_ACP, CP_OEMCP,
        037, 437, 500, 708, 709, 710, 720, 737, 775, 850, 852, 855, 857, 858, 860,
        861, 862, 863, 864, 865, 866, 869, 870, 874, 875, 932, 936, 949, 950, 1026,
        1047, 1140, 1141, 1142, 1143, 1144, 1145, 1146, 1147, 1148, 1149, 1200, 1201,
        1250, 1251, 1252, 1253, 1254, 1255, 1256, 1257, 1258, 1361, 10000, 10001, 10002,
        10003, 10004, 10005, 10006, 10007, 10008, 10010, 10017, 10021, 10029, 10079, 10081,
        10082, 12000, 12001, 20000, 20001, 20002, 20003, 20004, 20005, 20105, 20106, 20107,
        20108, 20127, 20261, 20269, 20273, 20277, 20278, 20280, 20284, 20285, 20290, 20297,
        20420, 20423, 20424, 20833, 20838, 20866, 20871, 20880, 20905, 20924, 20932, 20936,
        20949, 21025, 21027, 21866, 28591, 28592, 28593, 28594, 28595, 28596, 28597, 28598,
        28599, 28603, 28605, 29001, 38598, 50930, 50931, 50933, 50935, 50936, 50937, 50939,
        51932, 51936, 51949, 51950, 52936, 54936
    };

    for(UINT cp : codepages)
    {
        CPINFOEXA info = {};
        GetCPInfoExA(cp, 0, &info);
        const char* current_locale_str = (GetACP() == cp) ? " (current locale code page)" : "";

        const std::string_view narrow_str((char*)text_bytes.data(), text_bytes.size());
        int utf16_chars = narrow_to_wide_string(cp, narrow_str, wide_tmp);
        if(utf16_chars <= 0)
        {
            LOG_WARN("Failed to convert to codepage %u/%s%s: %d/%s", cp, info.CodePageName, current_locale_str, GetLastError(), GetLastErrorString());
            continue;
        }
        LOG_INFO("Successfully converted %d chars to UTF-16 using codepage %s%s", utf16_chars, info.CodePageName, current_locale_str);

        const std::wstring_view wide_str = std::wstring_view(wide_tmp.data(), utf16_chars);
        int utf8_bytes = wide_to_narrow_string(CP_UTF8, wide_str, narrow_tmp);
        if(utf8_bytes <= 0)
        {
            LOG_WARN("Failed to convert wide string back to UTF8: %d/%s", GetLastError(), GetLastErrorString());
            continue;
        }

        LOG_INFO("Successfully converted %d bytes back into UTF-8", utf8_bytes);
        const std::string result = std::string(narrow_tmp.data(), utf8_bytes);
        return result;
    }

    LOG_WARN("Failed to convert lyric bytes to UTF-8 after exhausting all available source encodings");
    return "";
}

static LyricDataUnstructured raw_to_unstructured(const LyricDataRaw& raw)
{
    LyricDataUnstructured unstructured(raw);
    if(!raw.text_bytes.empty())
    {
        unstructured.text = decode_to_utf8(raw.text_bytes);
        ensure_windows_newlines(unstructured.text);
    }
    return unstructured;
}

static void internal_search_for_lyrics(LyricUpdateHandle& handle, bool local_only)
{
    handle.set_started();
    std::string tag_artist = track_metadata(handle.get_track_info(), "artist");
    std::string tag_album = track_metadata(handle.get_track_info(), "album");
    std::string tag_title = track_metadata(handle.get_track_info(), "title");
    LOG_INFO("Searching for lyrics for artist='%s', album='%s', title='%s'...", tag_artist.c_str(), tag_album.c_str(), tag_title.c_str());

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
            if(!source->is_local())
            {
                handle.set_remote_source_searched();
            }
            std::vector<LyricDataRaw> search_results = source->search(handle.get_track(), handle.get_track_info(), handle.get_checked_abort());

            for(LyricDataRaw& result : search_results)
            {
                // NOTE: Some sources don't return an album so we ignore album data if the source didn't give us any.
                //       Similarly, the local tag data might not contain an album, in which case we shouldn't reject
                //       candidates because they have non-empty album data.
                bool tag_match = (result.album.empty() || tag_album.empty() || tag_values_match(tag_album, result.album)) &&
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
                    if(result.text_bytes.empty())
                    {
                        LOG_INFO("Source %s returned an empty lyric, skipping...", friendly_name.c_str());
                    }
                    else
                    {
                        lyric_data_raw = std::move(result);
                        LOG_INFO("Successfully retrieved lyrics from source: %s", friendly_name.c_str());
                        break;
                    }
                }
                else
                {
                    bool lyrics_found = source->lookup(result, handle.get_checked_abort());
                    if(lyrics_found)
                    {
                        if(result.text_bytes.empty())
                        {
                            LOG_INFO("Received empty successful lookup from source: %s", friendly_name.c_str());
                        }
                        else
                        {
                            lyric_data_raw = std::move(result);
                            LOG_INFO("Successfully looked-up lyrics from source: %s", friendly_name.c_str());
                            break;
                        }
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

        if(!lyric_data_raw.text_bytes.empty())
        {
            break;
        }
        LOG_INFO("Failed to retrieve lyrics from source: %s", friendly_name.c_str());
    }

    LOG_INFO("Parsing lyrics text...");
    handle.set_progress("Parsing...");

    const LyricDataUnstructured unstructured = raw_to_unstructured(lyric_data_raw);
    LyricData lyric_data = parsers::lrc::parse(unstructured);

    if(lyric_data.IsEmpty())
    {
        search_avoidance_log_search_failure(handle.get_track());
    }
    else
    {
        // Clear here so that we will continue searching even if auto-save is disabled and the user doesn't save
        clear_search_avoidance(handle.get_track());
    }

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
            search_results = source->search(handle.get_track(), handle.get_track_info(), handle.get_checked_abort());
        }
        else
        {
            handle.set_remote_source_searched();
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

            std::optional<LyricDataUnstructured> lyric;
            if(result.lookup_id.empty())
            {
                if(!result.text_bytes.empty())
                {
                    lyric = raw_to_unstructured(result);
                }
            }
            else
            {
                bool lyrics_found = source->lookup(result, handle.get_checked_abort());
                if(lyrics_found && !result.text_bytes.empty())
                {
                    lyric = raw_to_unstructured(result);
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

        source_handles.emplace_back(handle.get_type(), handle.get_track(), handle.get_track_info(), handle.get_checked_abort());
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

            bool is_complete = src_handle.wait_for_complete(100);
            // It's critically important that we wait for update handle completion above,
            // *then* check for any pending results, *then* erase the handle if it is actually complete.
            // If not, we could get a result while in the wait for completion and then if we immediately
            // erase it without checking and passing out results, we'll sometimes miss passing valid
            // search results out to the UI (in a manner dependent on the relative thread executions).

            while(src_handle.has_result())
            {
                handle.set_result(src_handle.get_result(), false);
            }

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

bool io::should_lyric_update_be_saved(bool loaded_from_local_src, AutoSaveStrategy autosave, LyricUpdateHandle::Type update_type, bool is_timestamped)
{
    const bool is_configured_to_autosave = (autosave == AutoSaveStrategy::Always) ||
                                           ((autosave == AutoSaveStrategy::OnlySynced) && is_timestamped) ||
                                           ((autosave == AutoSaveStrategy::OnlyUnsynced) && !is_timestamped);
    const bool should_autosave = ((update_type == LyricUpdateHandle::Type::AutoSearch) && is_configured_to_autosave && !loaded_from_local_src);

    const bool is_a_user_edit = (update_type == LyricUpdateHandle::Type::Edit);
    const bool user_requested_search = ((update_type == LyricUpdateHandle::Type::ManualSearch) && !loaded_from_local_src);

    // NOTE: We previously changed this to:
    //       `should_autosave && (is_edit || !loaded_from_local_src)`
    //       This makes all the behaviour consistent in the sense that the *only* time it will
    //       save if you set auto-save to "never" is when you explicitly click the "Save" button
    //       in the context menu. However as a user pointed out (here: https://github.com/jacquesh/foo_openlyrics/issues/18)
    //       this doesn't really make sense. If you make an edit then you almost certainly want
    //       to save your edits (and if you just made them then you can always undo them).
    const bool should_save = should_autosave || is_a_user_edit || user_requested_search; // Don't save to the source we just loaded from
    return should_save;
}

bool io::save_overwrite_allowed(LyricUpdateHandle::Type update_type)
{
    const bool allow_overwrite = (update_type == LyricUpdateHandle::Type::Edit) || (update_type == LyricUpdateHandle::Type::ManualSearch);
    return allow_overwrite;
}

bool io::should_auto_edits_be_applied(bool loaded_from_local_src, LyricUpdateHandle::Type update_type)
{
    const bool was_search = (update_type == LyricUpdateHandle::Type::AutoSearch) || (update_type == LyricUpdateHandle::Type::ManualSearch);
    const bool should_auto_edit = was_search && !loaded_from_local_src;
    return should_auto_edit;
}

std::optional<LyricData> io::process_available_lyric_update(LyricUpdateHandle& update)
{
    if(!update.has_result())
    {
        LOG_INFO("Received lyric update with no results, ignoring...");
        return {};
    }

    assert(update.has_result());
    LyricData lyrics = update.get_result();

    if(lyrics.IsEmpty())
    {
        LOG_INFO("Received empty lyric update, ignoring...");
        return {};
    }

    const LyricSourceBase* source = LyricSourceBase::get(lyrics.source_id);
    const bool loaded_from_local_src = ((source != nullptr) && source->is_local());
    const AutoSaveStrategy autosave = preferences::saving::autosave_strategy();

    const bool should_save = should_lyric_update_be_saved(loaded_from_local_src, autosave, update.get_type(), lyrics.IsTimestamped());
    if(should_save)
    {
        try
        {
            const bool should_auto_edit = should_auto_edits_be_applied(loaded_from_local_src, update.get_type());
            if(should_auto_edit)
            {
                for(AutoEditType type : preferences::editing::automated_auto_edits())
                {
                    std::optional<LyricData> maybe_lyrics = auto_edit::RunAutoEdit(type, lyrics);
                    if(maybe_lyrics.has_value())
                    {
                        lyrics = std::move(maybe_lyrics.value());
                    }
                }
            }

            const bool allow_overwrite = save_overwrite_allowed(update.get_type());
            io::save_lyrics(update.get_track(), update.get_track_info(), lyrics, allow_overwrite, update.get_checked_abort());
        }
        catch(const std::exception& e)
        {
            LOG_ERROR("Failed to save downloaded lyrics: %s", e.what());
        }
    }
    else
    {
        LOG_INFO("Skipping lyric save. Type: %d, Local: %s, Timestamped: %s, Autosave: %d",
                int(update.get_type()),
                loaded_from_local_src ? "yes" : "no",
                lyrics.IsTimestamped() ? "yes" : "no",
                int(autosave));
    }

    return {std::move(lyrics)};
}

bool io::delete_saved_lyrics(metadb_handle_ptr track, const LyricData& lyrics)
{
    if(lyrics.save_source.has_value())
    {
        // These lyrics have been saved, delete them from the save source
        LyricSourceBase* source = LyricSourceBase::get(lyrics.save_source.value());
        if(source == nullptr)
        {
            LOG_WARN("Failed to look up save source for lyric deletion request");
            return false;
        }

        LOG_INFO("Lyric was saved to a local source, deleting with the saver source");
        return source->delete_persisted(track, lyrics.save_path);
    }
    else
    {
        // These lyrics have not been saved, but they may have been loaded from a local source
        LyricSourceBase* source = LyricSourceBase::get(lyrics.source_id);
        if(source == nullptr)
        {
            LOG_WARN("Failed to look up originating source for lyric deletion request");
            return false;
        }

        if(source->is_local())
        {
            LOG_INFO("Lyric was loaded from a local source, deleting with the loader source");
            return source->delete_persisted(track, lyrics.source_path);
        }
        else
        {
            LOG_INFO("Lyric was loaded from a non-local source and has not been saved, nothing to delete");
            return false;
        }
    }
}

LyricUpdateHandle::LyricUpdateHandle(Type type, metadb_handle_ptr track, metadb_v2_rec_t track_info, abort_callback& abort) :
    m_track(track),
    m_track_info(track_info),
    m_type(type),
    m_mutex({}),
    m_lyrics(),
    m_abort(abort),
    m_complete(nullptr),
    m_status(Status::Created),
    m_progress(),
    m_searched_remote_sources(false)
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
    m_progress(std::move(other.m_progress)),
    m_searched_remote_sources(other.m_searched_remote_sources)
{
    other.m_status = Status::Closed;
    InitializeCriticalSection(&m_mutex);

    BOOL other_complete = other.is_complete();
    m_complete = CreateEvent(nullptr, TRUE, other_complete, nullptr);
    if(!other_complete)
    {
        other.set_complete();
    }
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

bool LyricUpdateHandle::has_searched_remote_sources()
{
    EnterCriticalSection(&m_mutex);
    bool output = m_searched_remote_sources;
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

const metadb_v2_rec_t& LyricUpdateHandle::get_track_info()
{
    return m_track_info;
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

void LyricUpdateHandle::set_remote_source_searched()
{
    EnterCriticalSection(&m_mutex);
    m_searched_remote_sources = true;
    LeaveCriticalSection(&m_mutex);
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

