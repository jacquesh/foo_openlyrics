#pragma once

#include "lyric_io.h"
#include "metadb_index_search_avoidance.h"

struct SearchTracker
{
    std::unique_ptr<LyricSearchHandle> handle;
    SearchAvoidanceReason avoidance_reason;
};

class LyricUpdateQueue : public initquit, private play_callback
{
public:
    static void initiate_search(metadb_handle_ptr track, metadb_v2_rec_t track_info, bool ignore_search_avoidance);
    static std::optional<std::string> get_progress_message();

    void on_init() override;
    void on_quit() override;

private:
    void on_playback_starting(play_control::t_track_command /*cmd*/, bool /*paused*/) override {}
    void on_playback_new_track(metadb_handle_ptr track) override;
    void on_playback_stop(play_control::t_stop_reason /*reason*/) override {}
    void on_playback_seek(double /*time*/) override {}
    void on_playback_pause(bool /*state*/) override {}
    void on_playback_edited(metadb_handle_ptr /*track*/) override {}
    void on_playback_dynamic_info(const file_info& /*info*/) override {}
    void on_playback_dynamic_info_track(const file_info& info) override;
    void on_playback_time(double /*time*/) override {}
    void on_volume_change(float /*new_volume*/) override {}

    void internal_initiate_search(metadb_handle_ptr track, metadb_v2_rec_t track_info, bool ignore_search_avoidance);
    void internal_check_for_available_updates();
    std::optional<std::string> internal_get_progress_message();

    metadb_handle_ptr m_last_played_track;
    std::vector<SearchTracker> m_search_handles;
    std::mutex m_handle_mutex;
};
