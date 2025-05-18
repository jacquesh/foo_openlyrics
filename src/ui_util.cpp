#include "stdafx.h"

#include "logging.h"
#include "lyric_data.h"
#include "tag_util.h"

static std::string get_friendly_string(std::string_view artist, std::string_view title)
{
    std::string result;
    const bool has_artist = !artist.empty();
    const bool has_title = !title.empty();
    if(has_artist || has_title)
    {
        if(has_artist)
        {
            result += artist;
        }
        if(has_artist && has_title)
        {
            result += " - ";
        }
        if(has_title)
        {
            result += title;
        }
    }
    return result;
}

std::string get_track_friendly_string(const metadb_v2_rec_t& track_info)
{
    return get_friendly_string(track_metadata(track_info, "artist"), track_metadata(track_info, "title"));
}

metadb_handle_ptr get_format_preview_track()
{
    metadb_handle_ptr preview_track = nullptr;
    service_ptr_t<playback_control> playback = playback_control::get();
    if(playback->get_now_playing(preview_track))
    {
        LOG_INFO("Playback is currently active, using the now-playing track for format preview");
    }
    else
    {
        pfc::list_t<metadb_handle_ptr> selection;

        service_ptr_t<playlist_manager> playlist = playlist_manager::get();
        playlist->activeplaylist_get_selected_items(selection);

        if(selection.get_count() > 0)
        {
            LOG_INFO("Using the first selected item for format preview");
            preview_track = selection[0];
        }
        else if(playlist->activeplaylist_get_item_handle(preview_track, 0))
        {
            LOG_INFO("No selection available, using the first playlist item for format preview");
        }
        else
        {
            LOG_INFO("No selection available & no active playlist. There will be no format preview");
        }
    }

    return preview_track;
}
