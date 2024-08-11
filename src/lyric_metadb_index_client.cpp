#include "stdafx.h"

#include "logging.h"
#include "lyric_metadb_index_client.h"
#include "tag_util.h"

void lyric_metadb_index_client::setup_metadb_index(const char* index_title, GUID index_guid)
{
    auto mim = metadb_index_manager::get();
    try
    {
        mim->add(fb2k::service_new<lyric_metadb_index_client>(), index_guid, system_time_periods::week);
        mim->dispatch_global_refresh();
        LOG_INFO("Successfully initialised the %s metadb index", index_title);
    }
    catch(const std::exception& ex)
    {
        mim->remove(index_guid);
        LOG_INFO("Failed to initialise the %s metadb index: %s", index_title, ex.what());
    }
}

metadb_index_hash lyric_metadb_index_client::hash(const file_info& info)
{
    std::string artist = track_metadata(info, "artist");
    std::string album = track_metadata(info, "album");
    std::string title = track_metadata(info, "title");
    std::string key = artist + album + title;
    return static_api_ptr_t<hasher_md5>()->process_single_string(key.c_str()).xorHalve();
}

metadb_index_hash lyric_metadb_index_client::hash_handle(const metadb_v2_rec_t& track_meta)
{
    if(track_meta.info == nullptr)
    {
        return {};
    }
    return hash(track_meta.info->info());
}

metadb_index_hash lyric_metadb_index_client::transform(const file_info& info, const playable_location& /*location*/)
{
    return hash(info);
}
