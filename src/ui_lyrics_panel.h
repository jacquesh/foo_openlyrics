#pragma once

#include "stdafx.h"

#include "img_processing.h"
#include "lyric_io.h"

class LyricPanel : public CWindowImpl<LyricPanel>, private play_callback
{
public:
    // ATL window class declaration. Replace class name with your own when reusing code.
    DECLARE_WND_CLASS_EX(TEXT("{32CB89E1-3EA5-4AE7-A6E6-2DEA68A04D53}"), CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS, (-1))

    LyricPanel();

    void on_playback_starting(play_control::t_track_command /*cmd*/, bool /*paused*/) override {}
    void on_playback_new_track(metadb_handle_ptr track) override;
    void on_playback_stop(play_control::t_stop_reason reason) override;
    void on_playback_seek(double time) override;
    void on_playback_pause(bool state) override;
    void on_playback_edited(metadb_handle_ptr /*track*/) override {}
    void on_playback_dynamic_info(const file_info& /*info*/) override {}
    void on_playback_dynamic_info_track(const file_info& info) override;
    void on_playback_time(double /*time*/) override {}
    void on_volume_change(float /*new_volume*/) override {}

    CRect compute_background_image_rect();
    void load_custom_background_image();
    virtual void compute_background_image(); // TODO: Only virtual to support the external window

protected:
    BEGIN_MSG_MAP_EX(LyricPanel)
        MSG_WM_CREATE(OnWindowCreate)
        MSG_WM_DESTROY(OnWindowDestroy)
        MSG_WM_MOVE(OnWindowMove)
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

    virtual bool is_panel_ui_in_edit_mode() = 0;

    std::vector<std::unique_ptr<LyricUpdateHandle>> m_update_handles;
    void on_album_art_retrieved(album_art_data::ptr art_data);

private:
protected: // TODO: These 3 are only protected & virtual to support the external window
    virtual LRESULT OnWindowCreate(LPCREATESTRUCT);
    virtual void OnWindowDestroy();
    virtual void OnWindowMove(CPoint /*new_origin*/) {}
    virtual void OnWindowResize(UINT request_type, CSize new_size);
private:
    LRESULT OnTimer(WPARAM);
    virtual void OnPaint(CDCHandle); // TODO: Only virtual to support the external window
    BOOL OnEraseBkgnd(CDCHandle);
    void OnContextMenu(CWindow window, CPoint point);
    void OnDoubleClick(UINT virtualKeys, CPoint cursorPos);
    LRESULT OnMouseWheel(UINT virtualKeys, short rotation, CPoint point);
    void OnMouseMove(UINT virtualKeys, CPoint point);
    void OnLMBDown(UINT virtualKeys, CPoint point);
    void OnLMBUp(UINT virtualKeys, CPoint point);

    void StartTimer();
protected: // TODO: Only protected to support the external window
    void StopTimer();
private:

    void DrawNoLyrics(HDC dc, CRect client_area);
    void DrawUntimedLyrics(HDC dc, CRect client_area);
    void DrawTimestampedLyrics(HDC dc, CRect client_area);

    void InitiateLyricSearch();

protected: // TODO: Only protected to support the external window
    struct PlaybackTimeInfo
    {
        double current_time;
        double track_length;
    };
    PlaybackTimeInfo get_playback_time();
private:

    abort_callback_impl m_child_abort;

    bool m_timerRunning = false;
    UINT_PTR m_panel_update_timer;

protected: // TODO: These two are only protected to support the external window
    metadb_handle_ptr m_now_playing;
    metadb_v2_rec_t m_now_playing_info;
private:
    double m_now_playing_time_offset = 0.0;
protected: // TODO: Only protected to support the external window
    LyricData m_lyrics;
private:
    bool m_search_pending = false;
    bool m_auto_search_avoided = false;
    uint64_t m_auto_search_avoided_timestamp = 0;

    HDC m_back_buffer;
    HBITMAP m_back_buffer_bitmap;

    std::optional<CPoint> m_manual_scroll_start;
    int m_manual_scroll_distance;

    now_playing_album_art_notify* m_albumart_listen_handle = nullptr;
    Image m_albumart_original = {};
    Image m_custom_img_original = {};
protected: // TODO: Only protected to support the external window
    Image m_background_img = {};
};
