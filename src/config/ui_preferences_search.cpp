#include "stdafx.h"

#pragma warning(push, 0)
#include "resource.h"
#include "foobar2000/helpers/atl-misc.h"
#include "foobar2000/SDK/coreDarkMode.h"
#pragma warning(pop)

#include "config/config_auto.h"
#include "logging.h"
#include "preferences.h"
#include "sources/lyric_source.h"
#include "ui_util.h"
#include "win32_util.h"

extern const GUID GUID_PREFERENCES_PAGE_SEARCH = { 0x73e2261d, 0x4a71, 0x427a, { 0x92, 0x57, 0xec, 0xaa, 0x17, 0xb9, 0xa8, 0xc8 } };

static const GUID GUID_CFG_SEARCH_ACTIVE_SOURCES_GENERATION = { 0x9046aa4a, 0x352e, 0x4467, { 0xbc, 0xd2, 0xc4, 0x19, 0x47, 0xd2, 0xbf, 0x24 } };
static const GUID GUID_CFG_SEARCH_ACTIVE_SOURCES = { 0x7d3c9b2c, 0xb87b, 0x4250, { 0x99, 0x56, 0x8d, 0xf5, 0x80, 0xc9, 0x2f, 0x39 } };
static const GUID GUID_CFG_SEARCH_EXCLUDE_TRAILING_BRACKETS = { 0x2cbdf6c3, 0xdb8c, 0x43d4, { 0xb5, 0x40, 0x76, 0xc0, 0x4a, 0x39, 0xa7, 0xc7 } };
static const GUID GUID_CFG_SEARCH_SKIP_FILTER = { 0x4c6e3dac, 0xb668, 0x4056, { 0x8c, 0xb7, 0x52, 0x89, 0x1a, 0x57, 0x1f, 0x3a } };

// NOTE: These were copied from the relevant lyric-source source file.
//       It should not be a problem because these GUIDs must never change anyway (since it would
//       break everybody's config), but probably worth noting that the information is duplicated.
static const GUID localfiles_src_guid = { 0x76d90970, 0x1c98, 0x4fe2, { 0x94, 0x4e, 0xac, 0xe4, 0x93, 0xf3, 0x8e, 0x85 } };
static const GUID metadata_tags_src_guid = { 0x3fb0f715, 0xa097, 0x493a, { 0x94, 0x4e, 0xdb, 0x48, 0x66, 0x8, 0x86, 0x78 } };
static const GUID musixmatch_src_guid = { 0xf94ba31a, 0x7b33, 0x49e4, { 0x81, 0x9b, 0x0, 0xc, 0x36, 0x44, 0x29, 0xcd } };
static const GUID netease_src_guid = { 0xaac13215, 0xe32e, 0x4667, { 0xac, 0xd7, 0x1f, 0xd, 0xbd, 0x84, 0x27, 0xe4 } };
static const GUID qqmusic_src_guid = { 0x4b0b5722, 0x3a84, 0x4b8e, { 0x82, 0x7a, 0x26, 0xb9, 0xea, 0xb3, 0xb4, 0xe8 } };

static const GUID cfg_search_active_sources_default[] = {localfiles_src_guid, metadata_tags_src_guid, qqmusic_src_guid, netease_src_guid};

static cfg_int_t<uint64_t> cfg_search_active_sources_generation(GUID_CFG_SEARCH_ACTIVE_SOURCES_GENERATION, 0);
static cfg_objList<GUID>   cfg_search_active_sources(GUID_CFG_SEARCH_ACTIVE_SOURCES, cfg_search_active_sources_default);
static cfg_auto_bool       cfg_search_exclude_trailing_brackets(GUID_CFG_SEARCH_EXCLUDE_TRAILING_BRACKETS, IDC_SEARCH_EXCLUDE_BRACKETS, true);
static cfg_auto_string     cfg_search_skip_filter(GUID_CFG_SEARCH_SKIP_FILTER, IDC_SEARCH_SKIP_FILTER_STR, "$if($strstr($lower(%genre%),instrumental),skip,)$if($strstr($lower(%genre%),classical),skip,)");

static cfg_auto_property* g_searching_auto_properties[] =
{
    &cfg_search_exclude_trailing_brackets,
    &cfg_search_skip_filter,
};

uint64_t preferences::searching::source_config_generation()
{
    return cfg_search_active_sources_generation.get_value();
}

std::vector<GUID> preferences::searching::active_sources()
{
    GUID save_source_guid = preferences::saving::save_source();
    bool save_source_seen = false;

    const size_t source_count = cfg_search_active_sources.get_size();
    std::vector<GUID> result;
    result.reserve(source_count+1);
    for(size_t i=0; i<source_count; i++)
    {
        save_source_seen |= (save_source_guid == cfg_search_active_sources[i]);
        result.push_back(cfg_search_active_sources[i]);
    }

    if(!save_source_seen && (save_source_guid != GUID{}))
    {
        result.push_back(save_source_guid);
    }

    return result;
}

bool preferences::searching::exclude_trailing_brackets()
{
    return cfg_search_exclude_trailing_brackets.get_value();
}

std::vector<GUID> preferences::searching::raw::active_sources_configured()
{
    const size_t source_count = cfg_search_active_sources.get_size();
    std::vector<GUID> result;
    result.reserve(source_count+1);
    for(size_t i=0; i<source_count; i++)
    {
        result.push_back(cfg_search_active_sources[i]);
    }

    return result;
}

const pfc::string8& preferences::searching::skip_filter()
{
    return cfg_search_skip_filter.get();
}

LyricType preferences::searching::preferred_lyric_type()
{
    return LyricType::Synced; // TODO
}

const LRESULT MAX_SOURCE_NAME_LENGTH = 64;

class PreferencesSearching : public CDialogImpl<PreferencesSearching>, public auto_preferences_page_instance, private play_callback_impl_base
{
public:
    // Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
    PreferencesSearching(preferences_page_callback::ptr callback) : auto_preferences_page_instance(callback, g_searching_auto_properties) {}

    // Dialog resource ID - Required by WTL/Create()
    enum {IDD = IDD_PREFERENCES_SEARCHING};

    void apply() override;
    void reset() override;
    bool has_changed() override;

    BEGIN_MSG_MAP_EX(PreferencesSearching)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_HANDLER_EX(IDC_SEARCH_EXCLUDE_BRACKETS, BN_CLICKED, OnUIChange)
        COMMAND_HANDLER_EX(IDC_SOURCE_MOVE_UP_BTN, BN_CLICKED, OnMoveUp)
        COMMAND_HANDLER_EX(IDC_SOURCE_MOVE_DOWN_BTN, BN_CLICKED, OnMoveDown)
        COMMAND_HANDLER_EX(IDC_SOURCE_ACTIVATE_BTN, BN_CLICKED, OnSourceActivate)
        COMMAND_HANDLER_EX(IDC_SOURCE_DEACTIVATE_BTN, BN_CLICKED, OnSourceDeactivate)
        COMMAND_HANDLER_EX(IDC_ACTIVE_SOURCE_LIST, LBN_SELCHANGE, OnActiveSourceSelect)
        COMMAND_HANDLER_EX(IDC_INACTIVE_SOURCE_LIST, LBN_SELCHANGE, OnInactiveSourceSelect)
        COMMAND_HANDLER_EX(IDC_SEARCH_SKIP_FILTER_STR, EN_CHANGE, OnSkipFilterFormatChange)
        NOTIFY_HANDLER_EX(IDC_SEARCH_SYNTAX_HELP, NM_CLICK, OnSyntaxHelpClicked)
    END_MSG_MAP()

private:
    void on_playback_new_track(metadb_handle_ptr track) override;

    BOOL OnInitDialog(CWindow, LPARAM);
    void OnUIChange(UINT, int, CWindow);
    void OnMoveUp(UINT, int, CWindow);
    void OnMoveDown(UINT, int, CWindow);
    void OnSourceActivate(UINT, int, CWindow);
    void OnSourceDeactivate(UINT, int, CWindow);
    void OnActiveSourceSelect(UINT, int, CWindow);
    void OnInactiveSourceSelect(UINT, int, CWindow);
    void OnSkipFilterFormatChange(UINT, int, CWindow);
    LRESULT OnSyntaxHelpClicked(NMHDR*);

    void SourceListInitialise();
    void SourceListResetFromSaved();
    void SourceListResetToDefault();
    void SourceListApply();
    bool SourceListHasChanged();
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

    SourceListInitialise();
    UpdateSkipFilterPreview();
    init_auto_preferences();

    return FALSE;
}

void PreferencesSearching::OnUIChange(UINT, int, CWindow)
{
    on_ui_interaction();
}

void PreferencesSearching::OnMoveUp(UINT, int, CWindow)
{
    LRESULT select_index = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETCURSEL, 0, 0);
    if(select_index == LB_ERR)
    {
        return; // No selection
    }
    if(select_index == 0)
    {
        return; // Can't move the top item upwards
    }

    TCHAR buffer[MAX_SOURCE_NAME_LENGTH];
    LRESULT select_strlen = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETTEXTLEN, select_index, 0);
    assert(select_strlen+1 <= MAX_SOURCE_NAME_LENGTH);

    LRESULT strcopy_result = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETTEXT, select_index, (LPARAM)buffer);
    LRESULT select_data = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETITEMDATA, select_index, 0);
    LRESULT delete_result = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_DELETESTRING, select_index, 0);
    assert(strcopy_result != LB_ERR);
    assert(select_data != LB_ERR);
    assert(delete_result != LB_ERR);

    LRESULT new_index = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_INSERTSTRING, select_index-1, (LPARAM)buffer);
    LRESULT set_result = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_SETITEMDATA, new_index, select_data);
    LRESULT select_result = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_SETCURSEL, new_index, 0);
    assert((new_index != LB_ERR) && (new_index != LB_ERRSPACE));
    assert(set_result != LB_ERR);
    assert(select_result != LB_ERR);

    OnActiveSourceSelect(0, 0, {}); // Update the button enabled state (e.g if we moved an item to the bottom we should disable the "down" button)

    on_ui_interaction();
}

void PreferencesSearching::OnMoveDown(UINT, int, CWindow)
{
    LRESULT item_count = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETCOUNT, 0, 0);
    assert(item_count != LB_ERR);

    LRESULT select_index = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETCURSEL, 0, 0);
    if(select_index == LB_ERR)
    {
        return; // No selection
    }
    if(select_index+1 == item_count)
    {
        return; // Can't move the bottom item downwards
    }

    TCHAR buffer[MAX_SOURCE_NAME_LENGTH];
    LRESULT select_strlen = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETTEXTLEN, select_index, 0);
    assert(select_strlen+1 <= MAX_SOURCE_NAME_LENGTH);

    LRESULT strcopy_result = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETTEXT, select_index, (LPARAM)buffer);
    LRESULT select_data = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETITEMDATA, select_index, 0);
    LRESULT delete_result = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_DELETESTRING, select_index, 0);
    assert(strcopy_result != LB_ERR);
    assert(select_data != LB_ERR);
    assert(delete_result != LB_ERR);

    LRESULT new_index = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_INSERTSTRING, select_index+1, (LPARAM)buffer);
    LRESULT set_result = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_SETITEMDATA, new_index, select_data);
    LRESULT select_result = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_SETCURSEL, new_index, 0);
    assert((new_index != LB_ERR) && (new_index != LB_ERRSPACE));
    assert(set_result != LB_ERR);
    assert(select_result != LB_ERR);

    OnActiveSourceSelect(0, 0, {}); // Update the button enabled state (e.g if we moved an item to the bottom we should disable the "down" button)

    on_ui_interaction();
}

void PreferencesSearching::OnSourceActivate(UINT, int, CWindow)
{
    LRESULT select_index = SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_GETCURSEL, 0, 0);
    if(select_index == LB_ERR)
    {
        return; // No selection
    }

    TCHAR buffer[MAX_SOURCE_NAME_LENGTH];
    LRESULT select_strlen = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETTEXTLEN, select_index, 0);
    assert(select_strlen+1 <= MAX_SOURCE_NAME_LENGTH);

    LRESULT strcopy_result = SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_GETTEXT, select_index, (LPARAM)buffer);
    LRESULT select_data = SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_GETITEMDATA, select_index, 0);
    LRESULT delete_result = SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_DELETESTRING, select_index, 0);
    assert(strcopy_result != LB_ERR);
    assert(select_data != LB_ERR);
    assert(delete_result != LB_ERR);

    LRESULT new_index = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_ADDSTRING, 0, (LPARAM)buffer);
    LRESULT set_result = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_SETITEMDATA, new_index, select_data);
    assert((new_index != LB_ERR) && (new_index != LB_ERRSPACE));
    assert(set_result != LB_ERR);

    // Select the appropriate adjacent item in the active list so that you can spam (de)activate.
    LRESULT item_count = SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_GETCOUNT, 0, 0);
    assert(item_count != LB_ERR);
    if(item_count > 0)
    {
        LRESULT new_inactive_index;
        if(select_index < item_count)
        {
            new_inactive_index = select_index;
        }
        else
        {
            if(select_index == 0)
            {
                new_inactive_index = 0;
            }
            else
            {
                new_inactive_index = select_index-1;
            }
        }
        SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_SETCURSEL, new_inactive_index, 0);
    }

    on_ui_interaction();
}

void PreferencesSearching::OnSourceDeactivate(UINT, int, CWindow)
{
    LRESULT select_index = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETCURSEL, 0, 0);
    if(select_index == LB_ERR)
    {
        return; // No selection
    }

    TCHAR buffer[MAX_SOURCE_NAME_LENGTH];
    LRESULT select_strlen = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETTEXTLEN, select_index, 0);
    assert(select_strlen+1 <= MAX_SOURCE_NAME_LENGTH);

    LRESULT strcopy_result = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETTEXT, select_index, (LPARAM)buffer);
    LRESULT select_data = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETITEMDATA, select_index, 0);
    LRESULT delete_result = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_DELETESTRING, select_index, 0);
    assert(strcopy_result != LB_ERR);
    assert(select_data != LB_ERR);
    assert(delete_result != LB_ERR);

    LRESULT new_index = SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_ADDSTRING, 0, (LPARAM)buffer);
    LRESULT set_result = SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_SETITEMDATA, new_index, select_data);
    assert((new_index != LB_ERR) && (new_index != LB_ERRSPACE));
    assert(set_result != LB_ERR);

    // Select the appropriate adjacent item in the active list so that you can spam (de)activate.
    LRESULT item_count = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETCOUNT, 0, 0);
    assert(item_count != LB_ERR);
    if(item_count > 0)
    {
        LRESULT new_active_index;
        if(select_index < item_count)
        {
            new_active_index = select_index;
        }
        else
        {
            if(select_index == 0)
            {
                new_active_index = 0;
            }
            else
            {
                new_active_index = select_index-1;
            }
        }
        SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_SETCURSEL, new_active_index, 0);
    }

    on_ui_interaction();
}

void PreferencesSearching::OnActiveSourceSelect(UINT, int, CWindow)
{
    LRESULT select_index = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETCURSEL, 0, 0);
    LRESULT item_count = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETCOUNT, 0, 0);
    assert(item_count != LB_ERR);

    SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_SETCURSEL, WPARAM(-1));

    CWindow activate_btn = GetDlgItem(IDC_SOURCE_ACTIVATE_BTN);
    CWindow deactivate_btn = GetDlgItem(IDC_SOURCE_DEACTIVATE_BTN);
    assert(activate_btn != nullptr);
    assert(deactivate_btn != nullptr);
    activate_btn.EnableWindow(FALSE);
    deactivate_btn.EnableWindow(TRUE);

    CWindow move_up_btn = GetDlgItem(IDC_SOURCE_MOVE_UP_BTN);
    assert(move_up_btn != nullptr);
    move_up_btn.EnableWindow((select_index != LB_ERR) && (select_index != 0));

    CWindow move_down_btn = GetDlgItem(IDC_SOURCE_MOVE_DOWN_BTN);
    assert(move_down_btn != nullptr);
    move_down_btn.EnableWindow((select_index != LB_ERR) && (select_index+1 != item_count));
}

void PreferencesSearching::OnInactiveSourceSelect(UINT, int, CWindow)
{
    SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_SETCURSEL, WPARAM(-1));

    CWindow activate_btn = GetDlgItem(IDC_SOURCE_ACTIVATE_BTN);
    CWindow deactivate_btn = GetDlgItem(IDC_SOURCE_DEACTIVATE_BTN);
    assert(activate_btn != nullptr);
    assert(deactivate_btn != nullptr);
    activate_btn.EnableWindow(TRUE);
    deactivate_btn.EnableWindow(FALSE);

    CWindow move_up_btn = GetDlgItem(IDC_SOURCE_MOVE_UP_BTN);
    CWindow move_down_btn = GetDlgItem(IDC_SOURCE_MOVE_DOWN_BTN);
    assert(move_up_btn != nullptr);
    assert(move_down_btn != nullptr);
    move_up_btn.EnableWindow(FALSE);
    move_down_btn.EnableWindow(FALSE);
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

void PreferencesSearching::reset()
{
    SourceListResetToDefault();
    auto_preferences_page_instance::reset();
}

void PreferencesSearching::apply()
{
    SourceListApply();
    auto_preferences_page_instance::apply();

    const bool has_musixmatch_token = !preferences::searching::musixmatch_api_key().empty();
    bool musixmatch_enabled = false;

    size_t source_count = cfg_search_active_sources.get_size();
    for(size_t i=0; i<source_count; i++)
    {
        if(cfg_search_active_sources[i] == musixmatch_src_guid)
        {
            musixmatch_enabled = true;
            break;
        }
    }

    if(musixmatch_enabled && !has_musixmatch_token)
    {
        popup_message_v3::query_t query = {};
        query.title = "Musixmatch Warning";
        query.msg = "You have enabled the 'Musixmatch' source for the OpenLyrics component, but have not provided a token. Without a token the Musixmatch source will not work.\r\n\r\nYou can click on the '?' button for more information on how to get a token.";
        query.buttons = popup_message_v3::buttonOK;
        query.defButton = popup_message_v3::buttonOK;
        query.icon = popup_message_v3::iconWarning;
        popup_message_v3::get()->show_query_modal(query);
    }
}

bool PreferencesSearching::has_changed()
{
    if(SourceListHasChanged()) return true;
    return auto_preferences_page_instance::has_changed();
}

void PreferencesSearching::SourceListInitialise()
{
    SourceListResetFromSaved();
}

void PreferencesSearching::SourceListResetFromSaved()
{
    SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_RESETCONTENT, 0, 0);
    SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_RESETCONTENT, 0, 0);

    std::vector<GUID> all_src_ids = LyricSourceBase::get_all_ids();
    size_t total_source_count = all_src_ids.size();
    std::vector<bool> sources_active(total_source_count);

    size_t active_source_count = cfg_search_active_sources.get_count();
    for(size_t active_source_index=0; active_source_index<active_source_count; active_source_index++)
    {
        GUID src_guid = cfg_search_active_sources[active_source_index];
        LyricSourceBase* src = LyricSourceBase::get(src_guid);
        assert(src != nullptr);
        if(src == nullptr)
        {
            LOG_WARN("Search configuration contains an unrecognised active source, ignoring...");
            continue;
        }

        bool found = false;
        for(size_t i=0; i<total_source_count; i++)
        {
            if(all_src_ids[i] == src_guid)
            {
                sources_active[i] = true;
                found = true;
                break;
            }
        }
        assert(found);

        LRESULT new_index = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_ADDSTRING, 0, (LPARAM)src->friendly_name().data());
        LRESULT set_result = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_SETITEMDATA, new_index, (LPARAM)&src->id());
        assert(new_index != LB_ERR);
        assert(set_result != LB_ERR);
    }

    for(size_t entry_index=0; entry_index<total_source_count; entry_index++)
    {
        if(sources_active[entry_index]) continue;

        LyricSourceBase* src = LyricSourceBase::get(all_src_ids[entry_index]);
        assert(src != nullptr);
        if(src == nullptr)
        {
            LOG_WARN("Search configuration contains an unrecognised inactive source, ignoring...");
            continue;
        }

        LRESULT new_index = SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_ADDSTRING, 0, (LPARAM)src->friendly_name().data());
        LRESULT set_result = SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_SETITEMDATA, new_index, (LPARAM)&src->id());
        assert(new_index != LB_ERR);
        assert(set_result != LB_ERR);
    }
}

void PreferencesSearching::SourceListResetToDefault()
{
    SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_RESETCONTENT, 0, 0);
    SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_RESETCONTENT, 0, 0);

    std::vector<GUID> all_src_ids = LyricSourceBase::get_all_ids();
    size_t total_source_count = all_src_ids.size();
    std::vector<bool> sources_active(total_source_count);

    for(GUID src_guid : cfg_search_active_sources_default)
    {
        LyricSourceBase* src = LyricSourceBase::get(src_guid);
        assert(src != nullptr);
        if(src == nullptr)
        {
            LOG_WARN("Search configuration contains an unrecognised active source, ignoring...");
            continue;
        }

        bool found = false;
        for(size_t i=0; i<total_source_count; i++)
        {
            if(all_src_ids[i] == src_guid)
            {
                sources_active[i] = true;
                found = true;
                break;
            }
        }
        assert(found);

        LRESULT new_index = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_ADDSTRING, 0, (LPARAM)src->friendly_name().data());
        LRESULT set_result = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_SETITEMDATA, new_index, (LPARAM)&src->id());
        assert(new_index != LB_ERR);
        assert(set_result != LB_ERR);
    }

    for(size_t entry_index=0; entry_index<total_source_count; entry_index++)
    {
        if(sources_active[entry_index]) continue;

        LyricSourceBase* src = LyricSourceBase::get(all_src_ids[entry_index]);
        assert(src != nullptr);
        if(src == nullptr)
        {
            LOG_WARN("All-source list contains an unrecognised source, ignoring...");
            continue;
        }

        LRESULT new_index = SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_ADDSTRING, 0, (LPARAM)src->friendly_name().data());
        LRESULT set_result = SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_SETITEMDATA, new_index, (LPARAM)&src->id());
        assert(new_index != LB_ERR);
        assert(set_result != LB_ERR);
    }
}

void PreferencesSearching::SourceListApply()
{
    cfg_search_active_sources.remove_all();

    LRESULT item_count = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETCOUNT, 0, 0);
    assert(item_count != LB_ERR);

    for(LRESULT item_index=0; item_index<item_count; item_index++)
    {
        LRESULT item_data = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETITEMDATA, item_index, 0);
        assert(item_data != LB_ERR);
        const GUID* ui_item_id = (GUID*)item_data;
        assert(ui_item_id != nullptr);

        cfg_search_active_sources.add_item(*ui_item_id);
    }
}

bool PreferencesSearching::SourceListHasChanged()
{
    size_t saved_item_count = cfg_search_active_sources.get_count();
    LRESULT ui_item_count_result = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETCOUNT, 0, 0);
    assert(ui_item_count_result != LB_ERR);
    assert(ui_item_count_result >= 0);
    size_t ui_item_count = static_cast<size_t>(ui_item_count_result);

    if(saved_item_count != ui_item_count)
    {
        return true;
    }
    assert(saved_item_count == ui_item_count);

    for(size_t item_index=0; item_index<saved_item_count; item_index++)
    {
        LRESULT ui_item = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETITEMDATA, item_index, 0);
        assert(ui_item != LB_ERR);

        const GUID* ui_item_id = (const GUID*)ui_item;
        assert(ui_item_id != nullptr);

        GUID saved_item_id = cfg_search_active_sources[item_index];

        if(saved_item_id != *ui_item_id)
        {
            return true;
        }
    }

    return false;
}

class PreferencesSearchImpl : public preferences_page_impl<PreferencesSearching>
{
public:
    const char* get_name() { return "Searching"; }
    GUID get_guid() { return GUID_PREFERENCES_PAGE_SEARCH; }
    GUID get_parent_guid() { return GUID_PREFERENCES_PAGE_ROOT; }
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
