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
    LyricData* m_lyrics;
    abort_callback_impl m_abort;
    HANDLE m_complete;
};
