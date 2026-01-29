#include "stdafx.h"

#pragma warning(push, 0)
#include "foobar2000/SDK/coreDarkMode.h"
#include "foobar2000/helpers/atl-misc.h"
#include "libPPUI/EditBoxFixes.h"
#include "resource.h"
#pragma warning(pop)

#include "logging.h"
#include "lyric_io.h"
#include "lyric_metadata.h"
#include "metrics.h"
#include "parsers.h"
#include "ui_hooks.h"
#include "win32_util.h"

class LyricEditor : public CDialogImpl<LyricEditor>, private play_callback_impl_base
{
public:
    // Dialog resource ID
    enum
    {
        IDD = IDD_LYRIC_EDIT
    };

    LyricEditor(LyricDataCommon common_data, std::tstring text, metadb_handle_ptr track, metadb_v2_rec_t& track_info);
    ~LyricEditor() override;

    BEGIN_MSG_MAP_EX(LyricEditor)
    MSG_WM_INITDIALOG(OnInitDialog)
    MSG_WM_DESTROY(OnDestroyDialog)
    MSG_WM_CLOSE(OnClose)
    COMMAND_HANDLER_EX(IDC_LYRIC_TEXT, EN_CHANGE, OnEditChange)
    COMMAND_HANDLER_EX(IDC_LYRIC_EDIT_BACK5, BN_CLICKED, OnBackwardSeek)
    COMMAND_HANDLER_EX(IDC_LYRIC_EDIT_FWD5, BN_CLICKED, OnForwardSeek)
    COMMAND_HANDLER_EX(IDC_LYRIC_EDIT_PLAY, BN_CLICKED, OnPlaybackToggle)
    COMMAND_HANDLER_EX(IDC_LYRIC_EDIT_SYNC, BN_CLICKED, OnLineSync)
    COMMAND_HANDLER_EX(IDC_LYRIC_EDIT_RESET, BN_CLICKED, OnEditReset)
    COMMAND_HANDLER_EX(IDC_LYRIC_EDIT_APPLY_OFFSET, BN_CLICKED, OnOffsetApply)
    COMMAND_HANDLER_EX(IDC_LYRIC_EDIT_SYNC_OFFSET, BN_CLICKED, OnOffsetSync)
    COMMAND_HANDLER_EX(ID_LYRIC_EDIT_CANCEL, BN_CLICKED, OnCancel)
    COMMAND_HANDLER_EX(ID_LYRIC_EDIT_APPLY, BN_CLICKED, OnApply)
    COMMAND_HANDLER_EX(ID_LYRIC_EDIT_OK, BN_CLICKED, OnOK)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow parent, LPARAM clientData);
    void OnDestroyDialog();
    void OnClose();
    void OnEditChange(UINT, int, CWindow);
    void OnBackwardSeek(UINT btn_id, int notify_code, CWindow btn);
    void OnForwardSeek(UINT btn_id, int notify_code, CWindow btn);
    void OnPlaybackToggle(UINT btn_id, int notify_code, CWindow btn);
    void OnLineSync(UINT btn_id, int notify_code, CWindow btn);
    void OnEditReset(UINT btn_id, int notify_code, CWindow btn);
    void OnOffsetApply(UINT btn_id, int notify_code, CWindow btn);
    void OnOffsetSync(UINT btn_id, int notify_code, CWindow btn);
    void OnCancel(UINT btn_id, int notify_code, CWindow btn);
    void OnApply(UINT btn_id, int notify_code, CWindow btn);
    void OnOK(UINT btn_id, int notify_code, CWindow btn);

    void on_playback_new_track(metadb_handle_ptr track) override;
    void on_playback_stop(play_control::t_stop_reason reason) override;
    void on_playback_pause(bool state) override;
    void on_playback_seek(double new_time) override;
    void on_playback_time(double new_time) override;

    void update_play_button();
    void update_time_text(double time);
    bool HasContentChanged(size_t* new_length);
    void ApplyLyricEdits();

    void SelectLineWithTimestampGreaterOrEqual(double threshold_timestamp);
    void SetEditorContents(const LyricData& lyrics);
    std::tstring GetEditorContents();
    LyricData ParseEditorContents();

    LyricDataCommon m_common_data;
    std::tstring m_input_text;
    metadb_handle_ptr m_track;
    metadb_v2_rec_t m_track_info;

    HWND m_offset_apply_tooltip;
    HWND m_offset_sync_tooltip;

    fb2k::CCoreDarkModeHooks m_dark;
};

LyricEditor::LyricEditor(LyricDataCommon common_data,
                         std::tstring text,
                         metadb_handle_ptr track,
                         metadb_v2_rec_t& track_info)
    : m_common_data(common_data)
    , m_input_text(text)
    , m_track(track)
    , m_track_info(track_info)
{
}

LyricEditor::~LyricEditor() {}

static HWND create_tooltip(HWND parent)
{
    return CreateWindowEx(WS_EX_TOPMOST,
                          TOOLTIPS_CLASS,
                          nullptr,
                          WS_POPUP | TTS_ALWAYSTIP,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          parent,
                          nullptr,
                          core_api::get_my_instance(),
                          nullptr);
}

static void add_tooltip(HWND control, HWND tooltip, TCHAR* text)
{
    if(tooltip == nullptr)
    {
        LOG_WARN("Attempt to add null tooltip control to the window");
        return;
    }
    if(control == nullptr)
    {
        LOG_WARN("Attempt to add a tooltip to a null window control");
        return;
    }

    TOOLINFO tool_info = {};
    tool_info.cbSize = sizeof(tool_info);
    tool_info.hwnd = GetParent(control);
    tool_info.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    tool_info.uId = (UINT_PTR)control;
    tool_info.lpszText = text;
    SendMessage(tooltip, TTM_ADDTOOL, 0, (LPARAM)&tool_info);
}

BOOL LyricEditor::OnInitDialog(CWindow /*parent*/, LPARAM /*clientData*/)
{
    LOG_INFO("Initializing editor window...");
    metrics::log_used_lyric_editor();
    m_dark.AddDialogWithControls(m_hWnd);

    service_ptr_t<playback_control> playback = playback_control::get();
    update_time_text(playback->playback_get_position());
    update_play_button();
    metadb_handle_ptr now_playing = nullptr;
    playback->get_now_playing(now_playing);

    // NOTE: This fixes some weird text-edit behaviour that I have observed myself (namely Ctrl-Backspace inserting
    //       invalid characters instead of deleting the last word).
    //       I'm also hoping it will fix some weird behaviour that I have *not* observed myself,
    //       namely Ctrl-A not working on Windows 8.1, see https://github.com/jacquesh/foo_openlyrics/issues/190
    PP::editBoxFix(GetDlgItem(IDC_LYRIC_TEXT));

    GotoDlgCtrl(GetDlgItem(IDC_LYRIC_TEXT));

    if(!m_input_text.empty())
    {
        SetDlgItemText(IDC_LYRIC_TEXT, m_input_text.c_str());

        bool editing_now_playing = (now_playing == m_track);
        double playback_time = playback->playback_get_position();
        double select_time = editing_now_playing ? playback_time : 0.0;
        SelectLineWithTimestampGreaterOrEqual(select_time);
    }

    m_offset_apply_tooltip = create_tooltip(m_hWnd);
    m_offset_sync_tooltip = create_tooltip(m_hWnd);
    add_tooltip(
        GetDlgItem(IDC_LYRIC_EDIT_APPLY_OFFSET),
        m_offset_apply_tooltip,
        _T("Remove the existing 'offset' tag and apply the offset directly to every timestamp in these lyrics"));
    add_tooltip(
        GetDlgItem(IDC_LYRIC_EDIT_SYNC_OFFSET),
        m_offset_sync_tooltip,
        _T("Add an 'offset' tag that synchronises all lines instead of modifying the selected line's timestamp"));

    ShowWindow(SW_SHOW);

    // NOTE: Sending EM_SCROLLCARET does nothing if called before ShowWindow()
    SendDlgItemMessage(IDC_LYRIC_TEXT, EM_SCROLLCARET, 0, 0);

    const t_ui_font font = defaultui::default_font();
    if(font != nullptr)
    {
        GetDlgItem(IDC_LYRIC_TEXT).SetFont(font);
    }

    return FALSE; // Tell Windows that we've set the keyboard focus already
}

void LyricEditor::OnDestroyDialog()
{
    ::DestroyWindow(m_offset_apply_tooltip);
    ::DestroyWindow(m_offset_sync_tooltip);
}

void LyricEditor::OnClose()
{
    DestroyWindow();
}

void LyricEditor::OnEditChange(UINT, int, CWindow)
{
    size_t new_length = 0;
    bool changed = HasContentChanged(&new_length);
    bool is_empty = (new_length == 0);

    CWindow apply_btn = GetDlgItem(ID_LYRIC_EDIT_APPLY);
    assert(apply_btn != nullptr);
    apply_btn.EnableWindow(changed && !is_empty);
}

void LyricEditor::OnBackwardSeek(UINT /*btn_id*/, int /*notification_type*/, CWindow /*btn*/)
{
    service_ptr_t<playback_control> playback = playback_control::get();
    playback->playback_seek_delta(-5.0);
}

void LyricEditor::OnForwardSeek(UINT /*btn_id*/, int /*notification_type*/, CWindow /*btn*/)
{
    service_ptr_t<playback_control> playback = playback_control::get();
    playback->playback_seek_delta(5.0);
}

void LyricEditor::OnPlaybackToggle(UINT /*btn_id*/, int /*notification_type*/, CWindow /*btn*/)
{
    service_ptr_t<playback_control> playback = playback_control::get();
    playback->play_or_pause();
}

void LyricEditor::OnLineSync(UINT /*btn_id*/, int /*notification_type*/, CWindow /*btn*/)
{
    LOG_INFO("Synchronising editor line...");
    // NOTE: Passing -1 will give the line index of the line containing the start of the current selection
    LRESULT curr_line_index = SendDlgItemMessage(IDC_LYRIC_TEXT, EM_LINEFROMCHAR, WPARAM(-1), 0);
    LRESULT curr_line_start = SendDlgItemMessage(IDC_LYRIC_TEXT, EM_LINEINDEX, curr_line_index, 0);
    LRESULT curr_line_length = SendDlgItemMessage(IDC_LYRIC_TEXT, EM_LINELENGTH, curr_line_start, 0);

    assert((curr_line_length >= 0) && (curr_line_length <= INT_MAX));
    int replace_start = (int)curr_line_start;
    int replace_end = replace_start;

    const int line_buffer_len = 32;
    TCHAR line_buffer[line_buffer_len + 1] = {}; // +1 for the null-terminator
    line_buffer[0] = line_buffer_len; // EM_GETLINE reads the first word as the number of characters in the buffer
    LRESULT chars_copied = SendDlgItemMessage(IDC_LYRIC_TEXT, EM_GETLINE, curr_line_index, (LPARAM)line_buffer);
    assert(chars_copied <= line_buffer_len + 1);
    if(line_buffer[0] == _T('['))
    {
        for(int i = 0; i < line_buffer_len; i++)
        {
            if(line_buffer[i] == _T('\0')) break;
            if(line_buffer[i] == _T(']'))
            {
                double time = 0;
                std::string tag = from_tstring(
                    std::tstring_view { line_buffer,
                                        (size_t)i + 1 }); // +1 so that the length covers this current (']') character
                bool success = parsers::lrc::try_parse_timestamp(tag, time);
                if(success)
                {
                    replace_end = replace_start + i + 1;
                }
                break;
            }
        }
    }

    LyricData parsed = ParseEditorContents();
    service_ptr_t<playback_control> playback = playback_control::get();
    double timestamp = playback->playback_get_position() + parsed.timestamp_offset;
    std::string timestamp_str = parsers::lrc::print_timestamp(timestamp);
    std::tstring timestamp_tstr = to_tstring(timestamp_str);

    SendDlgItemMessage(IDC_LYRIC_TEXT, EM_SETSEL, replace_start, replace_end);
    SendDlgItemMessage(IDC_LYRIC_TEXT, EM_REPLACESEL, TRUE, (LPARAM)timestamp_tstr.c_str());

    LRESULT next_line_start = SendDlgItemMessage(IDC_LYRIC_TEXT, EM_LINEINDEX, curr_line_index + 1, 0);
    LRESULT next_line_length = SendDlgItemMessage(IDC_LYRIC_TEXT, EM_LINELENGTH, next_line_start, 0);
    LRESULT next_line_end = next_line_start + next_line_length;

    SendDlgItemMessage(IDC_LYRIC_TEXT, EM_SETSEL, next_line_start, next_line_end);
    SendDlgItemMessage(IDC_LYRIC_TEXT, EM_SCROLLCARET, 0, 0); // Scroll the selected line into view

    // We've (almost) certainly changed the contents so enable the "Apply" button
    CWindow apply_btn = GetDlgItem(ID_LYRIC_EDIT_APPLY);
    assert(apply_btn != nullptr);
    apply_btn.EnableWindow(true);
}

void LyricEditor::OnEditReset(UINT /*btn_id*/, int /*notification_type*/, CWindow /*btn*/)
{
    SetDlgItemText(IDC_LYRIC_TEXT, m_input_text.c_str());
}

void LyricEditor::SelectLineWithTimestampGreaterOrEqual(double threshold_timestamp)
{
    int select_start = 0;
    int select_end = 0;

    TCHAR* line_buffer = nullptr;
    int line_buffer_len = 0;
    int line_count = (int)SendDlgItemMessage(IDC_LYRIC_TEXT, EM_GETLINECOUNT, 0, 0);
    for(int i = 0; i < line_count; i++)
    {
        const LRESULT line_start_result = SendDlgItemMessage(IDC_LYRIC_TEXT, EM_LINEINDEX, i, 0);
        const LRESULT line_length_result = SendDlgItemMessage(IDC_LYRIC_TEXT, EM_LINELENGTH, line_start_result, 0);
        assert(line_start_result <= INT_MAX);
        assert(line_length_result <= INT_MAX);
        const int line_start = int(line_start_result);
        const int line_length = int(line_length_result);
        if(line_length <= 0) continue;

        if(line_buffer_len < line_length)
        {
            line_buffer = (TCHAR*)realloc(line_buffer, (line_length + 1) * sizeof(TCHAR));
            line_buffer_len = line_length;
        }

        line_buffer[0] = TCHAR(
            line_buffer_len); // EM_GETLINE reads the first word as the number of characters in the buffer
        LRESULT chars_copied = SendDlgItemMessage(IDC_LYRIC_TEXT, EM_GETLINE, i, (LPARAM)line_buffer);
        std::string linestr = from_tstring(std::tstring_view { line_buffer, (size_t)chars_copied });
        if(linestr.empty() || ((linestr.length() == 1) && is_char_whitespace(linestr[0]))) continue;
        if(parsers::lrc::is_tag_line(linestr)) continue;

        double line_timestamp = parsers::lrc::get_line_first_timestamp(linestr);
        if(line_timestamp == DBL_MAX)
        {
            // NOTE: For unsynced lyrics we just select the first non-empty line
            select_start = line_start;
            select_end = line_start + line_length;
            break;
        }
        else
        {
            // NOTE: We want to set the selection at least once, so that we don't open
            //       with no selection before the first timestamp has been reached
            bool selection_set = ((select_start != 0) || (select_end != 0));
            if(selection_set && (line_timestamp > threshold_timestamp))
            {
                break;
            }
            select_start = line_start;
            select_end = line_start + line_length;
        }
    }
    free(line_buffer);

    SendDlgItemMessage(IDC_LYRIC_TEXT, EM_SETSEL, select_start, select_end);
    SendDlgItemMessage(IDC_LYRIC_TEXT, EM_SCROLLCARET, 0, 0);
}

void LyricEditor::SetEditorContents(const LyricData& lyrics)
{
    std::tstring new_contents = parsers::lrc::expand_text(lyrics, false);
    SetDlgItemText(IDC_LYRIC_TEXT, new_contents.c_str());
    SendDlgItemMessage(IDC_LYRIC_TEXT, EM_SCROLLCARET, 0, 0);
}

void LyricEditor::OnOffsetApply(UINT /*btn_id*/, int /*notify_code*/, CWindow /*btn*/)
{
    LyricData parsed = ParseEditorContents();
    if(!parsed.IsTimestamped())
    {
        popup_message::g_show("Cannot apply offset tag to unsynchronised lyrics",
                              "Synchronisation Error",
                              popup_message::icon_error);
        return;
    }
    if(parsed.timestamp_offset == 0.0)
    {
        popup_message::g_show("Cannot apply offset tag as there is no offset to apply",
                              "Synchronisation Error",
                              popup_message::icon_error);
        return;
    }

    for(size_t i = 0; i < parsed.lines.size(); i++)
    {
        parsed.lines[i].timestamp = parsed.LineTimestamp(i);
    }
    parsers::lrc::remove_offset_tag(parsed);

    SetEditorContents(parsed);
    GetDlgItem(ID_LYRIC_EDIT_APPLY).EnableWindow(true);
}

void LyricEditor::OnOffsetSync(UINT /*btn_id*/, int /*notify_code*/, CWindow /*btn*/)
{
    LyricData parsed = ParseEditorContents();

    LOG_INFO("Synchronising editor line...");
    // NOTE: Passing -1 will give the line index of the line containing the start of the current selection
    LRESULT curr_line_index = SendDlgItemMessage(IDC_LYRIC_TEXT, EM_LINEFROMCHAR, WPARAM(-1), 0);
    LRESULT curr_line_start = SendDlgItemMessage(IDC_LYRIC_TEXT, EM_LINEINDEX, curr_line_index, 0);
    LRESULT curr_line_length = SendDlgItemMessage(IDC_LYRIC_TEXT, EM_LINELENGTH, curr_line_start, 0);
    assert((curr_line_length >= 0) && (curr_line_length <= INT_MAX));

    const int line_buffer_len = 32;
    TCHAR line_buffer[line_buffer_len + 1] = {}; // +1 for the null-terminator
    line_buffer[0] = line_buffer_len; // EM_GETLINE reads the first word as the number of characters in the buffer
    LRESULT chars_copied = SendDlgItemMessage(IDC_LYRIC_TEXT, EM_GETLINE, curr_line_index, (LPARAM)line_buffer);
    assert(chars_copied <= line_buffer_len + 1);
    double curr_line_timestamp = parsers::lrc::get_line_first_timestamp(
        from_tstring(std::tstring_view { line_buffer }));

    if(curr_line_timestamp == DBL_MAX)
    {
        popup_message::g_show("The currently-selected line does not have a timestamp",
                              "Synchronisation Error",
                              popup_message::icon_error);
    }
    else
    {
        service_ptr_t<playback_control> playback = playback_control::get();
        double current_timestamp = playback->playback_get_position();
        double required_offset_sec = curr_line_timestamp - current_timestamp;
        parsers::lrc::set_offset_tag(parsed, required_offset_sec);

        SetEditorContents(parsed);
        SelectLineWithTimestampGreaterOrEqual(curr_line_timestamp);
        GetDlgItem(ID_LYRIC_EDIT_APPLY).EnableWindow(true);
    }
}

void LyricEditor::OnCancel(UINT /*btn_id*/, int /*notification_type*/, CWindow /*btn*/)
{
    DestroyWindow();
}

void LyricEditor::OnApply(UINT /*btn_id*/, int /*notify_code*/, CWindow /*btn*/)
{
    assert(HasContentChanged(nullptr));
    ApplyLyricEdits();
}

void LyricEditor::OnOK(UINT /*btn_id*/, int /*notify_code*/, CWindow /*btn*/)
{
    if(HasContentChanged(nullptr))
    {
        ApplyLyricEdits();
    }
    DestroyWindow();
}

void LyricEditor::on_playback_new_track(metadb_handle_ptr track)
{
    bool edit_track_playing = (track == m_track);
    ::EnableWindow(GetDlgItem(IDC_LYRIC_EDIT_SYNC), edit_track_playing);
    ::EnableWindow(GetDlgItem(IDC_LYRIC_EDIT_BACK5), edit_track_playing);
    ::EnableWindow(GetDlgItem(IDC_LYRIC_EDIT_FWD5), edit_track_playing);
    ::EnableWindow(GetDlgItem(IDC_LYRIC_EDIT_PLAY), edit_track_playing);

    update_play_button();
    update_time_text(0);
}

void LyricEditor::on_playback_stop(play_control::t_stop_reason /*reason*/)
{
    ::EnableWindow(GetDlgItem(IDC_LYRIC_EDIT_SYNC), FALSE);
    ::EnableWindow(GetDlgItem(IDC_LYRIC_EDIT_BACK5), FALSE);
    ::EnableWindow(GetDlgItem(IDC_LYRIC_EDIT_FWD5), FALSE);
    ::EnableWindow(GetDlgItem(IDC_LYRIC_EDIT_PLAY), FALSE);

    update_play_button();
    update_time_text(0);
}

void LyricEditor::on_playback_pause(bool /*state*/)
{
    update_play_button();
}

void LyricEditor::on_playback_seek(double new_time)
{
    update_time_text(new_time);
}

void LyricEditor::on_playback_time(double new_time)
{
    update_time_text(new_time);
}

void LyricEditor::update_time_text(double new_time)
{
    // NOTE: The time text doesn't exist when this class first gets created, so we should handle
    //       that possibility in general (because we could create it just before a seek/time event).
    CWindow time_text = GetDlgItem(IDC_LYRIC_EDIT_TIME);
    if(time_text == nullptr) return;

    bool edit_track_playing = false;
    metadb_handle_ptr now_playing;
    playback_control::ptr playback = playback_control::get();
    if(playback->get_now_playing(now_playing))
    {
        edit_track_playing = (now_playing == m_track);
    }

    if(edit_track_playing)
    {
        int total_sec = static_cast<int>(new_time);
        int time_min = total_sec / 60;
        int time_sec = total_sec % 60;

        TCHAR buffer[64];
        constexpr size_t buffer_len = sizeof(buffer) / sizeof(buffer[0]);
        _sntprintf_s(buffer, buffer_len, _T("Playback time: %02d:%02d"), time_min, time_sec);

        time_text.SetWindowText(buffer);
    }
    else
    {
        time_text.SetWindowText(_T("This track is not playing..."));
    }
}

void LyricEditor::update_play_button()
{
    CWindow btn = GetDlgItem(IDC_LYRIC_EDIT_PLAY);
    if(btn == nullptr) return;

    service_ptr_t<playback_control> playback = playback_control::get();
    const TCHAR* newText = playback->is_playing() && !playback->is_paused() ? TEXT("Pause") : TEXT("Play");
    btn.SetWindowText(newText);
}

bool LyricEditor::HasContentChanged(size_t* new_length)
{
    const LRESULT lyric_length_result = SendDlgItemMessage(IDC_LYRIC_TEXT, WM_GETTEXTLENGTH, 0, 0);
    assert(lyric_length_result >= 0);
    assert(lyric_length_result <= INT_MAX);

    const size_t lyric_length = static_cast<size_t>(lyric_length_result);
    if(new_length)
    {
        *new_length = lyric_length;
    }

    if(lyric_length != m_input_text.size())
    {
        return true;
    }

    if(lyric_length > 0)
    {
        TCHAR* lyric_buffer = new TCHAR[lyric_length + 1]; // +1 for the null-terminator
        UINT chars_copied = GetDlgItemText(IDC_LYRIC_TEXT, lyric_buffer, int(lyric_length) + 1);
        if(chars_copied != lyric_length)
        {
            LOG_WARN("Dialog character count mismatch. Expected %ju, got %u", lyric_length, chars_copied);
        }

        bool changed = (_tcscmp(lyric_buffer, m_input_text.c_str()) != 0);
        delete[] lyric_buffer;

        return changed;
    }
    else
    {
        return false; // The new length is zero and it matches the old length
    }
}

void LyricEditor::ApplyLyricEdits()
{
    LOG_INFO("Saving lyrics from editor...");
    LyricData data = ParseEditorContents();
    if(data.IsEmpty())
    {
        return;
    }

    announce_lyric_update({
        std::move(data),
        m_track,
        m_track_info,
        LyricUpdate::Type::Edit,
    });
    lyric_metadata_log_edit(m_track_info);

    // Update m_input_text so that HasContentChanged() will return the correct value after the same
    m_input_text = GetEditorContents();

    // We know that if we ran HasContentChanged() now, it would return false.
    // So short-circuit it and just disable the apply button directly
    CWindow apply_btn = GetDlgItem(ID_LYRIC_EDIT_APPLY);
    apply_btn.EnableWindow(FALSE);
}

std::tstring LyricEditor::GetEditorContents()
{
    LRESULT lyric_length = SendDlgItemMessage(IDC_LYRIC_TEXT, WM_GETTEXTLENGTH, 0, 0);
    if(lyric_length <= 0)
    {
        return _T("");
    }
    assert(lyric_length <= INT_MAX);

    TCHAR* lyric_buffer = new TCHAR[lyric_length + 1]; // +1 for the null-terminator
    UINT chars_copied = GetDlgItemText(IDC_LYRIC_TEXT, lyric_buffer, int(lyric_length) + 1);
    if(chars_copied != UINT(lyric_length))
    {
        LOG_WARN("Dialog character count mismatch while saving. Expected %u, got %u", lyric_length, chars_copied);
    }

    std::tstring result = std::tstring { lyric_buffer, chars_copied };
    delete[] lyric_buffer;

    return result;
}

LyricData LyricEditor::ParseEditorContents()
{
    std::string lyrics = from_tstring(GetEditorContents());
    return parsers::lrc::parse(m_common_data, lyrics);
}

HWND SpawnLyricEditor(const LyricData& lyrics, metadb_handle_ptr track, metadb_v2_rec_t track_info)
{
    LOG_INFO("Spawning editor window...");
    core_api::ensure_main_thread();
    HWND result = nullptr;
    try
    {
        const std::tstring text = parsers::lrc::expand_text(lyrics, false);
        auto new_window = fb2k::newDialog<LyricEditor>(lyrics, text, track, track_info);
        result = new_window->m_hWnd;
    }
    catch(const std::exception& e)
    {
        popup_message::g_complain("Failed to create lyric editor dialog", e);
    }
    return result;
}
