#include "stdafx.h"

#pragma warning(push, 0)
#include "resource.h"
#include "foobar2000/helpers/atl-misc.h"
#pragma warning(pop)

#include "config_auto.h"
#include "logging.h"
#include "preferences.h"

static const GUID GUID_PREFERENCES_PAGE = { 0x29e96cfa, 0xab67, 0x4793, { 0xa1, 0xc3, 0xef, 0xc3, 0xa, 0xbc, 0x8b, 0x74 } };
static const GUID GUID_CFG_FILENAME_FORMAT = { 0x1f7a3804, 0x7147, 0x4b64, { 0x9d, 0x51, 0x4c, 0xdd, 0x90, 0xa7, 0x6d, 0xd6 } };
static const GUID GUID_CFG_ENABLE_AUTOSAVE = { 0xf25be2d9, 0x4442, 0x4602, { 0xa0, 0xf1, 0x81, 0xd, 0x8e, 0xab, 0x6a, 0x2 } };
static const GUID GUID_CFG_RENDER_LINEGAP = { 0x4cc61a5c, 0x58dd, 0x47ce, { 0xa9, 0x35, 0x9, 0xbb, 0xfa, 0xc6, 0x40, 0x43 } };
static const GUID GUID_CFG_SAVE_METHOD = { 0xdf39b51c, 0xec55, 0x41aa, { 0x93, 0xd3, 0x32, 0xb6, 0xc0, 0x5d, 0x4f, 0xcc } };

static cfg_auto_combo_option<SaveMethod> save_method_options[] =
{
    {_T("Don't save"), SaveMethod::None},
    {_T("Save to the configuration folder"), SaveMethod::ConfigDirectory},
    {_T("Save to tag (TODO)"), SaveMethod::Id3Tag}
};

static cfg_auto_bool cfg_auto_save_enabled(GUID_CFG_ENABLE_AUTOSAVE, IDC_AUTOSAVE_ENABLED_CHKBOX, true);
static cfg_auto_int cfg_render_linegap(GUID_CFG_RENDER_LINEGAP, IDC_RENDER_LINEGAP_EDIT, 4);
static cfg_auto_string cfg_filename_format(GUID_CFG_FILENAME_FORMAT, IDC_SAVE_FILENAME_FORMAT, "[%artist% - ][%title%]");
static cfg_auto_combo<SaveMethod, 3> cfg_save_method(GUID_CFG_SAVE_METHOD, IDC_SAVE_METHOD_COMBO, SaveMethod::ConfigDirectory, save_method_options);

static cfg_auto_property* g_all_properties[] = {&cfg_auto_save_enabled, &cfg_render_linegap, &cfg_filename_format, &cfg_save_method};

bool preferences::get_autosave_enabled()
{
    return cfg_auto_save_enabled.get_value();
}

SaveMethod preferences::get_save_method()
{
    return (SaveMethod)cfg_save_method.get_value();
}

int preferences::get_render_linegap()
{
    return cfg_render_linegap.get_value();
}

const char* preferences::get_filename_format()
{
    return cfg_filename_format.get_ptr();
}

// The UI for the root element (for OpenLyrics) in the preferences UI tree
class PreferencesRoot : public CDialogImpl<PreferencesRoot>, public preferences_page_instance
{
public:
    // Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
    PreferencesRoot(preferences_page_callback::ptr callback) : m_callback(callback) {}

    // Dialog resource ID - Required by WTL/Create()
    enum {IDD = IDD_PREFERENCES};

    // preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
    t_uint32 get_state() override;
    void apply() override;
    void reset() override;

    //WTL message map
    BEGIN_MSG_MAP_EX(PreferencesRoot)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_HANDLER_EX(IDC_SAVE_FILENAME_FORMAT, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_AUTOSAVE_ENABLED_CHKBOX, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_SAVE_METHOD_COMBO, CBN_SELCHANGE, OnSelectionChange)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnEditChange(UINT, int, CWindow);
    void OnSelectionChange(UINT, int, CWindow);
    bool HasChanged();
    void OnChanged();

    const preferences_page_callback::ptr m_callback;
};

BOOL PreferencesRoot::OnInitDialog(CWindow, LPARAM)
{
    for(cfg_auto_property* prop : g_all_properties)
    {
        prop->Initialise(m_hWnd);
    }

    return FALSE;
}

void PreferencesRoot::OnEditChange(UINT, int, CWindow)
{
    OnChanged();
}

void PreferencesRoot::OnSelectionChange(UINT, int, CWindow)
{
    OnChanged();
}

t_uint32 PreferencesRoot::get_state()
{
    t_uint32 state = preferences_state::resettable;
    if (HasChanged()) state |= preferences_state::changed;
    return state;
}

void PreferencesRoot::reset()
{
    for(cfg_auto_property* prop : g_all_properties)
    {
        prop->ResetFromSaved();
    }
    OnChanged();
}

void PreferencesRoot::apply()
{
    for(cfg_auto_property* prop : g_all_properties)
    {
        prop->Apply();
    }
    OnChanged(); // our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool PreferencesRoot::HasChanged()
{
    for(cfg_auto_property* prop : g_all_properties)
    {
        if(prop->HasChanged()) return true;
    }

    LRESULT ui_value = SendDlgItemMessage(IDC_SAVE_METHOD_COMBO, CB_GETCURSEL, 0, 0);
    LOG_INFO("Combo box selection = %d", ui_value);
    return false;
}

void PreferencesRoot::OnChanged()
{
    //tell the host that our state has changed to enable/disable the apply button appropriately.
    m_callback->on_state_changed();
}

class PreferencesRootImpl : public preferences_page_impl<PreferencesRoot>
{
public:
    const char * get_name() {return "OpenLyrics";}
    GUID get_guid() { return GUID_PREFERENCES_PAGE; }
    GUID get_parent_guid() { return guid_tools; }
};

static preferences_page_factory_t<PreferencesRootImpl> g_root_preferences_page_factory;
