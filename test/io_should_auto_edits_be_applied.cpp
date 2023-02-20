#include "bvtf.h"

#include "common_test_data.h"
#include "lyric_io.h"

BVTF_TEST(dont_apply_autoedits_to_edit_results)
{
    const LyricUpdateHandle::Type update_type = LyricUpdateHandle::Type::Edit;

    for(bool loaded_from_local_src : g_all_bools)
    {
        const bool applied = io::should_auto_edits_be_applied(loaded_from_local_src, update_type);
        ASSERT(!applied);
    }
}

BVTF_TEST(apply_autoedits_to_search_results_only_from_remote_sources)
{
    for(LyricUpdateHandle::Type update_type : g_search_update_types)
    {
        const bool applied_remote = io::should_auto_edits_be_applied(false, update_type);
        const bool applied_local =  io::should_auto_edits_be_applied(true, update_type);
        ASSERT(applied_remote);
        ASSERT(!applied_local);
    }
}
