#include "stdafx.h"

#include "localfiles.h"
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
        console::printf("WARN-OpenLyrics: Failed to listen for changes to local files in %s: %d", lyric_dir_os_path.c_str(), GetLastError());
        return 1;
    }

    console::printf("INFO-OpenLyrics: Listening for changes to %s", lyric_dir_os_path.c_str());
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
            console::printf("WARN-OpenLyrics: Failure waiting for file-watcher handle: %d -> %d", wait_result, GetLastError());
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
            console::printf("WARN-OpenLyrics: Failed to re-queue the listen for additional file change notifications: %d", GetLastError());
            break;
        }
    }

    FindCloseChangeNotification(change_listener);
    return 0;
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
                console::print(PFC_string_formatter() << "ERROR-OpenLyrics: Failed to create lyrics directory" << e);
            }
        }

        InitializeCriticalSection(&g_file_change_lock);
        g_shutdown_handle = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if(g_shutdown_handle == nullptr)
        {
            console::printf("ERROR-OpenLyrics: Failed to create shutdown handle: %d", GetLastError());
        }

        HANDLE thread_handle = CreateThread(nullptr, 0, &file_watch_thread, nullptr, 0, nullptr);
        if(thread_handle == nullptr)
        {
            console::printf("ERROR-OpenLyrics: Failed to spawn file-watcher thread: %d", GetLastError());
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

LyricDataRaw sources::localfiles::Query(const pfc::string8& artist, const pfc::string8& album, const pfc::string8& title)
{
    // TODO: Make this configurable
    pfc::string8 filename = artist;
    filename.add_string(" - ");
    filename.add_string(title);

    pfc::string8 lyric_path_prefix = GetLyricsDir();
    lyric_path_prefix.add_filename(filename);

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
        console::printf("Look for local file %s", file_path.c_str());

        try
        {
            abort_callback_dummy noAbort; // TODO: What should this be instead?
            if (filesystem::g_exists(file_path.c_str(), noAbort))
            {
                file_ptr file;
                filesystem::g_open_read(file, file_path.c_str(), noAbort);

                pfc::string8 file_contents;
                file->read_string_raw(file_contents, noAbort);

                LyricDataRaw result = {ext.format, title, file_contents};
                return result;
            }
        }
        catch(const std::exception& e)
        {
            console::print(PFC_string_formatter() << "WARN-OpenLyrics: Failed to open lyrics file " << file_path << e);
        }
    }

    return {};
}

void sources::localfiles::SaveLyrics(const pfc::string& title, LyricFormat format, const pfc::string8& lyrics)
{
    abort_callback_dummy noAbort; // TODO: What should this be instead?

    // TODO: Validate the filename to remove any bad chars. What are bad chars though?
    //       Maybe this list is a reasonable start:      |*?<>"\/:
    //       Actually, pfc::string8 has support for this. Looking into pfc::string8::fix_filename_chars()
    pfc::string8 output_path = GetLyricsDir();
    output_path.add_filename(title.c_str());
    switch(format)
    {
        case LyricFormat::Plaintext: output_path.add_string(".txt"); break;
        case LyricFormat::Timestamped: output_path.add_string(".lrc"); break;

        case LyricFormat::Unknown:
        default:
            console::printf("ERROR-OpenLyrics: Failed to compute output file path for title %s and format %d", title.c_str(), (int)format);
            return;
    }

    TCHAR temp_path_str[MAX_PATH+1];
    DWORD temp_path_str_len = GetTempPath(MAX_PATH+1, temp_path_str);
    pfc::string8 tmp_path = tchar_to_string(temp_path_str, temp_path_str_len);
    tmp_path.add_filename(title.c_str());

    try
    {
        // TODO: NOTE: Scoping to close the file and flush writes to disk (hopefully preventing "file in use" errors)
        {
            file_ptr tmp_file;
            filesystem::g_open_write_new(tmp_file, tmp_path.c_str(), noAbort);
            tmp_file->write_string_raw(lyrics.c_str(), noAbort);
        }

        service_ptr_t<filesystem> fs = filesystem::get(output_path.c_str());
        if(fs->is_our_path(tmp_path.c_str()))
        {
            fs->move_overwrite(tmp_path.c_str(), output_path.c_str(), noAbort);
            console::printf("Successfully saved lyrics to %s", output_path.c_str());
        }
        else
        {
            console::printf("WARN-OpenLyrics: Cannot save lyrics file. Temp path (%s) and output path (%s) are on different filesystems", tmp_path.c_str(), output_path.c_str());
        }
    }
    catch(std::exception& e)
    {
        console::print(PFC_string_formatter() << "ERROR-OpenLyrics: Failed to write lyrics file to disk" << e);
    }
}
