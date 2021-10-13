#include "stdafx.h"

#include "logging.h"
#include "lyric_io.h"
#include "parsers.h"
#include "ui_hooks.h"

static const GUID GUID_OPENLYRICS_CTX_GROUP = { 0x99cb0828, 0x6b73, 0x404f, { 0x95, 0xcd, 0x29, 0xca, 0x63, 0x50, 0x4c, 0xea } };
static contextmenu_group_factory g_openlyrics_ctx_group(GUID_OPENLYRICS_CTX_GROUP, contextmenu_groups::root, 0);

class OpenLyricsContextItem : public contextmenu_item_simple
{
public:
    GUID get_parent() override { return GUID_OPENLYRICS_CTX_GROUP; }
    unsigned get_num_items() override { return cmd_total; }

    void get_item_name(unsigned int index, pfc::string_base& out) override
    {
        switch(index)
        {
            case cmd_show_lyrics: out = "Show lyrics"; break;
            case cmd_bulksearch_lyrics: out = "Search for lyrics"; break;
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

            default:
                uBugCheck();
        }
    }

    GUID get_item_guid(unsigned int index) override
    {
        static const GUID GUID_ITEM_SHOW_LYRICS = { 0xff674f8, 0x8536, 0x4edc, { 0xb1, 0xd7, 0xf7, 0xad, 0x72, 0x87, 0x84, 0x3c } };
        static const GUID GUID_ITEM_BULKSEARCH_LYRICS = { 0x16d008, 0x83ec, 0x4d0a, { 0x9a, 0x1a, 0x9e, 0xae, 0xc8, 0x24, 0x2, 0x5d } };

        switch(index)
        {
            case cmd_show_lyrics: return GUID_ITEM_SHOW_LYRICS;
            case cmd_bulksearch_lyrics: return GUID_ITEM_BULKSEARCH_LYRICS;
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
            default:
                uBugCheck();
        }
    }

private:
    enum
    {
        cmd_show_lyrics = 0,
        cmd_bulksearch_lyrics,
        cmd_total
    };
};

static contextmenu_item_factory_t<OpenLyricsContextItem> g_myitem_factory;

