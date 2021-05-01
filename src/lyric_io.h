#pragma once

#include "stdafx.h"

#include "lyric_data.h"

class LyricUpdateHandle;

namespace io
{
    void search_for_lyrics(LyricUpdateHandle& handle, bool local_only);

    // Returns the path of the file on disk to which the lyrics were saved
    std::string save_lyrics(metadb_handle_ptr track, const LyricData& lyrics, bool allow_overwrite, abort_callback& abort);
}


class LyricUpdateHandle
{
public:
    enum class Type
    {
        Unknown,
        Search,
        Edit
    };

    LyricUpdateHandle(Type type, metadb_handle_ptr track, abort_callback& abort);
    LyricUpdateHandle(const LyricUpdateHandle& other) = delete;
    LyricUpdateHandle(LyricUpdateHandle&& other);
    ~LyricUpdateHandle();

    Type get_type();
    std::string get_progress();
    bool wait_for_complete(uint32_t timeout_ms);
    bool is_complete();
    bool has_result();
    LyricData get_result();

    abort_callback& get_checked_abort(); // Checks the abort flag (so it might throw) and returns it
    metadb_handle_ptr get_track();

    void set_started();
    void set_progress(std::string_view value);
    void set_result(LyricData&& data, bool final_result);

private:
    enum class Status
    {
        Unknown,
        Created,
        Running,
        ResultAvailable,
        Complete,
        Closed
    };

    const metadb_handle_ptr m_track;
    const Type m_type;

    CRITICAL_SECTION m_mutex;
    LyricData m_lyrics;
    abort_callback& m_abort;
    HANDLE m_complete;
    Status m_status;
    std::string m_progress;
};

