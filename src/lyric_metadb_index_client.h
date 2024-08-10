#pragma once

#include "stdafx.h"

struct lyric_metadb_index_client : metadb_index_client
{
    static lyric_metadb_index_client::ptr instance();
    static metadb_index_hash hash(const file_info& info);
    static metadb_index_hash hash_handle(const metadb_handle_ptr& info);

    metadb_index_hash transform(const file_info& info, const playable_location& /*location*/) override;
};
