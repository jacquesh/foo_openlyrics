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

static const GUID GUID_PREFERENCES_PAGE_DISPLAY = { 0xa31b1608, 0xe77f, 0x4fe5, { 0x80, 0x4b, 0xcf, 0x8c, 0xc8, 0x17, 0xd8, 0x69 } };

static const GUID GUID_CFG_DISPLAY_CUSTOM_FONT = { 0x828be475, 0x8e26, 0x4504, { 0x87, 0x53, 0x22, 0xf5, 0x69, 0xd, 0x53, 0xb7 } };
static const GUID GUID_CFG_DISPLAY_CUSTOM_FOREGROUND_COLOUR = { 0x675418e1, 0xe0b0, 0x4c85, { 0xbf, 0xde, 0x1c, 0x17, 0x9b, 0xbc, 0xca, 0xa7 } };
static const GUID GUID_CFG_DISPLAY_CUSTOM_BACKGROUND_COLOUR = { 0x13da3237, 0xaa1d, 0x4065, { 0x82, 0xb0, 0xe4, 0x3, 0x31, 0xe0, 0x69, 0x5b } };
static const GUID GUID_CFG_DISPLAY_CUSTOM_HIGHLIGHT_COLOUR = { 0xfa2fed99, 0x593c, 0x4828, { 0xbf, 0x7d, 0x95, 0x8e, 0x99, 0x26, 0x9d, 0xcb } };
static const GUID GUID_CFG_DISPLAY_FONT = { 0xc06f95f7, 0x9358, 0x42ab, { 0xb7, 0xa0, 0x19, 0xe6, 0x74, 0x5f, 0xb9, 0x16 } };
static const GUID GUID_CFG_DISPLAY_FOREGROUND_COLOUR = { 0x36724d22, 0xe51e, 0x4c84, { 0x9e, 0xb2, 0x58, 0xa4, 0xd8, 0x23, 0xb3, 0x67 } };
static const GUID GUID_CFG_DISPLAY_BACKGROUND_COLOUR = { 0x7eaeeae6, 0xd41d, 0x4c0d, { 0x97, 0x86, 0x20, 0xa2, 0x8f, 0x27, 0x98, 0xd4 } };
static const GUID GUID_CFG_DISPLAY_HIGHLIGHT_COLOUR = { 0xfa16da6c, 0xb22d, 0x49cb, { 0x97, 0x53, 0x94, 0x8c, 0xec, 0xf8, 0x37, 0x35 } };
static const GUID GUID_CFG_DISPLAY_LINEGAP = { 0x4cc61a5c, 0x58dd, 0x47ce, { 0xa9, 0x35, 0x9, 0xbb, 0xfa, 0xc6, 0x40, 0x43 } };
static const GUID GUID_CFG_DISPLAY_SCROLL_CONTINUOUS = { 0x9ccfe1b0, 0x3c8a, 0x4f3d, { 0x91, 0x1f, 0x1e, 0x3e, 0xdf, 0x71, 0x88, 0xd7 } };
static const GUID GUID_CFG_DISPLAY_SCROLL_TIME = { 0xc1c7dbf7, 0xd3ce, 0x40dc, { 0x83, 0x29, 0xed, 0xa0, 0xc6, 0xc8, 0xb6, 0x70 } };
static const GUID GUID_CFG_DISPLAY_SCROLL_DIRECTION = { 0x6b1f47ae, 0xa383, 0x434b, { 0xa7, 0xd2, 0x43, 0xbe, 0x55, 0x54, 0x2a, 0x33 } };
static const GUID GUID_CFG_DISPLAY_SCROLL_TYPE = { 0x3f2f17d8, 0x9309, 0x4721, { 0x9f, 0xa7, 0x79, 0x6d, 0x17, 0x84, 0x2a, 0x5d } };
static const GUID GUID_CFG_DISPLAY_HIGHLIGHT_FADE_TIME = { 0x63c31bb9, 0x2a83, 0x4685, { 0xb4, 0x15, 0x64, 0xd6, 0x5, 0x85, 0xbd, 0xa8 } };
static const GUID GUID_CFG_DEBUG_LOGS_ENABLED = { 0x57920cbe, 0xa27, 0x4fad, { 0x92, 0xc, 0x2b, 0x61, 0x3b, 0xf9, 0xd6, 0x13 } };

static const COLORREF cfg_display_fg_colour_default = RGB(35,85,125);
static const COLORREF cfg_display_bg_colour_default = RGB(255,255,255);
static const COLORREF cfg_display_hl_colour_default = RGB(225,65,60);

static const cfg_auto_combo_option<LineScrollDirection> g_scroll_direction_options[] =
{
    {_T("Vertical"), LineScrollDirection::Vertical},
    {_T("Horizontal"), LineScrollDirection::Horizontal},
};

static const cfg_auto_combo_option<LineScrollType> g_scroll_type_options[] =
{
    {_T("Automatic"), LineScrollType::Automatic},
    {_T("Manual"), LineScrollType::Manual},
};

static cfg_auto_bool                          cfg_display_custom_font(GUID_CFG_DISPLAY_CUSTOM_FONT, IDC_FONT_CUSTOM, false);
static cfg_auto_bool                          cfg_display_custom_fg_colour(GUID_CFG_DISPLAY_CUSTOM_FOREGROUND_COLOUR, IDC_FOREGROUND_COLOUR_CUSTOM, false);
static cfg_auto_bool                          cfg_display_custom_bg_colour(GUID_CFG_DISPLAY_CUSTOM_BACKGROUND_COLOUR, IDC_BACKGROUND_COLOUR_CUSTOM, false);
static cfg_auto_bool                          cfg_display_custom_hl_colour(GUID_CFG_DISPLAY_CUSTOM_HIGHLIGHT_COLOUR, IDC_HIGHLIGHT_COLOUR_CUSTOM, false);
static cfg_font_t                             cfg_display_font(GUID_CFG_DISPLAY_FONT);
static cfg_int_t<uint32_t>                    cfg_display_fg_colour(GUID_CFG_DISPLAY_FOREGROUND_COLOUR, cfg_display_fg_colour_default);
static cfg_int_t<uint32_t>                    cfg_display_bg_colour(GUID_CFG_DISPLAY_BACKGROUND_COLOUR, cfg_display_bg_colour_default);
static cfg_int_t<uint32_t>                    cfg_display_hl_colour(GUID_CFG_DISPLAY_HIGHLIGHT_COLOUR, cfg_display_hl_colour_default);
static cfg_auto_int                           cfg_display_linegap(GUID_CFG_DISPLAY_LINEGAP, IDC_RENDER_LINEGAP_EDIT, 4);
static cfg_auto_bool                          cfg_display_scroll_continuous(GUID_CFG_DISPLAY_SCROLL_CONTINUOUS, IDC_DISPLAY_SCROLL_CONTINUOUS, false);
static cfg_auto_ranged_int                    cfg_display_scroll_time(GUID_CFG_DISPLAY_SCROLL_TIME, IDC_DISPLAY_SCROLL_TIME, 0, 2000, 20, 500);
static cfg_auto_combo<LineScrollDirection, 2> cfg_display_scroll_direction(GUID_CFG_DISPLAY_SCROLL_DIRECTION, IDC_DISPLAY_SCROLL_DIRECTION, LineScrollDirection::Vertical, g_scroll_direction_options);
static cfg_auto_combo<LineScrollType, 2>      cfg_display_scroll_type(GUID_CFG_DISPLAY_SCROLL_TYPE, IDC_DISPLAY_SCROLL_TYPE, LineScrollType::Automatic, g_scroll_type_options);
static cfg_auto_ranged_int                    cfg_display_highlight_fade_time(GUID_CFG_DISPLAY_HIGHLIGHT_FADE_TIME, IDC_DISPLAY_HIGHLIGHT_FADE_TIME, 0, 1000, 20, 500);
static cfg_auto_bool                          cfg_debug_logs_enabled(GUID_CFG_DEBUG_LOGS_ENABLED, IDC_DEBUG_LOGS_ENABLED, false);

static cfg_auto_property* g_display_auto_properties[] =
{
    &cfg_display_custom_font,
    &cfg_display_custom_fg_colour,
    &cfg_display_custom_bg_colour,
    &cfg_display_custom_hl_colour,

    &cfg_display_linegap,
    &cfg_display_scroll_continuous,
    &cfg_display_scroll_time,
    &cfg_display_scroll_direction,
    &cfg_display_scroll_type,

    &cfg_display_highlight_fade_time,

    &cfg_debug_logs_enabled,
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
t_ui_font preferences::display::font()
{
    if(cfg_display_custom_font.get_value())
    {
        if(g_display_font == nullptr)
        {
            LOG_INFO("Creating new display font handle");
            g_display_font = CreateFontIndirect(&cfg_display_font.get_value());
        }
        return g_display_font;
    }
    return nullptr;
}

std::optional<t_ui_color> preferences::display::foreground_colour()
{
    if(cfg_display_custom_fg_colour.get_value())
    {
        return (COLORREF)cfg_display_fg_colour.get_value();
    }
    return {};
}

std::optional<t_ui_color> preferences::display::background_colour()
{
    if(cfg_display_custom_bg_colour.get_value())
    {
        return (COLORREF)cfg_display_bg_colour.get_value();
    }
    return {};
}

std::optional<t_ui_color> preferences::display::highlight_colour()
{
    if(cfg_display_custom_hl_colour.get_value())
    {
        return (COLORREF)cfg_display_hl_colour.get_value();
    }
    return {};
}

double preferences::display::highlight_fade_seconds()
{
    return static_cast<double>(cfg_display_highlight_fade_time.get_value())/1000.0;
}

int preferences::display::linegap()
{
    return cfg_display_linegap.get_value();
}

double preferences::display::scroll_time_seconds()
{
    const bool continuous = cfg_display_scroll_continuous.get_value();
    if(continuous)
    {
        return DBL_MAX;
    }

    return ((double)cfg_display_scroll_time.get_value())/1000.0;
}

LineScrollDirection preferences::display::scroll_direction()
{
    return cfg_display_scroll_direction.get_value();
}

LineScrollType preferences::display::scroll_type()
{
    return cfg_display_scroll_type.get_value();
}

bool preferences::display::debug_logs_enabled()
{
    return cfg_debug_logs_enabled.get_value();
}

bool preferences::display::raw::font_is_custom()
{
    return cfg_display_custom_font.get_value();
}

//
// Preference page UI
//

class PreferencesDisplay : public CDialogImpl<PreferencesDisplay>, public auto_preferences_page_instance
{
public:
    // Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
    PreferencesDisplay(preferences_page_callback::ptr callback);
    ~PreferencesDisplay() override;

    // Dialog resource ID - Required by WTL/Create()
    enum {IDD = IDD_PREFERENCES_DISPLAY};

    void apply() override;
    void reset() override;
    bool has_changed() override;

    //WTL message map
    BEGIN_MSG_MAP_EX(PreferencesDisplay)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_HSCROLL(OnScrollTimeChange)
        COMMAND_HANDLER_EX(IDC_FONT_CUSTOM, BN_CLICKED, OnCustomToggle)
        COMMAND_HANDLER_EX(IDC_FOREGROUND_COLOUR_CUSTOM, BN_CLICKED, OnCustomToggle)
        COMMAND_HANDLER_EX(IDC_BACKGROUND_COLOUR_CUSTOM, BN_CLICKED, OnCustomToggle)
        COMMAND_HANDLER_EX(IDC_HIGHLIGHT_COLOUR_CUSTOM, BN_CLICKED, OnCustomToggle)
        COMMAND_HANDLER_EX(IDC_FONT, BN_CLICKED, OnFontChange)
        COMMAND_HANDLER_EX(IDC_FOREGROUND_COLOUR, BN_CLICKED, OnFgColourChange)
        COMMAND_HANDLER_EX(IDC_BACKGROUND_COLOUR, BN_CLICKED, OnBgColourChange)
        COMMAND_HANDLER_EX(IDC_HIGHLIGHT_COLOUR, BN_CLICKED, OnHlColourChange)
        COMMAND_HANDLER_EX(IDC_RENDER_LINEGAP_EDIT, EN_CHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_DISPLAY_SCROLL_CONTINUOUS, BN_CLICKED, OnScrollContinuousChange)
        COMMAND_HANDLER_EX(IDC_DISPLAY_SCROLL_DIRECTION, CBN_SELCHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_DISPLAY_SCROLL_TYPE, CBN_SELCHANGE, OnUIChange)
        MESSAGE_HANDLER_EX(WM_CTLCOLORBTN, ColourButtonPreDraw)
        COMMAND_HANDLER_EX(IDC_DEBUG_LOGS_ENABLED, BN_CLICKED, OnUIChange)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnFontChange(UINT, int, CWindow);
    void OnFgColourChange(UINT, int, CWindow);
    void OnBgColourChange(UINT, int, CWindow);
    void OnHlColourChange(UINT, int, CWindow);
    void OnCustomToggle(UINT, int, CWindow);
    void OnScrollContinuousChange(UINT, int, CWindow);
    void OnUIChange(UINT, int, CWindow);
    void OnScrollTimeChange(int, int, HWND);
    LRESULT ColourButtonPreDraw(UINT, WPARAM, LPARAM);

    void UpdateFontButtonText();
    void SelectBrushColour(HBRUSH& brush);
    void RepaintColours();
    void UpdateScrollTimePreview();
    void UpdateFadeTimePreview();
    void UpdateRangedIntegerPreview(const cfg_auto_ranged_int& cfg, int preview_control_id);
    void SetScrollTimeEnabled(bool enabled);

    LOGFONT m_font;
    HBRUSH m_brush_foreground;
    HBRUSH m_brush_background;
    HBRUSH m_brush_highlight;

    fb2k::CCoreDarkModeHooks m_dark;
};

PreferencesDisplay::PreferencesDisplay(preferences_page_callback::ptr callback) :
    auto_preferences_page_instance(callback, g_display_auto_properties)
{
    m_font = cfg_display_font.get_value();
    m_brush_foreground = CreateSolidBrush(cfg_display_fg_colour.get_value());
    m_brush_background = CreateSolidBrush(cfg_display_bg_colour.get_value());
    m_brush_highlight = CreateSolidBrush(cfg_display_hl_colour.get_value());
}

PreferencesDisplay::~PreferencesDisplay()
{
    DeleteObject(m_brush_foreground);
    DeleteObject(m_brush_background);
    DeleteObject(m_brush_highlight);
}

void PreferencesDisplay::apply()
{
    // Reset the global configured font handle so that it can be re-created with the new value
    DeleteObject(g_display_font);
    g_display_font = nullptr;
    cfg_display_font.set_value(m_font);

    LOGBRUSH brushes[3] = {};
    GetObject(m_brush_foreground, sizeof(brushes[0]), &brushes[0]);
    GetObject(m_brush_background, sizeof(brushes[0]), &brushes[1]);
    GetObject(m_brush_highlight, sizeof(brushes[0]), &brushes[2]);
    cfg_display_fg_colour = brushes[0].lbColor;
    cfg_display_bg_colour = brushes[1].lbColor;
    cfg_display_hl_colour = brushes[2].lbColor;

    auto_preferences_page_instance::apply();
    repaint_all_lyric_panels();
}

void PreferencesDisplay::reset()
{
    m_font = cfg_display_font.get_value();

    DeleteObject(m_brush_foreground);
    DeleteObject(m_brush_background);
    DeleteObject(m_brush_highlight);
    m_brush_foreground = CreateSolidBrush(cfg_display_fg_colour_default);
    m_brush_background = CreateSolidBrush(cfg_display_bg_colour_default);
    m_brush_highlight = CreateSolidBrush(cfg_display_hl_colour_default);
    auto_preferences_page_instance::reset();

    SetScrollTimeEnabled(true);
    UpdateScrollTimePreview();
    UpdateFadeTimePreview();
    UpdateFontButtonText();
    RepaintColours();
}

bool PreferencesDisplay::has_changed()
{
    LOGBRUSH brushes[3] = {};
    GetObject(m_brush_foreground, sizeof(brushes[0]), &brushes[0]);
    GetObject(m_brush_background, sizeof(brushes[0]), &brushes[1]);
    GetObject(m_brush_highlight, sizeof(brushes[0]), &brushes[2]);

    bool changed = false;
    changed |= !(cfg_display_font == m_font);
    changed |= (cfg_display_fg_colour != brushes[0].lbColor);
    changed |= (cfg_display_bg_colour != brushes[1].lbColor);
    changed |= (cfg_display_hl_colour != brushes[2].lbColor);

    return changed || auto_preferences_page_instance::has_changed();
}

BOOL PreferencesDisplay::OnInitDialog(CWindow, LPARAM)
{
    m_dark.AddDialogWithControls(m_hWnd);

    init_auto_preferences();
    UpdateFontButtonText();
    UpdateScrollTimePreview();
    UpdateFadeTimePreview();
    RepaintColours();
    SetScrollTimeEnabled(!cfg_display_scroll_continuous.get_value());
    return FALSE;
}

void PreferencesDisplay::OnFontChange(UINT, int, CWindow)
{
    CHOOSEFONT fontOpts = {};
    fontOpts.lStructSize = sizeof(fontOpts);
    fontOpts.hwndOwner = m_hWnd;
    fontOpts.lpLogFont = &m_font;
    fontOpts.Flags = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT;
    fontOpts.nFontType = SCREEN_FONTTYPE;
    BOOL font_selected = ChooseFont(&fontOpts);
    if(font_selected)
    {
        UpdateFontButtonText();
        on_ui_interaction();
    }

    on_ui_interaction();
}

void PreferencesDisplay::OnFgColourChange(UINT, int, CWindow)
{
    LRESULT custom = SendDlgItemMessage(IDC_FOREGROUND_COLOUR_CUSTOM, BM_GETCHECK, 0, 0);
    if(custom == BST_CHECKED)
    {
        SelectBrushColour(m_brush_foreground);
    }
}

void PreferencesDisplay::OnBgColourChange(UINT, int, CWindow)
{
    LRESULT custom = SendDlgItemMessage(IDC_BACKGROUND_COLOUR_CUSTOM, BM_GETCHECK, 0, 0);
    if(custom == BST_CHECKED)
    {
        SelectBrushColour(m_brush_background);
    }
}

void PreferencesDisplay::OnHlColourChange(UINT, int, CWindow)
{
    LRESULT custom = SendDlgItemMessage(IDC_HIGHLIGHT_COLOUR_CUSTOM, BM_GETCHECK, 0, 0);
    if(custom == BST_CHECKED)
    {
        SelectBrushColour(m_brush_highlight);
    }
}

void PreferencesDisplay::OnCustomToggle(UINT, int, CWindow)
{
    UpdateFontButtonText();
    RepaintColours();
    on_ui_interaction();
}

void PreferencesDisplay::SetScrollTimeEnabled(bool enabled)
{
    CWindow scroll_time = GetDlgItem(IDC_DISPLAY_SCROLL_TIME);
    CWindow scroll_label = GetDlgItem(IDC_DISPLAY_SCROLL_TIME_PREVIEW);
    assert((scroll_time != nullptr) && (scroll_label != nullptr));

    scroll_time.EnableWindow(enabled);
    scroll_label.EnableWindow(enabled);
}

void PreferencesDisplay::OnScrollContinuousChange(UINT, int, CWindow)
{
    LRESULT continuous = SendDlgItemMessage(IDC_DISPLAY_SCROLL_CONTINUOUS, BM_GETCHECK, 0, 0);
    const bool is_continuous = (continuous == BST_CHECKED);
    SetScrollTimeEnabled(!is_continuous);

    on_ui_interaction();
}

void PreferencesDisplay::OnUIChange(UINT, int, CWindow)
{
    on_ui_interaction();
}

void PreferencesDisplay::OnScrollTimeChange(int /*request_type*/, int /*new_position*/, HWND source_window)
{
    // Docs say this handle will be the scroll bar that sent the message if any, otherwise null.
    // Currently the only scroll bar is the setting for synced-line-scroll-time
    if(source_window == nullptr) return;

    const int source_control_id = ::GetDlgCtrlID(source_window);
    if(source_control_id == IDC_DISPLAY_SCROLL_TIME)
    {
        UpdateScrollTimePreview();
    }
    else if(source_control_id == IDC_DISPLAY_HIGHLIGHT_FADE_TIME)
    {
        UpdateFadeTimePreview();
    }

    on_ui_interaction();
}

LRESULT PreferencesDisplay::ColourButtonPreDraw(UINT, WPARAM, LPARAM lparam)
{
    static_assert(sizeof(LRESULT) >= sizeof(HBRUSH));
    HWND btn_handle = (HWND)lparam;
    int btn_id = ::GetDlgCtrlID(btn_handle);
    if(btn_id == IDC_FOREGROUND_COLOUR)
    {
        LRESULT custom_fg = SendDlgItemMessage(IDC_FOREGROUND_COLOUR_CUSTOM, BM_GETCHECK, 0, 0);
        if(custom_fg == BST_CHECKED)
        {
            return (LRESULT)m_brush_foreground;
        }
    }
    else if(btn_id == IDC_HIGHLIGHT_COLOUR)
    {
        LRESULT custom_hl = SendDlgItemMessage(IDC_HIGHLIGHT_COLOUR_CUSTOM, BM_GETCHECK, 0, 0);
        if(custom_hl == BST_CHECKED)
        {
            return (LRESULT)m_brush_highlight;
        }
    }
    else if(btn_id == IDC_BACKGROUND_COLOUR)
    {
        LRESULT custom_bg = SendDlgItemMessage(IDC_BACKGROUND_COLOUR_CUSTOM, BM_GETCHECK, 0, 0);
        if(custom_bg == BST_CHECKED)
        {
            return (LRESULT)m_brush_background;
        }
    }

    return FALSE;
}

void PreferencesDisplay::UpdateFontButtonText()
{
    bool custom_font = (SendDlgItemMessage(IDC_FONT_CUSTOM, BM_GETCHECK, 0, 0) == BST_CHECKED);
    CWindow font_btn = GetDlgItem(IDC_FONT);
    assert(font_btn != nullptr);
    font_btn.EnableWindow(custom_font);

    HDC dc = GetDC();
    int point_size = -MulDiv(m_font.lfHeight, 72, GetDeviceCaps(dc, LOGPIXELSY));
    ReleaseDC(dc);
    const int point_buffer_len = 32;
    TCHAR point_buffer[point_buffer_len];
    _sntprintf_s(point_buffer, point_buffer_len, _T(", %dpt"), point_size);

    std::tstring font_str;
    font_str += m_font.lfFaceName;
    font_str += point_buffer;

    if(m_font.lfWeight == FW_BOLD)
    {
        font_str += _T(", Bold");
    }
    if(m_font.lfItalic)
    {
        font_str += _T(", Italic");
    }

    font_btn.SetWindowText(font_str.c_str());
    ReleaseDC(dc);
}

void PreferencesDisplay::SelectBrushColour(HBRUSH& handle)
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

void PreferencesDisplay::RepaintColours()
{
    int ids_to_repaint[] = {
        IDC_FOREGROUND_COLOUR,
        IDC_BACKGROUND_COLOUR,
        IDC_HIGHLIGHT_COLOUR
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

void PreferencesDisplay::UpdateScrollTimePreview()
{
    UpdateRangedIntegerPreview(cfg_display_scroll_time, IDC_DISPLAY_SCROLL_TIME_PREVIEW);
}

void PreferencesDisplay::UpdateFadeTimePreview()
{
    UpdateRangedIntegerPreview(cfg_display_highlight_fade_time, IDC_DISPLAY_HIGHLIGHT_FADE_TIME_PREVIEW);
}

void PreferencesDisplay::UpdateRangedIntegerPreview(const cfg_auto_ranged_int& cfg, int preview_control_id)
{
    // We get the value from the autocfg because that handles the increments correctly
    int value = cfg.get_ui_value();

    const int preview_length = 32;
    TCHAR preview[preview_length];
    _sntprintf_s(preview, preview_length, _T("%d milliseconds"), value);
    SetDlgItemText(preview_control_id, preview);
}

class PreferencesDisplayImpl : public preferences_page_impl<PreferencesDisplay>
{
public:
    const char* get_name() final { return "Display"; }
    GUID get_guid() final { return GUID_PREFERENCES_PAGE_DISPLAY; }
    GUID get_parent_guid() final { return GUID_PREFERENCES_PAGE_ROOT; }
};
static preferences_page_factory_t<PreferencesDisplayImpl> g_preferences_page_display_factory;
