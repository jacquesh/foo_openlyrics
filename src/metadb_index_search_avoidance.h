#pragma once

#include "stdafx.h"

enum class SearchAvoidanceReason
{
    Allowed,
    RepeatedFailures,
    MarkedInstrumental,
    MatchesSkipFilter,
    NoVisiblePanels,
};

SearchAvoidanceReason search_avoidance_allows_search(metadb_handle_ptr track, const metadb_v2_rec_t& track_info);
void search_avoidance_log_search_failure(const metadb_v2_rec_t& track_info);
void clear_search_avoidance(const metadb_v2_rec_t& track_info);

void search_avoidance_force_by_mark_instrumental(metadb_handle_ptr track, const metadb_v2_rec_t& track_info);

const char* search_avoid_reason_to_string(SearchAvoidanceReason reason);
