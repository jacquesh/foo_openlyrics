#include "stdafx.h"

#pragma warning(push, 0)
#include "resource.h"
#include "foobar2000/helpers/atl-misc.h"
#pragma warning(pop)

#include "config/config_auto.h"
#include "logging.h"
#include "preferences.h"

static const GUID GUID_PREFERENCES_PAGE_EDIT = { 0x6187e852, 0x199c, 0x4dc2, { 0x85, 0x21, 0x65, 0x39, 0x9, 0xc0, 0xeb, 0x3c } };

static const GUID GUID_CFG_EDIT_AUTO_AUTO_EDITS = { 0x3b416210, 0x85fa, 0x4406, { 0xb5, 0xd6, 0x4b, 0x39, 0x72, 0x8e, 0xee, 0xab } };

static const int cfg_edit_auto_auto_edits_default[] = {static_cast<int>(AutoEditType::ReplaceHtmlEscapedChars)};

static cfg_objList<int> cfg_edit_auto_auto_edits(GUID_CFG_EDIT_AUTO_AUTO_EDITS, cfg_edit_auto_auto_edits_default);

static const std::pair<AutoEditType, const TCHAR*> g_autoedit_options[] =
{
    {AutoEditType::ReplaceHtmlEscapedChars, _T("Replace &-named HTML characters")},
    {AutoEditType::RemoveRepeatedSpaces, _T("Remove repeated spaces")},
    {AutoEditType::RemoveRepeatedBlankLines, _T("Remove repeated blank lines")},
    {AutoEditType::RemoveAllBlankLines, _T("Remove all blank lines")},
    {AutoEditType::ResetCapitalisation, _T("Reset capitalisation")},
};

std::vector<AutoEditType> preferences::editing::automated_auto_edits()
{
    size_t edit_count = cfg_edit_auto_auto_edits.get_size();
    std::vector<AutoEditType> result;
    result.reserve(edit_count+1);
    for(size_t i=0; i<edit_count; i++)
    {
        result.push_back(static_cast<AutoEditType>(cfg_edit_auto_auto_edits[i]));
    }

    return result;
}

class PreferencesEdit : public CDialogImpl<PreferencesEdit>, public preferences_page_instance
{
public:
    // Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
    PreferencesEdit(preferences_page_callback::ptr callback) : m_callback(callback) {}

    // Dialog resource ID - Required by WTL/Create()
    enum {IDD = IDD_PREFERENCES_EDIT};

    void apply() override;
    void reset() override;
    bool has_changed();

    BEGIN_MSG_MAP_EX(PreferencesRoot)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_NOTIFY(OnNotify)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);

    void EditListInitialise();
    void EditListResetFromSaved();
    void EditListResetToDefault();
    void EditListApply();
    bool EditListHasChanged();

    preferences_page_callback::ptr m_callback;
    void on_ui_interaction();
    t_uint32 get_state() override;
};

void PreferencesEdit::on_ui_interaction()
{
    m_callback->on_state_changed();
}
t_uint32 PreferencesEdit::get_state()
{
    t_uint32 state = preferences_state::resettable;
    if (has_changed()) state |= preferences_state::changed;
    return state;
}

BOOL PreferencesEdit::OnInitDialog(CWindow, LPARAM)
{
    EditListInitialise();
    return FALSE;
}

LRESULT PreferencesEdit::OnNotify(int /*idCtrl*/, LPNMHDR notify)
{
    if((notify->idFrom != IDC_EDIT_AUTOEDITS_AUTOMATED) ||
        (notify->code != LVN_ITEMCHANGED))
    {
        on_ui_interaction();
    }

    return 0;
}

void PreferencesEdit::reset()
{
    EditListResetToDefault();
}

void PreferencesEdit::apply()
{
    EditListApply();
}

bool PreferencesEdit::has_changed()
{
    if(EditListHasChanged()) return true;
    return false;
}

static bool list_contains(const cfg_objList<int>& list, int value)
{
    size_t len = list.get_count();
    for(size_t i=0; i<len; i++)
    {
        if(list[i] == value)
        {
            return true;
        }
    }

    return false;
}

void PreferencesEdit::EditListInitialise()
{
    SendDlgItemMessage(IDC_EDIT_AUTOEDITS_AUTOMATED, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);

    int index = 0;
    for(const auto& [type, ui_str] : g_autoedit_options)
    {
        LVITEM item = {};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = index;
        item.pszText = const_cast<TCHAR*>(ui_str);
        item.lParam = static_cast<LPARAM>(type);
        LRESULT item_index = SendDlgItemMessageW(IDC_EDIT_AUTOEDITS_AUTOMATED, LVM_INSERTITEM, 0, (LPARAM)&item);
        assert(item_index == index);

        index++;
    }

    EditListResetFromSaved();
}

void PreferencesEdit::EditListResetFromSaved()
{
    int index = 0;
    for(const auto& [type, ui_str] : g_autoedit_options)
    {
        bool checked = list_contains(cfg_edit_auto_auto_edits, static_cast<int>(type));
        ListView_SetCheckState(GetDlgItem(IDC_EDIT_AUTOEDITS_AUTOMATED), index, checked)

        (void)ui_str; // unused
        index++;
    }
}

void PreferencesEdit::EditListResetToDefault()
{
    const auto enabled_by_default = [](AutoEditType type)
    {
        for(int i : cfg_edit_auto_auto_edits_default)
        {
            if(static_cast<int>(type) == i)
            {
                return true;
            }
        }
        return false;
    };

    int index = 0;
    for(const auto& [type, ui_str] : g_autoedit_options)
    {
        bool checked = enabled_by_default(type);
        ListView_SetCheckState(GetDlgItem(IDC_EDIT_AUTOEDITS_AUTOMATED), index, checked)

        (void)ui_str; // unused
        index++;
    }
}

void PreferencesEdit::EditListApply()
{
    cfg_edit_auto_auto_edits.remove_all();
    int index = 0;
    for(const auto& [type, ui_str] : g_autoedit_options)
    {
        bool checked = ListView_GetCheckState(GetDlgItem(IDC_EDIT_AUTOEDITS_AUTOMATED), index);
        if(checked)
        {
            cfg_edit_auto_auto_edits.add_item(static_cast<int>(type));
        }

        (void)ui_str; // unused
        index++;
    }
}

bool PreferencesEdit::EditListHasChanged()
{
    int index = 0;
    for(const auto& [type, ui_str] : g_autoedit_options)
    {
        bool enabled_config = list_contains(cfg_edit_auto_auto_edits, static_cast<int>(type));
        bool enabled_ui = ListView_GetCheckState(GetDlgItem(IDC_EDIT_AUTOEDITS_AUTOMATED), index);
        if(enabled_config != enabled_ui)
        {
            return true;
        }

        (void)ui_str; // unused
        index++;
    }

    return false;
}

class PreferencesEditImpl : public preferences_page_impl<PreferencesEdit>
{
public:
    const char* get_name() final { return "Editing"; }
    GUID get_guid() final { return GUID_PREFERENCES_PAGE_EDIT; }
    GUID get_parent_guid() final { return GUID_PREFERENCES_PAGE_ROOT; }
};

static preferences_page_factory_t<PreferencesEditImpl> g_preferences_page_edit_factory;
