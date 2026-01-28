#include "stdafx.h"

#pragma warning(push, 0)
#include <foobar2000/SDK/metadb_info_container_impl.h>
#include <foobar2000/helpers/BumpableElem.h>
#include <libPPUI/win32_op.h>
#pragma warning(pop)

#include "img_processing.h"
#include "logging.h"
#include "lyric_auto_edit.h"
#include "lyric_data.h"
#include "lyric_io.h"
#include "lyric_metadata.h"
#include "lyric_search.h"
#include "math_util.h"
#include "metadb_index_search_avoidance.h"
#include "metrics.h"
#include "preferences.h"
#include "sources/lyric_source.h"
#include "tag_util.h"
#include "timer_block.h"
#include "ui_hooks.h"
#include "ui_lyrics_panel.h"
#include "ui_util.h"
#include "win32_util.h"

namespace
{
    // NOTE: This needs to be stored per-instance so that they all have their own
    //       timers, and stopping/starting doesn't fail when they conflict
    //       (e.g by having several panels all try to stop the same timer).
    static UINT_PTR PANEL_UPDATE_TIMER = 2304692;

    static std::vector<LyricPanel*> g_active_panels;
}

LyricPanel::LyricPanel()
    : m_panel_update_timer(PANEL_UPDATE_TIMER)
    , m_now_playing(nullptr)
    , m_lyrics()
{
    PANEL_UPDATE_TIMER++;
}

void LyricPanel::on_album_art_retrieved(album_art_data::ptr art_data)
{
    if(art_data == nullptr)
    {
        return;
    }
    if(preferences::background::image_type() != BackgroundImageType::AlbumArt)
    {
        return;
    }

    TIME_FUNCTION();
    LOG_INFO("New album art data retrieved");

    std::optional<Image> maybe_img = decode_image((const uint8_t*)art_data->data(), art_data->size());
    if(!maybe_img.has_value())
    {
        LOG_WARN("Failed to load album art image");
        return;
    }
    m_albumart_original = std::move(maybe_img.value());
    compute_background_image();
}

CRect LyricPanel::compute_background_image_rect()
{
    CRect client_rect;
    WIN32_OP_D(GetClientRect(&client_rect))

    CSize size = {};
    size.cx = client_rect.Width();
    size.cy = client_rect.Height();

    if(preferences::background::maintain_img_aspect_ratio())
    {
        const double img_aspect_ratio = double(m_albumart_original.width) / double(m_albumart_original.height);
        const int width_when_scaling_to_fit_y = int(double(client_rect.Height()) * img_aspect_ratio);
        const int height_when_scaling_to_fit_x = int(double(client_rect.Width()) / img_aspect_ratio);

        if(width_when_scaling_to_fit_y > client_rect.Width())
        {
            size.cx = client_rect.Width();
            size.cy = height_when_scaling_to_fit_x;
        }
        else if(height_when_scaling_to_fit_x > client_rect.Height())
        {
            size.cx = width_when_scaling_to_fit_y;
            size.cy = client_rect.Height();
        }
    }

    // Centre the image in the available space
    CPoint topleft = {};
    topleft.x = (client_rect.Width() - size.cx) / 2;
    topleft.y = (client_rect.Height() - size.cy) / 2;

    CRect result = {};
    result.left = topleft.x;
    result.top = topleft.y;
    result.right = topleft.x + size.cx;
    result.bottom = topleft.y + size.cy;
    return result;
}

void LyricPanel::load_custom_background_image()
{
    const std::string path = preferences::background::custom_image_path();
    LOG_INFO("Load custom background image from: %s", path.c_str());

    std::optional<Image> maybe_img = load_image(path.c_str());
    if(maybe_img.has_value())
    {
        assert(maybe_img.value().valid());
        LOG_INFO("Loaded custom %dx%d background image", maybe_img.value().width, maybe_img.value().height);
        m_custom_img_original = std::move(maybe_img.value());
    }
    else
    {
        LOG_WARN("Failed to load custom background image from path: %s", path.c_str());
        m_custom_img_original = {};
    }
}

void LyricPanel::compute_background_image()
{
    TIME_FUNCTION();

    CRect client_rect;
    WIN32_OP_D(GetClientRect(&client_rect))
    if((client_rect.Width() == 0) || (client_rect.Height() == 0))
    {
        // This happens, for example, when we minimize the external window.
        // In that case there's no point re-computing the (now empty) background.
        // If we keep the original image around then it'll still be here when we
        // maximise again.
        LOG_INFO("Ignoring request to compute zero-size background image");
        return;
    }

    Image bg_colour = {};
    switch(preferences::background::fill_type())
    {
        case BackgroundFillType::Default:
        {
            const RGBAColour colour = from_colorref(defaultui::background_colour());
            bg_colour = generate_background_colour(client_rect.Width(), client_rect.Height(), colour);
        }
        break;

        case BackgroundFillType::SolidColour:
        {
            const RGBAColour colour = from_colorref(preferences::background::colour());
            bg_colour = generate_background_colour(client_rect.Width(), client_rect.Height(), colour);
        }
        break;

        case BackgroundFillType::Gradient:
        {
            RGBAColour topleft = from_colorref(preferences::background::gradient_tl());
            RGBAColour topright = from_colorref(preferences::background::gradient_tr());
            RGBAColour botleft = from_colorref(preferences::background::gradient_bl());
            RGBAColour botright = from_colorref(preferences::background::gradient_br());

            bg_colour = generate_background_colour(client_rect.Width(),
                                                   client_rect.Height(),
                                                   topleft,
                                                   topright,
                                                   botleft,
                                                   botright);
        }
        break;
    }

    const BackgroundImageType img_type = preferences::background::image_type();
    if(img_type == BackgroundImageType::None)
    {
        m_background_img = std::move(bg_colour);
    }
    else
    {
        const CRect img_rect = compute_background_image_rect();
        const double bg_opacity = preferences::background::image_opacity();
        const int blur_radius = preferences::background::blur_radius();

        Image resized_img = {};
        if(img_type == BackgroundImageType::AlbumArt)
        {
            resized_img = resize_image(m_albumart_original, img_rect.Width(), img_rect.Height());
        }
        else if(img_type == BackgroundImageType::CustomImage)
        {
            resized_img = resize_image(m_custom_img_original, img_rect.Width(), img_rect.Height());
        }
        else
        {
            LOG_WARN("Unrecognised background image type: %d", int(img_type));
            assert(false);
        }

        // The resized image will be invalid if there was no album art or no custom image.
        if(resized_img.valid())
        {
            const Image combined_img = lerp_offset_image(bg_colour, resized_img, img_rect.TopLeft(), bg_opacity);
            m_background_img = blur_image(combined_img, blur_radius);
        }
        else
        {
            m_background_img = std::move(bg_colour);
        }
    }

    toggle_image_rgba_bgra_inplace(m_background_img);
}

void LyricPanel::on_playback_new_track(metadb_handle_ptr track)
{
    const bool track_changed = (track != m_now_playing);
    m_now_playing = track;
    m_now_playing_info = get_full_metadata(track);
    m_manual_scroll_distance = 0;
    m_now_playing_time_offset = 0.0;

    // NOTE: If playback is paused on startup then this gets called with the paused track,
    //       but playback is paused so we don't actually want to run the timer
    service_ptr_t<playback_control> playback = playback_control::get();
    if(!playback->is_paused())
    {
        StartTimer();
    }

    if(track_changed && (preferences::background::image_type() == BackgroundImageType::AlbumArt))
    {
        m_background_img = {};
        m_albumart_original = {};
    }

    if(track_changed)
    {
        m_lyrics = {};
    }
}

void LyricPanel::on_playback_dynamic_info_track(const file_info& info)
{
    // NOTE: This is not called when we start playing tracks that are not remote/internet radio
    service_ptr_t<metadb_info_container_const_impl> info_container_impl =
        new service_impl_t<metadb_info_container_const_impl>();
    info_container_impl->m_info = info;

    metadb_v2_rec_t meta_record = {};
    meta_record.info = info_container_impl;

    m_now_playing_info = meta_record;
    m_manual_scroll_distance = 0;
    m_lyrics = {};

    // Set the new "current time" offset/baseline so that we can at least compute timestamps
    // that are approximately-correct for internet radio streams that go beyond a single track.
    // We're assuming here that the only time we ever get dynamic track info is when we've either
    // just started playback or when the track changes.
    service_ptr_t<playback_control> playback = playback_control::get();
    m_now_playing_time_offset = playback->playback_get_position();
}

void LyricPanel::on_playback_stop(play_control::t_stop_reason reason)
{
    // When starting another track, track-change calculations will all be
    // handled when the new track starts (immediately after this callback).
    if(reason == play_control::t_stop_reason::stop_reason_starting_another)
    {
        return;
    }

    m_now_playing = nullptr;
    m_now_playing_info = {};
    m_lyrics = {};
    m_auto_search_avoided_reason = SearchAvoidanceReason::Allowed;
    StopTimer();

    m_albumart_original = {};
    compute_background_image();
    Invalidate(); // Draw one more time to clear the panel
}

void LyricPanel::on_playback_pause(bool state)
{
    if(state)
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

void LyricPanel::ui_colors_changed()
{
    // This callback executes when the fb2k UI colour config is changed (including toggling dark mode).
    // We depend on the UI colour config in some cases for background creation, so re-run that when
    // the config changes for consistency.
    compute_background_image();
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

    // Register for notifications about available album art
    now_playing_album_art_notify_manager::ptr art_manager = now_playing_album_art_notify_manager::get();
    m_albumart_listen_handle = art_manager->add([this](album_art_data::ptr art_data)
                                                { return on_album_art_retrieved(art_data); });

    // If there's already a track playing that we have art for, then immediately process
    // that art, because we're not going to get a callback for it (that's already happened!)
    album_art_data::ptr current_art = art_manager->current();
    if(current_art != nullptr)
    {
        on_album_art_retrieved(current_art);
    }

    if(preferences::background::image_type() == BackgroundImageType::CustomImage)
    {
        load_custom_background_image();
        // We don't need to compute the composited background image here, because this
        // callback is going to be followed shortly by WM_SIZE, where we'll recompute
        // anyway.
    }

    const uint32_t playback_flags = flag_on_playback_new_track | flag_on_playback_stop | flag_on_playback_seek
                                    | flag_on_playback_pause | flag_on_playback_dynamic_info_track;
    play_callback_manager::get()->register_callback(this, playback_flags, false);
    return 0;
}

void LyricPanel::OnWindowDestroy()
{
    play_callback_manager::get()->unregister_callback(this);

    if(m_back_buffer_bitmap != nullptr) DeleteObject(m_back_buffer_bitmap);
    if(m_back_buffer != nullptr) DeleteDC(m_back_buffer);

    // Prevent us from getting album-art callbacks after destruction
    now_playing_album_art_notify_manager::ptr art_manager = now_playing_album_art_notify_manager::get();
    art_manager->remove(m_albumart_listen_handle);

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
    if(!m_background_img.valid() || (client_rect.Width() != m_background_img.width)
       || (client_rect.Height() != m_background_img.height))
    {
        compute_background_image();
    }
}

LRESULT LyricPanel::OnNonClientCalcSize(BOOL /*calc_valid_rects*/, LPARAM /*lparam*/)
{
    SetMsgHandled(false);
    return 0;
}

UINT LyricPanel::OnNonClientHitTest(CPoint /*point*/)
{
    SetMsgHandled(false);
    return 0;
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

static int _WrapSimpleLyricsLineToRect(HDC dc, CRect clip_rect, std::tstring_view line, const CPoint* origin)
{
    TEXTMETRIC font_metrics = {};
    WIN32_OP_D(GetTextMetrics(dc, &font_metrics))
    const int line_height = font_metrics.tmHeight + preferences::display::linegap();

    if(line.length() == 0)
    {
        return line_height;
    }

    int visible_width = clip_rect.Width();

    // This serves as an upper bound on the number of chars we draw on a single line.
    // Used to prevent GDI from having to compute the size of very long strings.
    int generous_max_chars = 256;
    if(font_metrics.tmAveCharWidth > 0)
    {
        assert(visible_width >= 0);
        const int avg_chars_that_fit = (visible_width / font_metrics.tmAveCharWidth) + 1;
        generous_max_chars = 3 * avg_chars_that_fit;
    }

    assert(line.length() <= INT_MAX);
    std::tstring_view text_outstanding = line;
    int total_height = 0;
    while(text_outstanding.length() > 0)
    {
        size_t leading_spaces = find_first_nonwhitespace(text_outstanding);
        text_outstanding.remove_prefix(std::min(leading_spaces, text_outstanding.size()));

        size_t last_not_space = find_last_nonwhitespace(text_outstanding);
        if(last_not_space != std::tstring_view::npos)
        {
            size_t trailing_spaces = text_outstanding.length() - 1 - last_not_space;
            text_outstanding.remove_suffix(trailing_spaces);
        }

        size_t next_line_start_index = text_outstanding.length();
        int chars_to_draw = std::min(int(text_outstanding.length()), generous_max_chars);
        while(true)
        {
            SIZE line_size;
            BOOL extent_success = GetTextExtentPoint32(dc, text_outstanding.data(), chars_to_draw, &line_size);
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
                const int previous_space_index = int(find_last_whitespace(text_outstanding, chars_to_draw - 1));
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
            bool clipped = (draw_y + font_metrics.tmDescent < clip_rect.top)
                           || (draw_y - font_metrics.tmAscent > clip_rect.bottom);
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
static int _WrapCompoundLyricsLineToRect(HDC dc, CRect clip_rect, std::tstring_view line, CPoint* origin)
{
    if(line.length() == 0)
    {
        return _WrapSimpleLyricsLineToRect(dc, clip_rect, line, origin);
    }

    int result = 0;
    size_t start_index = 0;
    while(start_index < line.length())
    {
        size_t end_index = std::min(line.length(), line.find('\n', start_index));
        size_t length = end_index - start_index;
        std::tstring_view view(&line.data()[start_index], length);
        int row_height = _WrapSimpleLyricsLineToRect(dc, clip_rect, view, origin);
        if(origin != nullptr)
        {
            origin->y += row_height;
        }
        result += row_height;
        start_index = end_index + 1;
    }
    return result;
}

static int ComputeWrappedLyricLineHeight(HDC dc, CRect clip_rect, const std::tstring& line)
{
    return _WrapCompoundLyricsLineToRect(dc, clip_rect, line, nullptr);
}

static int DrawWrappedLyricLine(HDC dc, CRect clip_rect, const std::tstring_view line, CPoint origin)
{
    return _WrapCompoundLyricsLineToRect(dc, clip_rect, line, &origin);
}

static CPoint get_text_origin(CRect client_rect, TEXTMETRIC& font_metrics)
{
    const CPoint centre = client_rect.CenterPoint();
    LONG top_x = 0;
    LONG top_y = 0;
    switch(preferences::display::text_alignment())
    {
        case TextAlignment::MidCentre:
        case TextAlignment::TopCentre: top_x = centre.x; break;

        case TextAlignment::MidLeft:
        case TextAlignment::TopLeft: top_x = client_rect.left; break;

        case TextAlignment::MidRight:
        case TextAlignment::TopRight: top_x = client_rect.right; break;

        default: LOG_WARN("Unrecognised text alignment option"); return {};
    }

    switch(preferences::display::text_alignment())
    {
        case TextAlignment::MidCentre:
        case TextAlignment::MidLeft:
        case TextAlignment::MidRight: top_y = centre.y; break;

        case TextAlignment::TopCentre:
        case TextAlignment::TopLeft:
        case TextAlignment::TopRight: top_y = client_rect.top + font_metrics.tmAscent; break;

        default: LOG_WARN("Unrecognised text alignment option"); return {};
    }

    // NOTE: The drawing call uses the glyph baseline as the origin.
    //       We want our text to be perfectly vertically centered, so we need to offset it
    //       but the difference between the baseline and the vertical centre of the font.
    const int baseline_centre_correction = (font_metrics.tmAscent - font_metrics.tmDescent) / 2;
    top_y += baseline_centre_correction;
    return { top_x, top_y };
}

static bool is_text_top_aligned()
{
    switch(preferences::display::text_alignment())
    {
        case TextAlignment::TopCentre:
        case TextAlignment::TopLeft:
        case TextAlignment::TopRight: return true;

        case TextAlignment::MidCentre:
        case TextAlignment::MidLeft:
        case TextAlignment::MidRight: return false;

        default: LOG_WARN("Unrecognised text alignment option"); return false;
    }
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

    std::string artist = track_metadata(m_now_playing_info, "artist");
    std::string album = track_metadata(m_now_playing_info, "album");
    std::string title = track_metadata(m_now_playing_info, "title");

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

    TEXTMETRIC font_metrics = {};
    WIN32_OP_D(GetTextMetrics(dc, &font_metrics))
    CPoint origin = get_text_origin(client_rect, font_metrics);
    if(!is_text_top_aligned())
    {
        origin.y -= total_height / 2;
    }

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

    std::optional<std::string> progress_msg = get_autosearch_progress_message();
    if(progress_msg.has_value())
    {
        std::tstring progress_text = to_tstring(progress_msg.value());
        origin.y += DrawWrappedLyricLine(dc, client_rect, progress_text, origin);
    }
    else if(m_auto_search_avoided_reason != SearchAvoidanceReason::Allowed)
    {
        // We specifically only consider the avoidance reason if we *don't* have a progress
        // message. So the `else` is important in the `else if` above.
        // This is because "avoiding" a search just means we don't search *remote* sources.
        // We always search local sources. There's no point in skipping an attempt to load
        // a file from disk (or load data from a metadata tag).

        const double search_avoided_msg_seconds = 15.0;
        // A "tick" here means "100-nanoseconds"
        uint64_t search_avoided_msg_ticks = static_cast<uint64_t>(search_avoided_msg_seconds * 10'000'000);
        uint64_t ticks_since_search_avoided = filetimestamp_from_system_timer() - m_auto_search_avoided_timestamp;
        if(ticks_since_search_avoided < search_avoided_msg_ticks)
        {
            origin.y += DrawWrappedLyricLine(dc, client_rect, _T(""), origin);
            switch(m_auto_search_avoided_reason)
            {
                case SearchAvoidanceReason::RepeatedFailures:
                {
                    origin.y += DrawWrappedLyricLine(dc,
                                                     client_rect,
                                                     _T("Auto-search skipped: search failed too many times."),
                                                     origin);
                    origin.y += DrawWrappedLyricLine(dc,
                                                     client_rect,
                                                     _T("Manually request a lyrics search to try again."),
                                                     origin);
                }
                break;

                case SearchAvoidanceReason::MarkedInstrumental:
                {
                    origin.y += DrawWrappedLyricLine(
                        dc,
                        client_rect,
                        _T("Auto-search skipped: track was explicitly marked 'instrumental'"),
                        origin);
                    origin.y += DrawWrappedLyricLine(dc,
                                                     client_rect,
                                                     _T("Manually request a lyrics search to clear that status."),
                                                     origin);
                }
                break;

                case SearchAvoidanceReason::MatchesSkipFilter:
                {
                    origin.y += DrawWrappedLyricLine(dc,
                                                     client_rect,
                                                     _T("Auto-search skipped: track matched the skip filter."),
                                                     origin);
                }
                break;

                case SearchAvoidanceReason::Allowed: // fallthrough
                default: LOG_WARN("Unrecognised search avoidance reason when rendering no-lyrics"); break;
            }
        }
    }
}

void LyricPanel::DrawUntimedLyrics(HDC dc, CRect client_area)
{
    double track_fraction = 0.0;
    if(preferences::display::scroll_type() == LineScrollType::Automatic)
    {
        const PlaybackTimeInfo playback_time = get_playback_time();
        track_fraction = playback_time.current_time / playback_time.track_length;
    }

    TEXTMETRIC font_metrics = {};
    WIN32_OP_D(GetTextMetrics(dc, &font_metrics))

    const int total_height = std::accumulate(m_lyrics.lines.begin(),
                                             m_lyrics.lines.end(),
                                             0,
                                             [dc, client_area](int x, const LyricDataLine& line)
                                             { return x + ComputeWrappedLyricLineHeight(dc, client_area, line.text); });

    const int total_scrollable_height = total_height - font_metrics.tmHeight - preferences::display::linegap();

    CPoint origin = get_text_origin(client_area, font_metrics);
    origin.y -= (int)(track_fraction * total_scrollable_height);

    // clang-format off: Don't wrap the calculation lines, even if they're too long
    // NOTE: We support the manual scroll distance here so that people can offset
    //       the default automated scrolling (which is often significantly wrong)
    //       We want to restrict it to:
    //
    //       1) Not scroll completely off the top of the panel ("minimum" scroll):
    //          bottom_baseline_y >= ascent
    //          top_baseline_y + total_scrollable_height >= ascent
    //          top_baseline_y >= ascent - total_scrollable_height
    //          origin_y - (fraction * total_scrollable_height) + m_manual_scroll_distance >= ascent - total_scrollable_height
    //          m_manual_scroll_distance >= ascent - total_scrollable_height - (origin_y - (fraction * total_scrollable_height))
    //       2) Not scroll completely off the bottom of the panel ("maximum" scroll)
    //          top_baseline_y <= panel_height  ("maximum" scroll)
    //          origin_y - (fraction * total_scrollable_height) + m_manual_scroll_distance <= panel_height
    //          m_manual_scroll_distance <= panel_height - (origin_y - (fraction * total_scrollable_height))
    //
    //       and recall that we've already subtracted fraction*total_scrollable_height from origin_y above,
    //       so we don't need to do it again below, we just use the origin value as-is.
    // clang-format on
    const int min_scroll = font_metrics.tmAscent - total_scrollable_height - origin.y;
    const int max_scroll = client_area.Height() - origin.y;
    m_manual_scroll_distance = std::min(std::max(m_manual_scroll_distance, min_scroll), max_scroll);
    origin.y += m_manual_scroll_distance;

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

struct LyricScrollPosition
{
    int active_line_index;
    double next_line_scroll_factor; // How far away from the active line (and towards the next line) we should be
                                    // scrolled. Values are in the range [0,1]
};

static LyricScrollPosition get_scroll_position(const LyricData& lyrics, double current_time, double scroll_duration)
{
    int active_line_index = -1;
    int lyric_line_count = static_cast<int>(lyrics.lines.size());
    while((active_line_index + 1 < lyric_line_count) && (current_time > lyrics.LineTimestamp(active_line_index + 1)))
    {
        active_line_index++;
    }

    const double active_line_time = lyrics.LineTimestamp(active_line_index);
    const double next_line_time = lyrics.LineTimestamp(active_line_index + 1);

    const double scroll_start_time = std::max(active_line_time, next_line_time - scroll_duration);
    const double scroll_end_time = next_line_time;

    double next_line_scroll_factor = lerp_inverse_clamped(scroll_start_time, scroll_end_time, current_time);
    return { active_line_index, next_line_scroll_factor };
}

void LyricPanel::DrawTimestampedLyrics(HDC dc, CRect client_area)
{
    // NOTE: The drawing call uses the glyph baseline as the origin.
    //       We want our text to be perfectly vertically centered, so we need to offset it
    //       but the difference between the baseline and the vertical centre of the font.
    TEXTMETRIC font_metrics = {};
    WIN32_OP_D(GetTextMetrics(dc, &font_metrics))

    t_ui_color past_text_colour = preferences::display::past_text_colour();
    t_ui_color main_text_colour = preferences::display::main_text_colour();
    t_ui_color hl_colour = preferences::display::highlight_colour();

    const PlaybackTimeInfo playback_time = get_playback_time();
    const double scroll_time = preferences::display::scroll_time_seconds();
    const LyricScrollPosition scroll = get_scroll_position(m_lyrics, playback_time.current_time, scroll_time);

    const double fade_duration = preferences::display::highlight_fade_seconds();
    const LyricScrollPosition fade = get_scroll_position(m_lyrics, playback_time.current_time, fade_duration);

    int text_height_above_active_line = 0;
    int active_line_height = 0;
    if(scroll.active_line_index >= 0)
    {
        for(int i = 0; i < scroll.active_line_index; i++)
        {
            text_height_above_active_line += ComputeWrappedLyricLineHeight(dc, client_area, m_lyrics.lines[i].text);
        }
        active_line_height = ComputeWrappedLyricLineHeight(dc,
                                                           client_area,
                                                           m_lyrics.lines[scroll.active_line_index].text);
    }

    int next_line_scroll = (int)((double)active_line_height * scroll.next_line_scroll_factor);
    CPoint origin = get_text_origin(client_area, font_metrics);
    origin.y -= text_height_above_active_line + next_line_scroll;

    const int lyric_line_count = static_cast<int>(m_lyrics.lines.size());
    for(int line_index = 0; line_index < lyric_line_count; line_index++)
    {
        const LyricDataLine& line = m_lyrics.lines[line_index];
        if(line_index == scroll.active_line_index)
        {
            t_ui_color colour = lerp(hl_colour, past_text_colour, fade.next_line_scroll_factor);
            SetTextColor(dc, colour);
        }
        else if(line_index == scroll.active_line_index + 1)
        {
            t_ui_color colour = lerp(main_text_colour, hl_colour, fade.next_line_scroll_factor);
            SetTextColor(dc, colour);
        }
        else if(line_index < scroll.active_line_index)
        {
            SetTextColor(dc, past_text_colour);
        }
        else
        {
            SetTextColor(dc, main_text_colour);
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

void LyricPanel::OnPaint(CDCHandle)
{
    // As suggested in this article: https://docs.microsoft.com/en-us/previous-versions/ms969905(v=msdn.10)
    // We get flickering if we draw everything to the UI directly, so instead we render everything to a back buffer
    // and then blit the whole thing to the screen at the end.
    PAINTSTRUCT paintstruct;
    HDC front_buffer = BeginPaint(&paintstruct);

    CRect client_rect;
    WIN32_OP_D(GetClientRect(&client_rect))

    if(m_background_img.valid())
    {
        BITMAPINFO bmp = {};
        bmp.bmiHeader.biSize = sizeof(bmp.bmiHeader);
        bmp.bmiHeader.biWidth = m_background_img.width;

        // Positive for origin in bottom-left, negative for origin in top-left
        bmp.bmiHeader.biHeight = -m_background_img.height;

        bmp.bmiHeader.biPlanes = 1;
        bmp.bmiHeader.biBitCount = 32;
        bmp.bmiHeader.biCompression = BI_RGB;
        int scan_lines_copied = StretchDIBits(m_back_buffer,
                                              client_rect.left,
                                              client_rect.top,
                                              client_rect.Width(),
                                              client_rect.Height(),
                                              0,
                                              0,
                                              m_background_img.width,
                                              m_background_img.height,
                                              m_background_img.pixels,
                                              &bmp,
                                              DIB_RGB_COLORS,
                                              SRCCOPY);
        if(scan_lines_copied == 0)
        {
            LOG_WARN("Failed to draw background gradient");
        }
    }
    else
    {
        const t_ui_color bg_colour = defaultui::background_colour();
        HBRUSH bg_brush = CreateSolidBrush(bg_colour);
        FillRect(m_back_buffer, &client_rect, bg_brush);
        DeleteObject(bg_brush);
    }

    SelectObject(m_back_buffer, preferences::display::font());
    COLORREF color_result = SetTextColor(m_back_buffer, preferences::display::main_text_colour());
    if(color_result == CLR_INVALID)
    {
        LOG_WARN("Failed to set text colour: %d", GetLastError());
    }

    UINT horizontal_alignment = 0;
    switch(preferences::display::text_alignment())
    {
        case TextAlignment::MidCentre:
        case TextAlignment::TopCentre: horizontal_alignment = TA_CENTER; break;

        case TextAlignment::MidLeft:
        case TextAlignment::TopLeft: horizontal_alignment = TA_LEFT; break;

        case TextAlignment::MidRight:
        case TextAlignment::TopRight: horizontal_alignment = TA_RIGHT; break;

        default: LOG_WARN("Unrecognised text alignment option"); break;
    }
    UINT align_result = SetTextAlign(m_back_buffer, TA_BASELINE | horizontal_alignment);
    if(align_result == GDI_ERROR)
    {
        LOG_WARN("Failed to set text alignment: %d", GetLastError());
    }
    if(m_lyrics.IsEmpty())
    {
        DrawNoLyrics(m_back_buffer, client_rect);
    }
    else if(m_lyrics.IsTimestamped() && (preferences::display::scroll_type() == LineScrollType::Automatic))
    {
        DrawTimestampedLyrics(m_back_buffer, client_rect);
    }
    else // We have lyrics, but no timestamps
    {
        DrawUntimedLyrics(m_back_buffer, client_rect);
    }

    BitBlt(front_buffer,
           client_rect.left,
           client_rect.top,
           client_rect.Width(),
           client_rect.Height(),
           m_back_buffer,
           0,
           0,
           SRCCOPY);
    EndPaint(&paintstruct);
}

void LyricPanel::OnContextMenu(CWindow window, CPoint point)
{
    if(is_panel_ui_in_edit_mode())
    {
        // NOTE: When edit-mode is enabled then we want the default behaviour for allowing users
        //       to change this panel, so we mark the message as unhandled and let foobar's default
        //       handling take care of it for us.
        SetMsgHandled(FALSE);
        return;
    }

    // handle the context menu key case - center the menu
    if(point == CPoint(-1, -1))
    {
        CRect rc;
        WIN32_OP(window.GetWindowRect(&rc))
        point = rc.CenterPoint();
    }

    try
    {
        UINT disabled_without_nowplaying = (m_now_playing == nullptr) ? MF_GRAYED : 0;
        UINT disabled_without_lyrics = m_lyrics.IsEmpty() ? MF_GRAYED : 0;
        UINT disabled_without_timestamps = m_lyrics.IsTimestamped() ? 0 : MF_GRAYED;
        enum
        {
            ID_SEARCH_LYRICS = 1,
            ID_SEARCH_LYRICS_MANUAL,
            ID_SAVE_LYRICS,
            ID_SHOW_LYRIC_INFO,
            ID_PREFERENCES,
            ID_EDIT_LYRICS,
            ID_OPEN_FILE_DIR,
            ID_AUTO_MARK_INSTRUMENTAL,
            ID_AUTO_REMOVE_EXTRA_SPACES,
            ID_AUTO_REMOVE_EXTRA_BLANK_LINES,
            ID_AUTO_REMOVE_ALL_BLANK_LINES,
            ID_AUTO_REPLACE_XML_CHARS,
            ID_AUTO_RESET_CAPITALISATION,
            ID_AUTO_FIX_MALFORMED_TIMESTAMPS,
            ID_AUTO_REMOVE_TIMESTAMPS,
            ID_AUTO_REMOVE_SURROUNDING_SPACE,
            ID_DELETE_CURRENT_LYRICS,
            ID_OPEN_EXTERNAL_WINDOW,
            ID_CMD_COUNT,
        };

        // clang-format off: Don't wrap long menu lines
        CMenu menu_edit = nullptr;
        WIN32_OP(menu_edit.CreatePopupMenu())
        AppendMenu(menu_edit, MF_STRING | disabled_without_nowplaying, ID_AUTO_MARK_INSTRUMENTAL, _T("Mark as instrumental"));
        AppendMenu(menu_edit, MF_SEPARATOR, 0, nullptr);
        AppendMenu(menu_edit, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_AUTO_REPLACE_XML_CHARS, _T("Replace &&-named HTML characters"));
        AppendMenu(menu_edit, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_AUTO_REMOVE_EXTRA_SPACES, _T("Remove repeated spaces"));
        AppendMenu(menu_edit, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_AUTO_REMOVE_SURROUNDING_SPACE, _T("Remove surrounding whitespace from each line"));
        AppendMenu(menu_edit, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_AUTO_REMOVE_EXTRA_BLANK_LINES, _T("Remove repeated blank lines"));
        AppendMenu(menu_edit, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_AUTO_REMOVE_ALL_BLANK_LINES, _T("Remove all blank lines"));
        AppendMenu(menu_edit, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_AUTO_RESET_CAPITALISATION, _T("Reset capitalisation"));
        AppendMenu(menu_edit, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics | disabled_without_timestamps, ID_AUTO_FIX_MALFORMED_TIMESTAMPS, _T("Fix malformed timestamps"));
        AppendMenu(menu_edit, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics | disabled_without_timestamps, ID_AUTO_REMOVE_TIMESTAMPS, _T("Remove timestamps"));
        AppendMenu(menu_edit, MF_SEPARATOR, 0, nullptr);
        AppendMenu(menu_edit, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_DELETE_CURRENT_LYRICS, _T("Delete current lyrics"));

        CMenu menu = nullptr;
        WIN32_OP(menu.CreatePopupMenu())
        AppendMenu(menu, MF_STRING | disabled_without_nowplaying, ID_SEARCH_LYRICS, _T("Search for lyrics"));
        AppendMenu(menu, MF_STRING | disabled_without_nowplaying, ID_SEARCH_LYRICS_MANUAL, _T("Search for lyrics (manually)"));
        AppendMenu(menu, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_SAVE_LYRICS, _T("Save lyrics"));
        AppendMenu(menu, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_SHOW_LYRIC_INFO, _T("About current lyrics"));
        AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenu(menu, MF_STRING | disabled_without_nowplaying, ID_EDIT_LYRICS, _T("Edit lyrics"));
        AppendMenu(menu, MF_STRING | MF_POPUP, (UINT_PTR)menu_edit.m_hMenu, _T("Auto-edit lyrics"));
        AppendMenu(menu, MF_STRING, ID_OPEN_EXTERNAL_WINDOW, _T("Open external window (experimental)"));
        AppendMenu(menu, MF_STRING | disabled_without_nowplaying | disabled_without_lyrics, ID_OPEN_FILE_DIR, _T("Open file location"));
        AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenu(menu, MF_STRING, ID_PREFERENCES, _T("Preferences"));

        CMenuDescriptionHybrid menudesc(m_hWnd);
        menudesc.Set(ID_SEARCH_LYRICS, "Start a completely new search for lyrics");
        menudesc.Set(ID_SEARCH_LYRICS_MANUAL, "Start a new search for lyrics with customisable search terms and multiple results");
        menudesc.Set(ID_SAVE_LYRICS, "Save the current lyrics, even if they would not be auto-saved");
        menudesc.Set(ID_SHOW_LYRIC_INFO, "Show extra info about this track's lyrics");
        menudesc.Set(ID_PREFERENCES, "Open the OpenLyrics preferences page");
        menudesc.Set(ID_EDIT_LYRICS, "Open the lyric editor with the current lyrics");
        menudesc.Set(ID_OPEN_FILE_DIR, "Open explorer to the location of the lyrics file");
        menudesc.Set(ID_AUTO_MARK_INSTRUMENTAL, "Remove existing lyrics and skip future automated lyric searches");
        menudesc.Set(ID_AUTO_REPLACE_XML_CHARS, "Replace &-encoded named HTML characters (e.g &lt;) with the characters they represent (e.g <)");
        menudesc.Set(ID_AUTO_REMOVE_EXTRA_SPACES, "Replace sequences of multiple whitespace characters with a single space");
        menudesc.Set(ID_AUTO_REMOVE_EXTRA_BLANK_LINES, "Replace sequences of multiple empty lines with just a single empty line");
        menudesc.Set(ID_AUTO_REMOVE_ALL_BLANK_LINES, "Remove all empty lines");
        menudesc.Set(ID_AUTO_RESET_CAPITALISATION, "Reset capitalisation of each line so that only the first character is upper case");
        menudesc.Set(ID_AUTO_FIX_MALFORMED_TIMESTAMPS, "Fix timestamps that are slightly malformed so that they're recognised as timestamps and not shown in the text");
        menudesc.Set(ID_AUTO_REMOVE_TIMESTAMPS, "Remove timestamps, changing from synced lyrics to unsynced lyrics");
        menudesc.Set(ID_AUTO_REMOVE_SURROUNDING_SPACE, "Remove excess whitespace surrounding each line of lyrics");
        // clang-format on

        std::optional<LyricData> updated_lyrics;
        int cmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
                                      point.x,
                                      point.y,
                                      menudesc,
                                      nullptr);
        switch(cmd)
        {
            case ID_OPEN_EXTERNAL_WINDOW:
            {
                SpawnExternalLyricWindow();
            }
            break;
            case ID_SEARCH_LYRICS:
            {
                if(m_now_playing == nullptr) break;

                m_lyrics = {};
                const bool ignore_search_avoidance = true;
                initiate_lyrics_autosearch(m_now_playing, m_now_playing_info, ignore_search_avoidance);
            }
            break;

            case ID_SEARCH_LYRICS_MANUAL:
            {
                if(m_now_playing == nullptr) break;

                SpawnManualLyricSearch(m_now_playing, m_now_playing_info);
            }
            break;

            case ID_SAVE_LYRICS:
            {
                if(m_now_playing == nullptr) break;

                if(m_lyrics.IsEmpty())
                {
                    LOG_INFO("Attempt to manually save empty lyrics, ignoring...");
                    break;
                }

                try
                {
                    const bool allow_overwrite = true;
                    io::save_lyrics(m_now_playing, m_now_playing_info, m_lyrics, allow_overwrite);
                }
                catch(const std::exception& e)
                {
                    LOG_ERROR("Failed to complete manually requested lyric save: %s", e.what());
                }
            }
            break;

            case ID_SHOW_LYRIC_INFO:
            {
                const char* dialog_title = "Lyric info";
                const std::string info = get_lyric_metadata_string(m_lyrics, m_now_playing_info);
                popup_message::g_show(info.c_str(), dialog_title);
            }
            break;

            case ID_PREFERENCES:
            {
                ui_control::get()->show_preferences(GUID_PREFERENCES_PAGE_ROOT);
            }
            break;

            case ID_EDIT_LYRICS:
            {
                if(m_now_playing == nullptr) break;

                SpawnLyricEditor(m_lyrics, m_now_playing, m_now_playing_info);
            }
            break;

            case ID_OPEN_FILE_DIR:
            {
                if(m_now_playing == nullptr) break;

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
                    for(size_t i = pathstr.length(); i > 0; i--)
                    {
                        if(pathstr[i] == _T('\\'))
                        {
                            pathstr = pathstr.substr(0, i);
                            break;
                        }
                    }

                    // Passing nullptr as the operation invokes the default "verb" for that object (as if the user had
                    // double-clicked on it in explorer).
                    // See Raymond Chen's "The default verb is not necessarily 'open'":
                    // https://devblogs.microsoft.com/oldnewthing/20070430-00/?p=27063
                    // This is important to ensure that the directory is opened in the default file explorer,
                    // if users have defined one other than Windows Explorer
                    // (e.g https://github.com/derceg/explorerplusplus)
                    HINSTANCE exec_result = ShellExecute(m_hWnd,
                                                         nullptr,
                                                         pathstr.c_str(),
                                                         nullptr,
                                                         nullptr,
                                                         SW_SHOWNORMAL);
                    if(exec_result <= HINSTANCE(32))
                    {
                        LOG_WARN("Failed to open lyric file directory: %d", exec_result);
                    }
                }
            }
            break;

            case ID_AUTO_MARK_INSTRUMENTAL:
            {
                if(m_now_playing == nullptr) break;
                metrics::log_used_mark_instrumental();

                std::string msg = "This will delete the lyrics stored locally for the current track ";
                std::string track_str = get_track_friendly_string(m_now_playing_info);
                if(!track_str.empty())
                {
                    msg += "(" + track_str + ") ";
                }
                msg += "and mark the track as instrumental. OpenLyrics will no longer search for lyrics for this track "
                       "automatically so it will not show any lyrics for this track until you explicitly request a "
                       "search for it.\n\nAre you sure you want to proceed?";
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

                LOG_INFO("Marking current track as instrumental from the panel context menu");
                if(!m_lyrics.IsEmpty())
                {
                    io::delete_saved_lyrics(m_now_playing, m_lyrics);
                    m_lyrics = {};
                }
                search_avoidance_force_by_mark_instrumental(m_now_playing, m_now_playing_info);
            }
            break;

            case ID_AUTO_REMOVE_EXTRA_SPACES:
            {
                updated_lyrics = auto_edit::RunAutoEdit(AutoEditType::RemoveRepeatedSpaces,
                                                        m_lyrics,
                                                        m_now_playing_info);
            }
            break;

            case ID_AUTO_REMOVE_EXTRA_BLANK_LINES:
            {
                updated_lyrics = auto_edit::RunAutoEdit(AutoEditType::RemoveRepeatedBlankLines,
                                                        m_lyrics,
                                                        m_now_playing_info);
            }
            break;

            case ID_AUTO_REMOVE_ALL_BLANK_LINES:
            {
                updated_lyrics = auto_edit::RunAutoEdit(AutoEditType::RemoveAllBlankLines,
                                                        m_lyrics,
                                                        m_now_playing_info);
            }
            break;

            case ID_AUTO_REPLACE_XML_CHARS:
            {
                updated_lyrics = auto_edit::RunAutoEdit(AutoEditType::ReplaceHtmlEscapedChars,
                                                        m_lyrics,
                                                        m_now_playing_info);
            }
            break;

            case ID_AUTO_RESET_CAPITALISATION:
            {
                updated_lyrics = auto_edit::RunAutoEdit(AutoEditType::ResetCapitalisation,
                                                        m_lyrics,
                                                        m_now_playing_info);
            }
            break;

            case ID_AUTO_FIX_MALFORMED_TIMESTAMPS:
            {
                updated_lyrics = auto_edit::RunAutoEdit(AutoEditType::FixMalformedTimestamps,
                                                        m_lyrics,
                                                        m_now_playing_info);
            }
            break;

            case ID_AUTO_REMOVE_TIMESTAMPS:
            {
                if(m_now_playing == nullptr) break;

                LOG_INFO("Removing persisted lyrics and re-saving them without timestamps");
                io::delete_saved_lyrics(m_now_playing, m_lyrics);
                updated_lyrics = auto_edit::RunAutoEdit(AutoEditType::RemoveTimestamps, m_lyrics, m_now_playing_info);
            }
            break;

            case ID_AUTO_REMOVE_SURROUNDING_SPACE:
            {
                updated_lyrics = auto_edit::RunAutoEdit(AutoEditType::RemoveSurroundingWhitespace,
                                                        m_lyrics,
                                                        m_now_playing_info);
            }
            break;

            case ID_DELETE_CURRENT_LYRICS:
            {
                if(m_now_playing == nullptr) break;

                std::string msg = "This will delete the lyrics stored locally for the current track ";
                std::string track_str = get_track_friendly_string(m_now_playing_info);
                if(!track_str.empty())
                {
                    msg += "(" + track_str + ")";
                }
                msg += "'\n\nThis operation cannot be undone. Are you sure you want to proceed?";
                popup_message_v3::query_t query = {};
                query.title = "Confirm delete";
                query.msg = msg.c_str();
                query.buttons = popup_message_v3::buttonYes | popup_message_v3::buttonNo;
                query.defButton = popup_message_v3::buttonNo;
                query.icon = popup_message_v3::iconWarning;
                uint32_t popup_result = popup_message_v3::get()->show_query_modal(query);
                if(popup_result != popup_message_v3::buttonYes)
                {
                    break;
                }

                LOG_INFO("Removing current track lyrics from the panel context menu");
                bool deleted = io::delete_saved_lyrics(m_now_playing, m_lyrics);
                if(deleted)
                {
                    m_lyrics = {};
                }
            }
            break;

            case 0: break; // Do nothing, the user clicked away

            default:
            {
                LOG_ERROR("Unrecognised auto-edit ID: %d", cmd);
            }
            break;
        }

        if(updated_lyrics.has_value())
        {
            std::optional<LyricData> maybe_lyrics = io::process_available_lyric_update({
                std::move(updated_lyrics.value()),
                m_now_playing,
                m_now_playing_info,
                LyricUpdate::Type::Edit,
            });
            assert(maybe_lyrics.has_value()); // Round-trip through the processing to avoid copies
            m_lyrics = std::move(maybe_lyrics.value());
        }
    }
    catch(std::exception const& e)
    {
        LOG_ERROR("Failed to create OpenLyrics context menu: %s", e.what());
    }
}

void LyricPanel::OnDoubleClick(UINT /*virtualKeys*/, CPoint /*cursorPos*/)
{
    if(m_now_playing == nullptr) return;

    SpawnLyricEditor(m_lyrics, m_now_playing, m_now_playing_info);
}

LRESULT LyricPanel::OnMouseWheel(UINT /*virtualKeys*/, short rotation, CPoint /*point*/)
{
    // NOTE: WHEEL_DELTA is defined to be 120
    // rotation > 0 (usually 120) means we scrolled up
    // rotation < 0 (usually -120) means we scrolled down
    double scroll_ticks = double(rotation) / double(WHEEL_DELTA);

    RECT fake_client_area = {};
    int one_line_height = ComputeWrappedLyricLineHeight(m_back_buffer, fake_client_area, _T(""));
    m_manual_scroll_distance += int(scroll_ticks * one_line_height);

    Invalidate();

    // We only actually support scrolling on unsynced lyrics
    if(!m_lyrics.IsTimestamped())
    {
        metrics::log_used_manual_scroll();
    }
    return 0;
}

void LyricPanel::OnMouseMove(UINT /*virtualKeys*/, CPoint point)
{
    if(m_manual_scroll_start.has_value())
    {
        const int scroll_delta = point.y - m_manual_scroll_start.value().y;
        m_manual_scroll_distance += scroll_delta;
        m_manual_scroll_start = point;
        Invalidate();

        // We only actually support scrolling on unsynced lyrics
        if(!m_lyrics.IsTimestamped())
        {
            metrics::log_used_manual_scroll();
        }
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

void LyricPanel::StartTimer()
{
    if(m_timerRunning) return;
    m_timerRunning = true;

    UINT_PTR result = SetTimer(m_panel_update_timer, 16, nullptr);
    if(result != m_panel_update_timer)
    {
        LOG_WARN("Unexpected timer result when starting playback timer");
    }
}

void LyricPanel::StopTimer()
{
    if(!m_timerRunning) return;
    m_timerRunning = false;

    KillTimer(m_panel_update_timer);
}

// (Attempt to) Compute the current playback time and duration for the currently-playing track.
// This should be trivial for everything playing from a local file, but for remote files (namely
// internet radio streams), we can't do the naive computation.
// In particular the playback control interface does not appear to return a valid track length
// (admittedly neither does the track info, but I'm leaving it in here in case that changes
// or is true some of the time) and playback position does not get reset when a new track
// starts playing on the radio stream.
LyricPanel::PlaybackTimeInfo LyricPanel::get_playback_time()
{
    service_ptr_t<playback_control> playback = playback_control::get();

    PlaybackTimeInfo result = {};
    result.current_time = playback->playback_get_position() - m_now_playing_time_offset;
    result.track_length = playback->playback_get_length_ex();
    if(result.track_length <= 0.0)
    {
        result.track_length = m_now_playing_info.info->info().get_length();
    }
    if(result.track_length <= 0.0)
    {
        result.track_length = -1.0; // We specifically want to avoid zero because we're likely to divide by this value
    }

    return result;
}

void announce_lyric_update(LyricUpdate lyric_update)
{
    fb2k::inMainThread2(
        [update = std::move(lyric_update)]
        {
            metadb_v2_rec_t track_info =
                update.track_info; // Copy this out so we can move update into process_available_lyric_update
            std::optional<LyricData> maybe_lyrics = io::process_available_lyric_update(std::move(update));
            if(maybe_lyrics.has_value())
            {
                lyric_metadata_log_retrieved(track_info, maybe_lyrics.value());
            }

            if(maybe_lyrics.has_value())
            {
                for(LyricPanel* panel : g_active_panels)
                {
                    assert(panel != nullptr);
                    if(update.track != panel->m_now_playing)
                    {
                        continue;
                    }

                    panel->m_lyrics = maybe_lyrics.value();
                    panel->m_auto_search_avoided_reason = SearchAvoidanceReason::Allowed;
                    ::InvalidateRect(panel->m_hWnd, nullptr, TRUE);
                }
            }
        });
}

void announce_lyric_search_avoided(metadb_handle_ptr track, SearchAvoidanceReason avoid_reason)
{
    fb2k::inMainThread2(
        [track, avoid_reason]
        {
            const uint64_t avoided_timestamp = filetimestamp_from_system_timer();
            for(LyricPanel* panel : g_active_panels)
            {
                assert(panel != nullptr);
                if(panel->m_now_playing != track)
                {
                    continue;
                }

                panel->m_lyrics = {};
                panel->m_auto_search_avoided_reason = avoid_reason;
                panel->m_auto_search_avoided_timestamp = avoided_timestamp;
                ::InvalidateRect(panel->m_hWnd, nullptr, TRUE);
            }
        });
}

size_t num_visible_lyric_panels()
{
    core_api::ensure_main_thread();

    size_t result = 0;
    for(LyricPanel* panel : g_active_panels)
    {
        assert(panel != nullptr);
        RECT r = {};
        const BOOL rect_success = GetWindowRect(panel->m_hWnd, &r);
        const bool rect_nonempty = rect_success && (r.left != r.right) && (r.top != r.bottom);
        const bool window_visible = IsWindowVisible(panel->m_hWnd);
        if(window_visible && rect_nonempty)
        {
            result++;
        }
    }
    return result;
}

void repaint_all_lyric_panels()
{
    fb2k::inMainThread2(
        []()
        {
            core_api::ensure_main_thread();
            for(LyricPanel* panel : g_active_panels)
            {
                assert(panel != nullptr);
                InvalidateRect(panel->m_hWnd, nullptr, TRUE);
            }
        });
}

void recompute_lyric_panel_backgrounds()
{
    fb2k::inMainThread2(
        []()
        {
            core_api::ensure_main_thread();
            for(LyricPanel* panel : g_active_panels)
            {
                assert(panel != nullptr);

                if(preferences::background::image_type() == BackgroundImageType::AlbumArt)
                {
                    now_playing_album_art_notify_manager::ptr art_manager = now_playing_album_art_notify_manager::get();
                    panel->on_album_art_retrieved(art_manager->current());
                }

                if(preferences::background::image_type() == BackgroundImageType::CustomImage)
                {
                    panel->load_custom_background_image();
                }
                panel->compute_background_image();
            }
        });
}
