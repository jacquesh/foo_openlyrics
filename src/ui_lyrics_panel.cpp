#include "stdafx.h"

#pragma warning(push, 0)
#include <libPPUI/win32_op.h>
#include <foobar2000/helpers/BumpableElem.h>
#pragma warning(pop)

#include "logging.h"
#include "lyric_data.h"
#include "preferences.h"
#include "sources/localfiles.h"
#include "ui_lyric_editor.h"
#include "winstr_util.h"

// TODO: This is currently just copied from preferences.cpp
static const GUID GUID_PREFERENCES_PAGE = { 0x29e96cfa, 0xab67, 0x4793, { 0xa1, 0xc3, 0xef, 0xc3, 0xa, 0xbc, 0x8b, 0x74 } };

namespace parsers
{
namespace plaintext { LyricData parse(const LyricDataRaw& input); } // TODO: From parsers/plaintext.cpp
namespace lrc { LyricData parse(const LyricDataRaw& input); } // TODO: From parsers/lrc.cpp
}

// TODO: from sources/azlyricscom.cpp
namespace sources::azlyricscom
{
    LyricDataRaw Query(const pfc::string8& artist, const pfc::string8& album, const pfc::string8& title);
}

namespace {
    static const GUID GUID_LYRICS_PANEL = { 0x6e24d0be, 0xad68, 0x4bc9,{ 0xa0, 0x62, 0x2e, 0xc7, 0xb3, 0x53, 0xd5, 0xbd } };
    static const UINT_PTR PANEL_UPDATE_TIMER = 2304692;

    class LyricPanel : public ui_element_instance, public CWindowImpl<LyricPanel>, private play_callback_impl_base
    {
    public:
        // ATL window class declaration. Replace class name with your own when reusing code.
        //DECLARE_WND_CLASS_EX(TEXT("{DC2917D5-1288-4434-A28C-F16CFCE13C4B}"),CS_VREDRAW | CS_HREDRAW,(-1));
        DECLARE_WND_CLASS_EX(NULL, CS_VREDRAW | CS_HREDRAW, (-1)); // TODO: Does this work? Docs say that if the first parma is null then ATL will generate a class name internally

        LyricPanel(ui_element_config::ptr,ui_element_instance_callback_ptr p_callback);
        HWND get_wnd();
        void initialize_window(HWND parent);
        void set_configuration(ui_element_config::ptr config);
        ui_element_config::ptr get_configuration();

        static GUID g_get_guid();
        static GUID g_get_subclass();
        static void g_get_name(pfc::string_base & out);
        static ui_element_config::ptr g_get_default_configuration();
        static const char * g_get_description();

        void notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size);

        void on_playback_new_track(metadb_handle_ptr p_track);
        void on_playback_stop(play_control::t_stop_reason p_reason);
        void on_playback_pause(bool p_state);
        void on_playback_seek(double new_time);

    private:
        LRESULT OnLyricSavedToDisk(UINT, WPARAM, LPARAM, BOOL&);
        LRESULT OnWindowCreate(LPCREATESTRUCT);
        void OnWindowDestroy();
        LRESULT OnTimer(WPARAM);
        void OnPaint(CDCHandle);
        BOOL OnEraseBkgnd(CDCHandle);
        void OnSize(UINT nType, CSize size);
        void OnContextMenu(CWindow window, CPoint point);

        void GetTrackMetaIdentifiers(metadb_handle_ptr track_handle, pfc::string8& out_artist, pfc::string8& out_album, pfc::string8& out_title);

        void StartTimer();
        void StopTimer();

        void LoadTrackLyrics(metadb_handle_ptr track);

        ui_element_config::ptr m_config;

        bool m_timerRunning;

        metadb_handle_ptr m_now_playing;
        LyricData m_lyrics;
        int m_lyrics_render_height;

    protected:
        // this must be declared as protected for ui_element_impl_withpopup<> to work.
        const ui_element_instance_callback_ptr m_callback;

        BEGIN_MSG_MAP_EX(LyricPanel)
            MESSAGE_HANDLER(WM_USER+1, OnLyricSavedToDisk); // TODO: Define a constant for this message
            MSG_WM_CREATE(OnWindowCreate)
            MSG_WM_DESTROY(OnWindowDestroy)
            MSG_WM_TIMER(OnTimer)
            MSG_WM_ERASEBKGND(OnEraseBkgnd)
            MSG_WM_PAINT(OnPaint)
            MSG_WM_SIZE(OnSize)
            MSG_WM_CONTEXTMENU(OnContextMenu)
        END_MSG_MAP()
    };

    HWND LyricPanel::get_wnd() { return *this; }
    void LyricPanel::initialize_window(HWND parent) { WIN32_OP(Create(parent) != NULL); }
    void LyricPanel::set_configuration(ui_element_config::ptr config) { m_config = config; }
    ui_element_config::ptr LyricPanel::get_configuration() { return m_config; }

    GUID LyricPanel::g_get_guid() { return GUID_LYRICS_PANEL; }
    GUID LyricPanel::g_get_subclass() { return ui_element_subclass_utility; }
    ui_element_config::ptr LyricPanel::g_get_default_configuration() { return ui_element_config::g_create_empty(g_get_guid()); }
    void LyricPanel::g_get_name(pfc::string_base & out) { out = "OpenLyrics Panel"; }
    const char * LyricPanel::g_get_description() { return "The OpenLyrics Lyrics Panel"; }

    LyricPanel::LyricPanel(ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback) :
        m_callback(p_callback),
        m_config(config),
        m_timerRunning(false),
        m_now_playing(nullptr),
        m_lyrics(),
        m_lyrics_render_height(0)
    {
    }

    void LyricPanel::notify(const GUID & what, t_size /*param1*/, const void * /*param2*/, t_size /*param2size*/)
    {
        if ((what == ui_element_notify_colors_changed) || (what == ui_element_notify_font_changed))
        {
            // we use global colors and fonts - trigger a repaint whenever these change.
            Invalidate();
        }
    }

    void LyricPanel::on_playback_new_track(metadb_handle_ptr p_track)
    {
        m_now_playing = p_track;

        LoadTrackLyrics(p_track);
        StartTimer();
    }

    void LyricPanel::on_playback_stop(play_control::t_stop_reason /*p_reason*/)
    {
        m_now_playing = nullptr;
        StopTimer();
        Invalidate(); // Draw one more time to clear the panel
    }

    void LyricPanel::on_playback_pause(bool p_state)
    {
        if (p_state)
        {
            StopTimer();
        }
        else
        {
            StartTimer();
        }
    }

    void LyricPanel::on_playback_seek(double new_time)
    {
        // TODO: Update the rendered lyrics text for the new time...maybe we just need to `Invalidate()`?
    }

    LRESULT LyricPanel::OnLyricSavedToDisk(UINT, WPARAM, LPARAM, BOOL&)
    {
        if(m_now_playing != nullptr)
        {
            LoadTrackLyrics(m_now_playing);
        }

        return 0;
    }

    LRESULT LyricPanel::OnWindowCreate(LPCREATESTRUCT /*create*/)
    {
        sources::localfiles::RegisterLyricPanel(get_wnd());
        return 0;
    }

    void LyricPanel::OnWindowDestroy()
    {
        sources::localfiles::DeregisterLyricPanel(get_wnd());
    }

    LRESULT LyricPanel::OnTimer(WPARAM /*wParam*/)
    {
        Invalidate();
        return 0;
    }

    BOOL LyricPanel::OnEraseBkgnd(CDCHandle dc)
    {
        return TRUE;
    }

    void LyricPanel::OnPaint(CDCHandle)
    {
        CRect client_rect;
        WIN32_OP_D(GetClientRect(&client_rect));

        CPaintDC front_buffer(*this);
        HDC back_buffer = CreateCompatibleDC(front_buffer.m_hDC);
        // As suggested in this article: https://docs.microsoft.com/en-us/previous-versions/ms969905(v=msdn.10)
        // We get flickering if we draw everything to the UI directly, so instead we render everything to a back buffer
        // and then blit the whole thing to the screen at the end.

        HBITMAP back_buffer_bitmap = CreateCompatibleBitmap(front_buffer, client_rect.Width(), client_rect.Height());
        SelectObject(back_buffer, back_buffer_bitmap);

        HBRUSH bg_brush = CreateSolidBrush(m_callback->query_std_color(ui_color_background));
        FillRect(back_buffer, &client_rect, bg_brush);
        DeleteObject(bg_brush);

        SetBkMode(back_buffer, TRANSPARENT);
        SelectObject(back_buffer, m_callback->query_font_ex(ui_font_console));
        SetTextColor(back_buffer, m_callback->query_std_color(ui_color_text));

        TEXTMETRIC font_metrics = {};
        WIN32_OP_D(GetTextMetrics(back_buffer, &font_metrics));

        service_ptr_t<playback_control> playback = playback_control::get();
        double current_position = playback->playback_get_position();
        double total_length = playback->playback_get_length_ex();

        const UINT draw_format = DT_NOPREFIX | DT_CENTER | DT_WORDBREAK;
        int line_gap = preferences::get_render_linegap();
        if(m_lyrics.format == LyricFormat::Plaintext)
        {
            double track_fraction = current_position / total_length;
            int text_rect_y = client_rect.CenterPoint().y - (int)(track_fraction * m_lyrics_render_height);

            for(size_t line_index=0; line_index < m_lyrics.line_count; line_index++)
            {
                CRect text_rect = client_rect;
                text_rect.bottom = text_rect_y + font_metrics.tmDescent;
                text_rect.top = text_rect_y - font_metrics.tmAscent;

                TCHAR* line_text = m_lyrics.lines[line_index];
                size_t line_length = m_lyrics.line_lengths[line_index];
                DrawText(back_buffer, line_text, line_length, &text_rect, draw_format | DT_CALCRECT);

                // NOTE: CALCRECT will shrink the width to that of the text, but we want the text to still be centred, so reset it.
                text_rect.left = client_rect.left;
                text_rect.right = client_rect.right;

                int draw_text_height = DrawTextW(back_buffer, line_text , line_length, &text_rect, draw_format);
                if (draw_text_height <= 0)
                {
                    LOG_WARN("Failed to draw text. Returned result %d. Error=%d", draw_text_height, GetLastError());
                    StopTimer();
                }

                // NOTE: We clamp the drawn height to the font height so that blank lines show up as such.
                if(draw_text_height < font_metrics.tmHeight)
                {
                    draw_text_height = font_metrics.tmHeight;
                }

                text_rect_y += draw_text_height + line_gap;
            }
        }
        else if(m_lyrics.format == LyricFormat::Timestamped)
        {
            // TODO: Don't highlight the first line until we've reached its timestamp (and don't scroll to it either, stay just above it)
            size_t active_line_index = 0;
            int text_height_above_active_line = 0;
            while((active_line_index+1 < m_lyrics.line_count) && (current_position > m_lyrics.timestamps[active_line_index+1]))
            {
                CRect text_rect = client_rect;
                text_rect.bottom = font_metrics.tmDescent;
                text_rect.top = -font_metrics.tmAscent;

                TCHAR* line_text = m_lyrics.lines[active_line_index];
                size_t line_length = m_lyrics.line_lengths[active_line_index];
                DrawText(back_buffer, line_text, line_length, &text_rect, draw_format | DT_CALCRECT);

                text_height_above_active_line += text_rect.Height() + line_gap;
                active_line_index++;
            }

            int text_rect_y = client_rect.CenterPoint().y - text_height_above_active_line; // TODO: Subtract an additional descent and line_gap?

            for(size_t line_index=0; line_index < m_lyrics.line_count; line_index++)
            {
                CRect text_rect = client_rect;
                text_rect.bottom = text_rect_y + font_metrics.tmDescent;
                text_rect.top = text_rect_y - font_metrics.tmAscent;

                TCHAR* line_text = m_lyrics.lines[line_index];
                size_t line_length = m_lyrics.line_lengths[line_index];

                int draw_text_height = DrawText(back_buffer, line_text, line_length, &text_rect, draw_format | DT_CALCRECT);

                // NOTE: CALCRECT will shrink the width to that of the text, but we want the text to still be centred, so reset it.
                text_rect.left = client_rect.left;
                text_rect.right = client_rect.right;

                if(line_index == active_line_index)
                {
                    COLORREF previous_colour = SetTextColor(back_buffer, m_callback->query_std_color(ui_color_highlight));
                    DrawTextW(back_buffer, line_text , line_length, &text_rect, draw_format);
                    SetTextColor(back_buffer, previous_colour);
                }
                else
                {
                    DrawTextW(back_buffer, line_text , line_length, &text_rect, draw_format);
                }

                if (draw_text_height <= 0)
                {
                    LOG_WARN("Failed to draw text. Returned result %d. Error=%d", draw_text_height, GetLastError());
                    StopTimer();
                }

                // NOTE: We clamp the drawn height to the font height so that blank lines show up as such.
                if(draw_text_height < font_metrics.tmHeight)
                {
                    draw_text_height = font_metrics.tmHeight;
                }

                text_rect_y += draw_text_height + line_gap;
            }
        }
        else
        {
            if(m_now_playing != nullptr)
            {
                // TODO: Line-wrapping for TextOut
                pfc::string8 artist;
                pfc::string8 album;
                pfc::string8 title;
                GetTrackMetaIdentifiers(m_now_playing, artist, album, title);

                TCHAR* artist_line = nullptr;
                TCHAR* album_line = nullptr;
                TCHAR* title_line = nullptr;
                size_t artist_len = 0;
                size_t album_len = 0;
                size_t title_len = 0;

                int line_height = font_metrics.tmHeight + line_gap;
                int total_height = 0;
                if(!artist.is_empty())
                {
                    pfc::string8 temp = "Artist: ";
                    temp.add_string(artist);
                    artist_len = string_to_tchar(temp, artist_line);

                    total_height += line_height;
                }

                if(!album.is_empty())
                {
                    pfc::string8 temp = "Album: ";
                    temp.add_string(album);
                    album_len = string_to_tchar(temp, album_line);

                    total_height += line_height;
                }

                if(!title.is_empty())
                {
                    pfc::string8 temp = "Title: ";
                    temp.add_string(title);
                    title_len = string_to_tchar(temp, title_line);

                    total_height += line_height;
                }

                CPoint centre = client_rect.CenterPoint();
                int current_y = centre.y - total_height/2;

                UINT align_result = SetTextAlign(back_buffer, TA_BASELINE | TA_CENTER); // TODO: Do this up at the top where we set the font
                if(align_result == GDI_ERROR)
                {
                    LOG_WARN("Failed to set alignment: %d", GetLastError());
                }

                if(artist_line != nullptr)
                {
                    TextOut(back_buffer, centre.x, current_y, artist_line, artist_len);
                    current_y += line_height;
                }
                if(album_line != nullptr)
                {
                    TextOut(back_buffer, centre.x, current_y, album_line, album_len);
                    current_y += line_height;
                }
                if(title_line != nullptr)
                {
                    TextOut(back_buffer, centre.x, current_y, title_line, title_len);
                    current_y += line_height;
                }

                delete[] title_line;
                delete[] album_line;
                delete[] artist_line;
            }
        }

        BitBlt(front_buffer, client_rect.left, client_rect.top,
                client_rect.Width(), client_rect.Height(),
                back_buffer, 0, 0,
                SRCCOPY);

        DeleteObject(back_buffer_bitmap);
        DeleteDC(back_buffer);
    }

    void LyricPanel::OnSize(UINT nType, CSize size)
    {
        // TODO: Recalculate the string height
    }


    void LyricPanel::OnContextMenu(CWindow window, CPoint point)
    {
        if(m_callback->is_edit_mode_enabled())
        {
            // NOTE: When edit-mode is enabled then we want the default behaviour for allowing users
            //       to change this panel, so we mark the message as unhandled and let foobar's default
            //       handling take care of it for us.
            SetMsgHandled(FALSE);
            return;
        }

        // handle the context menu key case - center the menu
        if (point == CPoint(-1, -1))
        {
            CRect rc;
            WIN32_OP(window.GetWindowRect(&rc));
            point = rc.CenterPoint();
        }

        try
        {
            // TODO: This is from the playback_state sample
            // CMenuDescriptionHybrid menudesc(*this); //this class manages all the voodoo necessary for descriptions of our menu items to show in the status bar.

            service_ptr_t<contextmenu_manager> api = contextmenu_manager::get();
            CMenu menu;
            WIN32_OP(menu.CreatePopupMenu());
            enum {
                ID_SEARCH_LYRICS = 1,
                ID_PREFERENCES,
                ID_EDIT_LYRICS,
                ID_OPEN_FILE_DIR,
                ID_CMD_COUNT,
            };
                    // TODO: Store our own metadb handle that we update at the same time as the lyric data to ensure that the two are always in-sync

            UINT disabled_without_nowplaying = (m_now_playing == nullptr) ? MF_GRAYED : 0;
            AppendMenu(menu, MF_STRING | disabled_without_nowplaying, ID_SEARCH_LYRICS, _T("Search for lyrics"));
            AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenu(menu, MF_STRING, ID_PREFERENCES, _T("Preferences"));
            AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenu(menu, MF_STRING | disabled_without_nowplaying, ID_EDIT_LYRICS, _T("Edit lyrics"));
            AppendMenu(menu, MF_STRING, ID_OPEN_FILE_DIR, _T("Open file location"));
            // TODO: Delete lyrics (delete the currently-loaded file, maybe search again). Maybe this button actually belongs in the lyric editor window?

            // TODO: We should add a submenu for selecting from all of the lyrics that we found, dynamically populated by the search for this track.
            //       For example we could have:
            //
            //       -------------------
            //      |Search Lyrics      |
            //      |Preferences        |
            //      |Edit Lyrics        |
            //      |Open File Location |  --------------------------------
            //      |Choose Lyrics      ->|(Local) filename1.txt           |
            //       -------------------  |(Local) filename2.txt           |
            //                            |(Local) filename3.lrc           |
            //                            |(AZLyrics) Artist - Title       |
            //                            |(Genius) Artist - Album - Title |
            //                             --------------------------------
            //
            //       We might need to have a menu option to populate this list though, because it seems silly
            //       to keep searching all sources by default once we've already found one (say, on disk locally)

            int cmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, get_wnd(), 0);
            switch(cmd)
            {
                case ID_SEARCH_LYRICS:
                {
                    if(m_now_playing != nullptr)
                    {
                        LoadTrackLyrics(m_now_playing);
                    }
                } break;

                case ID_PREFERENCES:
                {
                    ui_control::get()->show_preferences(GUID_PREFERENCES_PAGE);
                } break;

                case ID_EDIT_LYRICS:
                {
                    if(m_lyrics.format == LyricFormat::Unknown) break;

                    if(m_now_playing != nullptr)
                    {
                        LyricDataRaw data = {};
                        data.source = m_lyrics.source;
                        data.format = m_lyrics.format;
                        data.text = m_lyrics.text;
                        SpawnLyricEditor(data, m_now_playing);
                    }
                } break;

                case ID_OPEN_FILE_DIR:
                {
                    // TODO: Really we should be opening the directory for *the current* file, rather than just the default one (there might be multiple directories?)
                    pfc::string8 lyric_dir_utf8 = sources::localfiles::GetLyricsDir();
                    size_t wide_len = pfc::stringcvt::estimate_utf8_to_wide(lyric_dir_utf8.c_str(), lyric_dir_utf8.length());
                    wchar_t* lyric_dir = new wchar_t[wide_len];
                    pfc::stringcvt::convert_utf8_to_wide(lyric_dir, wide_len, lyric_dir_utf8.c_str(), lyric_dir_utf8.length());

                    int exec_result = (int)ShellExecute(get_wnd(), _T("explore"), lyric_dir, nullptr, nullptr, SW_SHOWMAXIMIZED);
                    if(exec_result <= 32)
                    {
                        char buffer[512];
                        snprintf(buffer, sizeof(buffer), "Failed to open lyric file directory.\nError: %d", exec_result);
                        popup_message::g_show(buffer, "foo_openlyrics Error");
                    }
                } break;
            }
        }
        catch(std::exception const & e)
        {
            LOG_ERROR("Failed to create OpenLyrics context menu: %s", e.what());
        }
    }

    void LyricPanel::GetTrackMetaIdentifiers(metadb_handle_ptr track_handle, pfc::string8& out_artist, pfc::string8& out_album, pfc::string8& out_title)
    {
        const metadb_info_container::ptr& track_info_container = track_handle->get_info_ref();
        const file_info& track_info = track_info_container->info();
        // t_filetimestamp track_timestamp = track_info_container->stats().m_timestamp; // TODO: This could be useful for setting a cached timestamp to not reload lyrics all the time? Oh but we need to get this for the lyrics file, not the track itself... although I guess if the lyrics are stored in an id3 tag?

        size_t track_artist_index = track_info.meta_find("artist");
        size_t track_album_index = track_info.meta_find("album");
        size_t track_title_index = track_info.meta_find("title");
        const size_t invalid_index = static_cast<size_t>(pfc_infinite);
        static_assert(invalid_index == pfc_infinite, "These types are different but they should still compare equal");
        if((track_artist_index != invalid_index) && (track_info.meta_enum_value_count(track_artist_index) > 0))
        {
            out_artist = pfc::string8(track_info.meta_enum_value(track_artist_index, 0));
        }
        else
        {
            out_artist.reset();
        }

        if((track_album_index != invalid_index) && (track_info.meta_enum_value_count(track_album_index) > 0))
        {
            out_album = pfc::string8(track_info.meta_enum_value(track_album_index, 0));
        }
        else
        {
            out_album.reset();
        }

        if((track_title_index != invalid_index) && (track_info.meta_enum_value_count(track_title_index) > 0))
        {
            out_title = pfc::string8(track_info.meta_enum_value(track_title_index, 0));
        }
        else
        {
            out_title.reset();
        }
    }

    void LyricPanel::StartTimer()
    {
        if (m_timerRunning) return;
        m_timerRunning = true;

        // TODO: How often do we need to re-draw? If its not that often then can we instead just use the per-second callback from play_callback_impl?
        // TODO: Similarly, the other lyrics example uses timeSetEvent (https://docs.microsoft.com/en-us/previous-versions/dd757634(v=vs.85)) instead. Should we be using that?
        UINT_PTR result = SetTimer(PANEL_UPDATE_TIMER, 20, nullptr);
        if (result != PANEL_UPDATE_TIMER)
        {
            LOG_WARN("Unexpected timer result when starting playback timer");
        }
    }

    void LyricPanel::StopTimer()
    {
        if (!m_timerRunning) return;
        m_timerRunning = false;

        WIN32_OP(KillTimer(PANEL_UPDATE_TIMER))
    }

    void LyricPanel::LoadTrackLyrics(metadb_handle_ptr track)
    {
        // TODO: Return a progress percentage while searching, and show "Searching: 63%" along with a visual progress bar
        pfc::string8 track_artist;
        pfc::string8 track_album;
        pfc::string8 track_title;
        GetTrackMetaIdentifiers(track, track_artist, track_album, track_title);

        LyricSource all_sources[] = {LyricSource::LocalFiles, LyricSource::AZLyricsCom};

        /* TODO: Make this async?
        fb2k::splitTask([shared_ptr_to_shared_data](){
                // Use the shared data
        });
        */
        LyricDataRaw lyric_data_raw = {};
        for(LyricSource source : all_sources)
        {
            switch(source)
            {
                case LyricSource::LocalFiles:
                {
                    // TODO: Only load files if the file that gets loaded has a newer timestamp than the existing one
                    lyric_data_raw = std::move(sources::localfiles::Query(track));
                } break;

                case LyricSource::AZLyricsCom:
                {
                    lyric_data_raw = std::move(sources::azlyricscom::Query(track_artist, track_album, track_title));
                } break;

                case LyricSource::None:
                default:
                {
                    LOG_WARN("Invalid lyric source configured: %d", (int)source);
                } break;
            }

            if(lyric_data_raw.format != LyricFormat::Unknown)
            {
                break;
            }
        }

        // TODO: Clean up the existing m_lyrics data (in particular the lines)... or we could give it a constructor?
        if(m_lyrics.format != LyricFormat::Unknown)
        {
            if(m_lyrics.lines != nullptr)
            {
                for(size_t i=0; i<m_lyrics.line_count; i++)
                {
                    delete[] m_lyrics.lines[i];
                }
                delete[] m_lyrics.lines;
            }

            delete[] m_lyrics.line_lengths;
            delete[] m_lyrics.timestamps;
        }

        LyricData lyric_data = {};
        switch(lyric_data_raw.format)
        {
            case LyricFormat::Plaintext:
            {
                LOG_INFO("Parsing lyrics as plaintext...");
                lyric_data = parsers::plaintext::parse(lyric_data_raw);
            } break;

            case LyricFormat::Timestamped:
            {
                LOG_INFO("Parsing lyrics as LRC...");
                lyric_data = parsers::lrc::parse(lyric_data_raw);
                if(lyric_data.format != LyricFormat::Timestamped)
                {
                    LOG_INFO("Failed to parse lyrics as LRC, falling back to plaintext...");
                    lyric_data_raw.format = LyricFormat::Plaintext;
                    lyric_data = parsers::plaintext::parse(lyric_data_raw);
                }
            } break;

            case LyricFormat::Unknown:
            default:
            {
                m_lyrics = {};
                LOG_INFO("Could not find lyrics for %s", track_title.c_str());
                return;
            }
        }

        // TODO: If we load from tags, should we save to file (or vice-versa)?
        if((lyric_data.source != LyricSource::None) && (lyric_data.source != LyricSource::LocalFiles))
        {
            if(preferences::get_autosave_enabled())
            {
                SaveMethod method = preferences::get_save_method();
                switch(method)
                {
                    case SaveMethod::ConfigDirectory:
                    {
                        // TODO: This save triggers an immediate reload from the directory watcher. This is not *necessarily* a problem, but it is some unnecessary work and it means that we immediately lose the source information for downloaded lyrics
                        sources::localfiles::SaveLyrics(track, lyric_data.format, lyric_data.text);
                    } break;

                    case SaveMethod::Id3Tag:
                    {
                        LOG_WARN("Saving lyrics to file tags is not currently supported");
                        assert(false);
                    } break;

                    case SaveMethod::None: break;

                    default:
                        LOG_WARN("Unrecognised save method: %d", (int)method);
                        assert(false);
                        break;
                }
            }
        }

        // Calculate string height
        // TODO: This is almost exactly the same as the painting function, we should do the splitting separately so we can just enumerate lines
        CRect clientRect;
        WIN32_OP_D(GetClientRect(&clientRect));

        CDC panel_dc(GetDC());
        TEXTMETRIC font_metrics = {};
        WIN32_OP_D(GetTextMetrics(panel_dc, &font_metrics));

        int line_gap = preferences::get_render_linegap();
        int text_height = 0;
        size_t line_start_index = 0;
        for(size_t line_index=0; line_index<lyric_data.line_count; line_index++)
        {
            TCHAR* line_start = lyric_data.lines[line_index];
            size_t line_length = lyric_data.line_lengths[line_index];
            const UINT draw_format = DT_NOPREFIX | DT_CENTER | DT_WORDBREAK;

            CRect text_rect = clientRect;
            int draw_text_height = DrawText(panel_dc, line_start, line_length, &text_rect, draw_format | DT_CALCRECT);
            // TODO: Replace this with TextOut or ExTextOut (which might be faster, otherwise we may need to cache the rendered text)

            // NOTE: We clamp the drawn height to the font height so that blank lines show up as such.
            if(draw_text_height < font_metrics.tmHeight)
            {
                draw_text_height = font_metrics.tmHeight;
            }

            text_height += draw_text_height + line_gap;
        }

        m_lyrics = lyric_data;
        m_lyrics_render_height = text_height;
        LOG_INFO("Lyric loading complete");
    }

    // ui_element_impl_withpopup autogenerates standalone version of our component and proper menu commands. Use ui_element_impl instead if you don't want that.
    class ui_element_myimpl : public ui_element_impl_withpopup<LyricPanel> {};
    static service_factory_single_t<ui_element_myimpl> g_lyric_panel_factory;

} // namespace
