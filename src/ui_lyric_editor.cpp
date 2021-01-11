#include "stdafx.h"

#pragma warning(push, 0)
#include "resource.h"
#include "foobar2000/helpers/atl-misc.h"
#pragma warning(pop)

#include "sources/localfiles.h"
#include "ui_lyric_editor.h"
#include "winstr_util.h"

class LyricEditor : public CDialogImpl<LyricEditor>
{
public:
    // Dialog resource ID
    enum { IDD = IDD_LYRIC_EDIT };

    LyricEditor(const LyricEditData& data);
    ~LyricEditor();

    BEGIN_MSG_MAP_EX(LyricEditor)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_CLOSE(OnClose)
        COMMAND_HANDLER_EX(ID_LYRIC_EDIT_CANCEL, BN_CLICKED, OnCancel)
        COMMAND_HANDLER_EX(ID_LYRIC_EDIT_APPLY, BN_CLICKED, OnApply);
        COMMAND_HANDLER_EX(ID_LYRIC_EDIT_OK, BN_CLICKED, OnOK);
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow parent, LPARAM clientData);
    void OnClose();
    void OnCancel(UINT btn_id, int notify_code, CWindow btn);
    void OnApply(UINT btn_id, int notify_code, CWindow btn);
    void OnOK(UINT btn_id, int notify_code, CWindow btn);

    void SaveLyricEdits();

    pfc::string8 m_save_file_title;
    TCHAR* m_input_text;
};

LyricEditor::LyricEditor(const LyricEditData& data) :
    m_save_file_title(data.file_title),
    m_input_text(nullptr)
{
    string_to_tchar(data.text, m_input_text);
}

LyricEditor::~LyricEditor()
{
    if(m_input_text)
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

void LyricEditor::OnCancel(UINT /*btn_id*/, int /*notification_type*/, CWindow /*btn*/)
{
    DestroyWindow();
}

void LyricEditor::OnApply(UINT /*btn_id*/, int /*notify_code*/, CWindow /*btn*/)
{
    SaveLyricEdits();
}

void LyricEditor::OnOK(UINT /*btn_id*/, int /*notify_code*/, CWindow /*btn*/)
{
    SaveLyricEdits();
    DestroyWindow();
}

void LyricEditor::SaveLyricEdits()
{
    // TODO: Disable the OK/Cancel buttons if the lyrics text is empty (and )
    LRESULT lyric_length = SendDlgItemMessage(IDC_LYRIC_TEXT, WM_GETTEXTLENGTH, 0, 0);
    if(lyric_length <= 0) return;

    TCHAR* lyric_buffer = new TCHAR[lyric_length+1]; // +1 for the null-terminator
    UINT chars_copied = GetDlgItemText(IDC_LYRIC_TEXT, lyric_buffer, lyric_length+1);
    if(chars_copied != lyric_length)
    {
        console::printf("WARN-OpenLyrics: Dialog character count mismatch. Expeccted %u, got %u", lyric_length, chars_copied);
    }
    pfc::string8 lyrics = tchar_to_string(lyric_buffer, chars_copied);
    delete[] lyric_buffer;

    sources::localfiles::SaveLyrics(m_save_file_title, LyricFormat::Plaintext, lyrics);

    // TODO: Should we get passed a handle to the actual lyric window? To send the message just to that? We may as well pass in a reference of some form to the actual panel class then though?
    SendMessage(GetParent(), WM_USER+1, 0, 0);
}

void SpawnLyricEditor(const LyricEditData& edit_data)
{
    try
    {
        new CWindowAutoLifetime<ImplementModelessTracking<LyricEditor>>(core_api::get_main_window(), edit_data);
    }
    catch(const std::exception& e)
    {
        popup_message::g_complain("Failed to create lyric editor dialog", e);
    }
}

