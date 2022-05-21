#include "stdafx.h"

#include "metadb_index_search_avoidance.h"

#include "logging.h"
#include "preferences.h"
#include "tag_util.h"

static const GUID GUID_METADBINDEX_LYRIC_HISTORY = { 0x915bee72, 0xfd1d, 0x4cf8, { 0x90, 0xd4, 0x8e, 0x2c, 0x18, 0xfd, 0x5, 0xbf } };

struct lyric_metadb_index_client : metadb_index_client
{
    static lyric_metadb_index_client::ptr instance()
    {
        static lyric_metadb_index_client::ptr singleton = new service_impl_single_t<lyric_metadb_index_client>();
        return singleton;
    }

    static metadb_index_hash hash(const file_info& info)
    {
        std::string artist = track_metadata(info, "artist");
        std::string album = track_metadata(info, "album");
        std::string title = track_metadata(info, "title");
        std::string key = artist + album + title;
        return static_api_ptr_t<hasher_md5>()->process_single_string(key.c_str()).xorHalve();
    }

    static metadb_index_hash hash_handle(const metadb_handle_ptr& info)
    {
        metadb_info_container::ptr container = info->get_info_ref();
        return hash(container->info());
    }

    metadb_index_hash transform(const file_info& info, const playable_location& /*location*/) override
    {
        return hash(info);
    }
};

class lyric_metadb_index_init : public init_stage_callback
{
    void on_init_stage(t_uint32 stage) override
    {
        if(stage != init_stages::before_config_read)
        {
            return;
        }

        auto mim = metadb_index_manager::get();
        try
        {
            mim->add(lyric_metadb_index_client::instance(), GUID_METADBINDEX_LYRIC_HISTORY, system_time_periods::week);
            mim->dispatch_global_refresh();
            LOG_INFO("Successfully initialised the lyric metadb index");
        }
        catch(const std::exception& ex)
        {
            mim->remove(GUID_METADBINDEX_LYRIC_HISTORY);
            LOG_INFO("Failed to initialise the lyric metadb index: %s", ex.what());
        }
    }
};
static service_factory_single_t<lyric_metadb_index_init> g_lyric_metadb_index_init;

static lyric_search_avoidance load_search_avoidance(metadb_handle_ptr track)
{
    char data_buffer[512] = {};

    auto meta_index = metadb_index_manager::get();
    metadb_index_hash our_index_hash = lyric_metadb_index_client::hash_handle(track);
    size_t data_bytes = meta_index->get_user_data_here(GUID_METADBINDEX_LYRIC_HISTORY,
                                                       our_index_hash,
                                                       data_buffer,
                                                       sizeof(data_buffer));

    try
    {
        lyric_search_avoidance result = {};
        stream_reader_formatter_simple<false> reader(data_buffer, data_bytes);
        reader >> result.failed_searches;
        reader >> result.first_fail_time;
        reader >> result.search_config_generation;
        return result;
    }
    catch(const std::exception& ex)
    {
        LOG_INFO("Failed to read search-avoidance info: %s", ex.what());
        return {};
    }
}

bool search_avoidance_allows_search(metadb_handle_ptr track)
{
    lyric_search_avoidance avoidance = load_search_avoidance(track);
    const bool expected_to_fail = (avoidance.failed_searches > 3);
    const bool trial_period_expired = ((avoidance.first_fail_time + system_time_periods::week) < filetimestamp_from_system_timer());
    const bool same_generation = (avoidance.search_config_generation == preferences::searching::source_config_generation());
    return !same_generation || !expected_to_fail || !trial_period_expired;
}

static void save_search_avoidance(metadb_handle_ptr track, lyric_search_avoidance avoidance)
{
    auto meta_index = metadb_index_manager::get();
    metadb_index_hash our_index_hash = lyric_metadb_index_client::hash_handle(track);

    stream_writer_formatter_simple<false> writer;
    writer << avoidance.failed_searches;
    writer << avoidance.first_fail_time;
    writer << avoidance.search_config_generation;

    meta_index->set_user_data(GUID_METADBINDEX_LYRIC_HISTORY,
                              our_index_hash,
                              writer.m_buffer.get_ptr(),
                              writer.m_buffer.get_size());
}

void search_avoidance_log_search_failure(metadb_handle_ptr track)
{
    lyric_search_avoidance avoidance = load_search_avoidance(track);
    avoidance.search_config_generation = preferences::searching::source_config_generation();
    if(avoidance.first_fail_time == 0)
    {
        avoidance.first_fail_time = filetimestamp_from_system_timer();
    }
    if(avoidance.failed_searches < INT_MAX)
    {
        avoidance.failed_searches++;
    }
    save_search_avoidance(track, avoidance);
}

void search_avoidance_force_avoidance(metadb_handle_ptr track)
{
    lyric_search_avoidance avoidance = load_search_avoidance(track);
    avoidance.search_config_generation = preferences::searching::source_config_generation();
    avoidance.first_fail_time = 0;
    avoidance.failed_searches = INT_MAX;
    save_search_avoidance(track, avoidance);

#ifndef NDEBUG
    // Sanity check this in debug builds to ensure we have successfully prevented searches
    assert(!search_avoidance_allows_search(track));
#endif
}

void clear_search_avoidance(metadb_handle_ptr track)
{
    auto meta_index = metadb_index_manager::get();
    metadb_index_hash our_index_hash = lyric_metadb_index_client::hash_handle(track);

    meta_index->set_user_data(GUID_METADBINDEX_LYRIC_HISTORY, our_index_hash, nullptr, 0);
}

