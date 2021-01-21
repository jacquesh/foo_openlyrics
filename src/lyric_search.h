#pragma once

#include "stdafx.h"

#include "lyric_data.h"

class LyricSearch
{
public:
    LyricSearch(metadb_handle_ptr track);
    ~LyricSearch();

    LyricData* get_result();

private:
    void run_async();

    metadb_handle_ptr m_track;

    CRITICAL_SECTION m_mutex;
    bool m_finished;

    LyricData* m_lyrics;
};
