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
};

static const LyricSourceFactory<ID3TagLyricSource> src_factory;

static std::string GetTrackMetaString(const file_info& track_info, const char* key)
{
    size_t value_index = track_info.meta_find(key);
    if(value_index != pfc::infinite_size)
    {
        std::string result;
        size_t value_count = track_info.meta_enum_value_count(value_index);
        for(size_t i=0; i<value_count; i++)
        {
            const char* value = track_info.meta_enum_value(value_index, i);
            result += value;
        }

        if(!result.empty())
        {
            LOG_INFO("Found lyrics in ID3 tag '%s'", key);
        }
        return result;
    }
    else
    {
        LOG_INFO("No ID3 tag '%s' found", key);
        return "";
    }
}

LyricDataRaw ID3TagLyricSource::query(metadb_handle_ptr track, abort_callback& abort)
{
    const metadb_info_container::ptr& track_info_container = track->get_full_info_ref(abort);
    const file_info& track_info = track_info_container->info();

    LyricDataRaw result = {};
    result.source_id = src_guid;

    std::string text = GetTrackMetaString(track_info, "UNSYNCEDLYRICS");
    if(!text.empty())
    {
        result.text = text;
        result.format = LyricFormat::Plaintext;
    }

    return result;
}
