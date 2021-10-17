#pragma once

#include "stdafx.h"

struct lyric_search_avoidance
{
    int failed_searches;
    t_filetimestamp first_fail_time;
    uint64_t search_config_generation;
};

lyric_search_avoidance load_search_avoidance(metadb_handle_ptr track);
void save_search_avoidance(metadb_handle_ptr track, lyric_search_avoidance avoidance);

