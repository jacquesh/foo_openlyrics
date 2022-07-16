#include "stdafx.h"

#include "logging.h"
#include "lyric_io.h"
#include "metadb_index_search_avoidance.h"
#include "parsers.h"
#include "ui_hooks.h"
#include "ui_util.h"

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
            default: uBugCheck();
        }
    }

    void context_command(unsigned int index, metadb_handle_list_cref data, const GUID& /*caller*/) override
    {
        switch(index)
        {
            case cmd_show_lyrics:
            {
                if(data.get_count() == 0) break;
                metadb_handle_ptr track = data.get_item(0);

                const auto async_search = [track](threaded_process_status& /*status*/, abort_callback& abort)
                {
                    std::string dialog_title = "Track lyrics";
                    std::string track_title = track_metadata(track, "title");
                    if(!track_title.empty())
                    {
                        dialog_title = '\'';
                        dialog_title += track_title;
                        dialog_title += "' lyrics";
                    }

                    LyricUpdateHandle update(LyricUpdateHandle::Type::AutoSearch, track, abort);
                    io::search_for_lyrics(update, true);
                    bool success = update.wait_for_complete(30'000);
                    if(success)
                    {
                        LyricData lyrics = update.get_result();
                        std::tstring text = parsers::lrc::expand_text(lyrics);
                        if(text.empty())
                        {
                            popup_message::g_show("No lyrics saved", dialog_title.c_str());
                        }
                        else
                        {
                            std::string text_utf8 = from_tstring(text);
                            popup_message::g_show(text_utf8.c_str(), dialog_title.c_str());
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

                auto update = std::make_unique<LyricUpdateHandle>(LyricUpdateHandle::Type::ManualSearch, track, fb2k::noAbort/*TODO*/);
                if(are_there_any_lyric_panels())
                {
                    SpawnManualLyricSearch(core_api::get_main_window(), *update);
                }
                else
                {
                    update->set_started();
                }
                register_update_handle_with_lyric_panels(std::move(update));
            } break;

            case cmd_edit_lyrics:
            {
                if(data.get_count() == 0) break;
                metadb_handle_ptr track = data.get_item(0);

                const auto async_instrumental = [track](threaded_process_status& /*status*/, abort_callback& abort)
                {
                    LyricUpdateHandle search_update(LyricUpdateHandle::Type::AutoSearch, track, abort);
                    io::search_for_lyrics(search_update, true);
                    bool success = search_update.wait_for_complete(30'000);
                    if(success)
                    {
                        LyricData lyrics = search_update.get_result();
                        fb2k::inMainThread2([lyrics, track]()
                        {
                            auto edit_update = std::make_unique<LyricUpdateHandle>(LyricUpdateHandle::Type::Edit, track, fb2k::noAbort);

                            if(are_there_any_lyric_panels())
                            {
                                SpawnLyricEditor(core_api::get_main_window(), lyrics, *edit_update);
                            }
                            else
                            {
                                edit_update->set_started();
                            }
                            register_update_handle_with_lyric_panels(std::move(edit_update));
                        });
                    }
                };

                threaded_process::g_run_modeless(threaded_process_callback_lambda::create(async_instrumental),
                                                 threaded_process::flag_show_delayed | threaded_process::flag_show_abort,
                                                 core_api::get_main_window(),
                                                 "Retrieving saved lyrics...");
            } break;

            case cmd_mark_instrumental:
            {
                size_t track_count = data.get_count();
                std::vector<metadb_handle_ptr> all_tracks;
                all_tracks.reserve(track_count);
                for(size_t i=0; i<track_count; i++)
                {
                    metadb_handle_ptr track = data.get_item(i);
                    all_tracks.push_back(track);
                }

                std::string msg;
                if(track_count == 1)
                {
                    msg = "This will delete the lyrics stored locally for the selected track ";
                    std::string track_str = get_track_friendly_string(all_tracks[0]);
                    if(!track_str.empty())
                    {
                        msg += "(" + track_str + ") ";
                    }
                    msg += "and mark the track as instrumental. OpenLyrics will no longer search for lyrics for this track automatically so it will not show any lyrics for this track until you explicitly request a search for it.\n\nAre you sure you want to proceed?";
                }
                else
                {
                    msg = "This will delete the lyrics stored locally for the ";
                    msg += std::to_string(all_tracks.size());
                    msg += " selected tracks";
                    msg += "and mark those tracks as instrumental. OpenLyrics will no longer search for lyrics for those tracks automatically so it will not show any lyrics for those tracks until you explicitly request a search for it.\n\nAre you sure you want to proceed?";
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

                const auto async_instrumental = [all_tracks](threaded_process_status& /*status*/, abort_callback& abort)
                {
                    size_t failure_count = 0;
                    for(metadb_handle_ptr track : all_tracks)
                    {
                        LyricUpdateHandle update(LyricUpdateHandle::Type::AutoSearch, track, abort);
                        io::search_for_lyrics(update, true);
                        bool success = update.wait_for_complete(30'000);
                        if(success)
                        {
                            LyricData lyrics = update.get_result();
                            if(!lyrics.IsEmpty())
                            {
                                bool delete_success = io::delete_saved_lyrics(track, lyrics);
                                if(!delete_success)
                                {
                                    failure_count++;
                                }
                            }
                        }
                        search_avoidance_force_avoidance(track);
                    }

                    fb2k::inMainThread2([failure_count, total_count = all_tracks.size()]()
                    {
                        std::string msg;
                        if(failure_count == 0)
                        {
                            msg += "Successfully marked all ";
                            msg += std::to_string(total_count);
                            msg += " tracks as instrumental";
                        }
                        else
                        {
                            msg += "Failed to mark ";
                            msg += std::to_string(failure_count);
                            msg += " (of ";
                            msg += std::to_string(total_count);
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

            default:
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

        switch(index)
        {
            case cmd_show_lyrics: return GUID_ITEM_SHOW_LYRICS;
            case cmd_bulksearch_lyrics: return GUID_ITEM_BULKSEARCH_LYRICS;
            case cmd_manualsearch_lyrics: return GUID_ITEM_MANUALSEARCH_LYRICS;
            case cmd_edit_lyrics: return GUID_ITEM_EDIT_LYRICS;
            case cmd_mark_instrumental: return GUID_ITEM_MARK_INSTRUMENTAL;
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
        cmd_total
    };
};

static contextmenu_item_factory_t<OpenLyricsContextSubItem> g_ctx_subitem_factory;

