#include "stdafx.h"

#include "metadb_index_search_avoidance.h"

#include "logging.h"
#include "lyric_metadb_index_client.h"
#include "preferences.h"

static const GUID GUID_METADBINDEX_LYRIC_HISTORY = { 0x915bee72, 0xfd1d, 0x4cf8, { 0x90, 0xd4, 0x8e, 0x2c, 0x18, 0xfd, 0x5, 0xbf } };
DECLARE_OPENLYRICS_METADB_INDEX("lyric search history", GUID_METADBINDEX_LYRIC_HISTORY);

// Much like preferences, these constants must be preserved forever because they get
// persisted in the search avoidance database on the user's machine.
enum AvoidanceFlags : uint32_t
{
    None               = 0,
    MarkedInstrumental = 1 << 0,
};

struct lyric_search_avoidance
{
    // Struct versions:
    // v1: 20 bytes
    // v2: 24 bytes
    int failed_searches;
    t_filetimestamp first_fail_time;
    uint64_t search_config_generation;
    uint32_t flags; // Added in v2
};

static lyric_search_avoidance load_search_avoidance(const metadb_v2_rec_t& track_info)
{
    char data_buffer[512] = {};

    auto meta_index = metadb_index_manager::get();
    metadb_index_hash our_index_hash = lyric_metadb_index_client::hash_handle(track_info);
    size_t data_bytes = meta_index->get_user_data_here(GUID_METADBINDEX_LYRIC_HISTORY,
                                                       our_index_hash,
                                                       data_buffer,
                                                       sizeof(data_buffer));
    if(data_bytes == 0)
    {
        LOG_INFO("No search avoidance info available for track");
        return {};
    }

    try
    {
        lyric_search_avoidance result = {};
        stream_reader_formatter_simple<false> reader(data_buffer, data_bytes);
        if(data_bytes == 20) // v1
        {
            reader >> result.failed_searches;
            reader >> result.first_fail_time;
            reader >> result.search_config_generation;
            result.flags = 0;
        }
        else if(data_bytes == 24) // v2
        {
            reader >> result.failed_searches;
            reader >> result.first_fail_time;
            reader >> result.search_config_generation;
            reader >> result.flags;
        }
        return result;
    }
    catch(const std::exception& ex)
    {
        LOG_WARN("Failed to read search-avoidance info: %s", ex.what());
        return {};
    }
}

static bool track_matches_skip_filter(metadb_handle_ptr track, const metadb_v2_rec_t& track_info)
{
    const pfc::string8& skip_filter_str = preferences::searching::skip_filter();
    if(skip_filter_str.isEmpty())
    {
        return false;
    }

    titleformat_object::ptr skip_filter;
    const bool filter_compile_success = titleformat_compiler::get()->compile(skip_filter, skip_filter_str.c_str());
    if(!filter_compile_success)
    {
        LOG_WARN("Failed to compile skip filter format: %s", skip_filter_str.c_str());
        return false;
    }

    pfc::string8 filter_result;
    track->formatTitle_v2_(track_info, nullptr, filter_result, skip_filter, nullptr);

    return !filter_result.isEmpty();
}

SearchAvoidanceReason search_avoidance_allows_search(metadb_handle_ptr track, const metadb_v2_rec_t& track_info)
{
    if(track_matches_skip_filter(track, track_info))
    {
        return SearchAvoidanceReason::MatchesSkipFilter;
    }

    lyric_search_avoidance avoidance = load_search_avoidance(track_info);
    if((avoidance.flags & AvoidanceFlags::MarkedInstrumental) != 0)
    {
        return SearchAvoidanceReason::MarkedInstrumental;
    }

    const bool expected_to_fail = (avoidance.failed_searches > 3);
    const bool trial_period_expired = ((avoidance.first_fail_time + system_time_periods::week) < filetimestamp_from_system_timer());
    const bool same_generation = (avoidance.search_config_generation == preferences::searching::source_config_generation());
    const bool has_repeated_failures = (same_generation && expected_to_fail && trial_period_expired);
    if(has_repeated_failures)
    {
        return SearchAvoidanceReason::RepeatedFailures;
    }

    return SearchAvoidanceReason::Allowed;
}

static void save_search_avoidance(const metadb_v2_rec_t& track_info, lyric_search_avoidance avoidance)
{
    auto meta_index = metadb_index_manager::get();
    metadb_index_hash our_index_hash = lyric_metadb_index_client::hash_handle(track_info);

    stream_writer_formatter_simple<false> writer;
    writer << avoidance.failed_searches;
    writer << avoidance.first_fail_time;
    writer << avoidance.search_config_generation;
    writer << avoidance.flags;

    meta_index->set_user_data(GUID_METADBINDEX_LYRIC_HISTORY,
                              our_index_hash,
                              writer.m_buffer.get_ptr(),
                              writer.m_buffer.get_size());
}

void search_avoidance_log_search_failure(const metadb_v2_rec_t& track_info)
{
    lyric_search_avoidance avoidance = load_search_avoidance(track_info);
    avoidance.search_config_generation = preferences::searching::source_config_generation();
    if(avoidance.first_fail_time == 0)
    {
        avoidance.first_fail_time = filetimestamp_from_system_timer();
    }
    if(avoidance.failed_searches < INT_MAX)
    {
        avoidance.failed_searches++;
    }
    save_search_avoidance(track_info, avoidance);
}

void search_avoidance_force_by_mark_instrumental(metadb_handle_ptr track, const metadb_v2_rec_t& track_info)
{
    lyric_search_avoidance avoidance = load_search_avoidance(track_info);
    avoidance.flags |= AvoidanceFlags::MarkedInstrumental;
    save_search_avoidance(track_info, avoidance);

#ifndef NDEBUG
    // Sanity check this in debug builds to ensure we have successfully prevented searches
    assert(search_avoidance_allows_search(track, track_info) == SearchAvoidanceReason::MarkedInstrumental);
#endif
}

void clear_search_avoidance(const metadb_v2_rec_t& track_info)
{
    auto meta_index = metadb_index_manager::get();
    metadb_index_hash our_index_hash = lyric_metadb_index_client::hash_handle(track_info);

    meta_index->set_user_data(GUID_METADBINDEX_LYRIC_HISTORY, our_index_hash, nullptr, 0);
}

const char* search_avoid_reason_to_string(SearchAvoidanceReason reason)
{
    switch(reason)
    {
        case SearchAvoidanceReason::Allowed: return "Allowed";
        case SearchAvoidanceReason::RepeatedFailures: return "Repeated failures";
        case SearchAvoidanceReason::MarkedInstrumental: return "Marked instrumental";
        case SearchAvoidanceReason::MatchesSkipFilter: return "Matches skip filter";
        default: return "<Unrecognised reason>";
    }
}
