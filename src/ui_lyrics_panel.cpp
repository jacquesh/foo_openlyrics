#include "stdafx.h"

#pragma warning(push, 0)
#include <libPPUI/win32_op.h>
#include <foobar2000/helpers/BumpableElem.h>
#pragma warning(pop)

#include "logging.h"
#include "lyric_data.h"
#include "lyric_io.h"
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

        void on_playback_new_track(metadb_handle_ptr p_track) override;
        void on_playback_stop(play_control::t_stop_reason p_reason) override;
        void on_playback_pause(bool p_state) override;
        void on_playback_seek(double p_time) override;

    private:
        void OnWindowDestroy();
        LRESULT OnTimer(WPARAM);
        void OnPaint(CDCHandle);
        BOOL OnEraseBkgnd(CDCHandle);
        void OnContextMenu(CWindow window, CPoint point);

        void GetTrackMetaIdentifiers(metadb_handle_ptr track_handle, pfc::string8& out_artist, pfc::string8& out_album, pfc::string8& out_title);
        t_ui_font get_font();
        t_ui_color get_fg_colour();
        t_ui_color get_bg_colour();
        t_ui_color get_highlight_colour();

        void StartTimer();
        void StopTimer();

        void InitiateLyricSearch(metadb_handle_ptr track);
        void ProcessAvailableLyricUpdate(LyricUpdateHandle& update);

        ui_element_config::ptr m_config;

        bool m_timerRunning;

        metadb_handle_ptr m_now_playing;
        std::vector<LyricUpdateHandle*> m_update_handles;
        LyricData m_lyrics;

    protected:
        // this must be declared as protected for ui_element_impl_withpopup<> to work.
        const ui_element_instance_callback_ptr m_callback;

        BEGIN_MSG_MAP_EX(LyricPanel)
            MSG_WM_DESTROY(OnWindowDestroy)
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

    void LyricPanel::on_playback_seek(double /*p_time*/)
    {
        Invalidate(); // Draw again to update the scroll for the new seek time
    }

    void LyricPanel::OnWindowDestroy()
    {
        // Cancel and clean up any pending updates
        for(LyricUpdateHandle* handle : m_update_handles)
        {
            delete handle;
        }
        m_update_handles.clear();
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

    void LyricPanel::OnPaint(CDCHandle)
    {
        for(auto iter=m_update_handles.begin(); iter!=m_update_handles.end(); /*omitted*/)
        {
            LyricUpdateHandle* update = *iter;
            assert(update != nullptr);
            if(update == nullptr)
            {
                continue;
            }

            if(update->has_result())
            {
                ProcessAvailableLyricUpdate(*update);
            }

            if(update->is_complete())
            {
                delete update;
                iter = m_update_handles.erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        CRect client_rect;
        WIN32_OP_D(GetClientRect(&client_rect))
        CPoint client_centre = client_rect.CenterPoint();

        CPaintDC front_buffer(*this);
        HDC back_buffer = CreateCompatibleDC(front_buffer.m_hDC);
        // As suggested in this article: https://docs.microsoft.com/en-us/previous-versions/ms969905(v=msdn.10)
        // We get flickering if we draw everything to the UI directly, so instead we render everything to a back buffer
        // and then blit the whole thing to the screen at the end.

        HBITMAP back_buffer_bitmap = CreateCompatibleBitmap(front_buffer, client_rect.Width(), client_rect.Height());
        SelectObject(back_buffer, back_buffer_bitmap);

        HBRUSH bg_brush = CreateSolidBrush(get_bg_colour());
        FillRect(back_buffer, &client_rect, bg_brush);
        DeleteObject(bg_brush);

        SetBkMode(back_buffer, TRANSPARENT);
        SelectObject(back_buffer, get_font());
        COLORREF color_result = SetTextColor(back_buffer, get_fg_colour());
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
        WIN32_OP_D(GetTextMetrics(back_buffer, &font_metrics))

        service_ptr_t<playback_control> playback = playback_control::get();
        double current_position = playback->playback_get_position();
        int line_gap = preferences::display::render_linegap();

        // TODO: Line-wrapping for TextOut (look into GDI's GetTextExtentPoint32)
        if(m_lyrics.IsEmpty())
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

                if(!m_update_handles.empty())
                {
                    bool is_search = false;
                    std::string progress_msg;
                    for(LyricUpdateHandle* update : m_update_handles)
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
                        TCHAR* progress_text = nullptr;
                        size_t progress_text_len = string_to_tchar(progress_msg, progress_text);
                        TextOut(back_buffer, client_centre.x, current_y, progress_text, progress_text_len);
                        current_y += line_height;
                        delete[] progress_text;
                    }
                }

                delete[] title_line;
                delete[] album_line;
                delete[] artist_line;
            }
        }
        else if(m_lyrics.IsTimestamped())
        {
            int active_line_index = -1;
            int lyric_line_count = static_cast<int>(m_lyrics.lines.size());
            while((active_line_index+1 < lyric_line_count) && (current_position > m_lyrics.lines[active_line_index+1].timestamp))
            {
                active_line_index++;
            }

            int text_height_above_active_line = (font_metrics.tmHeight + line_gap) * active_line_index;
            int current_y = client_rect.CenterPoint().y - text_height_above_active_line;
            for(int line_index=0; line_index < lyric_line_count; line_index++)
            {
                const LyricDataLine& line = m_lyrics.lines[line_index];
                BOOL draw_success = FALSE;
                if(line_index == active_line_index)
                {
                    COLORREF previous_colour = SetTextColor(back_buffer, get_highlight_colour());
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
        else // We have lyrics, but no timestamps
        {
            double total_length = playback->playback_get_length_ex();
            double track_fraction = current_position / total_length;
            int lyrics_render_height = m_lyrics.lines.size() * (font_metrics.tmHeight + line_gap);
            int current_y = client_rect.CenterPoint().y - (int)(track_fraction * lyrics_render_height);

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

                    std::string text = parsers::lrc::expand_text(m_lyrics);
                    LyricUpdateHandle* update = new LyricUpdateHandle(LyricUpdateHandle::Type::Edit, m_now_playing);
                    m_update_handles.push_back(update);
                    SpawnLyricEditor(text, *update);
                } break;

                case ID_OPEN_FILE_DIR:
                {
                    if(m_lyrics.persistent_storage_path.empty())
                    {
                        popup_message::g_complain("The selected track does not have any lyrics stored locally");
                    }
                    else
                    {
                        TCHAR* buffer = nullptr;
                        size_t buffer_len = string_to_tchar(m_lyrics.persistent_storage_path, buffer);

                        // Truncate the string at the last directory separator to get a directory path
                        for(size_t i=buffer_len; i>0; i--)
                        {
                            if(buffer[i] == '\\')
                            {
                                buffer[i] = '\0';
                                break;
                            }
                        }

                        int exec_result = (int)ShellExecute(get_wnd(), _T("explore"), buffer, nullptr, nullptr, SW_SHOWMAXIMIZED);
                        if(exec_result <= 32)
                        {
                            LOG_WARN("Failed to open lyric file directory: %d", exec_result);
                        }
                        delete[] buffer;
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

        // TODO: How often do we need to re-draw? A quick calculation with a representative track suggests that we'd only need to redraw (untimestamped) lyrics every ~200ms to redraw once per vertical pixel moved
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
        LOG_INFO("Initiate lyric search");
        m_lyrics = {};

        LyricUpdateHandle* update = new LyricUpdateHandle(LyricUpdateHandle::Type::Search, track);
        m_update_handles.push_back(update);
        io::search_for_lyrics(*update);
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

        bool is_edit = (update.get_type() == LyricUpdateHandle::Type::Edit);
        bool autosave_enabled = preferences::saving::autosave_enabled();
        bool loaded_from_save_src = (lyrics.source_id == io::get_save_source());
        bool should_save = is_edit || (autosave_enabled && !loaded_from_save_src); // Don't save to the source we just loaded from
        if(should_save)
        {
            try
            {
                lyrics.persistent_storage_path = io::save_lyrics(update.get_track(), lyrics, update.get_checked_abort());
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
