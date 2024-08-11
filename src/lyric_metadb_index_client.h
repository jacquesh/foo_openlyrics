#pragma once

#include "stdafx.h"

struct lyric_metadb_index_client : metadb_index_client
{
    static void setup_metadb_index(const char* index_title, GUID index_guid);
    static metadb_index_hash hash(const file_info& info);
    static metadb_index_hash hash_handle(const metadb_v2_rec_t& info);

    metadb_index_hash transform(const file_info& info, const playable_location& /*location*/) override;
};
#define DECLARE_OPENLYRICS_METADB_INDEX(index_name, index_guid) FB2K_ON_INIT_STAGE([](){ lyric_metadb_index_client::setup_metadb_index(index_name, index_guid); }, init_stages::before_config_read)
