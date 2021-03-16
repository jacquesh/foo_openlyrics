#include "stdafx.h"

#pragma warning(push, 0)
#include "resource.h"
#include "foobar2000/helpers/atl-misc.h"
#pragma warning(pop)

#include "logging.h"
#include "parsers.h"
#include "sources/localfiles.h"
#include "ui_lyric_editor.h"
#include "winstr_util.h"

class LyricEditor : public CDialogImpl<LyricEditor>, private play_callback_impl_base
{
public:
    // Dialog resource ID
    enum { IDD = IDD_LYRIC_EDIT };

    LyricEditor(const LyricDataRaw& data, metadb_handle_ptr track_handle);
    ~LyricEditor();

    BEGIN_MSG_MAP_EX(LyricEditor)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_CLOSE(OnClose)
        COMMAND_HANDLER_EX(IDC_LYRIC_TEXT, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_LYRIC_EDIT_BACK5, BN_CLICKED, OnBackwardSeek);
        COMMAND_HANDLER_EX(IDC_LYRIC_EDIT_FWD5, BN_CLICKED, OnForwardSeek);
        COMMAND_HANDLER_EX(IDC_LYRIC_EDIT_PLAY, BN_CLICKED, OnPlaybackToggle);
        COMMAND_HANDLER_EX(IDC_LYRIC_EDIT_SYNC, BN_CLICKED, OnLineSync);
        COMMAND_HANDLER_EX(IDC_LYRIC_EDIT_RESET, BN_CLICKED, OnEditReset);
        COMMAND_HANDLER_EX(ID_LYRIC_EDIT_CANCEL, BN_CLICKED, OnCancel)
        COMMAND_HANDLER_EX(ID_LYRIC_EDIT_APPLY, BN_CLICKED, OnApply);
        COMMAND_HANDLER_EX(ID_LYRIC_EDIT_OK, BN_CLICKED, OnOK);
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow parent, LPARAM clientData);
    void OnClose();
    void OnEditChange(UINT, int, CWindow);
    void OnBackwardSeek(UINT btn_id, int notify_code, CWindow btn);
    void OnForwardSeek(UINT btn_id, int notify_code, CWindow btn);
    void OnPlaybackToggle(UINT btn_id, int notify_code, CWindow btn);
    void OnLineSync(UINT btn_id, int notify_code, CWindow btn);
    void OnEditReset(UINT btn_id, int notify_code, CWindow btn);
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
    void SaveLyricEdits();

    metadb_handle_ptr m_track_handle;
    LyricFormat m_lyric_format;
    TCHAR* m_input_text;
    size_t m_input_text_length;
};

static int find_line_start_index_fwd(TCHAR* text, int text_length, int start_index)
{
    assert(start_index >= 0);

    int result = 0;
    for(int index=start_index; index<text_length; index++)
    {
        if(text[index] == _T('\n'))
        {
            result = index + 1;
            break;
        }
    }
    return result;
}

static int find_line_start_index_back(TCHAR* text, int text_length, int start_index)
{
    assert(start_index < text_length);

    int result = 0;
    for(int index=start_index; index>=0; index--)
    {
        if(text[index] == _T('\n'))
        {
            result = index+1;
            break;
        }
    }
    return result;
}

LyricEditor::LyricEditor(const LyricDataRaw& data, metadb_handle_ptr track_handle) :
    m_track_handle(track_handle),
    m_lyric_format(data.format),
    m_input_text(nullptr),
    m_input_text_length(0)
{
    m_input_text_length = string_to_tchar(data.text, m_input_text) - 1; // -1 to not count the null-terminator
}

LyricEditor::~LyricEditor()
{
    if(m_input_text != nullptr)
    {
        delete[] m_input_text;
    }
}

BOOL LyricEditor::OnInitDialog(CWindow /*parent*/, LPARAM /*clientData*/)
{
    if(m_input_text)
    {
        SetDlgItemText(IDC_LYRIC_TEXT, m_input_text);

        int select_start = 0;
        int select_end = find_line_start_index_fwd(m_input_text, m_input_text_length, 0);
        SendDlgItemMessage(IDC_LYRIC_TEXT, EM_SETSEL, select_start, select_end);
    }

    service_ptr_t<playback_control> playback = playback_control::get();
    update_time_text(playback->playback_get_position());
    update_play_button();

    ShowWindow(SW_SHOW);
    return TRUE;
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
    LRESULT lyric_length = SendDlgItemMessage(IDC_LYRIC_TEXT, WM_GETTEXTLENGTH, 0, 0);
    TCHAR* lyric_buffer = new TCHAR[lyric_length+1]; // +1 for the null-terminator
    UINT chars_copied = GetDlgItemText(IDC_LYRIC_TEXT, lyric_buffer, lyric_length+1);
    if(chars_copied != lyric_length)
    {
        LOG_WARN("Dialog character count mismatch while saving. Expected %u, got %u", lyric_length, chars_copied);
    }

    DWORD select_index = 0;
    SendDlgItemMessage(IDC_LYRIC_TEXT, EM_GETSEL, (WPARAM)&select_index, (LPARAM)nullptr);

    LRESULT first_visible_line = SendDlgItemMessage(IDC_LYRIC_TEXT, EM_GETFIRSTVISIBLELINE, 0, 0);

    int line_start_index = find_line_start_index_back(lyric_buffer, chars_copied, select_index);
    int next_line_index = find_line_start_index_fwd(lyric_buffer, chars_copied, select_index);
    int next_next_index = find_line_start_index_fwd(lyric_buffer, chars_copied, next_line_index);

    std::string insert_prefix = tchar_to_string(lyric_buffer, line_start_index);
    std::string insert_suffix = tchar_to_string(lyric_buffer + line_start_index, chars_copied-line_start_index);
    delete[] lyric_buffer;

    service_ptr_t<playback_control> playback = playback_control::get();
    double timestamp = playback->playback_get_position();
    char timestamp_str[11] = {};
    parsers::lrc::print_6digit_timestamp(timestamp, timestamp_str);
    std::string new_text = insert_prefix + timestamp_str + insert_suffix;

    TCHAR* new_buffer = nullptr;
    size_t new_buffer_length = string_to_tchar(new_text, new_buffer);
    SetDlgItemText(IDC_LYRIC_TEXT, new_buffer);
    delete[] new_buffer;

    SendDlgItemMessage(IDC_LYRIC_TEXT, EM_LINESCROLL, 0, first_visible_line);

    // Add 10 to account for the timestamp we just added before these indices
    int new_select_start = next_line_index + 10;
    int new_select_end = next_next_index + 10;
    SendDlgItemMessage(IDC_LYRIC_TEXT, EM_SETSEL, new_select_start, new_select_end);

    m_lyric_format = LyricFormat::Timestamped;

    // We've (almost) certainly changed the contents so enable the "Apply" button
    CWindow apply_btn = GetDlgItem(ID_LYRIC_EDIT_APPLY);
    assert(apply_btn != nullptr);
    apply_btn.EnableWindow(true);
}

void LyricEditor::OnEditReset(UINT /*btn_id*/, int /*notification_type*/, CWindow /*btn*/)
{
    if(m_input_text)
    {
        SetDlgItemText(IDC_LYRIC_TEXT, m_input_text);
    }
    else
    {
        SetDlgItemText(IDC_LYRIC_TEXT, _T(""));
    }
}

void LyricEditor::OnCancel(UINT /*btn_id*/, int /*notification_type*/, CWindow /*btn*/)
{
    DestroyWindow();
}

void LyricEditor::OnApply(UINT /*btn_id*/, int /*notify_code*/, CWindow /*btn*/)
{
    assert(HasContentChanged(nullptr));
    SaveLyricEdits();
}

void LyricEditor::OnOK(UINT /*btn_id*/, int /*notify_code*/, CWindow /*btn*/)
{
    if(HasContentChanged(nullptr))
    {
        SaveLyricEdits();
    }
    DestroyWindow();
}

void LyricEditor::on_playback_new_track(metadb_handle_ptr /*track*/)
{
    update_play_button();
    update_time_text(0);
}

void LyricEditor::on_playback_stop(play_control::t_stop_reason /*reason*/)
{
    update_play_button();
    update_time_text(0);
}

void LyricEditor::on_playback_pause(bool state)
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

    int total_sec = static_cast<int>(new_time);
    int time_min = total_sec / 60;
    int time_sec = total_sec % 60;

    TCHAR buffer[64];
    constexpr size_t buffer_len = sizeof(buffer)/sizeof(buffer[0]);
    _sntprintf_s(buffer, buffer_len, _T("Playback time: %02d:%02d"), time_min, time_sec);

    time_text.SetWindowText(buffer);
}

void LyricEditor::update_play_button()
{
    CWindow btn = GetDlgItem(IDC_LYRIC_EDIT_PLAY);
    if(btn == nullptr) return;

    service_ptr_t<playback_control> playback = playback_control::get();
    const TCHAR* newText = playback->is_playing() && playback->is_paused() ? TEXT("Pause") : TEXT("Play");
    btn.SetWindowText(newText);
}

bool LyricEditor::HasContentChanged(size_t* new_length)
{
    LRESULT lyric_length_result = SendDlgItemMessage(IDC_LYRIC_TEXT, WM_GETTEXTLENGTH, 0, 0);
    assert(lyric_length_result >= 0);

    size_t lyric_length = static_cast<size_t>(lyric_length_result);
    if(new_length)
    {
        *new_length = lyric_length;
    }

    if(lyric_length != m_input_text_length)
    {
        return true;
    }

    if(lyric_length > 0)
    {
        TCHAR* lyric_buffer = new TCHAR[lyric_length+1]; // +1 for the null-terminator
        UINT chars_copied = GetDlgItemText(IDC_LYRIC_TEXT, lyric_buffer, lyric_length+1);
        if(chars_copied != lyric_length)
        {
            LOG_WARN("Dialog character count mismatch. Expected %u, got %u", lyric_length, chars_copied);
        }

        bool changed = (_tcscmp(lyric_buffer, m_input_text) != 0);
        delete[] lyric_buffer;

        return changed;
    }
    else
    {
        return false; // The new length is zero and it matches the old length
    }
}

void LyricEditor::SaveLyricEdits()
{
    LRESULT lyric_length = SendDlgItemMessage(IDC_LYRIC_TEXT, WM_GETTEXTLENGTH, 0, 0);
    if(lyric_length <= 0) return;

    TCHAR* lyric_buffer = new TCHAR[lyric_length+1]; // +1 for the null-terminator
    UINT chars_copied = GetDlgItemText(IDC_LYRIC_TEXT, lyric_buffer, lyric_length+1);
    if(chars_copied != lyric_length)
    {
        LOG_WARN("Dialog character count mismatch while saving. Expected %u, got %u", lyric_length, chars_copied);
    }

    // Update m_input_text (and length) so that HasContentChanged() will return the correct value after the same
    if(m_input_text != nullptr)
    {
        delete[] m_input_text;
    }
    m_input_text = lyric_buffer;
    m_input_text_length = chars_copied;

    // TODO: Should we do this even if we know the format?
    //       What if somebody adds timestamps to existing plaintext lyrics?
    LyricFormat format = m_lyric_format;
    if(format == LyricFormat::Unknown) 
    {
        format = LyricFormat::Plaintext;
        // TODO: Auto-detect format for new lyrics (IE: try parsing as LRC)
    }

    std::string lyrics = tchar_to_string(lyric_buffer, chars_copied);
    if(format == LyricFormat::Timestamped)
    {
        LyricDataRaw data_raw = {{}, format, lyrics};
        LyricData data = parsers::lrc::parse(data_raw);
        lyrics = parsers::lrc::shrink_text(data);
    }

    abort_callback_dummy noAbort;
    sources::localfiles::SaveLyrics(m_track_handle, format, lyrics, noAbort);

    // We know that if we ran HasContentChanged() now, it would return false.
    // So short-circuit it and just disable the apply button directly
    CWindow apply_btn = GetDlgItem(ID_LYRIC_EDIT_APPLY);
    assert(apply_btn != nullptr);
    apply_btn.EnableWindow(FALSE);
}

void SpawnLyricEditor(const LyricDataRaw& edit_data, metadb_handle_ptr lyric_to_edit_track)
{
    // TODO: We should split out LRC lines that include multiple timestamps so that the
    //       editor sees just one timestamp per line (and they're all sorted and nice)
    try
    {
        new CWindowAutoLifetime<ImplementModelessTracking<LyricEditor>>(core_api::get_main_window(), edit_data, lyric_to_edit_track);
    }
    catch(const std::exception& e)
    {
        popup_message::g_complain("Failed to create lyric editor dialog", e);
    }
}

