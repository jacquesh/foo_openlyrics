#include "stdafx.h"

#pragma warning(push, 0)
#include "resource.h"
#include "foobar2000/helpers/atl-misc.h"
#include "foobar2000/SDK/coreDarkMode.h"
#pragma warning(pop)

#include "config/config_auto.h"
#include "logging.h"
#include "preferences.h"

static const GUID GUID_PREFERENCES_PAGE_SAVING = { 0xd5a7534, 0x9f59, 0x444c, { 0x8d, 0x6f, 0xec, 0xf3, 0x7f, 0x61, 0xfc, 0xf1 } };

static const GUID GUID_CFG_SAVE_ENABLE_AUTOSAVE = { 0xf25be2d9, 0x4442, 0x4602, { 0xa0, 0xf1, 0x81, 0xd, 0x8e, 0xab, 0x6a, 0x2 } };
static const GUID GUID_CFG_SAVE_METHOD = { 0xdf39b51c, 0xec55, 0x41aa, { 0x93, 0xd3, 0x32, 0xb6, 0xc0, 0x5d, 0x4f, 0xcc } };
static const GUID GUID_CFG_SAVE_MERGE_LRC_LINES = { 0x97229606, 0x8fd5, 0x441a, { 0xa6, 0x84, 0x9f, 0x3d, 0x87, 0xc8, 0x27, 0x18 } };

static cfg_auto_combo_option<SaveMethod> save_method_options[] =
{
    {_T("Save to text file"), SaveMethod::LocalFile},
    {_T("Save to tag"), SaveMethod::Id3Tag},
};

static cfg_auto_combo_option<AutoSaveStrategy> autosave_strategy_options[] =
{
    {_T("Always"), AutoSaveStrategy::Always},
    {_T("Only synced lyrics"), AutoSaveStrategy::OnlySynced},
    {_T("Only unsynced lyrics"), AutoSaveStrategy::OnlyUnsynced},
    {_T("Never"), AutoSaveStrategy::Never}
};

static cfg_auto_combo<AutoSaveStrategy, 4>   cfg_save_auto_save_strategy(GUID_CFG_SAVE_ENABLE_AUTOSAVE, IDC_SAVE_AUTOSAVE_TYPE, AutoSaveStrategy::Always, autosave_strategy_options);
static cfg_auto_combo<SaveMethod, 2>         cfg_save_method(GUID_CFG_SAVE_METHOD, IDC_SAVE_METHOD_COMBO, SaveMethod::LocalFile, save_method_options);
static cfg_auto_bool                         cfg_save_merge_lrc_lines(GUID_CFG_SAVE_MERGE_LRC_LINES, IDC_SAVE_MERGE_EQUIVALENT_LRC_LINES, true);

static cfg_auto_property* g_saving_auto_properties[] =
{
    &cfg_save_auto_save_strategy,
    &cfg_save_method,
    &cfg_save_merge_lrc_lines,
};

AutoSaveStrategy preferences::saving::autosave_strategy()
{
    return cfg_save_auto_save_strategy.get_value();
}

GUID preferences::saving::save_source()
{
    // NOTE: These were copied from the relevant lyric-source source file.
    //       It should not be a problem because these GUIDs must never change anyway (since it would
    //       break everybody's config), but probably worth noting that the information is duplicated.
    const GUID localfiles_src_guid = { 0x76d90970, 0x1c98, 0x4fe2, { 0x94, 0x4e, 0xac, 0xe4, 0x93, 0xf3, 0x8e, 0x85 } };
    const GUID id3tag_src_guid = { 0x3fb0f715, 0xa097, 0x493a, { 0x94, 0x4e, 0xdb, 0x48, 0x66, 0x8, 0x86, 0x78 } };

    SaveMethod method = cfg_save_method.get_value();
    if(method == SaveMethod::LocalFile)
    {
        return localfiles_src_guid;
    }
    else if(method == SaveMethod::Id3Tag)
    {
        return id3tag_src_guid;
    }
    else
    {
        LOG_ERROR("Unrecognised save method: %d", (int)method);
        assert(false);
        return {};
    }
}

bool preferences::saving::merge_equivalent_lrc_lines()
{
    return cfg_save_merge_lrc_lines.get_value();
}

class PreferencesSaving : public CDialogImpl<PreferencesSaving>, public auto_preferences_page_instance
{
public:
    PreferencesSaving(preferences_page_callback::ptr callback) : auto_preferences_page_instance(callback, g_saving_auto_properties) {}

    // Dialog resource ID - Required by WTL/Create()
    enum {IDD = IDD_PREFERENCES_SAVING};

    //WTL message map
    BEGIN_MSG_MAP_EX(PreferencesSaving)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_HANDLER_EX(IDC_SAVE_METHOD_COMBO, CBN_SELCHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_SAVE_AUTOSAVE_TYPE, CBN_SELCHANGE, OnAutoSaveChange)
        COMMAND_HANDLER_EX(IDC_SAVE_MERGE_EQUIVALENT_LRC_LINES, BN_CLICKED, OnUIChange)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnUIChange(UINT, int, CWindow);
    void OnAutoSaveChange(UINT, int, CWindow);

    AutoSaveStrategy m_last_select_autosave_strategy;
    fb2k::CCoreDarkModeHooks m_dark;
};

BOOL PreferencesSaving::OnInitDialog(CWindow, LPARAM)
{
    m_dark.AddDialogWithControls(m_hWnd);

    init_auto_preferences();
    m_last_select_autosave_strategy = cfg_save_auto_save_strategy.get_value();
    return FALSE;
}

void PreferencesSaving::OnUIChange(UINT, int, CWindow)
{
    on_ui_interaction();
}

void PreferencesSaving::OnAutoSaveChange(UINT, int, CWindow)
{
    // NOTE: the auto-combo config sets item-data to the integral representation of that option's enum value
    LRESULT ui_index = SendDlgItemMessage(IDC_SAVE_AUTOSAVE_TYPE, CB_GETCURSEL, 0, 0);
    LRESULT logical_value = SendDlgItemMessage(IDC_SAVE_AUTOSAVE_TYPE, CB_GETITEMDATA, ui_index, 0);
    assert(logical_value != CB_ERR);

    bool reverted = false;
    const AutoSaveStrategy strategy = static_cast<AutoSaveStrategy>(logical_value);
    if((strategy == AutoSaveStrategy::Never) && (m_last_select_autosave_strategy != AutoSaveStrategy::Never))
    {
        popup_message_v3::query_t query = {};
        query.title = "Autosave warning";
        query.msg = "You have selected the 'never autosave' option, which will prevent OpenLyrics from auto-saving lyrics unless you explicitly ask it to. Edits you make in the editor and with the auto-edit menu will still be saved automatically though. This could cause lyric searches to take significantly longer than if lyrics were saved and is not what most users want.\r\n\r\nAre you sure you want to disable autosave completely?";
        query.buttons = popup_message_v3::buttonYes | popup_message_v3::buttonNo;
        query.defButton = popup_message_v3::buttonNo;
        query.icon = popup_message_v3::iconWarning;
        uint32_t popup_result = popup_message_v3::get()->show_query_modal(query);
        if(popup_result == popup_message_v3::buttonNo)
        {
            for(int i=0; i<cfg_save_auto_save_strategy.get_option_count(); i++)
            {
                LRESULT item_logical_value = SendDlgItemMessage(IDC_SAVE_AUTOSAVE_TYPE, CB_GETITEMDATA, i, 0);
                assert(item_logical_value != CB_ERR);
                AutoSaveStrategy item_strategy = static_cast<AutoSaveStrategy>(item_logical_value);

                if(item_strategy == m_last_select_autosave_strategy)
                {
                    LRESULT set_cur_result = SendDlgItemMessage(IDC_SAVE_AUTOSAVE_TYPE, CB_SETCURSEL, i, 0);
                    assert(set_cur_result != CB_ERR);
                    reverted = true;
                    break;
                }
            }
        }
    }

    if(!reverted)
    {
        m_last_select_autosave_strategy = strategy;
    }

    on_ui_interaction();
}

class PreferencesSavingImpl : public preferences_page_impl<PreferencesSaving>
{
public:
    const char* get_name() final { return "Saving"; }
    GUID get_guid() final { return GUID_PREFERENCES_PAGE_SAVING; }
    GUID get_parent_guid() final { return GUID_PREFERENCES_PAGE_ROOT; }
};
static preferences_page_factory_t<PreferencesSavingImpl> g_preferences_page_saving_factory;

