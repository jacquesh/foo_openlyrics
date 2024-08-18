#pragma once

#include "stdafx.h"

#include "lyric_data.h"

struct LyricUpdate
{
    enum class Type
    {
        Unknown,
        AutoSearch,
        ManualSearch,
        Edit,
    };

    LyricData lyrics;
    metadb_handle_ptr track;
    metadb_v2_rec_t track_info;
    Type type;
};

class LyricSearchHandle
{
public:
    LyricSearchHandle(bool was_invoked_automatically, metadb_handle_ptr track, metadb_v2_rec_t track_info, abort_callback& abort);
    LyricSearchHandle(const LyricSearchHandle& other) = delete;
    LyricSearchHandle(LyricSearchHandle&& other);
    ~LyricSearchHandle();

    LyricUpdate::Type get_type();
    std::string get_progress();
    bool wait_for_complete(uint32_t timeout_ms);
    bool is_complete();
    bool has_result();
    bool has_searched_remote_sources(); // True if this update handle has searched any remote sources
    LyricData get_result();

    abort_callback& get_checked_abort(); // Checks the abort flag (so it might throw) and returns it
    metadb_handle_ptr get_track();
    const metadb_v2_rec_t& get_track_info();

    void set_started();
    void set_progress(std::string_view value);
    void set_remote_source_searched();
    void set_result(LyricData&& data, bool final_result);
    void set_complete();

private:
    enum class Status
    {
        Unknown,
        Created,
        Running,
        Complete,
        Closed
    };

    const metadb_handle_ptr m_track;
    const metadb_v2_rec_t m_track_info;
    const LyricUpdate::Type m_type;

    CRITICAL_SECTION m_mutex;
    std::vector<LyricData> m_lyrics;
    abort_callback& m_abort;
    HANDLE m_complete;
    Status m_status;
    std::string m_progress;
    bool m_searched_remote_sources;
};

namespace io
{
    void search_for_lyrics(LyricSearchHandle& handle, bool local_only);
    void search_for_all_lyrics(LyricSearchHandle& handle, std::string artist, std::string album, std::string title);

    std::optional<LyricData> process_available_lyric_update(LyricUpdate update);

    // Updates the lyric data with the ID of the source used for saving, as well as the persistence path that it reports.
    // Returns a success flag
    bool save_lyrics(metadb_handle_ptr track, const metadb_v2_rec_t& track_info, LyricData& lyrics, bool allow_overwrite);

    bool delete_saved_lyrics(metadb_handle_ptr track, const LyricData& lyrics);
}

