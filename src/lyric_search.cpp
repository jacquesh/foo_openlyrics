#include "stdafx.h"

#pragma warning(push, 0)
#include <foobar2000/SDK/metadb_info_container_impl.h>
#pragma warning(pop)

#include "logging.h"
#include "lyric_io.h"
#include "lyric_search.h"
#include "metadb_index_search_avoidance.h"
#include "tag_util.h"
#include "ui_hooks.h"

struct SearchTracker
{
    std::unique_ptr<LyricSearchHandle> handle;
    SearchAvoidanceReason avoidance_reason;
};

class LyricUpdateQueue : public initquit, private play_callback
{
public:
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

    void initiate_search(metadb_handle_ptr track, metadb_v2_rec_t track_info, bool ignore_search_avoidance);
    void check_for_available_updates();
    std::optional<std::string> get_progress_message();

    metadb_handle_ptr m_last_played_track;
    std::vector<SearchTracker> m_search_handles;
    std::mutex m_handle_mutex;

    friend void initiate_lyrics_autosearch(metadb_handle_ptr, metadb_v2_rec_t, bool);
    friend std::optional<std::string> get_autosearch_progress_message();
};
namespace {
    static initquit_factory_t<LyricUpdateQueue> g_lyric_update_queue;
}

// =========================================
// Free functions that expose the public API
// =========================================
void initiate_lyrics_autosearch(metadb_handle_ptr track, metadb_v2_rec_t track_info, bool ignore_search_avoidance)
{
    g_lyric_update_queue.get_static_instance().initiate_search(track, track_info, ignore_search_avoidance);
}
std::optional<std::string> get_autosearch_progress_message()
{
    return g_lyric_update_queue.get_static_instance().get_progress_message();
}

// ==============================================
// Instance functions that handle the actual work
// ==============================================
void LyricUpdateQueue::on_init()
{
    play_callback_manager::get()->register_callback(this, flag_on_playback_all, false); // TODO: Check this

    fb2k::splitTask([this](){
        while(!fb2k::mainAborter().is_aborting())
        {
            check_for_available_updates();
            Sleep(50);
        }
    });
}

void LyricUpdateQueue::on_quit()
{
    play_callback_manager::get()->unregister_callback(this);
}

void LyricUpdateQueue::check_for_available_updates()
{
    const auto is_search_complete = [this](const SearchTracker& tracker)
    {
        const std::unique_ptr<LyricSearchHandle>& handle = tracker.handle;

        const bool has_result = handle->has_result();
        const bool is_complete = handle->is_complete();
        const bool didnt_find_anything = (!has_result && is_complete);
        if(has_result)
        {
            LyricUpdate update = {
                handle->get_result(),
                handle->get_track(),
                handle->get_track_info(),
                handle->get_type()
            };
            announce_lyric_update(std::move(update));
        }

        if(didnt_find_anything && (tracker.avoidance_reason != SearchAvoidanceReason::Allowed))
        {
            announce_lyric_search_avoided(tracker.handle->get_track(), tracker.avoidance_reason);
        }
        return is_complete;
    };

    m_handle_mutex.lock();
    auto new_end = std::remove_if(m_search_handles.begin(),
                                  m_search_handles.end(),
                                  is_search_complete);
    m_search_handles.erase(new_end, m_search_handles.end());
    m_handle_mutex.unlock();
}

void LyricUpdateQueue::initiate_search(metadb_handle_ptr track, metadb_v2_rec_t track_info, bool ignore_search_avoidance)
{
    const SearchAvoidanceReason avoid_reason = ignore_search_avoidance
                                               ? SearchAvoidanceReason::Allowed
                                               : search_avoidance_allows_search(track, track_info);
    const bool search_local_only = (avoid_reason != SearchAvoidanceReason::Allowed);
    // NOTE: We also track a generation counter that increments every time you change the search config
    //       so that if you don't find lyrics with some active sources and then add more, it'll search
    //       again at least once, possibly finding something if there are new active sources.
    if(search_local_only)
    {
        LOG_INFO("Search avoidance skipped remote sources for this track: %s", search_avoid_reason_to_string(avoid_reason));
    }

    auto handle = std::make_unique<LyricSearchHandle>(LyricUpdate::Type::AutoSearch, track, track_info, fb2k::mainAborter());
    io::search_for_lyrics(*handle, search_local_only);

    m_handle_mutex.lock();
    m_search_handles.push_back({std::move(handle), avoid_reason});
    m_handle_mutex.unlock();
}

std::optional<std::string> LyricUpdateQueue::get_progress_message()
{
    core_api::ensure_main_thread();

    metadb_handle_ptr now_playing = nullptr;
    service_ptr_t<playback_control> playback = playback_control::get();
    const bool has_now_playing = playback->get_now_playing(now_playing);

    if(has_now_playing)
    {
        std::lock_guard lock(m_handle_mutex);
        for(const SearchTracker& tracker : m_search_handles)
        {
            assert(tracker.handle != nullptr);
            if((tracker.handle->get_type() == LyricUpdate::Type::AutoSearch) &&
                (tracker.handle->get_track() == now_playing))
            {
                return tracker.handle->get_progress();
            }
        }
    }

    return {};
}

void LyricUpdateQueue::on_playback_new_track(metadb_handle_ptr track)
{
    assert(track != nullptr);

    const bool track_changed = (track != m_last_played_track);
    m_last_played_track = track;

    const bool search_postponed_for_dynamic_info = track_is_remote(track); // If this is an internet radio then don't search until we get dynamic track info
    const bool search_prevented_by_no_panels = (num_visible_lyric_panels() == 0) &&
                                               !preferences::searching::should_search_without_panels();
    const bool should_search = track_changed &&
                               !search_postponed_for_dynamic_info &&
                               !search_prevented_by_no_panels;
    if(!should_search)
    {
        return;
    }

    initiate_search(track, get_full_metadata(track), false);
}

void LyricUpdateQueue::on_playback_dynamic_info_track(const file_info& info)
{
    service_ptr_t<playback_control> playback = playback_control::get();
    metadb_handle_ptr track;
    if(!playback->get_now_playing(track))
    {
        return;
    }

    // NOTE: This is not called when we start playing tracks that are not remote/internet radio
    service_ptr_t<metadb_info_container_const_impl> info_container_impl = new service_impl_t<metadb_info_container_const_impl>();
    info_container_impl->m_info = info;

    metadb_v2_rec_t meta_record = {};
    meta_record.info = info_container_impl;

    initiate_search(track, std::move(meta_record), false);
}
