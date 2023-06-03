#include "stdafx.h"

#pragma warning(push, 0)
#include "resource.h"
#include "foobar2000/helpers/atl-misc.h"
#include "foobar2000/SDK/coreDarkMode.h"
#pragma warning(pop)

#include "config/config_auto.h"
#include "config/config_font.h"
#include "logging.h"
#include "preferences.h"
#include "ui_hooks.h"

static const GUID GUID_PREFERENCES_PAGE_DISPLAY_BACKGROUND = { 0xd4c823dc, 0xe71, 0x4c5c, { 0xad, 0x27, 0xcb, 0xf1, 0xd4, 0x78, 0x41, 0x4b } };

static const GUID GUID_CFG_DISPLAY_CUSTOM_BACKGROUND_COLOUR = { 0x13da3237, 0xaa1d, 0x4065, { 0x82, 0xb0, 0xe4, 0x3, 0x31, 0xe0, 0x69, 0x5b } };
static const GUID GUID_CFG_DISPLAY_BACKGROUND_COLOUR = { 0x7eaeeae6, 0xd41d, 0x4c0d, { 0x97, 0x86, 0x20, 0xa2, 0x8f, 0x27, 0x98, 0xd4 } };
static const GUID GUID_CFG_DISPLAY_BACKGROUND_MODE = { 0xdcb91bea, 0x942b, 0x4f0b, { 0xbc, 0xcd, 0x2f, 0x22, 0xb2, 0xaa, 0x89, 0xa9 } };

enum class BackgroundMode
{
    None,
    SolidColour,
    AlbumArt,
};
static const COLORREF cfg_display_bg_colour_default = RGB(255,255,255);

static const cfg_auto_combo_option<BackgroundMode> g_background_mode_options[] =
{
    {_T("Solid colour"), BackgroundMode::SolidColour},
    {_T("Album art"), BackgroundMode::AlbumArt},
};

static cfg_auto_combo<BackgroundMode, 2>      cfg_display_bg_mode(GUID_CFG_DISPLAY_BACKGROUND_MODE, IDC_BACKGROUND_MODE, BackgroundMode::SolidColour, g_background_mode_options);
static cfg_auto_bool                          cfg_display_custom_bg_colour(GUID_CFG_DISPLAY_CUSTOM_BACKGROUND_COLOUR, IDC_BACKGROUND_COLOUR_CUSTOM, false);
static cfg_int_t<uint32_t>                    cfg_display_bg_colour(GUID_CFG_DISPLAY_BACKGROUND_COLOUR, cfg_display_bg_colour_default);

static cfg_auto_property* g_display_auto_properties[] =
{
    &cfg_display_bg_mode,
    &cfg_display_custom_bg_colour,
};

//
// Globals
//
static HFONT g_display_font = nullptr;
static COLORREF g_custom_colours[16] = {
    RGB(255,255,255), RGB(255,255,255), RGB(255,255,255), RGB(255,255,255), RGB(255,255,255), RGB(255,255,255), RGB(255,255,255), RGB(255,255,255),
    RGB(255,255,255), RGB(255,255,255), RGB(255,255,255), RGB(255,255,255), RGB(255,255,255), RGB(255,255,255), RGB(255,255,255), RGB(255,255,255),
};

//
// Preference retrieval functions
//

std::optional<t_ui_color> preferences::display::background_colour()
{
    if(cfg_display_custom_bg_colour.get_value())
    {
        return (COLORREF)cfg_display_bg_colour.get_value();
    }
    return {};
}

//
// Preference page UI
//

class PreferencesDisplayBg : public CDialogImpl<PreferencesDisplayBg>, public auto_preferences_page_instance
{
public:
    // Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
    PreferencesDisplayBg(preferences_page_callback::ptr callback);
    ~PreferencesDisplayBg() override;

    // Dialog resource ID - Required by WTL/Create()
    enum {IDD = IDD_PREFERENCES_DISPLAY_BACKGROUND};

    void apply() override;
    void reset() override;
    bool has_changed() override;

    //WTL message map
    BEGIN_MSG_MAP_EX(PreferencesDisplayBg)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_HANDLER_EX(IDC_BACKGROUND_COLOUR_CUSTOM, BN_CLICKED, OnCustomToggle)
        COMMAND_HANDLER_EX(IDC_BACKGROUND_COLOUR, BN_CLICKED, OnBgColourChange)
        MESSAGE_HANDLER_EX(WM_CTLCOLORBTN, ColourButtonPreDraw)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnBgColourChange(UINT, int, CWindow);
    void OnCustomToggle(UINT, int, CWindow);
    void OnUIChange(UINT, int, CWindow);
    LRESULT ColourButtonPreDraw(UINT, WPARAM, LPARAM);

    void SelectBrushColour(HBRUSH& brush);
    void RepaintColours();

    HBRUSH m_brush_background;

    fb2k::CCoreDarkModeHooks m_dark;
};

PreferencesDisplayBg::PreferencesDisplayBg(preferences_page_callback::ptr callback) :
    auto_preferences_page_instance(callback, g_display_auto_properties)
{
    m_brush_background = CreateSolidBrush(cfg_display_bg_colour.get_value());
}

PreferencesDisplayBg::~PreferencesDisplayBg()
{
    DeleteObject(m_brush_background);
}

void PreferencesDisplayBg::apply()
{
    // Reset the global configured font handle so that it can be re-created with the new value
    DeleteObject(g_display_font);
    g_display_font = nullptr;

    LOGBRUSH brushes[1] = {};
    GetObject(m_brush_background, sizeof(brushes[0]), &brushes[0]);
    cfg_display_bg_colour = brushes[0].lbColor;

    auto_preferences_page_instance::apply();
    repaint_all_lyric_panels();
}

void PreferencesDisplayBg::reset()
{
    DeleteObject(m_brush_background);
    m_brush_background = CreateSolidBrush(cfg_display_bg_colour_default);

    auto_preferences_page_instance::reset();
}

bool PreferencesDisplayBg::has_changed()
{
    LOGBRUSH brushes[1] = {};
    GetObject(m_brush_background, sizeof(brushes[0]), &brushes[0]);

    bool changed = false;
    changed |= (cfg_display_bg_colour != brushes[0].lbColor);
    return changed || auto_preferences_page_instance::has_changed();
}

BOOL PreferencesDisplayBg::OnInitDialog(CWindow, LPARAM)
{
    m_dark.AddDialogWithControls(m_hWnd);

    init_auto_preferences();
    return FALSE;
}

void PreferencesDisplayBg::OnBgColourChange(UINT, int, CWindow)
{
    LRESULT custom = SendDlgItemMessage(IDC_BACKGROUND_COLOUR_CUSTOM, BM_GETCHECK, 0, 0);
    if(custom == BST_CHECKED)
    {
        SelectBrushColour(m_brush_background);
    }
}

void PreferencesDisplayBg::OnCustomToggle(UINT, int, CWindow)
{
    RepaintColours();
    on_ui_interaction();
}

void PreferencesDisplayBg::OnUIChange(UINT, int, CWindow)
{
    on_ui_interaction();
}

LRESULT PreferencesDisplayBg::ColourButtonPreDraw(UINT, WPARAM, LPARAM lparam)
{
    static_assert(sizeof(LRESULT) >= sizeof(HBRUSH));
    HWND btn_handle = (HWND)lparam;
    int btn_id = ::GetDlgCtrlID(btn_handle);
    if(btn_id == IDC_BACKGROUND_COLOUR)
    {
        LRESULT custom_bg = SendDlgItemMessage(IDC_BACKGROUND_COLOUR_CUSTOM, BM_GETCHECK, 0, 0);
        if(custom_bg == BST_CHECKED)
        {
            return (LRESULT)m_brush_background;
        }
    }

    return FALSE;
}

void PreferencesDisplayBg::SelectBrushColour(HBRUSH& handle)
{
    LOGBRUSH brush = {};
    GetObject(handle, sizeof(LOGBRUSH), &brush);

    CHOOSECOLOR colourOpts = {};
    colourOpts.lStructSize = sizeof(colourOpts);
    colourOpts.hwndOwner = m_hWnd;
    colourOpts.rgbResult = brush.lbColor;
    colourOpts.lpCustColors = g_custom_colours; 
    colourOpts.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT;
    BOOL colour_selected = ChooseColor(&colourOpts);
    if(colour_selected)
    {
        DeleteObject(handle);
        handle = CreateSolidBrush(colourOpts.rgbResult);

        RepaintColours();
        on_ui_interaction();
    }
}

void PreferencesDisplayBg::RepaintColours()
{
    int ids_to_repaint[] = {
        IDC_BACKGROUND_COLOUR,
    };
    for(int id : ids_to_repaint)
    {
        CWindow handle = GetDlgItem(id);
        if(handle != nullptr)
        {
            handle.RedrawWindow(nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
        }
    }
}

class PreferencesDisplayBgImpl : public preferences_page_impl<PreferencesDisplayBg>
{
public:
    const char* get_name() final { return "Background"; }
    GUID get_guid() final { return GUID_PREFERENCES_PAGE_DISPLAY_BACKGROUND; }
    GUID get_parent_guid() final { return GUID_PREFERENCES_PAGE_ROOT; }
};
static preferences_page_factory_t<PreferencesDisplayBgImpl> g_preferences_page_display_factory;
