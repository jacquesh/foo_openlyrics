#include "stdafx.h"

#pragma warning(push, 0)
#include <libPPUI/win32_op.h>
#include <foobar2000/helpers/BumpableElem.h>
#pragma warning(pop)

#include "logging.h"
#include "lyric_data.h"
#include "lyric_io.h"
#include "math_util.h"
#include "parsers.h"
#include "preferences.h"
#include "sources/lyric_source.h"
#include "ui_lyric_editor.h"
#include "uie_shim_panel.h"
#include "winstr_util.h"

namespace {
    static const GUID GUID_LYRICS_PANEL = { 0x6e24d0be, 0xad68, 0x4bc9,{ 0xa0, 0x62, 0x2e, 0xc7, 0xb3, 0x53, 0xd5, 0xbd } };
    static const UINT_PTR PANEL_UPDATE_TIMER = 2304692;

    class LyricPanel : public ui_element_instance, public CWindowImpl<LyricPanel>, private play_callback_impl_base
    {
    public:
        // ATL window class declaration. Replace class name with your own when reusing code.
        DECLARE_WND_CLASS_EX(TEXT("{32CB89E1-3EA5-4AE7-A6E6-2DEA68A04D53}"), CS_VREDRAW | CS_HREDRAW, (-1))

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

        void notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size) override;

        void on_playback_new_track(metadb_handle_ptr track) override;
        void on_playback_stop(play_control::t_stop_reason reason) override;
        void on_playback_pause(bool state) override;
        void on_playback_seek(double time) override;

    private:
        LRESULT OnWindowCreate(LPCREATESTRUCT);
        void OnWindowDestroy();
        void OnWindowResize(UINT request_type, CSize new_size);
        LRESULT OnTimer(WPARAM);
        void OnPaint(CDCHandle);
        BOOL OnEraseBkgnd(CDCHandle);
        void OnContextMenu(CWindow window, CPoint point);

        void GetTrackMetaIdentifiers(metadb_handle_ptr track_handle, std::string& out_artist, std::string& out_album, std::string& out_title);
        t_ui_font get_font();
        t_ui_color get_fg_colour();
        t_ui_color get_bg_colour();
        t_ui_color get_highlight_colour();

        void StartTimer();
        void StopTimer();

        void DrawNoLyrics(HDC dc, CRect client_area);
        void DrawUntimedLyrics(HDC dc, CRect client_area);
        void DrawTimestampedLyrics(HDC dc, CRect client_area);

        void InitiateLyricSearch(metadb_handle_ptr track);
        void ProcessAvailableLyricUpdate(LyricUpdateHandle& update);

        ui_element_config::ptr m_config;

        bool m_timerRunning;

        metadb_handle_ptr m_now_playing;
        std::vector<std::unique_ptr<LyricUpdateHandle>> m_update_handles;
        LyricData m_lyrics;

        HDC m_back_buffer;
        HBITMAP m_back_buffer_bitmap;

    protected:
        // this must be declared as protected for ui_element_impl_withpopup<> to work.
        const ui_element_instance_callback_ptr m_callback;

        BEGIN_MSG_MAP_EX(LyricPanel)
            MSG_WM_CREATE(OnWindowCreate)
            MSG_WM_DESTROY(OnWindowDestroy)
            MSG_WM_SIZE(OnWindowResize)
            MSG_WM_TIMER(OnTimer)
            MSG_WM_ERASEBKGND(OnEraseBkgnd)
            MSG_WM_PAINT(OnPaint)
            MSG_WM_CONTEXTMENU(OnContextMenu)
        END_MSG_MAP()
    };

    HWND LyricPanel::get_wnd() { return *this; }
    void LyricPanel::initialize_window(HWND parent) { WIN32_OP(Create(parent) != NULL) }
    void LyricPanel::set_configuration(ui_element_config::ptr config) { m_config = config; }
    ui_element_config::ptr LyricPanel::get_configuration() { return m_config; }

    GUID LyricPanel::g_get_guid() { return GUID_LYRICS_PANEL; }
    GUID LyricPanel::g_get_subclass() { return ui_element_subclass_utility; }
    ui_element_config::ptr LyricPanel::g_get_default_configuration() { return ui_element_config::g_create_empty(g_get_guid()); }
    void LyricPanel::g_get_name(pfc::string_base & out) { out = "OpenLyrics Panel"; }
    const char * LyricPanel::g_get_description() { return "Displays lyrics for the currently-playing track."; }

    LyricPanel::LyricPanel(ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback) :
        m_config(config),
        m_timerRunning(false),
        m_now_playing(nullptr),
        m_update_handles(),
        m_lyrics(),
        m_callback(p_callback)
    {
    }

    void LyricPanel::notify(const GUID& what, t_size /*param1*/, const void* /*param2*/, t_size /*param2size*/)
    {
        if ((what == ui_element_notify_colors_changed) || (what == ui_element_notify_font_changed))
        {
            // we use global colors and fonts - trigger a repaint whenever these change.
            Invalidate();
        }
    }

    void LyricPanel::on_playback_new_track(metadb_handle_ptr track)
    {
        m_now_playing = track;

        InitiateLyricSearch(track);
        StartTimer();
    }

    void LyricPanel::on_playback_stop(play_control::t_stop_reason /*reason*/)
    {
        m_now_playing = nullptr;
        m_lyrics = {};
        StopTimer();
        Invalidate(); // Draw one more time to clear the panel
    }

    void LyricPanel::on_playback_pause(bool state)
    {
        if (state)
        {
            StopTimer();
        }
        else
        {
            StartTimer();
        }
    }

    void LyricPanel::on_playback_seek(double /*time*/)
    {
        Invalidate(); // Draw again to update the scroll for the new seek time
    }

    LRESULT LyricPanel::OnWindowCreate(LPCREATESTRUCT /*params*/)
    {
        service_ptr_t<playback_control> playback = playback_control::get();
        metadb_handle_ptr track;
        if(playback->get_now_playing(track))
        {
            on_playback_new_track(track);
        }
        return 0;
    }

    void LyricPanel::OnWindowDestroy()
    {
        if(m_back_buffer_bitmap != nullptr) DeleteObject(m_back_buffer_bitmap);
        if(m_back_buffer != nullptr) DeleteDC(m_back_buffer);

        // Cancel and clean up any pending updates
        m_update_handles.clear();
    }

    void LyricPanel::OnWindowResize(UINT /*request_type*/, CSize new_size)
    {
        if(m_back_buffer != nullptr) DeleteDC(m_back_buffer);
        if(m_back_buffer_bitmap != nullptr) DeleteObject(m_back_buffer_bitmap);

        CRect client_rect;
        WIN32_OP_D(GetClientRect(&client_rect))

        HDC front_buffer = GetDC();
        m_back_buffer = CreateCompatibleDC(front_buffer);
        m_back_buffer_bitmap = CreateCompatibleBitmap(front_buffer, new_size.cx, new_size.cy);
        SelectObject(m_back_buffer, m_back_buffer_bitmap);
        ReleaseDC(front_buffer);

        SetBkMode(m_back_buffer, TRANSPARENT);
        UINT align_result = SetTextAlign(m_back_buffer, TA_BASELINE | TA_CENTER);
        if(align_result == GDI_ERROR)
        {
            LOG_WARN("Failed to set text alignment: %d", GetLastError());
        }
    }

    LRESULT LyricPanel::OnTimer(WPARAM /*wParam*/)
    {
        Invalidate();
        return 0;
    }

    BOOL LyricPanel::OnEraseBkgnd(CDCHandle /*dc*/)
    {
        // NOTE: It isn't strictly required to implement this behaviour, but it's an optimisation.
        //       By default (if we return FALSE here or don't overload this at all), the window
        //       will be erased (filled with the background colour) when invalidated (to prepare
        //       for drawing). However we draw the correct background during our paint anyway
        //       so there is no need for the system (or us) to do it again here.
        return TRUE;
    }

    int _WrapLyricsLineToRect(HDC dc, CRect clip_rect, const std::tstring& line, const CPoint* origin)
    {
        TEXTMETRIC font_metrics = {};
        WIN32_OP_D(GetTextMetrics(dc, &font_metrics))
        int line_height = font_metrics.tmHeight + preferences::display::render_linegap();

        if(line.length() == 0)
        {
            return line_height;
        }

        std::tstring_view text_outstanding = line;
        int visible_width = clip_rect.Width();
        int total_height = 0;
        while(text_outstanding.length() > 0)
        {
            size_t leading_spaces = text_outstanding.find_first_not_of(_T(' '));
            text_outstanding.remove_prefix(min(leading_spaces, text_outstanding.size()));

            size_t last_not_space = text_outstanding.find_last_not_of(_T(' '));
            if(last_not_space != std::tstring_view::npos)
            {
                size_t trailing_spaces = text_outstanding.length() - 1 - last_not_space;
                text_outstanding.remove_suffix(trailing_spaces);
            }

            size_t next_line_start_index = text_outstanding.length();
            size_t chars_to_draw = text_outstanding.length();
            while(true)
            {
                SIZE line_size;
                BOOL extent_success = GetTextExtentPoint32(dc,
                                                           text_outstanding.data(),
                                                           chars_to_draw,
                                                           &line_size);
                if(!extent_success)
                {
                    LOG_WARN("Failed to compute lyric line extents");
                    return 0;
                }

                if((chars_to_draw == 0) || (line_size.cx <= visible_width))
                {
                    break;
                }
                else
                {
                    assert(chars_to_draw > 0);
                    size_t previous_space_index = text_outstanding.rfind(' ', chars_to_draw-1);
                    if(previous_space_index == std::tstring::npos)
                    {
                        // There is a single word that doesn't fit on the line
                        // This should be rare so just draw it rather than trying to split words.
                        break;
                    }
                    else
                    {
                        next_line_start_index = previous_space_index;
                        chars_to_draw = previous_space_index;
                    }
                }
            }

            bool draw_requested = (origin != nullptr);
            if(draw_requested)
            {
                int draw_y = origin->y + total_height;
                bool clipped = (draw_y + font_metrics.tmDescent < clip_rect.top) || (draw_y - font_metrics.tmAscent > clip_rect.bottom);
                if(!clipped)
                {
                    BOOL draw_success = TextOut(dc, origin->x, draw_y, text_outstanding.data(), chars_to_draw);
                    if(!draw_success)
                    {
                        LOG_WARN("Failed to draw lyrics text: %d", GetLastError());
                        return 0;
                    }
                }
            }

            total_height += line_height;
            text_outstanding = text_outstanding.substr(next_line_start_index);
        }

        return total_height;
    }

    int ComputeLyricLineHeight(HDC dc, CRect clip_rect, const std::tstring& line)
    {
        return _WrapLyricsLineToRect(dc, clip_rect, line, nullptr);
    }

    int DrawLyricLine(HDC dc, CRect clip_rect, const std::tstring& line, CPoint origin)
    {
        return _WrapLyricsLineToRect(dc, clip_rect, line, &origin);
    }

    void LyricPanel::DrawNoLyrics(HDC dc, CRect client_rect)
    {
        if(m_now_playing == nullptr)
        {
            return;
        }

        std::string artist;
        std::string album;
        std::string title;
        GetTrackMetaIdentifiers(m_now_playing, artist, album, title);

        int total_height = 0;
        std::tstring artist_line;
        std::tstring album_line;
        std::tstring title_line;
        if(!artist.empty())
        {
            artist_line = _T("Artist: ") + to_tstring(artist);
            total_height += ComputeLyricLineHeight(dc, client_rect, artist_line);
        }
        if(!album.empty())
        {
            album_line = _T("Album: ") + to_tstring(album);
            total_height += ComputeLyricLineHeight(dc, client_rect, album_line);
        }
        if(!title.empty())
        {
            title_line = _T("Title: ") + to_tstring(title);
            total_height += ComputeLyricLineHeight(dc, client_rect, title_line);
        }

        CPoint centre = client_rect.CenterPoint();
        int top_y = centre.y - total_height/2;
        CPoint origin = {centre.x, top_y};
        if(!artist_line.empty())
        {
            origin.y += DrawLyricLine(dc, client_rect, artist_line, origin);
        }
        if(!album_line.empty())
        {
            origin.y += DrawLyricLine(dc, client_rect, album_line, origin);
        }
        if(!title_line.empty())
        {
            origin.y += DrawLyricLine(dc, client_rect, title_line, origin);
        }

        if(!m_update_handles.empty())
        {
            bool is_search = false;
            std::string progress_msg;
            for(std::unique_ptr<LyricUpdateHandle>& update : m_update_handles)
            {
                if((update != nullptr) && (update->get_type() == LyricUpdateHandle::Type::Search))
                {
                    progress_msg = update->get_progress();
                    is_search = true;
                    break;
                }
            }

            if(is_search)
            {
                std::tstring progress_text = to_tstring(progress_msg);
                origin.y += DrawLyricLine(dc, client_rect, progress_text, origin);
            }
        }
    }

    void LyricPanel::DrawUntimedLyrics(HDC dc, CRect client_area)
    {
        service_ptr_t<playback_control> playback = playback_control::get();
        double current_position = playback->playback_get_position();
        double total_length = playback->playback_get_length_ex();
        double track_fraction = current_position / total_length;

        int total_height = std::accumulate(m_lyrics.lines.begin(), m_lyrics.lines.end(), 0,
                                           [dc, client_area](int x, const LyricDataLine& line)
                                           {
                                               return x + ComputeLyricLineHeight(dc, client_area, line.text);
                                           });

        CPoint centre = client_area.CenterPoint();
        int top_y = centre.y - (int)(track_fraction * total_height);
        CPoint origin = {centre.x, top_y};
        for(const LyricDataLine& line : m_lyrics.lines)
        {
            int wrapped_line_height = DrawLyricLine(dc, client_area, line.text, origin);
            origin.y += wrapped_line_height;
        }
    }

    void LyricPanel::DrawTimestampedLyrics(HDC dc, CRect client_area)
    {
        t_ui_color fg_colour = get_fg_colour();
        t_ui_color hl_colour = get_highlight_colour();

        service_ptr_t<playback_control> playback = playback_control::get();
        double current_time = playback->playback_get_position();

        int active_line_height = 0;
        int text_height_above_active_line = 0;
        int active_line_index = -1;
        int lyric_line_count = static_cast<int>(m_lyrics.lines.size());
        while((active_line_index+1 < lyric_line_count) && (current_time > m_lyrics.lines[active_line_index+1].timestamp))
        {
            active_line_index++;
            text_height_above_active_line += active_line_height;
            active_line_height = ComputeLyricLineHeight(dc, client_area, m_lyrics.lines[active_line_index].text);
        }

        double next_line_time = DBL_MAX;
        if(active_line_index+1 < lyric_line_count)
        {
            next_line_time = m_lyrics.lines[active_line_index+1].timestamp;
        }

        double scroll_time = preferences::display::scroll_time_seconds();
        double next_line_scroll_factor = lerp_inverse_clamped(next_line_time - scroll_time, next_line_time, current_time);

        CPoint centre = client_area.CenterPoint();
        int next_line_scroll = (int)((double)active_line_height * next_line_scroll_factor);
        int top_y = (int)((double)centre.y - text_height_above_active_line - next_line_scroll);
        CPoint origin = {centre.x, top_y};
        for(int line_index=0; line_index < lyric_line_count; line_index++)
        {
            const LyricDataLine& line = m_lyrics.lines[line_index];
            if(line_index == active_line_index)
            {
                COLORREF colour = lerp(hl_colour, fg_colour, next_line_scroll_factor);
                SetTextColor(dc, colour);
            }
            else if(line_index == active_line_index+1)
            {
                COLORREF colour = lerp(fg_colour, hl_colour, next_line_scroll_factor);
                SetTextColor(dc, colour);
            }
            else
            {
                SetTextColor(dc, fg_colour);
            }

            int wrapped_line_height = DrawLyricLine(dc, client_area, line.text, origin);
            if(wrapped_line_height == 0)
            {
                LOG_WARN("Failed to draw text: %d", GetLastError());
                StopTimer();
                break;
            }

            origin.y += wrapped_line_height;
        }
    }

    void LyricPanel::OnPaint(CDCHandle)
    {
        for(auto iter=m_update_handles.begin(); iter!=m_update_handles.end(); /*omitted*/)
        {
            std::unique_ptr<LyricUpdateHandle>& update = *iter;
            if(update->has_result())
            {
                ProcessAvailableLyricUpdate(*update);
            }

            if(update->is_complete())
            {
                iter = m_update_handles.erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        // As suggested in this article: https://docs.microsoft.com/en-us/previous-versions/ms969905(v=msdn.10)
        // We get flickering if we draw everything to the UI directly, so instead we render everything to a back buffer
        // and then blit the whole thing to the screen at the end.
        PAINTSTRUCT paintstruct;
        HDC front_buffer = BeginPaint(&paintstruct);

        CRect client_rect;
        WIN32_OP_D(GetClientRect(&client_rect))

        HBRUSH bg_brush = CreateSolidBrush(get_bg_colour());
        FillRect(m_back_buffer, &client_rect, bg_brush);
        DeleteObject(bg_brush);

        SelectObject(m_back_buffer, get_font());
        COLORREF color_result = SetTextColor(m_back_buffer, get_fg_colour());
        if(color_result == CLR_INVALID)
        {
            LOG_WARN("Failed to set text colour: %d", GetLastError());
        }

        if(m_lyrics.IsEmpty())
        {
            DrawNoLyrics(m_back_buffer, client_rect);
        }
        else if(m_lyrics.IsTimestamped())
        {
            DrawTimestampedLyrics(m_back_buffer, client_rect);
        }
        else // We have lyrics, but no timestamps
        {
            DrawUntimedLyrics(m_back_buffer, client_rect);
        }

        BitBlt(front_buffer, client_rect.left, client_rect.top,
                client_rect.Width(), client_rect.Height(),
                m_back_buffer, 0, 0,
                SRCCOPY);
        EndPaint(&paintstruct);
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
            WIN32_OP(window.GetWindowRect(&rc))
            point = rc.CenterPoint();
        }

        try
        {
            service_ptr_t<contextmenu_manager> api = contextmenu_manager::get();
            CMenu menu = nullptr;
            WIN32_OP(menu.CreatePopupMenu())
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

            CMenuDescriptionHybrid menudesc(get_wnd());
            menudesc.Set(ID_SEARCH_LYRICS, "Start a completely new search for lyrics");
            menudesc.Set(ID_PREFERENCES, "Open the OpenLyrics preferences page");
            menudesc.Set(ID_EDIT_LYRICS, "Open the lyric editor with the current lyrics");
            menudesc.Set(ID_OPEN_FILE_DIR, "Open explorer to the location of the lyrics file");

            // TODO: We should add a submenu for selecting from all of the lyrics that we found, dynamically populated by the search for this track.
            //       For example we could have:
            //
            //       -------------------
            //      |Search Lyrics      |
            //      |Preferences        |
            //      |Edit Lyrics        |
            //      |Open File Location |  --------------------------------
            //      |Choose Lyrics      ->|(Local) filename1.txt           |
            //       -------------------  |(Local) filename2.lrc           |
            //                            |(AZLyrics) Artist - Title       |
            //                            |(Genius) Artist - Album - Title |
            //                             --------------------------------
            //
            //       We might need to have a menu option to populate this list though, because it seems silly
            //       to keep searching all sources by default once we've already found one (say, on disk locally)

            int cmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, menudesc, nullptr);
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
                    ui_control::get()->show_preferences(GUID_PREFERENCES_PAGE_ROOT);
                } break;

                case ID_EDIT_LYRICS:
                {
                    if(m_now_playing == nullptr) break;

                    std::tstring text = parsers::lrc::expand_text(m_lyrics);
                    auto update = std::make_unique<LyricUpdateHandle>(LyricUpdateHandle::Type::Edit, m_now_playing);
                    SpawnLyricEditor(text, *update);
                    m_update_handles.push_back(std::move(update));
                } break;

                case ID_OPEN_FILE_DIR:
                {
                    if(m_lyrics.persistent_storage_path.empty())
                    {
                        popup_message::g_complain("The selected track does not have any lyrics stored locally");
                    }
                    else
                    {
                        std::tstring pathstr = to_tstring(m_lyrics.persistent_storage_path);

                        // Truncate the string at the last directory separator to get a directory path
                        for(size_t i=pathstr.length(); i>0; i--)
                        {
                            if(pathstr[i] == _T('\\'))
                            {
                                pathstr = pathstr.substr(0, i);
                                break;
                            }
                        }

                        int exec_result = (int)ShellExecute(get_wnd(), _T("explore"), pathstr.c_str(), nullptr, nullptr, SW_SHOWMAXIMIZED);
                        if(exec_result <= 32)
                        {
                            LOG_WARN("Failed to open lyric file directory: %d", exec_result);
                        }
                    }
                } break;
            }
        }
        catch(std::exception const & e)
        {
            LOG_ERROR("Failed to create OpenLyrics context menu: %s", e.what());
        }
    }

    void LyricPanel::GetTrackMetaIdentifiers(metadb_handle_ptr track_handle, std::string& out_artist, std::string& out_album, std::string& out_title)
    {
        const metadb_info_container::ptr& track_info_container = track_handle->get_info_ref();
        const file_info& track_info = track_info_container->info();

        size_t track_artist_index = track_info.meta_find("artist");
        size_t track_album_index = track_info.meta_find("album");
        size_t track_title_index = track_info.meta_find("title");
        if((track_artist_index != pfc::infinite_size) && (track_info.meta_enum_value_count(track_artist_index) > 0))
        {
            out_artist = std::string(track_info.meta_enum_value(track_artist_index, 0));
        }
        else
        {
            out_artist.clear();
        }

        if((track_album_index != pfc::infinite_size) && (track_info.meta_enum_value_count(track_album_index) > 0))
        {
            out_album = std::string(track_info.meta_enum_value(track_album_index, 0));
        }
        else
        {
            out_album.clear();
        }

        if((track_title_index != pfc::infinite_size) && (track_info.meta_enum_value_count(track_title_index) > 0))
        {
            out_title = std::string(track_info.meta_enum_value(track_title_index, 0));
        }
        else
        {
            out_title.clear();
        }
    }

    t_ui_font LyricPanel::get_font()
    {
        t_ui_font result = preferences::display::font();
        if(result == nullptr)
        {
            result = m_callback->query_font_ex(ui_font_console);
        }
        return result;
    }

    t_ui_color LyricPanel::get_fg_colour()
    {
        std::optional<t_ui_color> colour = preferences::display::foreground_colour();
        if(colour.has_value())
        {
            return colour.value();
        }
        else
        {
            return m_callback->query_std_color(ui_color_text);
        }
    }

    t_ui_color LyricPanel::get_bg_colour()
    {
        std::optional<t_ui_color> colour = preferences::display::background_colour();
        if(colour.has_value())
        {
            return colour.value();
        }
        else
        {
            return m_callback->query_std_color(ui_color_background);
        }
    }

    t_ui_color LyricPanel::get_highlight_colour()
    {
        std::optional<t_ui_color> colour = preferences::display::highlight_colour();
        if(colour.has_value())
        {
            return colour.value();
        }
        else
        {
            return m_callback->query_std_color(ui_color_highlight);
        }
    }

    void LyricPanel::StartTimer()
    {
        if (m_timerRunning) return;
        m_timerRunning = true;

        UINT_PTR result = SetTimer(PANEL_UPDATE_TIMER, 50, nullptr);
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
        LOG_INFO("Initiate lyric search");
        m_lyrics = {};

        auto update = std::make_unique<LyricUpdateHandle>(LyricUpdateHandle::Type::Search, track);
        io::search_for_lyrics(*update);
        m_update_handles.push_back(std::move(update));
    }

    void LyricPanel::ProcessAvailableLyricUpdate(LyricUpdateHandle& update)
    {
        assert(update.has_result());
        LyricData lyrics = update.get_result();

        if(lyrics.IsEmpty())
        {
            LOG_INFO("Received empty lyric update, ignoring...");
            return;
        }

        AutoSaveStrategy autosave = preferences::saving::autosave_strategy();
        bool should_autosave = (autosave == AutoSaveStrategy::Always) ||
                               ((autosave == AutoSaveStrategy::OnlySynced) && lyrics.IsTimestamped());

        bool is_edit = (update.get_type() == LyricUpdateHandle::Type::Edit);
        bool loaded_from_save_src = (lyrics.source_id == preferences::saving::save_source());
        bool should_save = is_edit || (should_autosave && !loaded_from_save_src); // Don't save to the source we just loaded from
        if(should_save)
        {
            try
            {
                bool allow_overwrite = is_edit;
                lyrics.persistent_storage_path = io::save_lyrics(update.get_track(), lyrics, allow_overwrite, update.get_checked_abort());
            }
            catch(const std::exception& e)
            {
                LOG_ERROR("Failed to save downloaded lyrics: %s", e.what());
            }
        }

        if(update.get_track() == m_now_playing)
        {
            m_lyrics = std::move(lyrics);
        }
    }

    // ui_element_impl_withpopup autogenerates standalone version of our component and proper menu commands. Use ui_element_impl instead if you don't want that.
    class LyricPanelImpl : public ui_element_impl_withpopup<LyricPanel> {};
    FB2K_SERVICE_FACTORY(LyricPanelImpl)
    UIE_SHIM_PANEL_FACTORY(LyricPanel)

} // namespace
