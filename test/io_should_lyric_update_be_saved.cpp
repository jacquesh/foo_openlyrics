#define BVTF_IMPLEMENTATION
#include "bvtf.h"

#include "common_test_data.h"
#include "lyric_io.h"

BVTF_TEST(always_save_edit_updates)
{
    const LyricUpdateHandle::Type update_type = LyricUpdateHandle::Type::Edit;
    for(bool loaded_from_local_src : g_all_bools)
    {
        for(AutoSaveStrategy autosave : g_all_save_strategies)
        {
            for(bool is_timestamped : g_all_bools)
            {
                bool should_save = io::should_lyric_update_be_saved(loaded_from_local_src, autosave, update_type, is_timestamped);
                ASSERT(should_save);
            }
        }
    }
}

// This test, combined with the edit one (always_save_edit_updates)
// covers all possibilities when loaded_from_local_src is true. Other tests need only check when its false
BVTF_TEST(never_save_search_results_loaded_from_local_sources)
{
    const bool loaded_from_local_src = true;

    for(LyricUpdateHandle::Type update_type : g_search_update_types)
    {
        for(AutoSaveStrategy autosave : g_all_save_strategies)
        {
            for(bool is_timestamped : g_all_bools)
            {
                bool should_save = io::should_lyric_update_be_saved(loaded_from_local_src, autosave, update_type, is_timestamped);
                ASSERT(!should_save);
            }
        }
    }
}

// This test combined with the local source one (never_save_search_results_loaded_from_local_sources)
// covers all possibilities when the update type is ManualSearch. Since edit is completely covered
// by (always_save_edit_updates), we now only need to test AutoSearch.
BVTF_TEST(always_save_manual_search_updates_from_remote_sources)
{
    const LyricUpdateHandle::Type update_type = LyricUpdateHandle::Type::ManualSearch;
    const bool loaded_from_local_src = false;

    for(AutoSaveStrategy autosave : g_all_save_strategies)
    {
        for(bool is_timestamped : g_all_bools)
        {
            bool should_save = io::should_lyric_update_be_saved(loaded_from_local_src, autosave, update_type, is_timestamped);
            ASSERT(should_save);
        }
    }
}

BVTF_TEST(always_save_autosearch_results_with_save_strategy_always)
{
    const bool loaded_from_local_src = false;
    const LyricUpdateHandle::Type update_type = LyricUpdateHandle::Type::AutoSearch;
    const AutoSaveStrategy autosave = AutoSaveStrategy::Always;

    for(bool is_timestamped : g_all_bools)
    {
        bool should_save = io::should_lyric_update_be_saved(loaded_from_local_src, autosave, update_type, is_timestamped);
        ASSERT(should_save);
    }
}

BVTF_TEST(never_save_autosearch_results_with_save_strategy_never)
{
    const bool loaded_from_local_src = false;
    const LyricUpdateHandle::Type update_type = LyricUpdateHandle::Type::AutoSearch;
    const AutoSaveStrategy autosave = AutoSaveStrategy::Never;

    for(bool is_timestamped : g_all_bools)
    {
        bool should_save = io::should_lyric_update_be_saved(loaded_from_local_src, autosave, update_type, is_timestamped);
        ASSERT(!should_save);
    }
}

BVTF_TEST(only_save_synced_autosearch_results_with_save_strategy_onlysynced)
{
    const bool loaded_from_local_src = false;
    const LyricUpdateHandle::Type update_type = LyricUpdateHandle::Type::AutoSearch;
    const AutoSaveStrategy autosave = AutoSaveStrategy::OnlySynced;

    bool save_synced = io::should_lyric_update_be_saved(loaded_from_local_src, autosave, update_type, true);
    bool save_unsynced =  io::should_lyric_update_be_saved(loaded_from_local_src, autosave, update_type, false);
    ASSERT(save_synced);
    ASSERT(!save_unsynced);
}

BVTF_TEST(only_save_unsynced_autosearch_results_with_save_strategy_onlyunsynced)
{
    const bool loaded_from_local_src = false;
    const LyricUpdateHandle::Type update_type = LyricUpdateHandle::Type::AutoSearch;
    const AutoSaveStrategy autosave = AutoSaveStrategy::OnlyUnsynced;

    bool save_synced = io::should_lyric_update_be_saved(loaded_from_local_src, autosave, update_type, true);
    bool save_unsynced =  io::should_lyric_update_be_saved(loaded_from_local_src, autosave, update_type, false);
    ASSERT(!save_synced);
    ASSERT(save_unsynced);
}

