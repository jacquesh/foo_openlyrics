#include "stdafx.h"

#pragma warning(push, 0)
#include "resource.h"
#include "foobar2000/helpers/atl-misc.h"
#pragma warning(pop)

#include "config/config_auto.h"
#include "logging.h"
#include "lyric_data.h"
#include "preferences.h"
#include "sources/lyric_source.h"
#include "winstr_util.h"

extern const GUID GUID_PREFERENCES_PAGE_ROOT = { 0x29e96cfa, 0xab67, 0x4793, { 0xa1, 0xc3, 0xef, 0xc3, 0xa, 0xbc, 0x8b, 0x74 } };

static const GUID GUID_CFG_SEARCH_ACTIVE_SOURCES = { 0x7d3c9b2c, 0xb87b, 0x4250, { 0x99, 0x56, 0x8d, 0xf5, 0x80, 0xc9, 0x2f, 0x39 } };
static const GUID GUID_CFG_SEARCH_TAGS = { 0xb7332708, 0xe70b, 0x4a6e, { 0xa4, 0xd, 0x14, 0x6d, 0xe3, 0x74, 0x56, 0x65 } };

static cfg_auto_string               cfg_search_tags(GUID_CFG_SEARCH_TAGS, IDC_SEARCH_TAGS, "LYRICS;SYNCEDLYRICS;UNSYNCEDLYRICS;UNSYNCED LYRICS");

static cfg_auto_property* g_root_auto_properties[] =
{
    &cfg_search_tags,
};

// NOTE: This was copied from the localfiles source file.
//       It should not be a problem because these GUIDs must never change anyway (since it would
//       break everybody's config), but probably worth noting that the information is duplicated.
static const GUID localfiles_src_guid = { 0x76d90970, 0x1c98, 0x4fe2, { 0x94, 0x4e, 0xac, 0xe4, 0x93, 0xf3, 0x8e, 0x85 } };

static const GUID cfg_search_active_sources_default[] = {localfiles_src_guid};
static cfg_objList<GUID> cfg_search_active_sources(GUID_CFG_SEARCH_ACTIVE_SOURCES, cfg_search_active_sources_default);

std::vector<GUID> preferences::searching::active_sources()
{
    size_t source_count = cfg_search_active_sources.get_size();
    std::vector<GUID> result;
    result.reserve(source_count);
    for(size_t i=0; i<source_count; i++)
    {
        result.push_back(cfg_search_active_sources[i]);
    }
    return result;
}

std::vector<std::string> preferences::searching::tags()
{
    const std::string_view setting = {cfg_search_tags.get_ptr(), cfg_search_tags.get_length()};
    std::vector<std::string> result;

    size_t prev_index = 0;
    for(size_t i=0; i<setting.length(); i++) // Avoid infinite loops
    {
        size_t next_index = setting.find(';', prev_index);
        size_t len = next_index - prev_index;
        if(len > 0)
        {
            result.emplace_back(setting.substr(prev_index, len));
        }

        if((next_index == std::string_view::npos) || (next_index >= setting.length()))
        {
            break;
        }
        prev_index = next_index + 1;
    }
    return result;
}

const LRESULT MAX_SOURCE_NAME_LENGTH = 64;

// The UI for the root element (for OpenLyrics) in the preferences UI tree
class PreferencesRoot : public CDialogImpl<PreferencesRoot>, public auto_preferences_page_instance
{
public:
    // Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
    PreferencesRoot(preferences_page_callback::ptr callback) : auto_preferences_page_instance(callback, g_root_auto_properties) {}

    // Dialog resource ID - Required by WTL/Create()
    enum {IDD = IDD_PREFERENCES_ROOT};

    void apply() override;
    void reset() override;
    bool has_changed() override;

    BEGIN_MSG_MAP_EX(PreferencesRoot)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_HANDLER_EX(IDC_SEARCH_TAGS, EN_CHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_SOURCE_MOVE_UP_BTN, BN_CLICKED, OnMoveUp)
        COMMAND_HANDLER_EX(IDC_SOURCE_MOVE_DOWN_BTN, BN_CLICKED, OnMoveDown)
        COMMAND_HANDLER_EX(IDC_SOURCE_ACTIVATE_BTN, BN_CLICKED, OnSourceActivate)
        COMMAND_HANDLER_EX(IDC_SOURCE_DEACTIVATE_BTN, BN_CLICKED, OnSourceDeactivate)
        COMMAND_HANDLER_EX(IDC_ACTIVE_SOURCE_LIST, LBN_SELCHANGE, OnActiveSourceSelect)
        COMMAND_HANDLER_EX(IDC_INACTIVE_SOURCE_LIST, LBN_SELCHANGE, OnInactiveSourceSelect)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnUIChange(UINT, int, CWindow);
    void OnMoveUp(UINT, int, CWindow);
    void OnMoveDown(UINT, int, CWindow);
    void OnSourceActivate(UINT, int, CWindow);
    void OnSourceDeactivate(UINT, int, CWindow);
    void OnActiveSourceSelect(UINT, int, CWindow);
    void OnInactiveSourceSelect(UINT, int, CWindow);

    void SourceListInitialise();
    void SourceListResetFromSaved();
    void SourceListResetToDefault();
    void SourceListApply();
    bool SourceListHasChanged();
};

BOOL PreferencesRoot::OnInitDialog(CWindow, LPARAM)
{
    SourceListInitialise();
    init_auto_preferences();
    return FALSE;
}

void PreferencesRoot::OnUIChange(UINT, int, CWindow)
{
    on_ui_interaction();
}

void PreferencesRoot::OnMoveUp(UINT, int, CWindow)
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

void PreferencesRoot::OnMoveDown(UINT, int, CWindow)
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

void PreferencesRoot::OnSourceActivate(UINT, int, CWindow)
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

void PreferencesRoot::OnSourceDeactivate(UINT, int, CWindow)
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

void PreferencesRoot::OnActiveSourceSelect(UINT, int, CWindow)
{
    LRESULT select_index = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETCURSEL, 0, 0);
    LRESULT item_count = SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_GETCOUNT, 0, 0);
    assert(item_count != LB_ERR);

    SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_SETCURSEL, -1);

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

void PreferencesRoot::OnInactiveSourceSelect(UINT, int, CWindow)
{
    SendDlgItemMessage(IDC_ACTIVE_SOURCE_LIST, LB_SETCURSEL, -1);

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

void PreferencesRoot::reset()
{
    SourceListResetToDefault();
    auto_preferences_page_instance::reset();
}

void PreferencesRoot::apply()
{
    SourceListApply();
    auto_preferences_page_instance::apply();
}

bool PreferencesRoot::has_changed()
{
    if(SourceListHasChanged()) return true;
    return auto_preferences_page_instance::has_changed();
}

void PreferencesRoot::SourceListInitialise()
{
    SourceListResetFromSaved();
}

void PreferencesRoot::SourceListResetFromSaved()
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

        LRESULT new_index = SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_ADDSTRING, 0, (LPARAM)src->friendly_name().data());
        LRESULT set_result = SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_SETITEMDATA, new_index, (LPARAM)&src->id());
        assert(new_index != LB_ERR);
        assert(set_result != LB_ERR);
    }
}

void PreferencesRoot::SourceListResetToDefault()
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

        LRESULT new_index = SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_ADDSTRING, 0, (LPARAM)src->friendly_name().data());
        LRESULT set_result = SendDlgItemMessage(IDC_INACTIVE_SOURCE_LIST, LB_SETITEMDATA, new_index, (LPARAM)&src->id());
        assert(new_index != LB_ERR);
        assert(set_result != LB_ERR);
    }
}

void PreferencesRoot::SourceListApply()
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

bool PreferencesRoot::SourceListHasChanged()
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

class PreferencesRootImpl : public preferences_page_impl<PreferencesRoot>
{
public:
    const char* get_name() { return "OpenLyrics"; }
    GUID get_guid() { return GUID_PREFERENCES_PAGE_ROOT; }
    GUID get_parent_guid() { return guid_tools; }
};

static preferences_page_factory_t<PreferencesRootImpl> g_preferences_page_root_factory;
