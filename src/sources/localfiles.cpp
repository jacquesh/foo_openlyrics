#include "stdafx.h"

#include "logging.h"
#include "lyric_source.h"
#include "preferences.h"
#include "tag_util.h"
#include "win32_util.h"

static const GUID src_guid = { 0x76d90970, 0x1c98, 0x4fe2, { 0x94, 0x4e, 0xac, 0xe4, 0x93, 0xf3, 0x8e, 0x85 } };

class LocalFileSource : public LyricSourceBase
{
    const GUID& id() const final
    {
        return src_guid;
    }
    std::tstring_view friendly_name() const final
    {
        return _T("Local files");
    }
    bool is_local() const final
    {
        return true;
    }

    std::vector<LyricDataRaw> search(metadb_handle_ptr track,
                                     const metadb_v2_rec_t& track_info,
                                     abort_callback& abort) final;
    bool lookup(LyricDataRaw& data, abort_callback& abort) final;

    std::string save(metadb_handle_ptr track,
                     const metadb_v2_rec_t& track_info,
                     bool is_timestamped,
                     std::string_view lyrics,
                     bool allow_overwrite,
                     abort_callback& abort) final;
    bool delete_persisted(metadb_handle_ptr track, const std::string& path) final;

    std::tstring get_file_path(metadb_handle_ptr track, const LyricData& lyrics) final;
};
static const LyricSourceFactory<LocalFileSource> src_factory;

std::vector<LyricDataRaw> LocalFileSource::search(metadb_handle_ptr track,
                                                  const metadb_v2_rec_t& track_info,
                                                  abort_callback& abort)
{
    std::string file_path_prefix = preferences::saving::filename(track, track_info);

    if(file_path_prefix.empty())
    {
        LOG_ERROR("Failed to determine query file path");
        return {};
    }

    std::vector<LyricDataRaw> output;
    for(LyricType type : { LyricType::Synced, LyricType::Unsynced })
    {
        std::string file_path = file_path_prefix;
        file_path += (type == LyricType::Synced) ? ".lrc" : ".txt";
        LOG_INFO("Querying for lyrics in %s...", file_path.c_str());

        try
        {
            if(filesystem::g_exists(file_path.c_str(), abort))
            {
                LyricDataRaw result = {};
                result.source_id = id();
                result.source_path = file_path;
                result.artist = track_metadata(track_info, "artist");
                result.album = track_metadata(track_info, "album");
                result.title = track_metadata(track_info, "title");
                result.lookup_id = file_path;
                result.type = type;
                output.push_back(std::move(result));
            }
        }
        catch(const std::exception& e)
        {
            LOG_WARN("Failed to open lyrics file %s: %s", file_path.c_str(), e.what());
        }
    }

    LOG_INFO("Found %d lyrics in local files: %s", output.size(), file_path_prefix.c_str());
    return output;
}

bool LocalFileSource::lookup(LyricDataRaw& data, abort_callback& abort)
{
    std::string& file_path = data.lookup_id;
    LOG_INFO("Lookup local-file %s for lyrics...", file_path.c_str());

    try
    {
        file_ptr file;
        filesystem::g_open_read(file, file_path.c_str(), abort);

        // NOTE: We need to use `read_till_eof` instead of `read_string_raw` because otherwise on some
        //       encodings it will see a null byte mid-way through and stop reading, dropping the rest
        //       of the string.
        //       For example if the input file is UTF-16 but only contains ASCII characters, then
        //       every second byte is 0x00 and it will stop reading almost immediately, discarding
        //       most of the actual lyric data. Issue #232 on github.
        pfc::array_t<uint8_t> file_bytes;
        file->read_till_eof(file_bytes, abort);
        LOG_INFO("Successfully retrieved lyrics from %s", file_path.c_str());

        data.text_bytes.clear();
        data.text_bytes.insert(data.text_bytes.begin(), file_bytes.begin(), file_bytes.end());
        return true;
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to open lyrics file %s: %s", file_path.c_str(), e.what());
        return false;
    }
}

static void ensure_dir_exists(const pfc::string& dir_path, abort_callback& abort)
{
    if(filesystem::g_exists(dir_path.c_str(), abort))
    {
        return;
    }

    pfc::string parent = pfc::io::path::getParent(dir_path);
    if(parent == "file://\\\\")
    {
        // If the parent path is "file://\\" then dir_path is something like "file:://\\machine_name" (a path on a
        // network filesystem).
        // filesystem::g_exists fails (at least in SDK version 20200728 with fb2k version 1.6.7) on a path like that.
        // This is fine because we couldn't "create that directory" anyway, if it decided it didn't exist, so we'll just
        // return here as if it does and then either the existence checks further down the path tree will fail, or the
        // actual file write will fail. Both are guarded against IO failure, so that'd be fine (we'd just do slightly
        // more work).
        return;
    }

    if(!parent.isEmpty())
    {
        ensure_dir_exists(parent, abort);
    }

    LOG_INFO("Save directory '%s' does not exist. Creating it...", dir_path.c_str());
    filesystem::g_create_directory(dir_path.c_str(), abort);
}

std::string LocalFileSource::save(metadb_handle_ptr track,
                                  const metadb_v2_rec_t& track_info,
                                  bool is_timestamped,
                                  std::string_view lyrics,
                                  bool allow_overwrite,
                                  abort_callback& abort)
{
    LOG_INFO("Saving lyrics to a local file...");
    std::string output_path_str = preferences::saving::filename(track, track_info);
    if(output_path_str.empty())
    {
        throw std::exception("Failed to determine save file path");
    }

    const char* extension = is_timestamped ? ".lrc" : ".txt";
    output_path_str += extension;

    const pfc::string output_path(output_path_str.c_str(), output_path_str.length());
    pfc::string output_file_name = pfc::io::path::getFileName(output_path);
    if(output_file_name.isEmpty())
    {
        throw std::exception("Calculated file path does not contain a file leaf node");
    }
    ensure_dir_exists(pfc::io::path::getParent(output_path), abort);
    LOG_INFO("Saving lyrics to %s...", output_path.c_str());

    if(!allow_overwrite && filesystem::g_exists(output_path.c_str(), abort))
    {
        LOG_INFO("Save file already exists and overwriting is disallowed. The file will not be modified");
        return output_path_str;
    }

    TCHAR temp_path_str[MAX_PATH + 1];
    DWORD temp_path_str_len = GetTempPath(MAX_PATH + 1, temp_path_str);
    std::string tmp_path = from_tstring(std::tstring_view { temp_path_str, temp_path_str_len });
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
        LOG_WARN("Cannot save lyrics file. Temp path (%s) and output path (%s) are on different filesystems",
                 tmp_path.c_str(),
                 output_path.c_str());
    }

    return output_path_str;
}

bool LocalFileSource::delete_persisted(metadb_handle_ptr /*track*/, const std::string& path)
{
    try
    {
        filesystem::g_remove(path.c_str(), fb2k::mainAborter());
        return true;
    }
    catch(const std::exception& ex)
    {
        LOG_WARN("Failed to delete lyrics file %s: %s", path.c_str(), ex.what());
    }

    return false;
}

std::tstring LocalFileSource::get_file_path(metadb_handle_ptr /*track*/, const LyricData& lyrics)
{
    if(lyrics.source_id == src_guid)
    {
        return to_tstring(lyrics.source_path);
    }
    else if(lyrics.save_source.has_value() && (lyrics.save_source.value() == src_guid))
    {
        return to_tstring(lyrics.save_path);
    }
    else
    {
        LOG_WARN("Attempt to get lyric file path for lyrics that were neither saved nor loaded from local files");
        return _T("");
    }
}
