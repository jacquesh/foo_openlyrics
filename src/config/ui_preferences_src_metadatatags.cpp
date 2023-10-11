#include "stdafx.h"

#pragma warning(push, 0)
#include "resource.h"
#include "foobar2000/helpers/atl-misc.h"
#include "foobar2000/SDK/coreDarkMode.h"
#pragma warning(pop)

#include "config/config_auto.h"
#include "preferences.h"

static const GUID GUID_PREFERENCES_PAGE_SRC_METATAGS = { 0x23c180eb, 0x1f8f, 0x4cf1, { 0x90, 0x2e, 0x31, 0x56, 0x2f, 0xa9, 0x4f, 0xf5 } };

static const GUID GUID_CFG_SEARCH_TAGS = { 0xb7332708, 0xe70b, 0x4a6e, { 0xa4, 0xd, 0x14, 0x6d, 0xe3, 0x74, 0x56, 0x65 } };
static const GUID GUID_CFG_SAVE_TAG_UNTIMED = { 0x39b0bc08, 0x5c3a, 0x4359, { 0x9d, 0xdb, 0xd4, 0x90, 0x84, 0xb, 0x31, 0x88 } };
static const GUID GUID_CFG_SAVE_TAG_TIMESTAMPED = { 0x337d0d40, 0xe9da, 0x4531, { 0xb0, 0x82, 0x13, 0x24, 0x56, 0xe5, 0xc4, 0x2 } };

static cfg_auto_string cfg_search_tags(GUID_CFG_SEARCH_TAGS, IDC_SEARCH_TAGS, "LYRICS;SYNCEDLYRICS;UNSYNCED LYRICS;UNSYNCEDLYRICS");

// NOTE: fb2k will silently handle "UNSYNCED LYRICS" as a special case and store the text in a USLT frame rather than a TXXX frame
//       Documented here: https://wiki.hydrogenaudio.org/index.php?title=Foobar2000:ID3_Tag_Mapping
//       fb2k does *not* support the SYLT frame, but OpenLyrics will recognise synced lyrics regardless of where they were loaded from, so we default
//       to using the same tag for both synced and unsynced lyrics to get it to at least save in the dedicated ID3 frame.
//       Also worth noting that not all container formats support ID3 tags. flac, for example, uses "vorbis comments" instead
static cfg_auto_string cfg_save_tag_untimed(GUID_CFG_SAVE_TAG_UNTIMED, IDC_SAVE_TAG_UNSYNCED, "UNSYNCED LYRICS");
static cfg_auto_string cfg_save_tag_timestamped(GUID_CFG_SAVE_TAG_TIMESTAMPED, IDC_SAVE_TAG_SYNCED, "UNSYNCED LYRICS"); // 

static cfg_auto_property* g_root_auto_properties[] =
{
    &cfg_search_tags,
    &cfg_save_tag_untimed,
    &cfg_save_tag_timestamped,
};

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

std::string_view preferences::saving::untimed_tag()
{
    return {cfg_save_tag_untimed.get_ptr(), cfg_save_tag_untimed.get_length()};
}

std::string_view preferences::saving::timestamped_tag()
{
    return {cfg_save_tag_timestamped.get_ptr(), cfg_save_tag_timestamped.get_length()};
}


class PreferencesSrcMetatags : public CDialogImpl<PreferencesSrcMetatags>, public auto_preferences_page_instance
{
public:
    // Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
    PreferencesSrcMetatags(preferences_page_callback::ptr callback) : auto_preferences_page_instance(callback, g_root_auto_properties) {}

    // Dialog resource ID - Required by WTL/Create()
    enum {IDD = IDD_PREFERENCES_SRC_METATAGS};

    void apply() override;
    void reset() override;
    bool has_changed() override;

    BEGIN_MSG_MAP_EX(PreferencesSrcMetatags)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_HANDLER_EX(IDC_SEARCH_TAGS, EN_CHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_SAVE_TAG_SYNCED, EN_CHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_SAVE_TAG_UNSYNCED, EN_CHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_SAVE_TAG_EXPLAIN, BN_CLICKED, OnTagExplain)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnUIChange(UINT, int, CWindow);
    void OnTagExplain(UINT, int, CWindow);

    fb2k::CCoreDarkModeHooks m_dark;
};

void PreferencesSrcMetatags::apply()
{
    auto_preferences_page_instance::apply();
}

void PreferencesSrcMetatags::reset()
{
    auto_preferences_page_instance::reset();
}

bool PreferencesSrcMetatags::has_changed()
{
    return auto_preferences_page_instance::has_changed();
}


BOOL PreferencesSrcMetatags::OnInitDialog(CWindow, LPARAM)
{
    m_dark.AddDialogWithControls(m_hWnd);

    init_auto_preferences();

    return FALSE;
}

void PreferencesSrcMetatags::OnUIChange(UINT, int, CWindow)
{
    on_ui_interaction();
}

void PreferencesSrcMetatags::OnTagExplain(UINT, int, CWindow)
{
    // Documented here: https://wiki.hydrogenaudio.org/index.php?title=Foobar2000:ID3_Tag_Mapping
    popup_message_v3::query_t query = {};
    query.title = "Save Tag Help";
    query.msg = "The 'UNSYNCED LYRICS' tag is handled differently to most other tags. It is stored in a way that is more compatible with other media players that might want to read lyric info.\n\nOpenLyrics will correctly load both synced and unsynced lyrics regardless of the tag in which they are stored.\n\nUnless you have a specific reason for wanting to use a different tag to store synced lyrics, leaving it as the 'UNSYNCED LYRICS' is strongly recommended.";
    query.buttons = popup_message_v3::buttonOK;
    query.defButton = popup_message_v3::buttonOK;
    query.icon = popup_message_v3::iconInformation;
    popup_message_v3::get()->show_query_modal(query);
}

class PreferencesSrcMetatagsImpl : public preferences_page_impl<PreferencesSrcMetatags>
{
public:
    const char* get_name() { return "Metadata Tags"; }
    GUID get_guid() { return GUID_PREFERENCES_PAGE_SRC_METATAGS; }
    GUID get_parent_guid() { return GUID_PREFERENCES_PAGE_SEARCH; }
};
static preferences_page_factory_t<PreferencesSrcMetatagsImpl> g_preferences_page_factory;
