#include "stdafx.h"

#include "logging.h"
#include "lyric_source.h"

static const GUID src_guid = { 0x3fb0f715, 0xa097, 0x493a, { 0x94, 0x4e, 0xdb, 0x48, 0x66, 0x8, 0x86, 0x78 } };

class ID3TagLyricSource : public LyricSourceBase
{
    const GUID& id() const final { return src_guid; }
    const TCHAR* friendly_name() const final { return _T("ID3 Tags"); }
    bool is_local() const final { return true; }

    LyricDataRaw query(metadb_handle_ptr track, abort_callback& abort) final;
    void save(metadb_handle_ptr track, bool is_timestamped, std::string_view lyrics, abort_callback& abort) final;
};

static const LyricSourceFactory<ID3TagLyricSource> src_factory;

static std::string GetTrackMetaString(const file_info& track_info, std::string_view key)
{
    size_t value_index = track_info.meta_find_ex(key.data(), key.length());
    if(value_index != pfc::infinite_size)
    {
        std::string result;
        size_t value_count = track_info.meta_enum_value_count(value_index);
        for(size_t i=0; i<value_count; i++)
        {
            const char* value = track_info.meta_enum_value(value_index, i);
            result += value;
        }
        return result;
    }
    else
    {
        return "";
    }
}

LyricDataRaw ID3TagLyricSource::query(metadb_handle_ptr track, abort_callback& abort)
{
    const metadb_info_container::ptr& track_info_container = track->get_full_info_ref(abort);
    const file_info& track_info = track_info_container->info();

    LyricDataRaw result = {};
    result.source_id = src_guid;

    for(const std::string& tag : preferences::searching::tags())
    {
        LOG_INFO("Searching for lyrics in tag: %s", tag.c_str());

        std::string text = GetTrackMetaString(track_info, tag);
        if(!text.empty())
        {
            LOG_INFO("Found lyrics in tag: %s", tag.c_str());
            result.text = text;
            break;
        }
    }

    return result;
}

void ID3TagLyricSource::save(metadb_handle_ptr track, bool is_timestamped, std::string_view lyric_view, abort_callback& /*abort*/)
{
    struct MetaCompletionLogger : public completion_notify
    {
        std::string metatag;
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

    std::string tag_name;
    if(is_timestamped)
    {
        tag_name = preferences::saving::timestamped_tag();
    }
    else
    {
        tag_name = preferences::saving::untimed_tag();
    }

    std::string lyrics(lyric_view);
    auto update_meta_tag = [tag_name, lyrics](trackRef /*location*/, t_filestats /*stats*/, file_info& info)
    {
        t_size tag_index = info.meta_find_ex(tag_name.data(), tag_name.length());
        if(tag_index != pfc::infinite_size)
        {
            info.meta_remove_index(tag_index);
        }
        info.meta_add_ex(tag_name.data(), tag_name.length(), lyrics.data(), lyrics.length());
        return true;
    };

    service_ptr_t<file_info_filter> updater = file_info_filter::create(update_meta_tag);
    service_ptr_t<MetaCompletionLogger> completion = fb2k::service_new<MetaCompletionLogger>(tag_name);
    service_ptr_t<metadb_io_v2> meta_io = metadb_io_v2::get();
	meta_io->update_info_async(pfc::list_single_ref_t<metadb_handle_ptr>(track),
                               updater,
                               core_api::get_main_window(),
                               metadb_io_v2::op_flag_no_errors | metadb_io_v2::op_flag_delay_ui,
                               completion);
}
