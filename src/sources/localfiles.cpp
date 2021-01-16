#include "stdafx.h"

#include "logging.h"
#include "localfiles.h"
#include "preferences.h"
#include "winstr_util.h"

static bool g_initialised = false;
static CRITICAL_SECTION g_file_change_lock = {};
static HANDLE g_shutdown_handle = nullptr;
static std::vector<HWND> g_notify_handles;

static DWORD WINAPI file_watch_thread(PVOID param)
{
    pfc::string8 lyric_dir_foobar_path = sources::localfiles::GetLyricsDir();
    pfc::string8 lyric_dir_os_path;
    filesystem::g_get_display_path(lyric_dir_foobar_path.c_str(), lyric_dir_os_path);

    // TODO: We probably need to change this watch directory if the user changes their configured save path
    TCHAR* path_buffer = nullptr;
    string_to_tchar(lyric_dir_os_path, path_buffer);
    HANDLE change_listener = FindFirstChangeNotification(path_buffer, TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_SIZE |
            FILE_NOTIFY_CHANGE_LAST_WRITE);
    delete[] path_buffer;

    if(change_listener == INVALID_HANDLE_VALUE)
    {
        LOG_WARN("Failed to listen for changes to local files in %s: %d", lyric_dir_os_path.c_str(), GetLastError());
        return 1;
    }

    LOG_INFO("Listening for changes to %s", lyric_dir_os_path.c_str());
    while(true)
    {
        const HANDLE handles[] = {change_listener, g_shutdown_handle};
        const DWORD handle_count = sizeof(handles)/sizeof(handles[0]);
        DWORD wait_result = WaitForMultipleObjects(handle_count, handles, FALSE, INFINITE);
        if(wait_result == (WAIT_OBJECT_0 + 1))
        {
            break; // Shutdown handle was signalled
        }

        if(wait_result != WAIT_OBJECT_0)
        {
            LOG_WARN("Failure waiting for file-watcher handle: %d -> %d", wait_result, GetLastError());
            break;
        }

        EnterCriticalSection(&g_file_change_lock);
        for(HWND handle : g_notify_handles)
        {
            PostMessage(handle, WM_USER+1, 0, 0); // TODO: Should we make a constant for this message ID?
        }
        LeaveCriticalSection(&g_file_change_lock);

        BOOL success = FindNextChangeNotification(change_listener);
        if(!success)
        {
            LOG_WARN("Failed to re-queue the listen for additional file change notifications: %d", GetLastError());
            break;
        }
    }

    FindCloseChangeNotification(change_listener);
    return 0;
}

static bool ComputeFileTitle(metadb_handle_ptr track, pfc::string8& out_title)
{
    const char* save_format = preferences::get_filename_format();
    titleformat_object::ptr format_script;
    bool compile_success = titleformat_compiler::get()->compile(format_script, save_format);
    if (!compile_success)
    {
        return false;
    }

    pfc::string8 save_file_title;
    track->format_title(nullptr, out_title, format_script, nullptr);
    return true;
}

// TODO: Consult IO.cpp and ui_and_threads.cpp for examples on doing async processing. Surely this is something we should be doing?
void sources::localfiles::RegisterLyricPanel(HWND panel_handle)
{
    if(!g_initialised)
    {
        g_initialised = true;
        abort_callback_dummy noAbort; // TODO: What should this be instead?
        pfc::string8 lyric_dir = sources::localfiles::GetLyricsDir();
        if(!filesystem::g_exists(lyric_dir.c_str(), noAbort))
        {
            try
            {
                filesystem::g_create_directory(sources::localfiles::GetLyricsDir().c_str(), noAbort);
            }
            catch(const std::exception& e)
            {
                LOG_ERROR("Failed to create lyrics directory: %s", e.what());
            }
        }

        InitializeCriticalSection(&g_file_change_lock);
        g_shutdown_handle = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if(g_shutdown_handle == nullptr)
        {
            LOG_ERROR("Failed to create shutdown handle: %d", GetLastError());
        }

        HANDLE thread_handle = CreateThread(nullptr, 0, &file_watch_thread, nullptr, 0, nullptr);
        if(thread_handle == nullptr)
        {
            LOG_ERROR("Failed to spawn file-watcher thread: %d", GetLastError());
        }
    }

    assert(g_initialised);
    EnterCriticalSection(&g_file_change_lock);
    g_notify_handles.push_back(panel_handle);
    LeaveCriticalSection(&g_file_change_lock);
}

void sources::localfiles::DeregisterLyricPanel(HWND panel_handle)
{
    EnterCriticalSection(&g_file_change_lock);
    auto current = g_notify_handles.begin();
    auto end = g_notify_handles.end();
    while(current != end)
    {
        if(*current == panel_handle)
        {
            g_notify_handles.erase(current);
            break;
        }
        current++;
    }
    LeaveCriticalSection(&g_file_change_lock);

    // TODO: Possibly clean up if the handle list is empty?
}

pfc::string8 sources::localfiles::GetLyricsDir()
{
    pfc::string8 lyricDirPath(core_api::get_profile_path());
    lyricDirPath.add_filename("lyrics");
    return lyricDirPath;
}

LyricDataRaw sources::localfiles::Query(metadb_handle_ptr track)
{
    pfc::string8 file_title;
    if(!ComputeFileTitle(track, file_title))
    {
        LOG_ERROR("Failed to determine query file title");
        return {};
    }
    LOG_INFO("Querying for lyrics in local files for %s...", file_title.c_str());

    pfc::string8 lyric_path_prefix = GetLyricsDir();
    lyric_path_prefix.add_filename(file_title);

    // TODO: LyricShow3 has a "Choose Lyrics" and "Next Lyrics" option...if we have .txt and .lrc we should possibly communicate that?
    struct ExtensionDefinition
    {
        const char* extension;
        LyricFormat format;
    };
    const ExtensionDefinition extensions[] = { {".lrc", LyricFormat::Timestamped}, {".txt", LyricFormat::Plaintext} };
    for (const ExtensionDefinition& ext : extensions)
    {
        pfc::string8 file_path = lyric_path_prefix;
        file_path.add_string(ext.extension);
        LOG_INFO("Querying for lyrics from %s...", file_path.c_str());

        try
        {
            abort_callback_dummy noAbort; // TODO: What should this be instead?
            if (filesystem::g_exists(file_path.c_str(), noAbort))
            {
                file_ptr file;
                filesystem::g_open_read(file, file_path.c_str(), noAbort);

                pfc::string8 file_contents;
                file->read_string_raw(file_contents, noAbort);

                LyricDataRaw result = {ext.format, file_contents};
                LOG_INFO("Successfully retrieved lyrics from %s", file_path.c_str());
                return result;
            }
        }
        catch(const std::exception& e)
        {
            LOG_WARN("Failed to open lyrics file %s: %s", file_path.c_str(), e.what());
        }
    }

    LOG_INFO("Failed to find lyrics in local files for %s", file_title.c_str());
    return {};
}

void sources::localfiles::SaveLyrics(metadb_handle_ptr track, LyricFormat format, const pfc::string8& lyrics)
{
    pfc::string8 save_file_title;
    if(!ComputeFileTitle(track, save_file_title))
    {
        LOG_ERROR("Failed to determine save file title");
        return;
    }

    // TODO: Validate the filename to remove any bad chars. What are bad chars though?
    //       Maybe this list is a reasonable start:      |*?<>"\/:
    //       Actually, pfc::string8 has support for this. Looking into pfc::string8::fix_filename_chars()
    pfc::string8 output_path = GetLyricsDir();
    output_path.add_filename(save_file_title.c_str());
    switch(format)
    {
        case LyricFormat::Plaintext: output_path.add_string(".txt"); break;
        case LyricFormat::Timestamped: output_path.add_string(".lrc"); break;

        case LyricFormat::Unknown:
        default:
            LOG_ERROR("Failed to compute output file path for title %s and format %d", save_file_title.c_str(), (int)format);
            return;
    }
    LOG_INFO("Saving lyrics to %s...", output_path.c_str());

    TCHAR temp_path_str[MAX_PATH+1];
    DWORD temp_path_str_len = GetTempPath(MAX_PATH+1, temp_path_str);
    pfc::string8 tmp_path = tchar_to_string(temp_path_str, temp_path_str_len);
    tmp_path.add_filename(save_file_title.c_str());

    try
    {
        // TODO: NOTE: Scoping to close the file and flush writes to disk (hopefully preventing "file in use" errors)
        abort_callback_dummy noAbort; // TODO: What should this be instead?
        {
            file_ptr tmp_file;
            filesystem::g_open_write_new(tmp_file, tmp_path.c_str(), noAbort);
            tmp_file->write_string_raw(lyrics.c_str(), noAbort);
        }

        service_ptr_t<filesystem> fs = filesystem::get(output_path.c_str());
        if(fs->is_our_path(tmp_path.c_str()))
        {
            fs->move_overwrite(tmp_path.c_str(), output_path.c_str(), noAbort);
            LOG_INFO("Successfully saved lyrics to %s", output_path.c_str());
        }
        else
        {
            LOG_WARN("Cannot save lyrics file. Temp path (%s) and output path (%s) are on different filesystems", tmp_path.c_str(), output_path.c_str());
        }
    }
    catch(std::exception& e)
    {
        LOG_ERROR("Failed to write lyrics file to disk", e.what());
    }
}
