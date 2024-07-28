#include "stdafx.h"

#pragma warning(push, 0)
#include "resource.h"
#include "foobar2000/helpers/atl-misc.h"
#include "foobar2000/SDK/coreDarkMode.h"
#pragma warning(pop)

#include "config/config_auto.h"
#include "preferences.h"
#include "ui_util.h"
#include "win32_util.h"

extern const GUID GUID_PREFERENCES_PAGE_SEARCH = { 0xf835ba65, 0x9a56, 0x4c0f, { 0xb1, 0x23, 0x8, 0x53, 0x67, 0x97, 0x4e, 0xed } };

static const GUID GUID_CFG_SEARCH_EXCLUDE_TRAILING_BRACKETS = { 0x2cbdf6c3, 0xdb8c, 0x43d4, { 0xb5, 0x40, 0x76, 0xc0, 0x4a, 0x39, 0xa7, 0xc7 } };
static const GUID GUID_CFG_SEARCH_SKIP_FILTER = { 0x4c6e3dac, 0xb668, 0x4056, { 0x8c, 0xb7, 0x52, 0x89, 0x1a, 0x57, 0x1f, 0x3a } };

static cfg_auto_bool       cfg_search_exclude_trailing_brackets(GUID_CFG_SEARCH_EXCLUDE_TRAILING_BRACKETS, IDC_SEARCH_EXCLUDE_BRACKETS, true);
static cfg_auto_string     cfg_search_skip_filter(GUID_CFG_SEARCH_SKIP_FILTER, IDC_SEARCH_SKIP_FILTER_STR, "$if($strstr($lower(%genre%),instrumental),skip,)$if($strstr($lower(%genre%),classical),skip,)");

static cfg_auto_property* g_searching_auto_properties[] =
{
    &cfg_search_exclude_trailing_brackets,
    &cfg_search_skip_filter,
};


bool preferences::searching::exclude_trailing_brackets()
{
    return cfg_search_exclude_trailing_brackets.get_value();
}

const pfc::string8& preferences::searching::skip_filter()
{
    return cfg_search_skip_filter.get();
}

LyricType preferences::searching::preferred_lyric_type()
{
    return LyricType::Synced;
}

class PreferencesSearching : public CDialogImpl<PreferencesSearching>, public auto_preferences_page_instance, private play_callback_impl_base
{
public:
    // Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
    PreferencesSearching(preferences_page_callback::ptr callback) : auto_preferences_page_instance(callback, g_searching_auto_properties) {}

    // Dialog resource ID - Required by WTL/Create()
    enum {IDD = IDD_PREFERENCES_SEARCHING};

    BEGIN_MSG_MAP_EX(PreferencesSearching)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_HANDLER_EX(IDC_SEARCH_EXCLUDE_BRACKETS, BN_CLICKED, OnUIChange)
        COMMAND_HANDLER_EX(IDC_SEARCH_SKIP_FILTER_STR, EN_CHANGE, OnSkipFilterFormatChange)
        NOTIFY_HANDLER_EX(IDC_SEARCH_SYNTAX_HELP, NM_CLICK, OnSyntaxHelpClicked)
    END_MSG_MAP()

private:
    void on_playback_new_track(metadb_handle_ptr track) override;

    BOOL OnInitDialog(CWindow, LPARAM);
    void OnUIChange(UINT, int, CWindow);
    void OnSkipFilterFormatChange(UINT, int, CWindow);
    LRESULT OnSyntaxHelpClicked(NMHDR*);

    void UpdateSkipFilterPreview();

    fb2k::CCoreDarkModeHooks m_dark;
};

void PreferencesSearching::on_playback_new_track(metadb_handle_ptr /*track*/)
{
    UpdateSkipFilterPreview();
}

BOOL PreferencesSearching::OnInitDialog(CWindow, LPARAM)
{
    m_dark.AddDialogWithControls(m_hWnd);

    UpdateSkipFilterPreview();
    init_auto_preferences();

    return FALSE;
}

void PreferencesSearching::OnUIChange(UINT, int, CWindow)
{
    on_ui_interaction();
}

void PreferencesSearching::UpdateSkipFilterPreview()
{
    const TCHAR* search_allowed_text = _T("Search is allowed");
    const TCHAR* search_skipped_text = _T("Output is not empty, search will be skipped");

    CWindow output_edit = GetDlgItem(IDC_SEARCH_SKIP_FILTER_OUTPUT);
    CWindow result_text = GetDlgItem(IDC_SEARCH_SKIP_FILTER_RESULT);
    assert(output_edit != nullptr);
    assert(result_text != nullptr);

    LRESULT format_text_length_result = SendDlgItemMessage(IDC_SEARCH_SKIP_FILTER_STR, WM_GETTEXTLENGTH, 0, 0);
    if(format_text_length_result <= 0)
    {
        output_edit.SetWindowText(_T(""));
        result_text.SetWindowText(search_allowed_text);
        return;
    }

    const size_t format_text_length = (size_t)format_text_length_result;
    TCHAR* format_text_buffer = new TCHAR[format_text_length+1]; // +1 for null-terminator
    GetDlgItemText(IDC_SEARCH_SKIP_FILTER_STR, format_text_buffer, int(format_text_length+1));
    const std::string format_text = from_tstring(std::tstring_view{format_text_buffer, format_text_length});
    delete[] format_text_buffer;

    titleformat_object::ptr format_script;
    const bool compile_success = titleformat_compiler::get()->compile(format_script, format_text.c_str());
    if(!compile_success)
    {
        output_edit.SetWindowText(_T("<Invalid format>"));
        result_text.SetWindowText(search_allowed_text);
        return;
    }

    const metadb_handle_ptr preview_track = get_format_preview_track();
    if(preview_track == nullptr)
    {
        output_edit.SetWindowText(_T("<No track selected or playing to preview>"));
        result_text.SetWindowText(_T(""));
        return;
    }

    pfc::string8 formatted;
    bool format_success = preview_track->format_title(nullptr, formatted, format_script, nullptr);
    if(!format_success)
    {
        output_edit.SetWindowText(_T("<Error while applying format>"));
        result_text.SetWindowText(_T(""));
    }

    std::tstring preview = to_tstring(formatted);
    output_edit.SetWindowText(preview.c_str());
    result_text.SetWindowText(preview.empty() ? search_allowed_text : search_skipped_text);
}

void PreferencesSearching::OnSkipFilterFormatChange(UINT, int, CWindow)
{
    on_ui_interaction();
    UpdateSkipFilterPreview();
}

LRESULT PreferencesSearching::OnSyntaxHelpClicked(NMHDR* /*notify_msg*/)
{
    standard_commands::main_titleformat_help();
    return 0;
}

class PreferencesSearchImpl : public preferences_page_impl<PreferencesSearching>
{
public:
    const char* get_name() final { return "Searching"; }
    GUID get_guid() final { return GUID_PREFERENCES_PAGE_SEARCH; }
    GUID get_parent_guid() final { return GUID_PREFERENCES_PAGE_ROOT; }
};
static preferences_page_factory_t<PreferencesSearchImpl> g_preferences_page_search_factory;


static void fix_skip_filter_default_on_init()
{
    // When the customisable skip filter was introduced in v1.7, it had the default given below.
    // This default is broken and will always produce an empty string (note the extra closing
    // parenthesis after `instrumental`). This seems like a bug in fb2k because I would have
    // expected the extra closing parenthesis to show up in the output, but apparently not.
    // Since defaults are persisted on first launch, now every user that launched fb2k with
    // openlyrics v1.7 will have this broken default.
    // Unless they fix it or remove the bad string entirely, the skip filter will never work
    // for them.
    // To work around this we check for it on startup and quietly migrate to the new default.
    // This could technically be a bad experience if a user in future specifically sets their
    // filter to the exact broken string we used to have as default and is then surprised when
    // it changes, but that seems terribly unlikely. And their filter wouldn't have worked anyway.
    //
    // If it helps, we can probably remove this conversion sometime in the future after a few
    // releases once there's been time for most users' default filters to have been fixed.
    //
    const std::string_view old_broken_default_filter = "$stricmp(%genre%,instrumental))$stricmp(%genre%,classical)";
    const std::string_view new_fixed_default_filter = cfg_search_skip_filter.get_default();
    const std::string_view current_filter = cfg_search_skip_filter.get_stringview();

    if(current_filter == old_broken_default_filter)
    {
        cfg_search_skip_filter.set_string_nc(new_fixed_default_filter.data(), new_fixed_default_filter.length());
    }
}
FB2K_RUN_ON_INIT(fix_skip_filter_default_on_init)
