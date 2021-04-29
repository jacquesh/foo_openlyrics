#include "stdafx.h"

#include "logging.h"
#include "lyric_source.h"
#include "preferences.h"
#include "win32_util.h"

static const GUID src_guid = { 0x76d90970, 0x1c98, 0x4fe2, { 0x94, 0x4e, 0xac, 0xe4, 0x93, 0xf3, 0x8e, 0x85 } };

class LocalFileSource : public LyricSourceBase
{
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("Local files"); }
    bool is_local() const final { return true; }

    LyricDataRaw query(metadb_handle_ptr track, abort_callback& abort) final;
    std::string save(metadb_handle_ptr track, bool is_timestamped, std::string_view lyrics, bool allow_overwrite, abort_callback& abort) final;
};
static const LyricSourceFactory<LocalFileSource> src_factory;

LyricDataRaw LocalFileSource::query(metadb_handle_ptr track, abort_callback& abort)
{
    std::string file_path_prefix = preferences::saving::filename(track);

    LyricDataRaw result = {};
    result.source_id = id();
    result.persistent_storage_path = file_path_prefix;

    if(file_path_prefix.empty())
    {
        LOG_ERROR("Failed to determine query file path");
        return result;
    }

    // TODO: LyricShow3 has a "Choose Lyrics" and "Next Lyrics" option...if we have .txt and .lrc we should possibly communicate that?
    // TODO: Should these extensions be configurable?
    const char* extensions[] = { ".lrc", ".txt" };
    for (const char* ext : extensions)
    {
        std::string file_path = file_path_prefix;
        file_path += ext;
        LOG_INFO("Querying for lyrics in %s...", file_path.c_str());

        try
        {
            if (filesystem::g_exists(file_path.c_str(), abort))
            {
                file_ptr file;
                filesystem::g_open_read(file, file_path.c_str(), abort);

                pfc::string8 file_contents;
                file->read_string_raw(file_contents, abort);

                result.persistent_storage_path = file_path;
                result.text = file_contents;
                LOG_INFO("Successfully retrieved lyrics from %s", file_path.c_str());
                return result;
            }
        }
        catch(const std::exception& e)
        {
            LOG_WARN("Failed to open lyrics file %s: %s", file_path.c_str(), e.what());
        }
    }

    LOG_INFO("Failed to find lyrics in local files %s", file_path_prefix.c_str());
    return result;
}

static void ensure_dir_exists(const pfc::string& dir_path, abort_callback& abort)
{
    pfc::string parent = pfc::io::path::getParent(dir_path);
    if(!parent.isEmpty())
    {
        ensure_dir_exists(parent, abort);
    }

    if(!filesystem::g_exists(dir_path.c_str(), abort))
    {
        LOG_INFO("Save directory '%s' does not exist. Creating it...", dir_path.c_str());
        filesystem::g_create_directory(dir_path.c_str(), abort);
    }
}

std::string LocalFileSource::save(metadb_handle_ptr track, bool is_timestamped, std::string_view lyrics, bool allow_overwrite, abort_callback& abort)
{
    LOG_INFO("Saving lyrics to a local file...");
    std::string output_path_str = preferences::saving::filename(track);
    if(output_path_str.empty())
    {
        throw std::exception("Failed to determine save file path");
    }

    // TODO: Switch to std::filesystem when we update to a version of visual studio that supports it
    pfc::string output_path(output_path_str.c_str(), output_path_str.length());
    pfc::string output_file_name = pfc::io::path::getFileName(output_path);
    if(output_file_name.isEmpty())
    {
        throw std::exception("Calculated file path does not contain a file leaf node");
    }
    ensure_dir_exists(pfc::io::path::getParent(output_path), abort);

    const char* extension = is_timestamped ? ".lrc" : ".txt";
    output_path += extension;
    LOG_INFO("Saving lyrics to %s...", output_path.c_str());

    if(!allow_overwrite && filesystem::g_exists(output_path.c_str(), abort))
    {
        LOG_INFO("Save file already exists and overwriting is disallowed. The file will not be modified");
        return output_path_str;
    }

    TCHAR temp_path_str[MAX_PATH+1];
    DWORD temp_path_str_len = GetTempPath(MAX_PATH+1, temp_path_str);
    std::string tmp_path = from_tstring(std::tstring_view{temp_path_str, temp_path_str_len});
    tmp_path += std::string_view(output_file_name.c_str(), output_file_name.length());

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

    return output_path_str;
}

