//#include <assert.h>

#include "lyric_io.h"

// TODO: This requires shared.dll to be copied to the binaries directory from the fb2k install dir.
// That kinda sucks but it's a consequence of linking to the fb2k SDK and the plugin DLL directly.
// I wonder if we can extract the relevant logic out into a static lib that doesn't depend on fb2k.

// TODO: Begin "bvtf": The Basic Viability Test Framework
#include <stdio.h>
// TODO: Do a debugbreak if the debugger is attached?
#define ASSERT(CONDITION) do{if(!(CONDITION)){ *bvtf_error_count = *bvtf_error_count + 1; return; }}while(false)

typedef void(BVTF_TEST_FUNCTION_TYPE)(int*);

typedef struct
{
    BVTF_TEST_FUNCTION_TYPE* ptr;
    const char* name;
} bvtf_function_metadata;

static int bvtf_test_count = 0;
static bvtf_function_metadata* bvtf_test_functions = nullptr;
int bvtf_register_function(BVTF_TEST_FUNCTION_TYPE* ptr, const char* name)
{
    // TODO: Only realloc when we reach a new power of 2 in the test count, to minimise reallocs
    bvtf_test_functions = (bvtf_function_metadata*)realloc(bvtf_test_functions, sizeof(*bvtf_test_functions) * (bvtf_test_count+1));
    bvtf_test_functions[bvtf_test_count] = { ptr, name };
    bvtf_test_count++;
    return bvtf_test_count;
}

#define BVTF_TEST(TEST_NAME) BVTF_TEST_FUNCTION_TYPE TEST_NAME; static int bvtf_test_##TEST_NAME = bvtf_register_function(&TEST_NAME, #TEST_NAME); void TEST_NAME(int* bvtf_error_count)

int main()
{
    int return_code = 0;
    printf("Executing %d test functions...\n", bvtf_test_count);
    for(int i=0; i<bvtf_test_count; i++)
    {
        int error_count = 0;
        bvtf_test_functions[i].ptr(&error_count);

        const char* status_str = "";
        if(error_count == 0)
        {
            status_str = "\033[32m" "PASSED" "\033[39m";
        }
        else
        {
            status_str = "\033[31m" "FAILED" "\033[39m";
            return_code = 1;
        }
        printf("[%s] %s\n", status_str, bvtf_test_functions[i].name);
    }

    printf("Done");
    return return_code;
}
// TODO: End "bvtf"


static bool g_all_bools[] = { true, false };
static AutoSaveStrategy g_all_save_strategies[] =
{
    AutoSaveStrategy::Never,
    AutoSaveStrategy::Always,
    AutoSaveStrategy::OnlySynced,
    AutoSaveStrategy::OnlyUnsynced
};
const LyricUpdateHandle::Type g_search_update_types[] =  // NOTE: Most tests assume that all update types are either a search type or "Edit"
{
    LyricUpdateHandle::Type::AutoSearch,
    LyricUpdateHandle::Type::ManualSearch
};

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

