#include "stdafx.h"

#include "logging.h"
#include "lyric_io.h"
#include "parsers.h"

static const GUID GUID_OPENLYRICS_CTX_GROUP = { 0x99cb0828, 0x6b73, 0x404f, { 0x95, 0xcd, 0x29, 0xca, 0x63, 0x50, 0x4c, 0xea } };
static contextmenu_group_factory g_openlyrics_ctx_group(GUID_OPENLYRICS_CTX_GROUP, contextmenu_groups::root, 0);

class OpenLyricsContextItem : public contextmenu_item_simple
{
public:
    GUID get_parent() { return GUID_OPENLYRICS_CTX_GROUP; }
    unsigned get_num_items() { return cmd_total; }

    void get_item_name(unsigned int index, pfc::string_base& out)
    {
        switch(index)
        {
            case cmd_show_lyrics: out = "Show lyrics"; break;
            default: uBugCheck();
        }
    }

    void context_command(unsigned int index, metadb_handle_list_cref data, const GUID& /*caller*/)
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
                    const metadb_info_container::ptr& track_info_container = track->get_info_ref();
                    const file_info& track_info = track_info_container->info();
                    size_t track_title_index = track_info.meta_find("title");
                    if((track_title_index != pfc::infinite_size) && (track_info.meta_enum_value_count(track_title_index) > 0))
                    {
                        dialog_title = '\'' + std::string(track_info.meta_enum_value(track_title_index, 0)) + "' lyrics";
                    }

                    LyricUpdateHandle update(LyricUpdateHandle::Type::Search, track, abort);
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

            default:
                uBugCheck();
        }
    }

    GUID get_item_guid(unsigned int index)
    {
        static const GUID GUID_ITEM_SHOW_LYRICS = { 0xff674f8, 0x8536, 0x4edc, { 0xb1, 0xd7, 0xf7, 0xad, 0x72, 0x87, 0x84, 0x3c } };

        switch(index)
        {
            case cmd_show_lyrics: return GUID_ITEM_SHOW_LYRICS;
            default: uBugCheck();
        }
    }

    bool get_item_description(unsigned int index, pfc::string_base& out)
    {
        switch(index)
        {
            case cmd_show_lyrics:
                out = "Show saved lyrics (if any) for this track";
                return true;
            default:
                uBugCheck();
        }
    }

private:
    enum
    {
        cmd_show_lyrics = 0,
        cmd_total
    };
};

static contextmenu_item_factory_t<OpenLyricsContextItem> g_myitem_factory;
