#include "stdafx.h"

#pragma warning(push, 0)
#include <libPPUI/win32_op.h>
#include <foobar2000/helpers/BumpableElem.h>
#pragma warning(pop)

#include "sources/localfiles.h"
#include "ui_lyric_editor.h"
#include "winstr_util.h"

// TODO: This is currently just copied from preferences.cpp
static const GUID GUID_PREFERENCES_PAGE = { 0x29e96cfa, 0xab67, 0x4793, { 0xa1, 0xc3, 0xef, 0xc3, 0xa, 0xbc, 0x8b, 0x74 } };

namespace {
    static const GUID GUID_LYRICS_PANEL = { 0x6e24d0be, 0xad68, 0x4bc9,{ 0xa0, 0x62, 0x2e, 0xc7, 0xb3, 0x53, 0xd5, 0xbd } };
    static const UINT_PTR PANEL_UPDATE_TIMER = 2304692;

    class LyricPanel : public ui_element_instance, public CWindowImpl<LyricPanel>, private play_callback_impl_base {
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

    private:
        LRESULT OnLyricRefreshRequested(UINT, WPARAM, LPARAM, BOOL&);
        LRESULT OnWindowCreate(LPCREATESTRUCT);
        void OnWindowDestroy();
        LRESULT OnTimer(WPARAM);
        void OnPaint(CDCHandle);
        BOOL OnEraseBkgnd(CDCHandle);
        void OnSize(UINT nType, CSize size);
        void OnContextMenu(CWindow window, CPoint point);

        pfc::string8 GetNowPlayingLyricTitle();

        void StartTimer();
        void StopTimer();

        void LoadCurrentlyPlayingLyrics();

        ui_element_config::ptr m_config;

        bool m_timerRunning;
        TCHAR* m_lyrics_string;
        size_t m_lyrics_string_length;
        int m_lyrics_string_height;

    protected:
        // this must be declared as protected for ui_element_impl_withpopup<> to work.
        const ui_element_instance_callback_ptr m_callback;

        BEGIN_MSG_MAP_EX(LyricPanel)
            MESSAGE_HANDLER(WM_USER+1, OnLyricRefreshRequested); // TODO: Define a constant for this message
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
        m_lyrics_string(nullptr),
        m_lyrics_string_length(0),
        m_lyrics_string_height(0)
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
        LoadCurrentlyPlayingLyrics();
        StartTimer();
    }

    void LyricPanel::on_playback_stop(play_control::t_stop_reason /*p_reason*/)
    {
        StopTimer();
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

    LRESULT LyricPanel::OnLyricRefreshRequested(UINT, WPARAM, LPARAM, BOOL&)
    {
        LoadCurrentlyPlayingLyrics();
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
        if (m_lyrics_string_length == 0) return;

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
        
        TEXTMETRIC fontMetrics = {};
        WIN32_OP_D(GetTextMetrics(back_buffer, &fontMetrics));

        service_ptr_t<playback_control> playback = playback_control::get();
        double current_position = playback->playback_get_position();
        double total_length = playback->playback_get_length_ex();
        double track_fraction = current_position / total_length;
        int text_rect_y = client_rect.CenterPoint().y - (int)(track_fraction * m_lyrics_string_height);

        size_t line_start_index = 0;
        while (line_start_index < m_lyrics_string_length)
        {
            size_t line_end_index = line_start_index;
            while ((line_end_index <= m_lyrics_string_length) &&
                (m_lyrics_string[line_end_index] != '\0') &&
                (m_lyrics_string[line_end_index] != '\n') &&
                (m_lyrics_string[line_end_index] != '\r'))
            {
                line_end_index++;
            }
            size_t line_length = line_end_index - line_start_index;
            wchar_t* line_start = &m_lyrics_string[line_start_index];
            const UINT draw_format = DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_WORDBREAK;// | DT_SINGLELINE;

            CRect textRect = client_rect;
            textRect.bottom = text_rect_y + fontMetrics.tmDescent;
            textRect.top = text_rect_y - fontMetrics.tmAscent;
            DrawText(back_buffer, line_start, line_length, &textRect, draw_format | DT_CALCRECT);

            // NOTE: CALCRECT will shrink the width to that of the text, but we want the text to still be centred, so reset it.
            textRect.left = client_rect.left;
            textRect.right = client_rect.right;

            int draw_text_height = DrawTextW(back_buffer, line_start , line_length, &textRect, draw_format);
            if (draw_text_height <= 0)
            {
                console::printf("WARN-OpenLyrics: Failed to draw text. Returned result %d. Error=%d", draw_text_height, GetLastError());
                StopTimer();
            }

            int line_gap = 4; // TODO: We're just using 4 here as some example value, really we want the font's line_gap value
            text_rect_y += draw_text_height + line_gap;
            if ((line_end_index + 1 < m_lyrics_string_length) &&
                (m_lyrics_string[line_end_index] == '\r') &&
                (m_lyrics_string[line_end_index + 1] == '\n'))
            {
                line_start_index = line_end_index + 2;
            }
            else
            {
                line_start_index = line_end_index + 1;
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
            AppendMenu(menu, MF_STRING | MF_GRAYED, ID_SEARCH_LYRICS, _T("Search lyrics (TODO)"));
            AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenu(menu, MF_STRING, ID_PREFERENCES, _T("Preferences"));
            AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenu(menu, MF_STRING, ID_EDIT_LYRICS, _T("Edit lyrics"));
            AppendMenu(menu, MF_STRING, ID_OPEN_FILE_DIR, _T("Open file location"));

            int cmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, m_hWnd, 0);
            switch(cmd)
            {
                case ID_SEARCH_LYRICS:
                {
                    popup_message::g_show("Blah!", "TODO: Search for lyrics");
                } break;

                case ID_PREFERENCES:
                {
                    ui_control::get()->show_preferences(GUID_PREFERENCES_PAGE);
                } break;

                case ID_EDIT_LYRICS:
                {
                    LyricEditData data = {};
                    data.format = LyricFormat::Plaintext; // TODO
                    data.file_title = GetNowPlayingLyricTitle();
                    data.text = tchar_to_string(m_lyrics_string, m_lyrics_string_length);
                    SpawnLyricEditor(data);
                } break;

                case ID_OPEN_FILE_DIR:
                {
                    // TODO: Really we should be opening the directory for *the current* file, rather than just the default one (there might be multiple directories?)
                    pfc::string8 lyric_dir_utf8 = sources::localfiles::GetLyricsDir();
                    size_t wide_len = pfc::stringcvt::estimate_utf8_to_wide(lyric_dir_utf8.c_str(), lyric_dir_utf8.length());
                    wchar_t* lyric_dir = new wchar_t[wide_len];
                    pfc::stringcvt::convert_utf8_to_wide(lyric_dir, wide_len, lyric_dir_utf8.c_str(), lyric_dir_utf8.length());

                    int exec_result = (int)ShellExecute(m_hWnd, _T("explore"), lyric_dir, nullptr, nullptr, SW_SHOWMAXIMIZED);
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
            console::complain("ERROR-OpenLyrics: Failed to create OpenLyrics context menu", e);
        }
    }

    pfc::string8 LyricPanel::GetNowPlayingLyricTitle()
    {
        titleformat_object::ptr format_script;
        bool compile_success = titleformat_compiler::get()->compile(format_script, "[%artist% - ][%title%]");
        if (!compile_success)
        {
            console::error("WARN-OpenLyrics: Failed to compile title format script");
            return "";
        }

        metadb_handle_ptr now_playing;
        bool now_playing_success = playback_control::get()->get_now_playing(now_playing);
        if(!now_playing_success)
        {
            console::info("WARN-OpenLyrics: Failed to retrieve now_playing");
            return "";
        }

        pfc::string8 track_title;
        now_playing->format_title(nullptr, track_title, format_script, nullptr);
        return track_title;
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
            console::error("WARN-OpenLyrics: Unexpected timer result when starting playback timer");
        }
    }

    void LyricPanel::StopTimer()
    {
        if (!m_timerRunning) return;
        m_timerRunning = false;

        WIN32_OP(KillTimer(PANEL_UPDATE_TIMER))
    }

    void LyricPanel::LoadCurrentlyPlayingLyrics()
    {
        // TODO: First check if the timestamp of the file has changed since we might get spurious notifications (either from another component for because the user spammed the "Apply" button...though we should disable that if nothing has changed....)

        pfc::string8 track_str = GetNowPlayingLyricTitle(); // TODO: Maybe we should pass in p_track?

        pfc::string8 lyric_str;
        pfc::string8 lyric_file_path = sources::localfiles::Query(track_str, lyric_str);
        if (lyric_file_path.is_empty())
        {
            // TODO: Custom "not found" string (configurable). LyricShowPanel3 defaults to: "Artist: %artist%\nTitle: %title%\nAlbum: %album%\n%current_time% / %track_length%"
            lyric_str = "Not found";
            console::printf("Could not find lyrics for %s", track_str.c_str());
        }

        if(m_lyrics_string != nullptr)
        {
            delete[] m_lyrics_string;
        }
        m_lyrics_string_length = string_to_tchar(lyric_str, m_lyrics_string);

        // Calculate string height
        // TODO: This is almost exactly the same as the painting function, we should do the splitting separately so we can just enumerate lines
        CRect clientRect;
        WIN32_OP_D(GetClientRect(&clientRect));

        CPaintDC dc(*this);
        int text_height = 0;
        size_t line_start_index = 0;
        while (line_start_index < m_lyrics_string_length)
        {
            size_t line_end_index = line_start_index;
            while ((line_end_index <= m_lyrics_string_length) &&
                (m_lyrics_string[line_end_index] != '\0') &&
                (m_lyrics_string[line_end_index] != '\n') &&
                (m_lyrics_string[line_end_index] != '\r'))
            {
                line_end_index++;
            }
            size_t line_length = line_end_index - line_start_index;
            wchar_t* line_start = &m_lyrics_string[line_start_index];
            const UINT draw_format = DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_WORDBREAK;// | DT_SINGLELINE;

            CRect textRect = clientRect;
            int draw_text_height = DrawText(dc, line_start, line_length, &textRect, draw_format | DT_CALCRECT);

            int line_gap = 4; // TODO: We're just using 4 here as some example value, really we want the font's line_gap value
            text_height += draw_text_height + line_gap;
            if ((line_end_index + 1 < m_lyrics_string_length) &&
                (m_lyrics_string[line_end_index] == '\r') &&
                (m_lyrics_string[line_end_index + 1] == '\n'))
            {
                line_start_index = line_end_index + 2;
            }
            else
            {
                line_start_index = line_end_index + 1;
            }
        }
        m_lyrics_string_height = text_height;
    }

    // ui_element_impl_withpopup autogenerates standalone version of our component and proper menu commands. Use ui_element_impl instead if you don't want that.
    class ui_element_myimpl : public ui_element_impl_withpopup<LyricPanel> {};
    static service_factory_single_t<ui_element_myimpl> g_lyric_panel_factory;

} // namespace
