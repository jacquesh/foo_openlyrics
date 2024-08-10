#include "stdafx.h"

#include "lyric_metadb_index_client.h"
#include "tag_util.h"

lyric_metadb_index_client::ptr lyric_metadb_index_client::instance()
{
    static lyric_metadb_index_client::ptr singleton = new service_impl_single_t<lyric_metadb_index_client>();
    return singleton;
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
