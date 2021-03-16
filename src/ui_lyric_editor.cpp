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

class LyricEditor : public CDialogImpl<LyricEditor>
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
        COMMAND_HANDLER_EX(ID_LYRIC_EDIT_CANCEL, BN_CLICKED, OnCancel)
        COMMAND_HANDLER_EX(ID_LYRIC_EDIT_APPLY, BN_CLICKED, OnApply);
        COMMAND_HANDLER_EX(ID_LYRIC_EDIT_OK, BN_CLICKED, OnOK);
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow parent, LPARAM clientData);
    void OnClose();
    void OnEditChange(UINT, int, CWindow);
    void OnCancel(UINT btn_id, int notify_code, CWindow btn);
    void OnApply(UINT btn_id, int notify_code, CWindow btn);
    void OnOK(UINT btn_id, int notify_code, CWindow btn);

    bool HasContentChanged(size_t* new_length);
    void SaveLyricEdits();

    metadb_handle_ptr m_track_handle;
    LyricFormat m_lyric_format;
    TCHAR* m_input_text;
    size_t m_input_text_length;
};

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
    }

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

