#include "stdafx.h"

#pragma warning(push, 0)
#include "resource.h"
#include "foobar2000/helpers/atl-misc.h"
#pragma warning(pop)

#include "logging.h"
#include "sources/localfiles.h"
#include "ui_lyric_editor.h"
#include "winstr_util.h"

class LyricEditor : public CDialogImpl<LyricEditor>
{
public:
    // Dialog resource ID
    enum { IDD = IDD_LYRIC_EDIT };

    LyricEditor(const LyricDataRaw& data, const pfc::string8& save_file_title);
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

    pfc::string8 m_save_file_title;
    TCHAR* m_input_text;
    size_t m_input_text_length;
};

LyricEditor::LyricEditor(const LyricDataRaw& data, const pfc::string8& save_file_title) :
    m_save_file_title(save_file_title),
    m_input_text(nullptr),
    m_input_text_length(0)
{
    m_input_text_length = string_to_tchar(data.text, m_input_text) - 1; // -1 to not count the null-terminator
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

    GetDlgItem(ID_LYRIC_EDIT_APPLY).EnableWindow(FALSE);

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
    GetDlgItem(ID_LYRIC_EDIT_APPLY).EnableWindow(changed && !is_empty);
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

#ifdef UNICODE
        bool changed = (wcscmp(lyric_buffer, m_input_text) != 0);
#else
        bool changed = (strcmp(lyric_buffer, m_input_text) != 0);
#endif // UNICODE
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
    // TODO: Disable the OK/Cancel buttons if the lyrics text is empty (and )
    LRESULT lyric_length = SendDlgItemMessage(IDC_LYRIC_TEXT, WM_GETTEXTLENGTH, 0, 0);
    if(lyric_length <= 0) return;

    TCHAR* lyric_buffer = new TCHAR[lyric_length+1]; // +1 for the null-terminator
    UINT chars_copied = GetDlgItemText(IDC_LYRIC_TEXT, lyric_buffer, lyric_length+1);
    if(chars_copied != lyric_length)
    {
        LOG_WARN("Dialog character count mismatch while saving. Expected %u, got %u", lyric_length, chars_copied);
    }
    pfc::string8 lyrics = tchar_to_string(lyric_buffer, chars_copied);
    delete[] lyric_buffer;

        // TODO: Allow the user to configure a search string format maybe?
        /*
        titleformat_object::ptr format_script;
        bool compile_success = titleformat_compiler::get()->compile(format_script, "[%artist% - ][%title%]");
        if (!compile_success)
        {
            LOG_WARN("Failed to compile title format script");
            out_artist.reset();
            out_album.reset();
            out_title.reset();
            return;
        }

        pfc::string8 track_title;
        now_playing->format_title(nullptr, track_title, format_script, nullptr);
        */

    // TODO: Only save if it actually changed since the last save...
    sources::localfiles::SaveLyrics(m_save_file_title, LyricFormat::Plaintext, lyrics);

    // TODO: Should we get passed a handle to the actual lyric window? To send the message just to that? We may as well pass in a reference of some form to the actual panel class then though?
    SendMessage(GetParent(), WM_USER+1, 0, 0);
}

void SpawnLyricEditor(const LyricDataRaw& edit_data, metadb_handle_ptr lyric_to_edit_track)
{
    // TODO: Allow the user to configure a search string format maybe?
    const char* save_format = "[%artist% - ][%title%]";
    titleformat_object::ptr format_script;
    bool compile_success = titleformat_compiler::get()->compile(format_script, save_format);
    if (!compile_success)
    {
        LOG_WARN("Failed to compile save-file title format: %s", save_format);
        return;
    }

    pfc::string8 formatted_save_title;
    lyric_to_edit_track->format_title(nullptr, formatted_save_title, format_script, nullptr);

    try
    {
        new CWindowAutoLifetime<ImplementModelessTracking<LyricEditor>>(core_api::get_main_window(), edit_data, formatted_save_title);
    }
    catch(const std::exception& e)
    {
        popup_message::g_complain("Failed to create lyric editor dialog", e);
    }
}

