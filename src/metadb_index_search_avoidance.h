#pragma once

#include "stdafx.h"

struct lyric_search_avoidance
{
    int failed_searches;
    t_filetimestamp first_fail_time;
    uint64_t search_config_generation;
};

bool search_avoidance_allows_search(metadb_handle_ptr track);
void search_avoidance_log_search_failure(metadb_handle_ptr track);
void search_avoidance_force_avoidance(metadb_handle_ptr track);
void clear_search_avoidance(metadb_handle_ptr track);

