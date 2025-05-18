#include "stdafx.h"

#pragma warning(push, 0)
#include "foobar2000/SDK/coreDarkMode.h"
#include "foobar2000/helpers/atl-misc.h"
#include "resource.h"
#pragma warning(pop)

#include "config/config_auto.h"
#include "preferences.h"
#include "sources/lyric_source.h"

// clang-format off: GUIDs should be one line
static const GUID GUID_PREFERENCES_PAGE_SRC_MUSIXMATCH = { 0x5abc3564, 0xefb5, 0x4464, { 0xb0, 0x38, 0x28, 0xde, 0x8, 0x16, 0xa, 0x76 } };

static const GUID GUID_CFG_SEARCH_MUSIXMATCH_TOKEN = { 0xb88a82a7, 0x746d, 0x44f3, { 0xb8, 0x34, 0x9b, 0x9b, 0xe2, 0x6f, 0x8, 0x4c } };
// clang-format on

static cfg_auto_string cfg_search_musixmatch_token(GUID_CFG_SEARCH_MUSIXMATCH_TOKEN, IDC_SEARCH_MUSIXMATCH_TOKEN, "");

static cfg_auto_property* g_root_auto_properties[] = {
    &cfg_search_musixmatch_token,
};

std::string_view preferences::searching::musixmatch_api_key()
{
    return std::string_view(cfg_search_musixmatch_token.get_ptr(), cfg_search_musixmatch_token.get_length());
}

class PreferencesSrcMusixmatch : public CDialogImpl<PreferencesSrcMusixmatch>, public auto_preferences_page_instance
{
public:
    // Invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
    PreferencesSrcMusixmatch(preferences_page_callback::ptr callback)
        : auto_preferences_page_instance(callback, g_root_auto_properties)
    {
    }

    // Dialog resource ID - Required by WTL/Create()
    enum
    {
        IDD = IDD_PREFERENCES_SRC_MUSIXMATCH
    };

    BEGIN_MSG_MAP_EX(PreferencesSrcMusixmatch)
    MSG_WM_INITDIALOG(OnInitDialog)
    COMMAND_HANDLER_EX(IDC_SEARCH_MUSIXMATCH_TOKEN, EN_CHANGE, OnUIChange)
    COMMAND_HANDLER_EX(IDC_SEARCH_MUSIXMATCH_HELP, BN_CLICKED, OnMusixmatchHelp)
    COMMAND_HANDLER_EX(IDC_SEARCH_MUSIXMATCH_SHOW, BN_CLICKED, OnMusixmatchShow)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnUIChange(UINT, int, CWindow);
    void OnMusixmatchHelp(UINT, int, CWindow);
    void OnMusixmatchShow(UINT, int, CWindow);

    LRESULT m_default_password_char;

    fb2k::CCoreDarkModeHooks m_dark;
};

BOOL PreferencesSrcMusixmatch::OnInitDialog(CWindow, LPARAM)
{
    m_dark.AddDialogWithControls(m_hWnd);

    init_auto_preferences();

    m_default_password_char = SendDlgItemMessage(IDC_SEARCH_MUSIXMATCH_TOKEN, EM_GETPASSWORDCHAR, 0, 0);

    return FALSE;
}

void PreferencesSrcMusixmatch::OnUIChange(UINT, int, CWindow)
{
    on_ui_interaction();
}

void PreferencesSrcMusixmatch::OnMusixmatchHelp(UINT, int, CWindow)
{
    popup_message_v3::query_t query = {};
    query.title = "Musixmatch Help";
    query.msg = "The Musixmatch source requires an authentication token to work. Without one it will not find any "
                "lyrics.\r\n\r\nAn authentication token is roughly like a randomly-generated password that musixmatch "
                "uses to differentiate between different users.\r\n\r\nWould you like OpenLyrics to attempt to get a "
                "token automatically for you now?";
    query.buttons = popup_message_v3::buttonYes | popup_message_v3::buttonNo;
    query.defButton = popup_message_v3::buttonNo;
    query.icon = popup_message_v3::iconInformation;
    uint32_t popup_result = popup_message_v3::get()->show_query_modal(query);
    if(popup_result == popup_message_v3::buttonYes)
    {
        std::string output_token;
        const auto async_search = [&output_token](threaded_process_status& /*status*/, abort_callback& abort)
        { output_token = musixmatch_get_token(abort); };
        bool success = threaded_process::g_run_modal(threaded_process_callback_lambda::create(async_search),
                                                     threaded_process::flag_show_abort,
                                                     m_hWnd,
                                                     "Attempting to get Musixmatch token...");

        if(!success || output_token.empty())
        {
            popup_message_v3::query_t failed_query = {};
            failed_query.title = "Musixmatch Help";
            failed_query.msg = "Failed to automatically get a Musixmatch token.\r\n\r\nYou could try to get a token "
                               "manually, using the instructions found "
                               "here:\r\nhttps://github.com/khanhas/genius-spicetify#musicxmatch";
            failed_query.buttons = popup_message_v3::buttonOK;
            failed_query.defButton = popup_message_v3::buttonOK;
            failed_query.icon = popup_message_v3::iconWarning;
            popup_message_v3::get()->show_query_modal(failed_query);
        }
        else
        {
            std::tstring ui_token = to_tstring(output_token);
            SetDlgItemText(IDC_SEARCH_MUSIXMATCH_TOKEN, ui_token.c_str());
        }
    }
}

void PreferencesSrcMusixmatch::OnMusixmatchShow(UINT, int, CWindow)
{
    CWindow token = GetDlgItem(IDC_SEARCH_MUSIXMATCH_TOKEN);
    LRESULT password_char = token.SendMessage(EM_GETPASSWORDCHAR, 0, 0);
    if(password_char == m_default_password_char)
    {
        token.SendMessage(EM_SETPASSWORDCHAR, 0, 0);
    }
    else
    {
        token.SendMessage(EM_SETPASSWORDCHAR, m_default_password_char, 0);
    }
    token.Invalidate(); // Force it to redraw with the new character
}

class PreferencesSrcMusixmatchImpl : public preferences_page_impl<PreferencesSrcMusixmatch>
{
public:
    const char* get_name() final
    {
        return "Musixmatch";
    }
    GUID get_guid() final
    {
        return GUID_PREFERENCES_PAGE_SRC_MUSIXMATCH;
    }
    GUID get_parent_guid() final
    {
        return GUID_PREFERENCES_PAGE_SEARCH_SOURCES;
    }
};
static preferences_page_factory_t<PreferencesSrcMusixmatchImpl> g_preferences_page_factory;
