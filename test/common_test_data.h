#pragma once

#include "lyric_io.h"

static const bool g_all_bools[] = { true, false };
static const AutoSaveStrategy g_all_save_strategies[] =
{
    AutoSaveStrategy::Never,
    AutoSaveStrategy::Always,
    AutoSaveStrategy::OnlySynced,
    AutoSaveStrategy::OnlyUnsynced
};
static const LyricUpdateHandle::Type g_search_update_types[] =  // NOTE: Most tests assume that all update types are either a search type or "Edit"
{
    LyricUpdateHandle::Type::AutoSearch,
    LyricUpdateHandle::Type::ManualSearch
};

