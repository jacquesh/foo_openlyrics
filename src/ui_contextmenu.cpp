#include "stdafx.h"

#include "logging.h"
#include "lyric_io.h"
#include "lyric_metadata.h"
#include "metadb_index_search_avoidance.h"
#include "metrics.h"
#include "parsers.h"
#include "tag_util.h"
#include "ui_hooks.h"
#include "ui_util.h"
#include "ui_lyrics_panel.h"

static const GUID GUID_OPENLYRICS_CTX_POPUP = { 0x99cb0828, 0x6b73, 0x404f, { 0x95, 0xcd, 0x29, 0xca, 0x63, 0x50, 0x4c, 0xea } };
static const GUID GUID_OPENLYRICS_CTX_SUBGROUP = { 0x119bf93d, 0xdeec, 0x4fd2, { 0x80, 0xbb, 0x91, 0x6a, 0x58, 0x6a, 0x2, 0x25 } };

static contextmenu_group_popup_factory g_ctx_item_factory(GUID_OPENLYRICS_CTX_POPUP, contextmenu_groups::root, "OpenLyrics");
static contextmenu_group_factory g_openlyrics_ctx_subgroup(GUID_OPENLYRICS_CTX_SUBGROUP, GUID_OPENLYRICS_CTX_POPUP, 0);

class OpenLyricsContextSubItem : public contextmenu_item_simple
{
public:
    GUID get_parent() override { return GUID_OPENLYRICS_CTX_SUBGROUP; }
    unsigned get_num_items() override { return cmd_total; }

    void get_item_name(unsigned int index, pfc::string_base& out) override
    {
        switch(index)
        {
            case cmd_show_lyrics: out = "Show lyrics"; break;
            case cmd_bulksearch_lyrics: out = "Search for lyrics"; break;
            case cmd_manualsearch_lyrics: out = "Search for lyrics (manually)"; break;
            case cmd_edit_lyrics: out = "Edit lyrics"; break;
            case cmd_mark_instrumental: out = "Mark as instrumental"; break;
            case cmd_seek_to_next_lyric: out = "Seek to next lyric timestamp"; break;
            case cmd_seek_to_prev_lyric: out = "Seek to prev lyric timestamp"; break;
            case cmd_seek_to_repeat_current_lyric: out = "Seek to repeat current lyric timestamp"; break;
            default: uBugCheck();
        }
    }

    bool context_get_display(unsigned int index, metadb_handle_list_cref data, pfc::string_base& out_name, unsigned int& out_display_flags, const GUID& caller) override
    {
        contextmenu_item_simple::context_get_display(index, data, out_name, out_display_flags, caller);

        // Disable & grey-out items that only apply to single selection, when many items are selected
        if(data.get_count() > 1)
        {
            switch(index)
            {
                case cmd_show_lyrics:
                case cmd_manualsearch_lyrics:
                case cmd_edit_lyrics:
                case cmd_seek_to_next_lyric:
                case cmd_seek_to_prev_lyric:
                case cmd_seek_to_repeat_current_lyric:
                {
                    out_display_flags = contextmenu_item_simple::FLAG_DISABLED_GRAYED;
                } break;

                case cmd_bulksearch_lyrics:
                case cmd_mark_instrumental:
                {
                    // No change, keep default behaviour
                } break;

                default: uBugCheck();
            }
        }
        return true;
    }

    void context_command(unsigned int index, metadb_handle_list_cref data, const GUID& /*caller*/) override
    {
        switch(index)
        {
            case cmd_show_lyrics:
            {
                if(data.get_count() == 0) break;
                metrics::log_used_show_lyrics();

                metadb_handle_ptr track = data.get_item(0);

                const auto async_search = [track](threaded_process_status& /*status*/, abort_callback& abort)
                {
                    const metadb_v2_rec_t track_info = get_full_metadata(track);
                    std::string dialog_title = "Track lyrics";
                    std::string track_title = track_metadata(track_info, "title");
                    if(!track_title.empty())
                    {
                        dialog_title = '\'';
                        dialog_title += track_title;
                        dialog_title += "' lyrics";
                    }

                    LyricSearchHandle handle(LyricUpdate::Type::InternalSearch, track, track_info, abort);
                    io::search_for_lyrics(handle, true);
                    bool success = handle.wait_for_complete(30'000);
                    if(success)
                {
                        if(handle.has_result())
                        {
                            const LyricData lyrics = handle.get_result();
                            const std::tstring text = parsers::lrc::expand_text(lyrics, false);
                            lyric_metadata_log_retrieved(track_info, lyrics);

                            const std::string dialog_contents = std::format("{}\n---\n\n{}",
                                    get_lyric_metadata_string(lyrics, track_info),
                                    from_tstring(text));
                            popup_message::g_show(dialog_contents.c_str(), dialog_title.c_str());
                        }
                        else
                        {
                            popup_message::g_show("No lyrics saved", dialog_title.c_str());
                        }
                    }
                    else
                    {
                        popup_message::g_show("Failed to retrieve locally-saved lyrics", dialog_title.c_str());
                    }
                };

                threaded_process::g_run_modeless(threaded_process_callback_lambda::create(async_search),
                                                 threaded_process::flag_show_delayed | threaded_process::flag_show_abort,
                                                 core_api::get_main_window(),
                                                 "Retrieving saved lyrics...");
            } break;

            case cmd_bulksearch_lyrics:
            {
                std::vector<metadb_handle_ptr> tracks;
                tracks.reserve(data.get_count());
                for(size_t i=0; i<data.get_count(); i++)
                {
                    tracks.push_back(data.get_item(i));
                }
                SpawnBulkLyricSearch(tracks);
            } break;

            case cmd_manualsearch_lyrics:
            {
                if(data.get_count() == 0) break;
                metadb_handle_ptr track = data.get_item(0);

                SpawnManualLyricSearch(track, get_full_metadata(track));
            } break;

            case cmd_edit_lyrics:
            {
                if(data.get_count() == 0) break;
                metadb_handle_ptr track = data.get_item(0);
                
                const auto async_edit = [track](threaded_process_status& /*status*/, abort_callback& abort)
                {
                    const metadb_v2_rec_t track_info = get_full_metadata(track);
                    LyricSearchHandle search_handle(LyricUpdate::Type::InternalSearch, track, track_info, abort);
                    io::search_for_lyrics(search_handle, true);
                    bool success = search_handle.wait_for_complete(30'000);
                    if(success)
                    {
                        // If we don't already have lyrics for a track then search will return no results,
                        // so we need to construct our own blank lyric data
                        LyricData lyrics;
                        if(search_handle.has_result())
                        {
                            lyrics = search_handle.get_result();
                            lyric_metadata_log_retrieved(track_info, lyrics);
                        }
                        else
                        {
                            lyrics.artist = track_metadata(track_info, "artist");
                            lyrics.album = track_metadata(track_info, "album");
                            lyrics.title = track_metadata(track_info, "title");
                            lyrics.duration_sec = track_duration_in_seconds(track_info);
                        }

                        fb2k::inMainThread2([lyrics, track, track_info]()
                        {
                            SpawnLyricEditor(lyrics, track, track_info);
                        });
                    }
                };

                threaded_process::g_run_modeless(threaded_process_callback_lambda::create(async_edit),
                                                 threaded_process::flag_show_delayed | threaded_process::flag_show_abort,
                                                 core_api::get_main_window(),
                                                 "Retrieving saved lyrics...");
            } break;

            case cmd_mark_instrumental:
            {
                metrics::log_used_mark_instrumental();

                size_t track_count = data.get_count();
                std::string msg;
                if(track_count == 1)
                {
                    msg = "This will delete the lyrics stored locally for the selected track ";
                    std::string track_str = get_track_friendly_string(get_full_metadata(data[0]));
                    if(!track_str.empty())
                    {
                        msg += "(" + track_str + ") ";
                    }
                    msg += "and mark the track as instrumental. OpenLyrics will no longer search for lyrics for this track automatically so it will not show any lyrics for this track until you explicitly request a search for it.\n\nAre you sure you want to proceed?";
                }
                else
                {
                    msg = "This will delete the lyrics stored locally for the ";
                    msg += std::to_string(track_count);
                    msg += " selected tracks and mark those tracks as instrumental. OpenLyrics will no longer search for lyrics for those tracks automatically so it will not show any lyrics for those tracks until you explicitly request a search for it.\n\nAre you sure you want to proceed?";
                }
                popup_message_v3::query_t query = {};
                query.title = "Confirm delete & mark as instrumental";
                query.msg = msg.c_str();
                query.buttons = popup_message_v3::buttonYes | popup_message_v3::buttonNo;
                query.defButton = popup_message_v3::buttonNo;
                query.icon = popup_message_v3::iconWarning;
                uint32_t popup_result = popup_message_v3::get()->show_query_modal(query);
                if(popup_result != popup_message_v3::buttonYes)
                {
                    break;
                }

                pfc::list_t<metadb_handle_ptr> data_copy;
                data_copy.add_items(data);
                const auto async_instrumental = [data_copy, track_count](threaded_process_status& /*status*/, abort_callback& abort)
                {
                    size_t failure_count = 0;

                    pfc::array_t<metadb_v2_rec_t> all_track_info;
                    all_track_info.resize(track_count);

                    metadb_v2::ptr meta;
                    if(metadb_v2::tryGet(meta)) // metadb_v2 is only available from fb2k v2.0 onwards
                    {
                        const auto fill_track_info = [&all_track_info](size_t idx, const metadb_v2_rec_t& rec) { all_track_info[idx] = rec; };
                        meta->queryMultiParallel_(data_copy, fill_track_info);
                    }
                    else
                    {
                        for(size_t i=0; i<track_count; i++)
                        {
                            all_track_info[i] = get_full_metadata(data_copy[i]);
                        }
                    }

                    LOG_INFO("Marking %d tracks as instrumental from the context menu...", int(track_count));
                    for(size_t i=0; i<track_count; i++)
                    {
                        metadb_handle_ptr track = data_copy.get_item(i);
                        const metadb_v2_rec_t& track_info = all_track_info[i];

                        LyricSearchHandle handle(LyricUpdate::Type::InternalSearch, track, track_info, abort);
                        io::search_for_lyrics(handle, true);
                        bool success = handle.wait_for_complete(30'000);
                        if(success && handle.has_result())
                        {
                            LyricData lyrics = handle.get_result();
                            assert(!lyrics.IsEmpty()); // Search should never return empty lyrics. Instead it should complete without a result

                            const bool delete_success = io::delete_saved_lyrics(track, lyrics);
                            if(!delete_success)
                            {
                                failure_count++;
                            }
                        }
                        search_avoidance_force_by_mark_instrumental(track, track_info);
                    }
                    LOG_INFO("Finished marking %d tracks as instrumental with %d failures", int(failure_count));

                    fb2k::inMainThread2([failure_count, track_count]()
                    {
                        std::string msg;
                        if(failure_count == 0)
                        {
                            msg += "Successfully marked all ";
                            msg += std::to_string(track_count);
                            msg += " tracks as instrumental";
                        }
                        else
                        {
                            msg += "Failed to mark ";
                            msg += std::to_string(failure_count);
                            msg += " (of ";
                            msg += std::to_string(track_count);
                            msg += ") tracks as instrumental! See the log for details.";
                        }

                        popup_message_v3::query_t result_query = {};
                        result_query.title = "Mark as Instrumental";
                        result_query.msg = msg.c_str();
                        result_query.buttons = popup_message_v3::buttonOK;
                        result_query.icon = (failure_count == 0) ? popup_message_v3::iconInformation : popup_message_v3::iconWarning;
                        popup_message_v3::get()->show_query_modal(result_query);
                    });
                };

                threaded_process::g_run_modeless(threaded_process_callback_lambda::create(async_instrumental),
                                                 threaded_process::flag_show_delayed | threaded_process::flag_show_abort,
                                                 core_api::get_main_window(),
                                                 "Marking tracks as instrumental...");
            } break;
            case cmd_seek_to_next_lyric: {
                auto& panels = get_active_panels();
                if (!panels.empty())
                {
                    LyricPanel* panel = panels[0];
                    LyricData lyrics = panel->get_lyrics();
                    service_ptr_t<playback_control> playback = playback_control::get();

                    double timeNow = playback->playback_get_position();

                    for (size_t i = 0; i < lyrics.lines.size(); ++i)
                    {
                        double ts = lyrics.lines[i].timestamp;
                        if (ts > timeNow)
                        {
                            playback->playback_seek(ts);
                            break;
                        }
                    }
                }
                break;
            }
            case cmd_seek_to_prev_lyric: {
                auto& panels = get_active_panels();
                if (!panels.empty())
                {
                    LyricPanel* panel = panels[0];
                    LyricData lyrics = panel->get_lyrics();
                    service_ptr_t<playback_control> playback = playback_control::get();

                    bool skipOne = true;
                    double timeNow = playback->playback_get_position();

                    for (size_t i = lyrics.lines.size(); i-- > 0; )
                    {
                        double ts = lyrics.lines[i].timestamp;
                        if (ts < timeNow)
                        {
                            if (skipOne) 
                            {
                                skipOne = false;
                                continue;
                            }
                            playback->playback_seek(ts);
                            break;
                        }
                    }
                }
                break;
            }
            case cmd_seek_to_repeat_current_lyric: {
                auto& panels = get_active_panels();
                if (!panels.empty())
                {
                    LyricPanel* panel = panels[0];
                    LyricData lyrics = panel->get_lyrics();
                    service_ptr_t<playback_control> playback = playback_control::get();

                    double timeNow = playback->playback_get_position();

                    for (size_t i = lyrics.lines.size(); i-- > 0; )
                    {
                        double ts = lyrics.lines[i].timestamp;
                        if (ts < timeNow)
                        {
                            playback->playback_seek(ts);
                            break;
                        }
                    }
                }
                break;
            }
            
            default:
                LOG_ERROR("Unexpected openlyrics context menu command: %d", int(index));
                uBugCheck();
        }
    }

    GUID get_item_guid(unsigned int index) override
    {
        static const GUID GUID_ITEM_SHOW_LYRICS = { 0xff674f8, 0x8536, 0x4edc, { 0xb1, 0xd7, 0xf7, 0xad, 0x72, 0x87, 0x84, 0x3c } };
        static const GUID GUID_ITEM_BULKSEARCH_LYRICS = { 0x16d008, 0x83ec, 0x4d0a, { 0x9a, 0x1a, 0x9e, 0xae, 0xc8, 0x24, 0x2, 0x5d } };
        static const GUID GUID_ITEM_MANUALSEARCH_LYRICS = { 0x9fac3e8e, 0xa847, 0x4b73, { 0x90, 0xe4, 0xc9, 0x5, 0x49, 0xf9, 0xe9, 0x32 } };
        static const GUID GUID_ITEM_EDIT_LYRICS = { 0x518d992d, 0xd61b, 0x4cfd, { 0x8f, 0xcf, 0x8c, 0x7f, 0x21, 0xd0, 0x59, 0x2c } };
        static const GUID GUID_ITEM_MARK_INSTRUMENTAL = { 0x23b658fc, 0x71e1, 0x4e3c, { 0x87, 0xe0, 0xb, 0x34, 0x8c, 0x26, 0x3f, 0x59 } };
        static const GUID GUID_ITEM_SEEK_TO_NEXT_LYRIC = { 0x3e8f4dc9, 0xa3c9, 0x4f95, { 0x94, 0x47, 0x32, 0xc5, 0x6b, 0x1a, 0xe1, 0x6e } };
        static const GUID GUID_ITEM_SEEK_TO_PREV_LYRIC = { 0x8b5a1a77, 0x4cc4, 0x4e1a, { 0x90, 0x39, 0xaa, 0xde, 0x3f, 0x48, 0x62, 0x1f } };
        static const GUID GUID_ITEM_SEEK_TO_REPEAT_LYRIC = { 0xa7d1f6c2, 0x2bd5, 0x4a7b, { 0x8f, 0x6e, 0x3a, 0x91, 0x2b, 0xcc, 0xd4, 0x55 } };

        switch(index)
        {
            case cmd_show_lyrics: return GUID_ITEM_SHOW_LYRICS;
            case cmd_bulksearch_lyrics: return GUID_ITEM_BULKSEARCH_LYRICS;
            case cmd_manualsearch_lyrics: return GUID_ITEM_MANUALSEARCH_LYRICS;
            case cmd_edit_lyrics: return GUID_ITEM_EDIT_LYRICS;
            case cmd_mark_instrumental: return GUID_ITEM_MARK_INSTRUMENTAL;
            case cmd_seek_to_next_lyric: return GUID_ITEM_SEEK_TO_NEXT_LYRIC;
            case cmd_seek_to_prev_lyric: return GUID_ITEM_SEEK_TO_PREV_LYRIC;
            case cmd_seek_to_repeat_current_lyric: return GUID_ITEM_SEEK_TO_REPEAT_LYRIC;
            default: uBugCheck();
        }
    }

    bool get_item_description(unsigned int index, pfc::string_base& out) override
    {
        switch(index)
        {
            case cmd_show_lyrics:
                out = "Show saved lyrics (if any) for this track";
                return true;
            case cmd_bulksearch_lyrics:
                out = "Search for lyrics for all selected tracks (as if you played them one after the other)";
                return true;
            case cmd_manualsearch_lyrics:
                out = "Search for lyrics for this track with customisable search terms and multiple results";
                return true;
            case cmd_edit_lyrics:
                out = "Open the lyric editor with the lyrics for this track";
                return true;
            case cmd_mark_instrumental:
                out = "Remove existing lyrics and skip future automated lyric searches";
                return true;
            case cmd_seek_to_next_lyric:
                out = "Seek to next lyrics timestamp";
                return true;
            case cmd_seek_to_prev_lyric:
                out = "Seek to prev lyrics timestamp";
                return true;
            case cmd_seek_to_repeat_current_lyric:
                out = "Seek to current lyrics timestamp,  repeat current line";
                return true;
            default:
                uBugCheck();
        }
    }

private:
    enum
    {
        cmd_show_lyrics = 0,
        cmd_bulksearch_lyrics,
        cmd_manualsearch_lyrics,
        cmd_edit_lyrics,
        cmd_mark_instrumental,
        cmd_seek_to_next_lyric,
        cmd_seek_to_prev_lyric,
        cmd_seek_to_repeat_current_lyric,
        cmd_total
    };
};

static contextmenu_item_factory_t<OpenLyricsContextSubItem> g_ctx_subitem_factory;

