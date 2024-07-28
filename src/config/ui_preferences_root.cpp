#include "stdafx.h"

#pragma warning(push, 0)
#include "resource.h"
#include "foobar2000/helpers/atl-misc.h"
#include "foobar2000/SDK/coreDarkMode.h"
#pragma warning(pop)

#include "config/config_auto.h"
#include "preferences.h"

extern const GUID GUID_PREFERENCES_PAGE_ROOT = { 0x29e96cfa, 0xab67, 0x4793, { 0xa1, 0xc3, 0xef, 0xc3, 0xa, 0xbc, 0x8b, 0x74 } };

static const GUID GUID_CFG_DEBUG_LOGS_ENABLED = { 0x57920cbe, 0xa27, 0x4fad, { 0x92, 0xc, 0x2b, 0x61, 0x3b, 0xf9, 0xd6, 0x13 } };

static cfg_auto_bool cfg_debug_logs_enabled(GUID_CFG_DEBUG_LOGS_ENABLED, IDC_DEBUG_LOGS_ENABLED, false);

static cfg_auto_property* g_root_auto_properties[] =
{
    &cfg_debug_logs_enabled,
};

bool preferences::display::debug_logs_enabled()
{
    return cfg_debug_logs_enabled.get_value();
}

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
        COMMAND_HANDLER_EX(IDC_DEBUG_LOGS_ENABLED, BN_CLICKED, OnUIChange)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnUIChange(UINT, int, CWindow);

    fb2k::CCoreDarkModeHooks m_dark;
};

void PreferencesRoot::apply()
{
    auto_preferences_page_instance::apply();
}

void PreferencesRoot::reset()
{
    auto_preferences_page_instance::reset();
}

bool PreferencesRoot::has_changed()
{
    return auto_preferences_page_instance::has_changed();
}


BOOL PreferencesRoot::OnInitDialog(CWindow, LPARAM)
{
    m_dark.AddDialogWithControls(m_hWnd);

    init_auto_preferences();

    return FALSE;
}

void PreferencesRoot::OnUIChange(UINT, int, CWindow)
{
    on_ui_interaction();
}

class PreferencesRootImpl : public preferences_page_impl<PreferencesRoot>
{
public:
    const char* get_name() final { return "OpenLyrics"; }
    GUID get_guid() final { return GUID_PREFERENCES_PAGE_ROOT; }
    GUID get_parent_guid() final { return guid_tools; }
};
static preferences_page_factory_t<PreferencesRootImpl> g_preferences_page_root_factory;
