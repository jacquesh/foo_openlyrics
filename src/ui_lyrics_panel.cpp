#include "stdafx.h"

#pragma warning(push, 0)
#include <libPPUI/win32_op.h>
#include <foobar2000/helpers/BumpableElem.h>
#pragma warning(pop)

#include "logging.h"
#include "lyric_auto_edit.h"
#include "lyric_data.h"
#include "lyric_io.h"
#include "math_util.h"
#include "metadb_index_search_avoidance.h"
#include "parsers.h"
#include "preferences.h"
#include "sources/lyric_source.h"
#include "ui_hooks.h"
#include "uie_shim_panel.h"
#include "win32_util.h"

namespace {
    static const GUID GUID_LYRICS_PANEL = { 0x6e24d0be, 0xad68, 0x4bc9,{ 0xa0, 0x62, 0x2e, 0xc7, 0xb3, 0x53, 0xd5, 0xbd } };
    static const UINT_PTR PANEL_UPDATE_TIMER = 2304692;

    class LyricPanel;
    static std::vector<LyricPanel*> g_active_panels;

    class LyricPanel : public ui_element_instance, public CWindowImpl<LyricPanel>, private play_callback_impl_base
    {
    public:
        // ATL window class declaration. Replace class name with your own when reusing code.
        DECLARE_WND_CLASS_EX(TEXT("{32CB89E1-3EA5-4AE7-A6E6-2DEA68A04D53}"), CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS, (-1))

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
        BEGIN_MSG_MAP_EX(LyricPanel)
            MSG_WM_CREATE(OnWindowCreate)
            MSG_WM_DESTROY(OnWindowDestroy)
            MSG_WM_SIZE(OnWindowResize)
            MSG_WM_TIMER(OnTimer)
            MSG_WM_ERASEBKGND(OnEraseBkgnd)
            MSG_WM_PAINT(OnPaint)
            MSG_WM_CONTEXTMENU(OnContextMenu)
            MSG_WM_LBUTTONDBLCLK(OnDoubleClick)
            MSG_WM_MOUSEWHEEL(OnMouseWheel)
            MSG_WM_MOUSEMOVE(OnMouseMove)
            MSG_WM_LBUTTONDOWN(OnLMBDown)
            MSG_WM_LBUTTONUP(OnLMBUp)
        END_MSG_MAP()

        LRESULT OnWindowCreate(LPCREATESTRUCT);
        void OnWindowDestroy();
        void OnWindowResize(UINT request_type, CSize new_size);
        LRESULT OnTimer(WPARAM);
        void OnPaint(CDCHandle);
        BOOL OnEraseBkgnd(CDCHandle);
        void OnContextMenu(CWindow window, CPoint point);
        void OnDoubleClick(UINT virtualKeys, CPoint cursorPos);
        LRESULT OnMouseWheel(UINT virtualKeys, short rotation, CPoint point);
        void OnMouseMove(UINT virtualKeys, CPoint point);
        void OnLMBDown(UINT virtualKeys, CPoint point);
        void OnLMBUp(UINT virtualKeys, CPoint point);

        t_ui_font get_font();
        t_ui_color get_fg_colour();
        t_ui_color get_bg_colour();
        t_ui_color get_highlight_colour();

        void StartTimer();
        void StopTimer();

        void DrawNoLyrics(HDC dc, CRect client_area);
        void DrawUntimedLyricsVertical(HDC dc, CRect client_area);
        void DrawUntimedLyricsHorizontal(HDC dc, CRect client_area);
        void DrawTimestampedLyricsVertical(HDC dc, CRect client_area);
        void DrawTimestampedLyricsHorizontal(HDC dc, CRect client_area);

        void InitiateLyricSearch(metadb_handle_ptr track);

        ui_element_config::ptr m_config;
        abort_callback_impl m_child_abort;

        bool m_timerRunning;

        metadb_handle_ptr m_now_playing;
        std::vector<std::unique_ptr<LyricUpdateHandle>> m_update_handles;
        LyricData m_lyrics;
        bool m_auto_search_avoided;
        uint64_t m_auto_search_avoided_timestamp;

        HDC m_back_buffer;
        HBITMAP m_back_buffer_bitmap;

        std::optional<CPoint> m_manual_scroll_start;
        int m_manual_scroll_distance;

    protected:
        // this must be declared as protected for ui_element_impl_withpopup<> to work.
        const ui_element_instance_callback_ptr m_callback;
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
        m_callback(p_callback),
        m_auto_search_avoided(false),
        m_auto_search_avoided_timestamp(0)
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
        m_manual_scroll_distance = 0;

        // NOTE: We also track a generation counter that increments every time you change the search config
        //       so that if you don't find lyrics with some active sources and then add more, it'll search
        //       again at least once, possibly finding something if there are new active sources.
        lyric_search_avoidance avoidance = load_search_avoidance(track);
        const bool expected_to_fail = (avoidance.failed_searches > 3);
        const bool trial_period_expired = ((avoidance.first_fail_time + system_time_periods::week) < filetimestamp_from_system_timer());
        const bool same_generation = (avoidance.search_config_generation == preferences::searching::source_config_generation());
        if(!same_generation || !expected_to_fail || !trial_period_expired)
        {
            InitiateLyricSearch(track);
        }
        else
        {
            LOG_INFO("Skipped search because it's expected to fail anyway and was not specifically requested");
            m_lyrics = {};
            m_auto_search_avoided = true;
            m_auto_search_avoided_timestamp = filetimestamp_from_system_timer();
        }

        // NOTE: If playback is paused on startup then this gets called with the paused track,
        //       but playback is paused so we don't actually want to run the timer
        service_ptr_t<playback_control> playback = playback_control::get();
        if(!playback->is_paused())
        {
            StartTimer();
        }
    }

    void LyricPanel::on_playback_stop(play_control::t_stop_reason /*reason*/)
    {
        m_now_playing = nullptr;
        m_lyrics = {};
        m_auto_search_avoided = false;
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

        g_active_panels.push_back(this);
        return 0;
    }

    void LyricPanel::OnWindowDestroy()
    {
        if(m_back_buffer_bitmap != nullptr) DeleteObject(m_back_buffer_bitmap);
        if(m_back_buffer != nullptr) DeleteDC(m_back_buffer);

        // Cancel and clean up any pending updates
        m_child_abort.abort();
        m_update_handles.clear();

        auto panel_iter = std::find(g_active_panels.begin(), g_active_panels.end(), this);
        assert(panel_iter != g_active_panels.end());
        if(panel_iter != g_active_panels.end())
        {
            g_active_panels.erase(panel_iter);
        }
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

    int _WrapSimpleLyricsLineToRect(HDC dc, CRect clip_rect, std::tstring_view line, const CPoint* origin)
    {
        TEXTMETRIC font_metrics = {};
        WIN32_OP_D(GetTextMetrics(dc, &font_metrics))
        int line_height = font_metrics.tmHeight + preferences::display::linegap();

        if(line.length() == 0)
        {
            return line_height;
        }

        int visible_width = clip_rect.Width();

        // This serves as an upper bound on the number of chars we draw on a single line.
        // Used to prevent GDI from having to compute the size of very long strings.
        size_t generous_max_chars = 256;
        if(font_metrics.tmAveCharWidth > 0)
        {
            assert(visible_width >= 0);
            size_t avg_chars_that_fit = ((size_t)visible_width/(size_t)font_metrics.tmAveCharWidth) + 1;
            generous_max_chars = 3*avg_chars_that_fit;
        }

        std::tstring_view text_outstanding = line;
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
            size_t chars_to_draw = min(text_outstanding.length(), generous_max_chars);
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

    // Ordinarily a single "line" from the lyric data is just one row (pre-wrapping) of text.
    // However if multiple lines have the exact same timestamp, they get combined and are presented
    // here as a single "line" that contains newline chars.
    // We refer to these here as simple & compound lines.
    int _WrapCompoundLyricsLineToRect(HDC dc, CRect clip_rect, std::tstring_view line, CPoint* origin)
    {
        if(line.length() == 0)
        {
            return _WrapSimpleLyricsLineToRect(dc, clip_rect, line, origin);
        }

        int result = 0;
        size_t start_index = 0;
        while(start_index < line.length())
        {
            size_t end_index = min(line.length(), line.find('\n', start_index));
            size_t length = end_index - start_index;
            std::tstring_view view(&line.data()[start_index], length);
            int row_height = _WrapSimpleLyricsLineToRect(dc, clip_rect, view, origin);
            if(origin != nullptr)
            {
                origin->y += row_height;
            }
            result += row_height;
            start_index = end_index+1;
        }
        return result;
    }

    int ComputeWrappedLyricLineHeight(HDC dc, CRect clip_rect, const std::tstring& line)
    {
        return _WrapCompoundLyricsLineToRect(dc, clip_rect, line, nullptr);
    }

    int DrawWrappedLyricLine(HDC dc, CRect clip_rect, const std::tstring_view line, CPoint origin)
    {
        return _WrapCompoundLyricsLineToRect(dc, clip_rect, line, &origin);
    }

    void LyricPanel::DrawNoLyrics(HDC dc, CRect client_rect)
    {
        if(m_now_playing == nullptr)
        {
            return;
        }

        // TODO: If we make this text configurable in future and we want to also show some text
        //       telling you that it didn't search because nothing was found, look into:
        //       metadb.h (in foo_SDK) -> metadb_display_field_provider
        //       which exists to let you hook into the title format process and add new fields.

        std::string artist = track_metadata(m_now_playing, "artist");
        std::string album = track_metadata(m_now_playing, "album");
        std::string title = track_metadata(m_now_playing, "title");

        int total_height = 0;
        std::tstring artist_line;
        std::tstring album_line;
        std::tstring title_line;
        if(!artist.empty())
        {
            artist_line = _T("Artist: ") + to_tstring(artist);
            total_height += ComputeWrappedLyricLineHeight(dc, client_rect, artist_line);
        }
        if(!album.empty())
        {
            album_line = _T("Album: ") + to_tstring(album);
            total_height += ComputeWrappedLyricLineHeight(dc, client_rect, album_line);
        }
        if(!title.empty())
        {
            title_line = _T("Title: ") + to_tstring(title);
            total_height += ComputeWrappedLyricLineHeight(dc, client_rect, title_line);
        }

        // NOTE: Since our calculation is for the *top* of the rendered text, we need to
        //       shift down by the font's ascent so that we get to the baseline (which
        //       is what is used as the rendering origin).
        TEXTMETRIC font_metrics = {};
        WIN32_OP_D(GetTextMetrics(dc, &font_metrics))

        CPoint centre = client_rect.CenterPoint();
        int top_y = centre.y - total_height/2 + font_metrics.tmAscent;
        CPoint origin = {centre.x, top_y};
        if(!artist_line.empty())
        {
            origin.y += DrawWrappedLyricLine(dc, client_rect, artist_line, origin);
        }
        if(!album_line.empty())
        {
            origin.y += DrawWrappedLyricLine(dc, client_rect, album_line, origin);
        }
        if(!title_line.empty())
        {
            origin.y += DrawWrappedLyricLine(dc, client_rect, title_line, origin);
        }

        if(!m_update_handles.empty())
        {
            bool is_search = false;
            std::string progress_msg;
            for(std::unique_ptr<LyricUpdateHandle>& update : m_update_handles)
            {
                if((update != nullptr) &&
                    (update->get_type() == LyricUpdateHandle::Type::AutoSearch) &&
                    (update->get_track() == m_now_playing))
                {
                    progress_msg = update->get_progress();
                    is_search = true;
                    break;
                }
            }

            if(is_search)
            {
                std::tstring progress_text = to_tstring(progress_msg);
                origin.y += DrawWrappedLyricLine(dc, client_rect, progress_text, origin);
            }
        }

        if(m_auto_search_avoided)
        {
            const double search_avoided_msg_seconds = 15.0;
            uint64_t search_avoided_msg_ticks = static_cast<uint64_t>(search_avoided_msg_seconds * 10'000'000); // A "tick" here means "100-nanoseconds"
            uint64_t ticks_since_search_avoided = filetimestamp_from_system_timer() - m_auto_search_avoided_timestamp;
            if(ticks_since_search_avoided < search_avoided_msg_ticks)
            {
                origin.y += DrawWrappedLyricLine(dc, client_rect, _T(""), origin);
                origin.y += DrawWrappedLyricLine(dc, client_rect, _T("Auto-search skipped because it failed too many times."), origin);
                origin.y += DrawWrappedLyricLine(dc, client_rect, _T("Manually request a lyrics search to try again."), origin);
            }
        }
    }

    void LyricPanel::DrawUntimedLyricsVertical(HDC dc, CRect client_area)
    {
        service_ptr_t<playback_control> playback = playback_control::get();
        double current_position = playback->playback_get_position();
        double total_length = playback->playback_get_length_ex();
        double track_fraction = current_position / total_length;

        int one_line_height = ComputeWrappedLyricLineHeight(dc, client_area, _T(""));
        int total_height = std::accumulate(m_lyrics.lines.begin(), m_lyrics.lines.end(), 0,
                                           [dc, client_area](int x, const LyricDataLine& line)
                                           {
                                               return x + ComputeWrappedLyricLineHeight(dc, client_area, line.text);
                                           });

        CPoint centre = client_area.CenterPoint();
        int top_y = 0;
        if(preferences::display::scroll_type() == LineScrollType::Manual)
        {
            // Shift the 'top' Y down by a single line so we can see the first line of text,
            // because the 'top y' is actually used as the *baseline*
            int default_top_y = one_line_height;
            if(total_height < client_area.Height())
            {
                int centre_y = client_area.Height()/2;
                default_top_y = centre_y + one_line_height - total_height/2;
            }

            // We want:
            // 1) bottom_y >= one_line_height
            //    top_y + total_height >= one_line_height
            //    top_y >= one_line_height - total_height
            //    default_top_y - one_line_height + m_manual_scroll_distance >= one_line_height - total_height
            //    m_manual_scroll_distance >= one_line_height - total_height - default_top_y + one_line_height
            // 2) top_y <= panel_height
            //    default_top_y + m_manual_scroll_distance <= panel_height
            //    m_manual_scroll_distance <= panel_height - default_top_y
            // So:
            //    (one_line_height - total_height - default_top_y + one_line_height) <= m_manual_scroll_distance <= (panel_height - default_top_y)
            int min_scroll = one_line_height - total_height - default_top_y + one_line_height;
            int max_scroll = client_area.Height() - default_top_y;
            if(m_manual_scroll_distance < min_scroll) m_manual_scroll_distance = min_scroll;
            if(m_manual_scroll_distance > max_scroll) m_manual_scroll_distance = max_scroll;

            top_y = default_top_y + m_manual_scroll_distance;
        }
        else
        {
            // NOTE: Since our calculation is for the *top* of the rendered text, we need to
            //       shift down by the font's ascent so that we get to the baseline (which
            //       is what is used as the rendering origin).
            TEXTMETRIC font_metrics = {};
            WIN32_OP_D(GetTextMetrics(dc, &font_metrics))

            top_y = centre.y - (int)(track_fraction * total_height) + font_metrics.tmAscent;

            // NOTE: We support the manual scroll distance here so that people can offset
            //       the default automated scrolling (which is often significantly wrong)
            const int min_scroll = - top_y - total_height + 2*one_line_height;
            const int max_scroll = client_area.Height() - top_y - font_metrics.tmAscent;
            if(m_manual_scroll_distance < min_scroll) m_manual_scroll_distance = min_scroll;
            if(m_manual_scroll_distance > max_scroll) m_manual_scroll_distance = max_scroll;
            top_y += m_manual_scroll_distance;
        }

        CPoint origin = {centre.x, top_y};
        for(const LyricDataLine& line : m_lyrics.lines)
        {
            int wrapped_line_height = DrawWrappedLyricLine(dc, client_area, line.text, origin);
            if(wrapped_line_height <= 0)
            {
                LOG_WARN("Failed to draw unsynced text: %d", GetLastError());
                StopTimer();
                break;
            }
            origin.y += wrapped_line_height;
        }
    }

    void LyricPanel::DrawUntimedLyricsHorizontal(HDC dc, CRect client_area)
    {
        if(m_lyrics.lines.empty()) return;
        assert(m_lyrics.lines.size() > 0);

        size_t linegap = static_cast<size_t>(max(0, preferences::display::linegap()));
        std::tstring glue(linegap, _T(' '));

        assert(preferences::display::scroll_direction() == LineScrollDirection::Horizontal);
        service_ptr_t<playback_control> playback = playback_control::get();
        double current_position = playback->playback_get_position();
        double total_length = playback->playback_get_length_ex();
        double track_fraction = current_position / total_length;

        std::tstring joined = m_lyrics.lines[0].text;
        joined = std::accumulate(++m_lyrics.lines.begin(), m_lyrics.lines.end(), joined,
                                 [&glue](const std::tstring& accum, const LyricDataLine& line)
                                 {
                                    return accum + glue + line.text;
                                 });

        SIZE line_size;
        BOOL extent_success = GetTextExtentPoint32(dc,
                                                   joined.c_str(),
                                                   joined.length(),
                                                   &line_size);
        if(!extent_success)
        {
            LOG_ERROR("Failed to compute unsynced text width");
            StopTimer();
            return;
        }

        // NOTE: The drawing call uses the glyph baseline as the origin.
        //       We want our text to be perfectly vertically centered, so we need to offset it
        //       but the difference between the baseline and the vertical centre of the font.
        TEXTMETRIC font_metrics = {};
        WIN32_OP_D(GetTextMetrics(dc, &font_metrics))
        int baseline_centre_correction = (font_metrics.tmAscent + font_metrics.tmDescent)/2;

        CPoint centre = client_area.CenterPoint();
        CPoint origin = centre;
        if(preferences::display::scroll_type() == LineScrollType::Manual)
        {
            const int half_width = client_area.Width()/2;
            const int min_scroll = -line_size.cx + half_width;
            const int max_scroll = half_width;
            if(m_manual_scroll_distance > max_scroll) m_manual_scroll_distance = max_scroll;
            if(m_manual_scroll_distance < min_scroll) m_manual_scroll_distance = min_scroll;

            origin.x += (line_size.cx - client_area.Width())/2 + m_manual_scroll_distance;
        }
        else
        {
            origin.x += (int)((0.5 - track_fraction) * (double)line_size.cx);

            // NOTE: We support the manual scroll distance here so that people can offset
            //       the default automated scrolling (which is often significantly wrong)
            const int epsilon = 16;
            const int min_scroll = - origin.x - line_size.cx/2 + epsilon; // origin.x + line_size.cx/2 + m_manual_scroll_distance > 0
            const int max_scroll = client_area.Width() - origin.x + line_size.cx/2 - epsilon; // origin.x - line_size.cx/2 + m_manual_scroll_distance < client_area.Width()
            if(m_manual_scroll_distance < min_scroll) m_manual_scroll_distance = min_scroll;
            if(m_manual_scroll_distance > max_scroll) m_manual_scroll_distance = max_scroll;
            origin.x += m_manual_scroll_distance;
        }
        origin.y += baseline_centre_correction;

        BOOL draw_success = DrawTextOut(dc, origin.x, origin.y, joined);
        if(!draw_success)
        {
            LOG_ERROR("Failed to draw unsynced text");
            StopTimer();
        }
    }


    void LyricPanel::DrawTimestampedLyricsVertical(HDC dc, CRect client_area)
    {
        // NOTE: The drawing call uses the glyph baseline as the origin.
        //       We want our text to be perfectly vertically centered, so we need to offset it
        //       but the difference between the baseline and the vertical centre of the font.
        TEXTMETRIC font_metrics = {};
        WIN32_OP_D(GetTextMetrics(dc, &font_metrics))
        int baseline_centre_correction = (font_metrics.tmAscent + font_metrics.tmDescent)/2;

        t_ui_color fg_colour = get_fg_colour();
        t_ui_color hl_colour = get_highlight_colour();

        service_ptr_t<playback_control> playback = playback_control::get();
        double current_time = playback->playback_get_position();

        int active_line_height = 0;
        int text_height_above_active_line = 0;
        int active_line_index = -1;
        int lyric_line_count = static_cast<int>(m_lyrics.lines.size());
        while((active_line_index+1 < lyric_line_count) && (current_time > m_lyrics.LineTimestamp(active_line_index+1)))
        {
            active_line_index++;
            text_height_above_active_line += active_line_height;
            active_line_height = ComputeWrappedLyricLineHeight(dc, client_area, m_lyrics.lines[active_line_index].text);
        }

        double next_line_time = m_lyrics.LineTimestamp(active_line_index+1);
        double scroll_time = preferences::display::scroll_time_seconds();
        double next_line_scroll_factor = lerp_inverse_clamped(next_line_time - scroll_time, next_line_time, current_time);

        CPoint centre = client_area.CenterPoint();
        int next_line_scroll = (int)((double)active_line_height * next_line_scroll_factor);
        int top_y = (int)((double)centre.y - text_height_above_active_line - next_line_scroll + baseline_centre_correction);
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

            int wrapped_line_height = DrawWrappedLyricLine(dc, client_area, line.text, origin);
            if(wrapped_line_height == 0)
            {
                LOG_ERROR("Failed to draw synced text");
                StopTimer();
                break;
            }

            origin.y += wrapped_line_height;
        }
    }

    void LyricPanel::DrawTimestampedLyricsHorizontal(HDC dc, CRect client_area)
    {
        // NOTE: The drawing call uses the glyph baseline as the origin.
        //       We want our text to be perfectly vertically centered, so we need to offset it
        //       but the difference between the baseline and the vertical centre of the font.
        TEXTMETRIC font_metrics = {};
        WIN32_OP_D(GetTextMetrics(dc, &font_metrics))
        int baseline_centre_correction = (font_metrics.tmAscent + font_metrics.tmDescent)/2;

        service_ptr_t<playback_control> playback = playback_control::get();
        double current_time = playback->playback_get_position();

        size_t linegap = static_cast<size_t>(max(0, preferences::display::linegap()));
        std::tstring glue(linegap, _T(' '));
        t_ui_color fg_colour = get_fg_colour();
        t_ui_color hl_colour = get_highlight_colour();

        int active_line_index = -1;
        int lyric_line_count = static_cast<int>(m_lyrics.lines.size());
        while((active_line_index+1 < lyric_line_count) && (current_time > m_lyrics.LineTimestamp(active_line_index+1)))
        {
            active_line_index++;
        }
        size_t next_line_index = static_cast<size_t>(active_line_index + 1);

        bool has_active_text = (active_line_index >= 0);
        bool has_preactive_text = (active_line_index > 0);
        bool has_postactive_text = (active_line_index < static_cast<int>(m_lyrics.lines.size() - 1));
        std::tstring active_line_text;
        std::tstring next_line_text;
        std::tstring pre_active_line_text;
        std::tstring post_active_line_text;
        if(has_active_text)
        {
            active_line_text = m_lyrics.lines[active_line_index].text;
        }
        if(has_preactive_text)
        {
            auto start = m_lyrics.lines.begin();
            auto end = m_lyrics.lines.begin();
            std::advance(end, active_line_index);

            pre_active_line_text = std::accumulate(++start, end, m_lyrics.lines[0].text,
                                                   [&glue](const std::tstring& accum, const LyricDataLine& line)
                                                   {
                                                      return accum + glue + line.text;
                                                   });

            if(has_active_text)
            {
                pre_active_line_text += glue;
            }
        }

        if(has_postactive_text)
        {
            next_line_text = glue + m_lyrics.lines[next_line_index].text;

            auto end = m_lyrics.lines.end();
            auto start = m_lyrics.lines.begin();
            std::advance(start, next_line_index + 1);

            if(start != end)
            {
                std::tstring initial_value = glue + start->text;
                post_active_line_text = std::accumulate(++start, end, initial_value,
                                                        [&glue](const std::tstring& accum, const LyricDataLine& line)
                                                        {
                                                           return accum + glue + line.text;
                                                        });
            }
        }

        std::optional<SIZE> active_line_extents = GetTextExtents(dc, active_line_text);
        std::optional<SIZE> next_line_extents = GetTextExtents(dc, next_line_text);
        std::optional<SIZE> pre_active_extents = GetTextExtents(dc, pre_active_line_text);
        std::optional<SIZE> post_active_extents = GetTextExtents(dc, post_active_line_text);
        std::optional<SIZE> glue_extents = GetTextExtents(dc, glue);
        if(!active_line_extents.has_value() ||
                !next_line_extents.has_value() ||
                !pre_active_extents.has_value() ||
                !post_active_extents.has_value() ||
                !glue_extents.has_value())
        {
            LOG_ERROR("Failed to compute synced text width");
            StopTimer();
            return;
        }
        int active_line_width = active_line_extents.value().cx;
        int next_line_width = next_line_extents.value().cx;
        int pre_active_width = pre_active_extents.value().cx;
        int post_active_width = post_active_extents.value().cx;
        int glue_width = glue_extents.value().cx;

        double next_line_time = m_lyrics.LineTimestamp(next_line_index);
        double scroll_time = preferences::display::scroll_time_seconds();
        double next_line_scroll_factor = lerp_inverse_clamped(next_line_time - scroll_time, next_line_time, current_time);

        // NOTE: We want to scroll by half of the active line and half of the next line because
        //       the rendering origin is at the centre of the text. However the "next line"
        //       already contains the glue string for rendering, so when we half that, we need
        //       to add in an extra half of the glue string width.
        int total_scroll_to_next_line = active_line_width + glue_width;
        int next_line_scroll = (int)((double)total_scroll_to_next_line * next_line_scroll_factor);

        CPoint centre = client_area.CenterPoint();
        int centre_y = centre.y + baseline_centre_correction;
        double active_x_alignment_factor = 0.15;
        int active_left_x = client_area.left + (int)((double)client_area.Width() * active_x_alignment_factor);
        int left_x = active_left_x - pre_active_width - next_line_scroll;
        BOOL draw_success = TRUE;

        SetTextColor(dc, fg_colour);
        draw_success &= DrawTextOut(dc, left_x + pre_active_width/2, centre_y, pre_active_line_text);
        left_x += pre_active_width;

        COLORREF active_line_colour = lerp(hl_colour, fg_colour, next_line_scroll_factor);
        SetTextColor(dc, active_line_colour);
        draw_success &= DrawTextOut(dc, left_x + active_line_width/2, centre_y, active_line_text);
        left_x += active_line_width;

        COLORREF next_line_colour = lerp(fg_colour, hl_colour, next_line_scroll_factor);
        SetTextColor(dc, next_line_colour);
        draw_success &= DrawTextOut(dc, left_x + next_line_width/2, centre_y, next_line_text);
        left_x += next_line_width;

        SetTextColor(dc, fg_colour);
        draw_success &= DrawTextOut(dc, left_x + post_active_width/2, centre_y, post_active_line_text);

        if(!draw_success)
        {
            LOG_ERROR("Failed to draw horizontally-scrolling synced text");
            StopTimer();
        }
    }

    void LyricPanel::OnPaint(CDCHandle)
    {
        for(auto iter=m_update_handles.begin(); iter!=m_update_handles.end(); /*omitted*/)
        {
            std::unique_ptr<LyricUpdateHandle>& update = *iter;
            if(update->has_result())
            {
                std::optional<LyricData> maybe_lyrics = io::process_available_lyric_update(*update);

                if((maybe_lyrics.has_value()) && (update->get_track() == m_now_playing))
                {
                    m_lyrics = std::move(maybe_lyrics.value());
                    m_auto_search_avoided = false;
                }
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
        else if(m_lyrics.IsTimestamped() &&
                (preferences::display::scroll_type() == LineScrollType::Automatic))
        {
            LineScrollDirection dir = preferences::display::scroll_direction();
            switch(dir)
            {
                case LineScrollDirection::Vertical: DrawTimestampedLyricsVertical(m_back_buffer, client_rect); break;
                case LineScrollDirection::Horizontal: DrawTimestampedLyricsHorizontal(m_back_buffer, client_rect); break;
                default:
                    LOG_ERROR("Unsupported scroll direction: %d", (int)dir);
                    uBugCheck();
            }
        }
        else // We have lyrics, but no timestamps
        {
            LineScrollDirection dir = preferences::display::scroll_direction();
            switch(dir)
            {
                case LineScrollDirection::Vertical: DrawUntimedLyricsVertical(m_back_buffer, client_rect); break;
                case LineScrollDirection::Horizontal: DrawUntimedLyricsHorizontal(m_back_buffer, client_rect); break;
                default:
                    LOG_ERROR("Unsupported scroll direction: %d", (int)dir);
                    uBugCheck();
            }
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
            UINT disabled_without_nowplaying = (m_now_playing == nullptr) ? MF_GRAYED : 0;
            UINT disabled_without_lyrics = m_lyrics.IsEmpty() ? MF_GRAYED : 0;
            UINT disabled_with_lyrics = m_lyrics.IsEmpty() ? 0 : MF_GRAYED;
            enum {
                ID_SEARCH_LYRICS = 1,
                ID_SEARCH_LYRICS_MANUAL,
                ID_SAVE_LYRICS,
                ID_PREFERENCES,
                ID_EDIT_LYRICS,
                ID_OPEN_FILE_DIR,
                ID_AUTO_ADD_INSTRUMENTAL,
                ID_AUTO_REMOVE_EXTRA_SPACES,
                ID_AUTO_REMOVE_EXTRA_BLANK_LINES,
                ID_AUTO_REMOVE_ALL_BLANK_LINES,
                ID_AUTO_REPLACE_XML_CHARS,
                ID_AUTO_RESET_CAPITALISATION,
                ID_FIX_MALFORMED_TIMESTAMPS,
                ID_CMD_COUNT,
            };

            CMenu menu_edit = nullptr;
            WIN32_OP(menu_edit.CreatePopupMenu())
            AppendMenu(menu_edit, MF_STRING | disabled_without_nowplaying | disabled_with_lyrics, ID_AUTO_ADD_INSTRUMENTAL, _T("Mark as 'instrumental'"));
            AppendMenu(menu_edit, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_AUTO_REPLACE_XML_CHARS, _T("Replace &&-named HTML characters"));
            AppendMenu(menu_edit, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_AUTO_REMOVE_EXTRA_SPACES, _T("Remove repeated spaces"));
            AppendMenu(menu_edit, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_AUTO_REMOVE_EXTRA_BLANK_LINES, _T("Remove repeated blank lines"));
            AppendMenu(menu_edit, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_AUTO_REMOVE_ALL_BLANK_LINES, _T("Remove all blank lines"));
            AppendMenu(menu_edit, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_AUTO_RESET_CAPITALISATION, _T("Reset capitalisation"));
            AppendMenu(menu_edit, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_FIX_MALFORMED_TIMESTAMPS, _T("Fix malformed timestamps"));

            CMenu menu = nullptr;
            WIN32_OP(menu.CreatePopupMenu())
            AppendMenu(menu, MF_STRING | disabled_without_nowplaying, ID_SEARCH_LYRICS, _T("Search for lyrics"));
            AppendMenu(menu, MF_STRING | disabled_without_nowplaying, ID_SEARCH_LYRICS_MANUAL, _T("Search for lyrics (manually)"));
            AppendMenu(menu, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_SAVE_LYRICS, _T("Save lyrics"));
            AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenu(menu, MF_STRING | disabled_without_nowplaying, ID_EDIT_LYRICS, _T("Edit lyrics"));
            AppendMenu(menu, MF_STRING | MF_POPUP, (UINT_PTR)menu_edit.m_hMenu, _T("Auto-edit lyrics"));
            AppendMenu(menu, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_OPEN_FILE_DIR, _T("Open file location"));
            AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenu(menu, MF_STRING, ID_PREFERENCES, _T("Preferences"));
            // TODO: Delete lyrics (delete the currently-loaded file, maybe search again). Maybe this button actually belongs in the lyric editor window?

            CMenuDescriptionHybrid menudesc(get_wnd());
            menudesc.Set(ID_SEARCH_LYRICS, "Start a completely new search for lyrics");
            menudesc.Set(ID_SEARCH_LYRICS_MANUAL, "Start a new search for lyrics with customisable search terms and multiple results");
            menudesc.Set(ID_SAVE_LYRICS, "Save the current lyrics, even if they would not be auto-saved");
            menudesc.Set(ID_PREFERENCES, "Open the OpenLyrics preferences page");
            menudesc.Set(ID_EDIT_LYRICS, "Open the lyric editor with the current lyrics");
            menudesc.Set(ID_OPEN_FILE_DIR, "Open explorer to the location of the lyrics file");
            menudesc.Set(ID_AUTO_ADD_INSTRUMENTAL, "Add lyrics containing just the text '[Instrumental]'");
            menudesc.Set(ID_AUTO_REPLACE_XML_CHARS, "Replace &-encoded named HTML characters (e.g &lt;) with the characters they represent (e.g <)");
            menudesc.Set(ID_AUTO_REMOVE_EXTRA_SPACES, "Replace sequences of multiple whitespace characters with a single space");
            menudesc.Set(ID_AUTO_REMOVE_EXTRA_BLANK_LINES, "Replace sequences of multiple empty lines with just a single empty line");
            menudesc.Set(ID_AUTO_REMOVE_ALL_BLANK_LINES, "Remove all empty lines");
            menudesc.Set(ID_AUTO_RESET_CAPITALISATION, "Reset capitalisation of each line so that only the first character is upper case");
            menudesc.Set(ID_FIX_MALFORMED_TIMESTAMPS, "Fix timestamps that are slightly malformed so that they're recognised as timestamps and not shown in the text");

            std::optional<LyricData> updated_lyrics;
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

                case ID_SEARCH_LYRICS_MANUAL:
                {
                    if(m_now_playing != nullptr)
                    {
                        LOG_INFO("Initiate manual lyric search");
                        auto update = std::make_unique<LyricUpdateHandle>(LyricUpdateHandle::Type::ManualSearch, m_now_playing, m_child_abort);
                        SpawnManualLyricSearch(get_wnd(), *update);
                        m_update_handles.push_back(std::move(update));
                    }
                } break;

                case ID_SAVE_LYRICS:
                {
                    if((m_now_playing == nullptr) || m_lyrics.IsEmpty())
                    {
                        LOG_INFO("Attempt to manually save empty lyrics, ignoring...");
                    }
                    else
                    {
                        try
                        {
                            const bool allow_overwrite = true;
                            io::save_lyrics(m_now_playing, m_lyrics, allow_overwrite, m_child_abort);
                        }
                        catch(const std::exception& e)
                        {
                            LOG_ERROR("Failed to complete manually requested lyric save: %s", e.what());
                        }
                    }
                } break;

                case ID_PREFERENCES:
                {
                    ui_control::get()->show_preferences(GUID_PREFERENCES_PAGE_ROOT);
                } break;

                case ID_EDIT_LYRICS:
                {
                    if(m_now_playing == nullptr) break;

                    auto update = std::make_unique<LyricUpdateHandle>(LyricUpdateHandle::Type::Edit, m_now_playing, m_child_abort);
                    SpawnLyricEditor(get_wnd(), m_lyrics, *update);
                    m_update_handles.push_back(std::move(update));
                } break;

                case ID_OPEN_FILE_DIR:
                {
                    LyricSourceBase* source = nullptr;
                    if(m_lyrics.save_source.has_value())
                    {
                        source = LyricSourceBase::get(m_lyrics.save_source.value());
                    }

                    if(source == nullptr)
                    {
                        LyricSourceBase* originating_source = LyricSourceBase::get(m_lyrics.source_id);
                        if((originating_source != nullptr) && originating_source->is_local())
                        {
                            source = originating_source;
                        }
                    }

                    std::tstring pathstr;
                    if(source != nullptr)
                    {
                        pathstr = source->get_file_path(m_now_playing, m_lyrics);
                    }

                    if(pathstr.empty())
                    {
                        popup_message::g_complain("The selected track does not have any lyrics stored locally");
                    }
                    else
                    {
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

                case ID_AUTO_ADD_INSTRUMENTAL:
                {
                    if(m_lyrics.IsEmpty())
                    {
                        updated_lyrics = auto_edit::CreateInstrumental(m_lyrics);
                    }
                    else
                    {
                        LOG_WARN("Current track already has lyrics. It will not be marked as instrumental to avoid overwriting existing lyrics");
                    }

                } break;

                case ID_AUTO_REMOVE_EXTRA_SPACES:
                {
                    updated_lyrics = auto_edit::RemoveRepeatedSpaces(m_lyrics);
                } break;

                case ID_AUTO_REMOVE_EXTRA_BLANK_LINES:
                {
                    updated_lyrics = auto_edit::RemoveRepeatedBlankLines(m_lyrics);
                } break;

                case ID_AUTO_REMOVE_ALL_BLANK_LINES:
                {
                    updated_lyrics = auto_edit::RemoveAllBlankLines(m_lyrics);
                } break;

                case ID_AUTO_REPLACE_XML_CHARS:
                {
                    updated_lyrics = auto_edit::ReplaceHtmlEscapedChars(m_lyrics);
                } break;

                case ID_AUTO_RESET_CAPITALISATION:
                {
                    updated_lyrics = auto_edit::ResetCapitalisation(m_lyrics);
                } break;

                case ID_FIX_MALFORMED_TIMESTAMPS:
                {
                    updated_lyrics = auto_edit::FixMalformedTimestamps(m_lyrics);
                } break;
            }

            if(updated_lyrics.has_value())
            {
                LyricUpdateHandle update(LyricUpdateHandle::Type::Edit, m_now_playing, m_child_abort);
                update.set_started();
                update.set_result(std::move(updated_lyrics.value()), true);

                std::optional<LyricData> maybe_lyrics = io::process_available_lyric_update(update);
                assert(maybe_lyrics.has_value()); // Round-trip through the processing to avoid copies
                m_lyrics = std::move(maybe_lyrics.value());
                m_auto_search_avoided = false;
            }
        }
        catch(std::exception const & e)
        {
            LOG_ERROR("Failed to create OpenLyrics context menu: %s", e.what());
        }
    }

    void LyricPanel::OnDoubleClick(UINT /*virtualKeys*/, CPoint /*cursorPos*/)
    {
        if(m_now_playing == nullptr) return;

        auto update = std::make_unique<LyricUpdateHandle>(LyricUpdateHandle::Type::Edit, m_now_playing, m_child_abort);
        SpawnLyricEditor(get_wnd(), m_lyrics, *update);
        m_update_handles.push_back(std::move(update));
    }

    LRESULT LyricPanel::OnMouseWheel(UINT /*virtualKeys*/, short rotation, CPoint /*point*/)
    {
        // NOTE: WHEEL_DELTA is defined to be 120
        // rotation > 0 (usually 120) means we scrolled up
        // rotation < 0 (usually -120) means we scrolled down
        double scroll_ticks = double(rotation)/double(WHEEL_DELTA);

        switch(preferences::display::scroll_direction())
        {
            case LineScrollDirection::Horizontal:
            {
                // NOTE: It's not clear how far we should scroll here so we just use the height
                //       of an empty line as a reasonable first approximation.
                RECT fake_client_area = {};
                int one_line_height = ComputeWrappedLyricLineHeight(m_back_buffer, fake_client_area, _T(""));
                m_manual_scroll_distance += int(scroll_ticks * one_line_height);
            } break;

            case LineScrollDirection::Vertical:
            {
                RECT fake_client_area = {};
                int one_line_height = ComputeWrappedLyricLineHeight(m_back_buffer, fake_client_area, _T(""));
                m_manual_scroll_distance += int(scroll_ticks * one_line_height);
            } break;

            default:
                LOG_ERROR("Unexpected scroll direction setting: %d", int(preferences::display::scroll_direction()));
                assert(false);
                break;
        }

        Invalidate();
        return 0;
    }

    void LyricPanel::OnMouseMove(UINT /*virtualKeys*/, CPoint point)
    {
        if(m_manual_scroll_start.has_value())
        {
            int scroll_delta = 0;
            switch(preferences::display::scroll_direction())
            {
                case LineScrollDirection::Horizontal:
                    scroll_delta = point.x - m_manual_scroll_start.value().x;
                    break;

                case LineScrollDirection::Vertical:
                    scroll_delta = point.y - m_manual_scroll_start.value().y;
                    break;

                default:
                    LOG_ERROR("Unexpected scroll direction setting: %d", int(preferences::display::scroll_direction()));
                    assert(false);
                    break;
            }

            m_manual_scroll_distance += scroll_delta;
            m_manual_scroll_start = point;
            Invalidate();
        }
    }

    void LyricPanel::OnLMBDown(UINT /*virtualKeys*/, CPoint point)
    {
        m_manual_scroll_start = point;
        SetCapture();
    }

    void LyricPanel::OnLMBUp(UINT /*virtualKeys*/, CPoint /*point*/)
    {
        m_manual_scroll_start.reset();
        ReleaseCapture();
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

        UINT_PTR result = SetTimer(PANEL_UPDATE_TIMER, 16, nullptr);
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
        m_auto_search_avoided = false;

        auto update = std::make_unique<LyricUpdateHandle>(LyricUpdateHandle::Type::AutoSearch, track, m_child_abort);
        io::search_for_lyrics(*update, false);
        m_update_handles.push_back(std::move(update));
    }

    // ui_element_impl_withpopup autogenerates standalone version of our component and proper menu commands. Use ui_element_impl instead if you don't want that.
    class LyricPanelImpl : public ui_element_impl_withpopup<LyricPanel> {};
    FB2K_SERVICE_FACTORY(LyricPanelImpl)
    UIE_SHIM_PANEL_FACTORY(LyricPanel)

} // namespace

void repaint_all_lyric_panels()
{
    fb2k::inMainThread2([]()
    {
        core_api::ensure_main_thread();
        for(LyricPanel* panel : g_active_panels)
        {
            assert(panel != nullptr);
            InvalidateRect(panel->get_wnd(), nullptr, TRUE);
        }
    });
}

