#include "stdafx.h"

#pragma warning(push, 0)
#include "foobar2000/SDK/coreDarkMode.h"
#include "foobar2000/helpers/atl-misc.h"
#include "resource.h"
#pragma warning(pop)

#include "config/config_auto.h"
#include "preferences.h"

// clang-format off: GUIDs should be one line
static const GUID GUID_PREFERENCES_PAGE_UPLOAD = { 0x8699d695, 0x1b56, 0x4898, { 0xaa, 0x57, 0xeb, 0xb3, 0x35, 0x7b, 0xd7, 0x9 } };

static const GUID GUID_CFG_UPLOAD_STRATEGY = { 0x28a7533a, 0x1f9c, 0x436e, { 0xba, 0xed, 0xa, 0x6a, 0x59, 0x26, 0xc3, 0xc7 } };
// clang-format on

static cfg_auto_combo_option<UploadStrategy> upload_strategy_options[] = {
    { _T("Never"), UploadStrategy::Never },
    { _T("After manual edit"), UploadStrategy::OnEdit },
};

static cfg_auto_combo<UploadStrategy, 2> cfg_upload_strategy(GUID_CFG_UPLOAD_STRATEGY,
                                                             IDC_UPLOAD_STRATEGY,
                                                             UploadStrategy::Never,
                                                             upload_strategy_options);

static cfg_auto_property* g_uploading_auto_properties[] = {
    &cfg_upload_strategy,
};

UploadStrategy preferences::upload::lrclib_upload_strategy()
{
    return cfg_upload_strategy.get_value();
}

class PreferencesUploading : public CDialogImpl<PreferencesUploading>, public auto_preferences_page_instance
{
public:
    PreferencesUploading(preferences_page_callback::ptr callback)
        : auto_preferences_page_instance(callback, g_uploading_auto_properties)
    {
    }

    // Dialog resource ID - Required by WTL/Create()
    enum
    {
        IDD = IDD_PREFERENCES_UPLOADING
    };

    // WTL message map
    BEGIN_MSG_MAP_EX(PreferencesUploading)
    MSG_WM_INITDIALOG(OnInitDialog)
    COMMAND_HANDLER_EX(IDC_UPLOAD_STRATEGY, CBN_SELCHANGE, OnUIChange)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnUIChange(UINT, int, CWindow);

    UploadStrategy m_last_selected_upload_strategy;
    fb2k::CCoreDarkModeHooks m_dark;
};

BOOL PreferencesUploading::OnInitDialog(CWindow, LPARAM)
{
    m_dark.AddDialogWithControls(m_hWnd);

    init_auto_preferences();
    m_last_selected_upload_strategy = cfg_upload_strategy.get_value();
    return FALSE;
}

void PreferencesUploading::OnUIChange(UINT, int, CWindow)
{
    on_ui_interaction();
}

class PreferencesUploadingImpl : public preferences_page_impl<PreferencesUploading>
{
public:
    const char* get_name() final
    {
        return "Uploading";
    }
    GUID get_guid() final
    {
        return GUID_PREFERENCES_PAGE_UPLOAD;
    }
    GUID get_parent_guid() final
    {
        return GUID_PREFERENCES_PAGE_ROOT;
    }
};
static preferences_page_factory_t<PreferencesUploadingImpl> g_preferences_page_uploading_factory;
