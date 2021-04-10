#pragma once

#include "stdafx.h"

#include "lyric_data.h"

class LyricUpdateHandle
{
public:
    LyricUpdateHandle(metadb_handle_ptr track);
    ~LyricUpdateHandle();

    bool is_complete();
    LyricData get_result();

    abort_callback& get_checked_abort(); // Checks the abort flag (so it might throw) and returns it
    metadb_handle_ptr get_track();

    void begin(); // TODO
    void set_result(LyricData&& data);

private:
    enum class Status
    {
        Initialized,
        Running,
        Complete,
        Retrieved
    };

    const metadb_handle_ptr m_track;

    CRITICAL_SECTION m_mutex;
    LyricData m_lyrics;
    abort_callback_impl m_abort;
    HANDLE m_complete;
    Status m_status;
};

void search_for_lyrics(LyricUpdateHandle* handle);
