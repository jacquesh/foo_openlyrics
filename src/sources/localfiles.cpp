#include "stdafx.h"

#include "logging.h"
#include "localfiles.h"
#include "lyric_source.h"
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
    bool format_success = track->format_title(nullptr, out_title, format_script, nullptr);
    out_title.fix_filename_chars();

    return format_success;
}

void sources::localfiles::RegisterLyricPanel(HWND panel_handle)
{
    if(!g_initialised)
    {
        g_initialised = true;
        abort_callback_dummy noAbort;
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

void sources::localfiles::SaveLyrics(metadb_handle_ptr track, LyricFormat format, std::string_view lyrics, abort_callback& abort)
{
    pfc::string8 save_file_title;
    if(!ComputeFileTitle(track, save_file_title))
    {
        LOG_ERROR("Failed to determine save file title");
        return;
    }

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
    pfc::string8 tmp_path = tchar_to_pfcstring(temp_path_str, temp_path_str_len);
    tmp_path.add_filename(save_file_title.c_str());

    try
    {
        {
            // NOTE: Scoping to close the file and flush writes to disk (hopefully preventing "file in use" errors)
            file_ptr tmp_file;
            filesystem::g_open_write_new(tmp_file, tmp_path.c_str(), abort);
            tmp_file->write_object(lyrics.data(), lyrics.size(), abort);
        }

        service_ptr_t<filesystem> fs = filesystem::get(output_path.c_str());
        if(fs->is_our_path(tmp_path.c_str()))
        {
            fs->move_overwrite(tmp_path.c_str(), output_path.c_str(), abort);
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

const GUID sources::localfiles::src_guid = { 0x76d90970, 0x1c98, 0x4fe2, { 0x94, 0x4e, 0xac, 0xe4, 0x93, 0xf3, 0x8e, 0x85 } };

class LocalFileSource : public LyricSourceBase
{
    const GUID& id() const final { return sources::localfiles::src_guid; }
    const TCHAR* friendly_name() const final { return _T("Configuration Folder Files"); }
    bool is_local() const final { return true; }

    LyricDataRaw query(metadb_handle_ptr track, abort_callback& abort) final;
};
static const LyricSourceFactory<LocalFileSource> src_factory;

LyricDataRaw LocalFileSource::query(metadb_handle_ptr track, abort_callback& abort)
{
    pfc::string8 file_title;
    if(!ComputeFileTitle(track, file_title))
    {
        LOG_ERROR("Failed to determine query file title");
        return {};
    }
    LOG_INFO("Querying for lyrics in local files for %s...", file_title.c_str());

    pfc::string8 lyric_path_prefix = sources::localfiles::GetLyricsDir();
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
            if (filesystem::g_exists(file_path.c_str(), abort))
            {
                file_ptr file;
                filesystem::g_open_read(file, file_path.c_str(), abort);

                pfc::string8 file_contents;
                file->read_string_raw(file_contents, abort);

                LyricDataRaw result = {id(), ext.format, file_contents};
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

