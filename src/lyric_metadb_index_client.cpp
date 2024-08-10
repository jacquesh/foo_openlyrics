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

metadb_index_hash lyric_metadb_index_client::hash_handle(const metadb_handle_ptr& info)
{
    metadb_info_container::ptr container = info->get_info_ref();
    return hash(container->info());
}

metadb_index_hash lyric_metadb_index_client::transform(const file_info& info, const playable_location& /*location*/)
{
    return hash(info);
}
