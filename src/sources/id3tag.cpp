#include "stdafx.h"

#include "logging.h"
#include "lyric_source.h"
#include "parsers.h"
#include "tag_util.h"

static const GUID src_guid = { 0x3fb0f715, 0xa097, 0x493a, { 0x94, 0x4e, 0xdb, 0x48, 0x66, 0x8, 0x86, 0x78 } };
static const GUID localfiles_src_guid = { 0x76d90970, 0x1c98, 0x4fe2, { 0x94, 0x4e, 0xac, 0xe4, 0x93, 0xf3, 0x8e, 0x85 } };

class ID3TagLyricSource : public LyricSourceBase
{
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("Metadata tags"); }
    bool is_local() const final { return true; }

    std::vector<LyricDataRaw> search(metadb_handle_ptr track, const metadb_v2_rec_t& track_info, abort_callback& abort) final;
    bool lookup(LyricDataRaw& data, abort_callback& abort) final;

    std::string save(metadb_handle_ptr track, const metadb_v2_rec_t& track_info, bool is_timestamped, std::string_view lyrics, bool allow_overwrite, abort_callback& abort) final;
    bool delete_persisted(metadb_handle_ptr track, const std::string& path) final;

    std::tstring get_file_path(metadb_handle_ptr track, const LyricData& lyrics) final;
};

static const LyricSourceFactory<ID3TagLyricSource> src_factory;

std::vector<LyricDataRaw> ID3TagLyricSource::search(metadb_handle_ptr track, const metadb_v2_rec_t& track_info, abort_callback& /*abort*/)
{
    if(track_info.info == nullptr)
    {
        return {};
    }
    std::vector<LyricDataRaw> result;
    const file_info& info = track_info.info->info();

    for(const std::string& tag : preferences::searching::tags())
    {
        LOG_INFO("Searching for lyrics in tag: '%s'", tag.c_str());
        size_t lyric_value_index = info.meta_find_ex(tag.c_str(), tag.length());
        if(lyric_value_index == pfc::infinite_size)
        {
            continue;
        }

        LyricDataRaw lyric = {};
        lyric.source_id = src_guid;
        lyric.source_path = tag;
        lyric.artist = track_metadata(track_info, "artist");
        lyric.album = track_metadata(track_info, "album");
        lyric.title = track_metadata(track_info, "title");
        lyric.duration_sec = track_duration_in_seconds(track_info);

        std::string text;
        size_t value_count = info.meta_enum_value_count(lyric_value_index);
        for(size_t i=0; i<value_count; i++)
        {
            const char* value = info.meta_enum_value(lyric_value_index, i);
            text += value;
        }
        lyric.text_bytes = string_to_raw_bytes(text);

        if(!lyric.text_bytes.empty())
        {
            LOG_INFO("Found lyrics in tag: '%s'", tag.c_str());
            const LyricData parsed = parsers::lrc::parse(lyric, text);
            lyric.type = parsed.IsTimestamped() ? LyricType::Synced : LyricType::Unsynced;

            result.push_back(std::move(lyric));
        }
    }

    return result;
}

bool ID3TagLyricSource::lookup(LyricDataRaw& /*data*/, abort_callback& /*abort*/)
{
    LOG_ERROR("We should never need to do a lookup of the %s source", friendly_name().data());
    assert(false);
    return false;
}

std::string ID3TagLyricSource::save(metadb_handle_ptr track, const metadb_v2_rec_t& track_info, bool is_timestamped, std::string_view lyric_view, bool allow_overwrite, abort_callback& abort)
{
    // We can't save lyrics for remote tracks to metadata (because we don't have a file to save
    // the metadata into). Redirect to saving to localfiles if we find ourselves attempting to
    // save lyrics for a remote track to metadata.
    if(track_is_remote(track))
    {
        LyricSourceBase* localfiles_source = LyricSourceBase::get(localfiles_src_guid);
        assert(localfiles_source != nullptr);
        return localfiles_source->save(track, track_info, is_timestamped, lyric_view, allow_overwrite, abort);
    }

    std::string tag_name;
    if(is_timestamped)
    {
        tag_name = preferences::saving::timestamped_tag();
    }
    else
    {
        tag_name = preferences::saving::untimed_tag();
    }
    LOG_INFO("Saving lyrics to ID3 tag %s...", tag_name.c_str());

    // NOTE: I'm actually not 100% sure this is necessary but lets ensure we've loaded the full tag data
    //       before we save it so that we don't accidentally overwrite some esoteric tag that wasn't loaded.
    track->get_full_info_ref(abort);

    std::string lyrics(lyric_view);
    const auto update_lyric_tag = [](metadb_handle_ptr track, const std::string& tag_name, const std::string& lyrics, bool allow_overwrite)
    {
        struct MetaCompletionLogger : public completion_notify
        {
            const std::string metatag;
            MetaCompletionLogger(std::string_view tag) : metatag(tag) {}
            void on_completion(unsigned int result_code) final
            {
                if(result_code == metadb_io::update_info_success)
                {
                    LOG_INFO("Successfully saved lyrics to %s", metatag.c_str());
                }
                else
                {
                    LOG_WARN("Failed to save lyrics to tag %s: %u", metatag.c_str(), result_code);
                }
            }
        };

        const auto update_meta_tag = [tag_name, lyrics, allow_overwrite](trackRef /*location*/, t_filestats /*stats*/, file_info& info)
        {
            t_size tag_index = info.meta_find_ex(tag_name.data(), tag_name.length());
            if(!allow_overwrite && (tag_index != pfc::infinite_size))
            {
                LOG_INFO("Save tag already exists and overwriting is disallowed. The tag will not be modified");
                return false;
            }
            info.meta_set_ex(tag_name.data(), tag_name.length(), lyrics.data(), lyrics.length());
            return true;
        };

        service_ptr_t<file_info_filter> updater = file_info_filter::create(update_meta_tag);
        service_ptr_t<MetaCompletionLogger> completion = fb2k::service_new<MetaCompletionLogger>(tag_name);
        service_ptr_t<metadb_io_v2> meta_io = metadb_io_v2::get();
        meta_io->update_info_async(pfc::list_single_ref_t<metadb_handle_ptr>(track),
                                   updater,
                                   core_api::get_main_window(),
                                   metadb_io_v2::op_flag_delay_ui | metadb_io_v2::op_flag_partial_info_aware,
                                   completion);
        LOG_INFO("Successfully wrote lyrics to ID3 tag %s", tag_name.c_str());
    };

    // metadb_io_v2's async need to be called from the main thread, but we want to have some idea of
    // whether or not we succeeded. So if we're already on the main thread then call it directly and
    // return the result, otherwise queue the update for the main thread and assume we succeeded.
    if(core_api::is_main_thread())
    {
        update_lyric_tag(track, tag_name, lyrics, allow_overwrite);
    }
    else
    {
        fb2k::inMainThread2([update_lyric_tag, track, tag_name, lyrics, allow_overwrite]() { update_lyric_tag(track, tag_name, lyrics, allow_overwrite); });
    }

    return tag_name;
}

bool ID3TagLyricSource::delete_persisted(metadb_handle_ptr track, const std::string& path)
{
    const auto delete_lyric_tag = [](metadb_handle_ptr track, const std::string& path) -> bool
    {
        struct MetaRemovalCompletionLogger : public completion_notify
        {
            std::string metatag;
            MetaRemovalCompletionLogger(std::string_view tag) : metatag(tag) {}
            void on_completion(unsigned int result_code) final
            {
                if(result_code == metadb_io::update_info_success)
                {
                    LOG_INFO("Successfully removed lyrics stored in tag '%s'", metatag.c_str());
                }
                else
                {
                    LOG_WARN("Failed to remove lyrics stored in tag '%s': %u", metatag.c_str(), result_code);
                }
            }
        };

        try
        {
            const auto update_meta_tag = [path](trackRef /*location*/, t_filestats /*stats*/, file_info& info)
            {
                size_t lyric_value_index = info.meta_find_ex(path.c_str(), path.length());
                if(lyric_value_index == pfc::infinite_size)
                {
                    return false;
                }

                t_size tag_index = info.meta_find_ex(path.data(), path.length());
                if(tag_index == pfc::infinite_size)
                {
                    LOG_WARN("Failed to find persisted tag '%s' for deletion", path.c_str());
                    return false;
                }
                info.meta_remove_index(tag_index);
                return true;
            };

            // NOTE: I'm actually not 100% sure this is necessary but lets ensure we've loaded the full tag data
            //       before we save it so that we don't accidentally overwrite some esoteric tag that wasn't loaded.
            track->get_full_info_ref(fb2k::mainAborter());

            service_ptr_t<file_info_filter> updater = file_info_filter::create(update_meta_tag);
            service_ptr_t<MetaRemovalCompletionLogger> completion = fb2k::service_new<MetaRemovalCompletionLogger>(path);
            service_ptr_t<metadb_io_v2> meta_io = metadb_io_v2::get();
            meta_io->update_info_async(pfc::list_single_ref_t<metadb_handle_ptr>(track),
                                       updater,
                                       core_api::get_main_window(),
                                       metadb_io_v2::op_flag_delay_ui | metadb_io_v2::op_flag_partial_info_aware,
                                       completion);
            return true;
        }
        catch(const std::exception& ex)
        {
            LOG_WARN("Failed to delete lyrics file %s: %s", path.c_str(), ex.what());
        }
        return false;
    };

    // metadb_io_v2's async need to be called from the main thread, but we want to have some idea of
    // whether or not we succeeded. So if we're already on the main thread then call it directly and
    // return the result, otherwise queue the update for the main thread and assume we succeeded.
    if(core_api::is_main_thread())
    {
        return delete_lyric_tag(track, path);
    }
    else
    {
        fb2k::inMainThread2([delete_lyric_tag, track, path]() { delete_lyric_tag(track, path); });
        return true;
    }
}

std::tstring ID3TagLyricSource::get_file_path(metadb_handle_ptr track, const LyricData& lyrics)
{
    const char* path = track->get_path();
    if((lyrics.source_id == src_guid) ||
       (lyrics.save_source.has_value() && (lyrics.save_source.value() == src_guid))
      )
    {
        return to_tstring(std::string_view(path, strlen(path)));
    }
    {
        LOG_WARN("Attempt to get lyric file path for lyrics that were neither saved nor loaded from metadata tags");
        return _T("");
    }
}
