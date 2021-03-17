#include "stdafx.h"

#pragma warning(push, 0)
#include <libPPUI/win32_op.h>
#include <foobar2000/helpers/BumpableElem.h>
#pragma warning(pop)

#include "logging.h"
#include "lyric_data.h"
#include "lyric_search.h"
#include "parsers.h"
#include "preferences.h"
#include "sources/localfiles.h"
#include "ui_lyric_editor.h"
#include "winstr_util.h"

// TODO: This is currently just copied from preferences.cpp
static const GUID GUID_PREFERENCES_PAGE = { 0x29e96cfa, 0xab67, 0x4793, { 0xa1, 0xc3, 0xef, 0xc3, 0xa, 0xbc, 0x8b, 0x74 } };

namespace {
    static const GUID GUID_LYRICS_PANEL = { 0x6e24d0be, 0xad68, 0x4bc9,{ 0xa0, 0x62, 0x2e, 0xc7, 0xb3, 0x53, 0xd5, 0xbd } };
    static const UINT_PTR PANEL_UPDATE_TIMER = 2304692;

    class LyricPanel : public ui_element_instance, public CWindowImpl<LyricPanel>, private play_callback_impl_base
    {
    public:
        // ATL window class declaration. Replace class name with your own when reusing code.
        DECLARE_WND_CLASS_EX(TEXT("{32CB89E1-3EA5-4AE7-A6E6-2DEA68A04D53}"), CS_VREDRAW | CS_HREDRAW, (-1));

        LyricPanel(ui_element_config::ptr,ui_element_instance_callback_ptr p_callback);
        HWND get_wnd() override;
        void initialize_window(HWND parent);
        void set_configuration(ui_element_config::ptr config) override;
        ui_element_config::ptr get_configuration() override;

        static GUID g_get_guid();
        static GUID g_get_subclass();
        static void g_get_name(pfc::string_base & out);
        static ui_element_config::ptr g_get_default_configuration();
        static const char * g_get_description();

        void notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size) override;

        void on_playback_new_track(metadb_handle_ptr p_track) override;
        void on_playback_stop(play_control::t_stop_reason p_reason) override;
        void on_playback_pause(bool p_state) override;
        void on_playback_seek(double p_time) override;

    private:
        LRESULT OnLyricSavedToDisk(UINT, WPARAM, LPARAM, BOOL&);
        LRESULT OnWindowCreate(LPCREATESTRUCT);
        void OnWindowDestroy();
        LRESULT OnTimer(WPARAM);
        void OnPaint(CDCHandle);
        BOOL OnEraseBkgnd(CDCHandle);
        void OnContextMenu(CWindow window, CPoint point);

        void GetTrackMetaIdentifiers(metadb_handle_ptr track_handle, pfc::string8& out_artist, pfc::string8& out_album, pfc::string8& out_title);

        void StartTimer();
        void StopTimer();

        void InitiateLyricSearch(metadb_handle_ptr track);
        void CompleteLyricSearch(LyricData* new_lyrics);

        ui_element_config::ptr m_config;

        bool m_timerRunning;

        metadb_handle_ptr m_now_playing;
        LyricSearch* m_search;
        LyricData m_lyrics;
        int m_lyrics_render_height;

    protected:
        // this must be declared as protected for ui_element_impl_withpopup<> to work.
        const ui_element_instance_callback_ptr m_callback;

        BEGIN_MSG_MAP_EX(LyricPanel)
            MESSAGE_HANDLER(WM_USER+1, OnLyricSavedToDisk) // TODO: Define a constant for this message
            MSG_WM_CREATE(OnWindowCreate)
            MSG_WM_DESTROY(OnWindowDestroy)
            MSG_WM_TIMER(OnTimer)
            MSG_WM_ERASEBKGND(OnEraseBkgnd)
            MSG_WM_PAINT(OnPaint)
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
        m_search(nullptr),
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

        InitiateLyricSearch(p_track);
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

    void LyricPanel::on_playback_seek(double p_time)
    {
        Invalidate(); // Draw again to update the scroll for the new seek time
    }

    LRESULT LyricPanel::OnLyricSavedToDisk(UINT, WPARAM, LPARAM, BOOL&)
    {
        if(m_now_playing != nullptr)
        {
            // TODO: Do we need to trigger an entire lyric search here? Just update from localfiles if needed?
            InitiateLyricSearch(m_now_playing);
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

        if(m_search != nullptr)
        {
            // Cancel and clean up a pending search, if there is one
            delete m_search;
            m_search = nullptr;
        }
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
        if(m_search != nullptr)
        {
            LyricData* new_lyrics = m_search->get_result();
            if(new_lyrics != nullptr)
            {
                CompleteLyricSearch(new_lyrics);

                delete m_search;
                m_search = nullptr;
            }
        }

        CRect client_rect;
        WIN32_OP_D(GetClientRect(&client_rect));
        CPoint client_centre = client_rect.CenterPoint();

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
        COLORREF color_result = SetTextColor(back_buffer, m_callback->query_std_color(ui_color_text));
        UINT align_result = SetTextAlign(back_buffer, TA_BASELINE | TA_CENTER);
        if(color_result == CLR_INVALID)
        {
            LOG_WARN("Failed to set text colour: %d", GetLastError());
        }
        if(align_result == GDI_ERROR)
        {
            LOG_WARN("Failed to set text alignment: %d", GetLastError());
        }

        TEXTMETRIC font_metrics = {};
        WIN32_OP_D(GetTextMetrics(back_buffer, &font_metrics));

        service_ptr_t<playback_control> playback = playback_control::get();
        double current_position = playback->playback_get_position();
        double total_length = playback->playback_get_length_ex();
        int line_gap = preferences::get_render_linegap();

        // TODO: Line-wrapping for TextOut (look into GDI's GetTextExtentPoint32)
        if(m_lyrics.format == LyricFormat::Plaintext)
        {
            double track_fraction = current_position / total_length;
            int text_rect_y = client_rect.CenterPoint().y - (int)(track_fraction * m_lyrics_render_height);
            int current_y = text_rect_y;

            for(const LyricDataLine& line : m_lyrics.lines)
            {
                BOOL draw_success = TextOut(back_buffer, client_centre.x, current_y, line.text, line.text_length);
                if(!draw_success)
                {
                    LOG_WARN("Failed to draw text: %d", GetLastError());
                    StopTimer();
                    break;
                }
                current_y += font_metrics.tmHeight + line_gap;
            }
        }
        else if(m_lyrics.format == LyricFormat::Timestamped)
        {
            int active_line_index = -1;
            while((active_line_index+1 < m_lyrics.lines.size()) && (current_position > m_lyrics.lines[active_line_index+1].timestamp))
            {
                active_line_index++;
            }

            int text_height_above_active_line = (font_metrics.tmHeight + line_gap) * active_line_index;
            int current_y = client_rect.CenterPoint().y - text_height_above_active_line;
            for(int line_index=0; line_index < m_lyrics.lines.size(); line_index++)
            {
                const LyricDataLine& line = m_lyrics.lines[line_index];
                BOOL draw_success = FALSE;
                if(line_index == active_line_index)
                {
                    COLORREF previous_colour = SetTextColor(back_buffer, m_callback->query_std_color(ui_color_highlight));
                    draw_success = TextOut(back_buffer, client_centre.x, current_y, line.text, line.text_length);
                    SetTextColor(back_buffer, previous_colour);
                }
                else
                {
                    draw_success = TextOut(back_buffer, client_centre.x, current_y, line.text, line.text_length);
                }

                if(!draw_success)
                {
                    LOG_WARN("Failed to draw text: %d", GetLastError());
                    StopTimer();
                    break;
                }

                current_y += font_metrics.tmHeight + line_gap;
            }
        }
        else
        {
            if(m_now_playing != nullptr)
            {
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

                int current_y = client_centre.y - total_height/2;
                if(artist_line != nullptr)
                {
                    TextOut(back_buffer, client_centre.x, current_y, artist_line, artist_len);
                    current_y += line_height;
                }
                if(album_line != nullptr)
                {
                    TextOut(back_buffer, client_centre.x, current_y, album_line, album_len);
                    current_y += line_height;
                }
                if(title_line != nullptr)
                {
                    TextOut(back_buffer, client_centre.x, current_y, title_line, title_len);
                    current_y += line_height;
                }

                if(m_search != nullptr)
                {
                    // TODO: Draw some progress info (be it a progress bar, or just text "63%", maybe the name of the current source being queried)
                    const TCHAR* search_text = _T("Searching...");
                    size_t search_text_len = _tcslen(search_text);
                    TextOut(back_buffer, client_centre.x, current_y, search_text, search_text_len);
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
                        InitiateLyricSearch(m_now_playing);
                    }
                } break;

                case ID_PREFERENCES:
                {
                    ui_control::get()->show_preferences(GUID_PREFERENCES_PAGE);
                } break;

                case ID_EDIT_LYRICS:
                {
                    if(m_now_playing == nullptr) break;

                    LyricDataRaw data = {};
                    if(m_lyrics.format != LyricFormat::Unknown)
                    {
                        data.source_id = m_lyrics.source_id;
                        data.format = m_lyrics.format;

                        // TODO: std::string everywhere!
                        std::string expanded_str = parsers::lrc::expand_text(m_lyrics);
                        data.text = pfc::string8(expanded_str.c_str());
                    }
                    SpawnLyricEditor(data, m_now_playing);
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
                        LOG_WARN("Failed to open lyric file directory: %d", exec_result);
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

        size_t track_artist_index = track_info.meta_find("artist");
        size_t track_album_index = track_info.meta_find("album");
        size_t track_title_index = track_info.meta_find("title");
        if((track_artist_index != pfc::infinite_size) && (track_info.meta_enum_value_count(track_artist_index) > 0))
        {
            out_artist = pfc::string8(track_info.meta_enum_value(track_artist_index, 0));
        }
        else
        {
            out_artist.reset();
        }

        if((track_album_index != pfc::infinite_size) && (track_info.meta_enum_value_count(track_album_index) > 0))
        {
            out_album = pfc::string8(track_info.meta_enum_value(track_album_index, 0));
        }
        else
        {
            out_album.reset();
        }

        if((track_title_index != pfc::infinite_size) && (track_info.meta_enum_value_count(track_title_index) > 0))
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
        // TODO: Another option is timeSetEvent (https://docs.microsoft.com/en-us/previous-versions/dd757634(v=vs.85)) instead. Should we be using that?
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

    void LyricPanel::InitiateLyricSearch(metadb_handle_ptr track)
    {
        m_lyrics = {};

        if(m_search != nullptr)
        {
            delete m_search;
        }
        m_search = new LyricSearch(track);
    }

    void LyricPanel::CompleteLyricSearch(LyricData* new_lyrics)
    {
        assert(new_lyrics != nullptr);

        // Calculate string height
        TEXTMETRIC font_metrics = {};
        {
            CDC panel_dc(GetDC());
            WIN32_OP_D(GetTextMetrics(panel_dc, &font_metrics));
        }

        // TODO: Line-wrapping for TextOut (look into GDI's GetTextExtentPoint32)
        int line_gap = preferences::get_render_linegap();
        int text_height = new_lyrics->lines.size() * (font_metrics.tmHeight + line_gap);

        // TODO: This isn't ideal: We're implicitly taking ownership of the memory allocated *inside* new_lyrics (IE timestamps etc)
        m_lyrics = std::move(*new_lyrics);
        m_lyrics_render_height = text_height;
    }

    // ui_element_impl_withpopup autogenerates standalone version of our component and proper menu commands. Use ui_element_impl instead if you don't want that.
    class ui_element_myimpl : public ui_element_impl_withpopup<LyricPanel> {};
    static service_factory_single_t<ui_element_myimpl> g_lyric_panel_factory;

} // namespace
