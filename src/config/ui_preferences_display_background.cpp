#include "stdafx.h"

#pragma warning(push, 0)
#include "foobar2000/SDK/coreDarkMode.h"
#include "foobar2000/helpers/atl-misc.h"
#include "resource.h"
#include <ShObjIdl_core.h>
#pragma warning(pop)

#include "config/config_auto.h"
#include "config/config_font.h"
#include "logging.h"
#include "preferences.h"
#include "ui_hooks.h"

// clang-format off: GUIDs should be one line
static const GUID GUID_PREFERENCES_PAGE_DISPLAY_BACKGROUND = { 0xd4c823dc, 0xe71, 0x4c5c, { 0xad, 0x27, 0xcb, 0xf1, 0xd4, 0x78, 0x41, 0x4b } };

static const GUID GUID_CFG_BACKGROUND_COLOUR_TYPE = { 0x13da3237, 0xaa1d, 0x4065, { 0x82, 0xb0, 0xe4, 0x3, 0x31, 0xe0, 0x69, 0x5b } };
static const GUID GUID_CFG_BACKGROUND_COLOUR = { 0x7eaeeae6, 0xd41d, 0x4c0d, { 0x97, 0x86, 0x20, 0xa2, 0x8f, 0x27, 0x98, 0xd4 } };
static const GUID GUID_CFG_BACKGROUND_MODE = { 0xdcb91bea, 0x942b, 0x4f0b, { 0xbc, 0xcd, 0x2f, 0x22, 0xb2, 0xaa, 0x89, 0xa9 } };
static const GUID GUID_CFG_BACKGROUND_IMAGE_OPACITY = { 0xf44e849f, 0x2f8f, 0x49cf, { 0x93, 0x6, 0x3a, 0x46, 0x76, 0x52, 0x5c, 0x3b } };
static const GUID GUID_CFG_BACKGROUND_BLUR_RADIUS = { 0xe9419593, 0x46b7, 0x403e, { 0xa7, 0xcc, 0x64, 0xd9, 0xed, 0x5b, 0x4a, 0x5a } };
static const GUID GUID_CFG_BACKGROUND_GRADIENT_TL = { 0x9b9066b0, 0xcb2a, 0x457e, { 0xa6, 0x98, 0x38, 0x3a, 0x73, 0x28, 0x5d, 0x89 } };
static const GUID GUID_CFG_BACKGROUND_GRADIENT_TR = { 0x5da8b259, 0x5d9d, 0x4ccc, { 0x9f, 0x5b, 0x48, 0xe9, 0x88, 0x7f, 0x89, 0xee } };
static const GUID GUID_CFG_BACKGROUND_GRADIENT_BL = { 0x1d5eec1c, 0x4981, 0x4b20, { 0x87, 0xb5, 0xe6, 0xec, 0x1c, 0xb7, 0x1b, 0x7c } };
static const GUID GUID_CFG_BACKGROUND_GRADIENT_BR = { 0x3c71b4fa, 0xe5a4, 0x46c6, { 0x92, 0x5c, 0xb2, 0xa, 0x3f, 0x3, 0x10, 0xc } };
static const GUID GUID_CFG_BACKGROUND_MAINTAIN_IMG_ASPECT_RATIO = { 0xb031bbce, 0xdb0c, 0x468f, { 0x9f, 0x64, 0xf1, 0xe8, 0xd, 0x5f, 0x2, 0x3c } };
static const GUID GUID_CFG_BACKGROUND_CUSTOM_IMAGE_PATH = { 0xc8ef264b, 0xa679, 0x4a63, { 0x99, 0x6, 0xc2, 0x5b, 0xff, 0x49, 0xe, 0x86 } };
static const GUID GUID_CFG_BACKGROUND_EXTERNALWIN_OPACITY = { 0xd7937a05, 0xbf33, 0x4647, { 0x8b, 0x24, 0xf4, 0x88, 0xb6, 0xc0, 0xca, 0x76 } };
// clang-format on

static const COLORREF cfg_background_colour_default = RGB(255, 255, 255);
static const COLORREF cfg_background_gradient_tl_default = RGB(11, 145, 255);
static const COLORREF cfg_background_gradient_tr_default = RGB(166, 215, 255);
static const COLORREF cfg_background_gradient_bl_default = RGB(100, 185, 255);
static const COLORREF cfg_background_gradient_br_default = RGB(255, 255, 255);

static const cfg_auto_combo_option<BackgroundFillType> g_background_fill_options[] = {
    { _T("Default"), BackgroundFillType::Default },
    { _T("Solid colour"), BackgroundFillType::SolidColour },
    { _T("Gradient"), BackgroundFillType::Gradient },
    { _T("Average image colour"), BackgroundFillType::AverageImageColor },
};

static const cfg_auto_combo_option<BackgroundImageType> g_background_image_options[] = {
    { _T("None"), BackgroundImageType::None },
    { _T("Album art"), BackgroundImageType::AlbumArt },
    { _T("Custom image"), BackgroundImageType::CustomImage },
};

static cfg_auto_combo<BackgroundFillType, 4> cfg_background_fill_type(GUID_CFG_BACKGROUND_MODE,
                                                                      IDC_BACKGROUND_FILL_TYPE,
                                                                      BackgroundFillType::Default,
                                                                      g_background_fill_options);
static cfg_auto_combo<BackgroundImageType, 3> cfg_background_image_type(GUID_CFG_BACKGROUND_COLOUR_TYPE,
                                                                        IDC_BACKGROUND_IMAGE_TYPE,
                                                                        BackgroundImageType::None,
                                                                        g_background_image_options);
static cfg_auto_colour cfg_background_colour(GUID_CFG_BACKGROUND_COLOUR,
                                             IDC_BACKGROUND_COLOUR,
                                             cfg_background_colour_default);
static cfg_auto_colour cfg_background_gradient_TL(GUID_CFG_BACKGROUND_GRADIENT_TL,
                                                  IDC_BACKGROUND_GRADIENT_TL,
                                                  cfg_background_gradient_tl_default);
static cfg_auto_colour cfg_background_gradient_TR(GUID_CFG_BACKGROUND_GRADIENT_TR,
                                                  IDC_BACKGROUND_GRADIENT_TR,
                                                  cfg_background_gradient_tr_default);
static cfg_auto_colour cfg_background_gradient_BL(GUID_CFG_BACKGROUND_GRADIENT_BL,
                                                  IDC_BACKGROUND_GRADIENT_BL,
                                                  cfg_background_gradient_bl_default);
static cfg_auto_colour cfg_background_gradient_BR(GUID_CFG_BACKGROUND_GRADIENT_BR,
                                                  IDC_BACKGROUND_GRADIENT_BR,
                                                  cfg_background_gradient_br_default);
static cfg_auto_ranged_int cfg_background_image_opacity(GUID_CFG_BACKGROUND_IMAGE_OPACITY,
                                                        IDC_BACKGROUND_IMG_OPACITY,
                                                        0,
                                                        100,
                                                        1,
                                                        16);
static cfg_auto_ranged_int cfg_background_blur_radius(GUID_CFG_BACKGROUND_BLUR_RADIUS,
                                                      IDC_BACKGROUND_BLUR_SLIDER,
                                                      0,
                                                      32,
                                                      1,
                                                      6); // This cannot allow a value greater than 127 or we will
                                                          // overflow the 16-bit integer accumulators used in blurring
static cfg_auto_bool cfg_background_maintain_img_aspect_ratio(GUID_CFG_BACKGROUND_MAINTAIN_IMG_ASPECT_RATIO,
                                                              IDC_BACKGROUND_MAINTAIN_IMG_ASPECT_RATIO,
                                                              true);
static cfg_auto_string cfg_background_custom_img_path(GUID_CFG_BACKGROUND_CUSTOM_IMAGE_PATH,
                                                      IDC_BACKGROUND_CUSTOM_IMG_PATH,
                                                      "");
static cfg_auto_bool cfg_background_externalwin_opaque(GUID_CFG_BACKGROUND_EXTERNALWIN_OPACITY,
                                                       IDC_BACKGROUND_EXTWIN_OPAQUE,
                                                       false);

static cfg_auto_property* g_display_auto_properties[] = {
    &cfg_background_fill_type,       &cfg_background_image_type,
    &cfg_background_colour,          &cfg_background_gradient_TL,
    &cfg_background_gradient_TR,     &cfg_background_gradient_BL,
    &cfg_background_gradient_BR,     &cfg_background_image_opacity,
    &cfg_background_blur_radius,     &cfg_background_maintain_img_aspect_ratio,
    &cfg_background_custom_img_path, &cfg_background_externalwin_opaque,
};

//
// Preference retrieval functions
//

BackgroundFillType preferences::background::fill_type()
{
    return cfg_background_fill_type.get_value();
}

BackgroundImageType preferences::background::image_type()
{
    return cfg_background_image_type.get_value();
}

t_ui_color preferences::background::colour()
{
    return cfg_background_colour.get_value();
}

t_ui_color preferences::background::gradient_tl()
{
    return (COLORREF)cfg_background_gradient_TL.get_value();
}

t_ui_color preferences::background::gradient_tr()
{
    return (COLORREF)cfg_background_gradient_TR.get_value();
}

t_ui_color preferences::background::gradient_bl()
{
    return (COLORREF)cfg_background_gradient_BL.get_value();
}

t_ui_color preferences::background::gradient_br()
{
    return (COLORREF)cfg_background_gradient_BR.get_value();
}

bool preferences::background::maintain_img_aspect_ratio()
{
    return cfg_background_maintain_img_aspect_ratio.get_value();
}

double preferences::background::image_opacity()
{
    return double(cfg_background_image_opacity.get_value()) / 100.0;
}

int preferences::background::blur_radius()
{
    return cfg_background_blur_radius.get_value();
}

std::string preferences::background::custom_image_path()
{
    const pfc::string8& result = cfg_background_custom_img_path.get();
    return std::string(result.c_str(), result.length());
}

bool preferences::background::external_window_opaque()
{
    return cfg_background_externalwin_opaque.get_value();
}

//
// Preference page UI
//

class PreferencesDisplayBg : public CDialogImpl<PreferencesDisplayBg>, public auto_preferences_page_instance
{
public:
    // Invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
    PreferencesDisplayBg(preferences_page_callback::ptr callback);
    ~PreferencesDisplayBg() override;

    // Dialog resource ID - Required by WTL/Create()
    enum
    {
        IDD = IDD_PREFERENCES_DISPLAY_BACKGROUND
    };

    void apply() override;
    void reset() override;
    bool has_changed() override;

    // WTL message map
    BEGIN_MSG_MAP_EX(PreferencesDisplayBg)
    MSG_WM_INITDIALOG(OnInitDialog)
    COMMAND_HANDLER_EX(IDC_BACKGROUND_COLOUR, BN_CLICKED, OnColourChangeRequest)
    COMMAND_HANDLER_EX(IDC_BACKGROUND_GRADIENT_TL, BN_CLICKED, OnColourChangeRequest)
    COMMAND_HANDLER_EX(IDC_BACKGROUND_GRADIENT_TR, BN_CLICKED, OnColourChangeRequest)
    COMMAND_HANDLER_EX(IDC_BACKGROUND_GRADIENT_BL, BN_CLICKED, OnColourChangeRequest)
    COMMAND_HANDLER_EX(IDC_BACKGROUND_GRADIENT_BR, BN_CLICKED, OnColourChangeRequest)
    COMMAND_HANDLER_EX(IDC_BACKGROUND_IMAGE_TYPE, CBN_SELCHANGE, OnImageTypeChange)
    COMMAND_HANDLER_EX(IDC_BACKGROUND_FILL_TYPE, CBN_SELCHANGE, OnFillTypeChange)
    COMMAND_HANDLER_EX(IDC_BACKGROUND_BLUR_SLIDER, WM_HSCROLL, OnUIChange)
    COMMAND_HANDLER_EX(IDC_BACKGROUND_IMG_OPACITY, WM_HSCROLL, OnUIChange)
    COMMAND_HANDLER_EX(IDC_BACKGROUND_MAINTAIN_IMG_ASPECT_RATIO, BN_CLICKED, OnUIChange)
    COMMAND_HANDLER_EX(IDC_BACKGROUND_CUSTOM_IMG_BROWSE, BN_CLICKED, OnCustomImageBrowse)
    COMMAND_HANDLER_EX(IDC_BACKGROUND_CUSTOM_IMG_PATH, EN_CHANGE, OnUIChange)
    COMMAND_HANDLER_EX(IDC_BACKGROUND_EXTWIN_OPAQUE, BN_CLICKED, OnUIChange)
    MSG_WM_HSCROLL(OnSliderMoved)
    MESSAGE_HANDLER_EX(WM_CTLCOLORBTN, ColourButtonPreDraw)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnColourChangeRequest(UINT, int, CWindow);
    void OnUIChange(UINT, int, CWindow);
    void OnFillTypeChange(UINT, int, CWindow);
    void OnImageTypeChange(UINT, int, CWindow);
    void OnSliderMoved(int, int, HWND);
    void OnCustomImageBrowse(UINT, int, CWindow);
    LRESULT ColourButtonPreDraw(UINT, WPARAM, LPARAM);

    void RepaintColours();
    void SetImageFieldsEnabled();

    fb2k::CCoreDarkModeHooks m_dark;
};

PreferencesDisplayBg::PreferencesDisplayBg(preferences_page_callback::ptr callback)
    : auto_preferences_page_instance(callback, g_display_auto_properties)
{
}

PreferencesDisplayBg::~PreferencesDisplayBg() {}

void PreferencesDisplayBg::apply()
{
    auto_preferences_page_instance::apply();
    recompute_lyric_panel_backgrounds();
    repaint_all_lyric_panels();
}

void PreferencesDisplayBg::reset()
{
    auto_preferences_page_instance::reset();
}

bool PreferencesDisplayBg::has_changed()
{
    return auto_preferences_page_instance::has_changed();
}

BOOL PreferencesDisplayBg::OnInitDialog(CWindow, LPARAM)
{
    m_dark.AddDialogWithControls(m_hWnd);

    init_auto_preferences();

    SetImageFieldsEnabled();
    return FALSE;
}

void PreferencesDisplayBg::OnColourChangeRequest(UINT, int control_id, CWindow)
{
    // NOTE: the auto-combo config sets item-data to the integral representation of that option's enum value
    const LRESULT ui_index = SendDlgItemMessage(IDC_BACKGROUND_FILL_TYPE, CB_GETCURSEL, 0, 0);
    const LRESULT logical_value = SendDlgItemMessage(IDC_BACKGROUND_FILL_TYPE, CB_GETITEMDATA, ui_index, 0);
    assert(logical_value != CB_ERR);
    const BackgroundFillType fill_type = static_cast<BackgroundFillType>(logical_value);

    bool changed = false;
    switch(control_id)
    {
        case IDC_BACKGROUND_COLOUR:
        {
            if(fill_type == BackgroundFillType::SolidColour)
            {
                changed = cfg_background_colour.SelectNewColour();
            }
        }
        break;

        case IDC_BACKGROUND_GRADIENT_TL:
        {
            if(fill_type == BackgroundFillType::Gradient)
            {
                changed = cfg_background_gradient_TL.SelectNewColour();
            }
        }
        break;
        case IDC_BACKGROUND_GRADIENT_TR:
        {
            if(fill_type == BackgroundFillType::Gradient)
            {
                changed = cfg_background_gradient_TR.SelectNewColour();
            }
        }
        break;
        case IDC_BACKGROUND_GRADIENT_BL:
        {
            if(fill_type == BackgroundFillType::Gradient)
            {
                changed = cfg_background_gradient_BL.SelectNewColour();
            }
        }
        break;
        case IDC_BACKGROUND_GRADIENT_BR:
        {
            if(fill_type == BackgroundFillType::Gradient)
            {
                changed = cfg_background_gradient_BR.SelectNewColour();
            }
        }
        break;
    }

    if(changed)
    {
        on_ui_interaction();
    }
}

void PreferencesDisplayBg::OnUIChange(UINT, int, CWindow)
{
    on_ui_interaction();
}

void PreferencesDisplayBg::OnFillTypeChange(UINT, int, CWindow)
{
    RepaintColours();
    on_ui_interaction();
}

void PreferencesDisplayBg::OnImageTypeChange(UINT, int, CWindow)
{
    SetImageFieldsEnabled();
    on_ui_interaction();
}

void PreferencesDisplayBg::OnSliderMoved(int /*request_type*/, int /*new_position*/, HWND source_window)
{
    // Docs say this handle will be the scroll bar that sent the message if any, otherwise null.
    if(source_window == nullptr) return;

    on_ui_interaction();
}

void PreferencesDisplayBg::OnCustomImageBrowse(UINT, int, CWindow)
{
    std::wstring result;
    bool success = false;
    IFileDialog* dialog = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if(SUCCEEDED(hr))
    {
        DWORD flags;
        hr = dialog->GetOptions(&flags);
        if(SUCCEEDED(hr))
        {
            hr = dialog->SetOptions(flags | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);
            if(SUCCEEDED(hr))
            {
                hr = dialog->Show(nullptr);
                if(SUCCEEDED(hr))
                {
                    IShellItem* selected_item;
                    hr = dialog->GetResult(&selected_item);
                    if(SUCCEEDED(hr))
                    {
                        TCHAR* selected_path = nullptr;
                        hr = selected_item->GetDisplayName(SIGDN_FILESYSPATH, &selected_path);
                        if(SUCCEEDED(hr))
                        {
                            result = std::wstring(selected_path);
                            CoTaskMemFree(selected_path);
                            success = true;
                        }

                        selected_item->Release();
                    }
                }
                else if(hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
                {
                    success = true;
                }
            }
        }

        dialog->Release();
    }

    if(success)
    {
        if(!result.empty())
        {
            SetDlgItemText(IDC_BACKGROUND_CUSTOM_IMG_PATH, result.c_str());
        }
    }
    else
    {
        LOG_INFO("Failure to get a path from the directory-select dialog");
    }
}

LRESULT PreferencesDisplayBg::ColourButtonPreDraw(UINT, WPARAM, LPARAM lparam)
{
    static_assert(sizeof(LRESULT) >= sizeof(HBRUSH));
    HWND btn_handle = (HWND)lparam;
    const int btn_id = ::GetDlgCtrlID(btn_handle);

    // NOTE: the auto-combo config sets item-data to the integral representation of that option's enum value
    const LRESULT ui_index = SendDlgItemMessage(IDC_BACKGROUND_FILL_TYPE, CB_GETCURSEL, 0, 0);
    const LRESULT logical_value = SendDlgItemMessage(IDC_BACKGROUND_FILL_TYPE, CB_GETITEMDATA, ui_index, 0);
    assert(logical_value != CB_ERR);
    const BackgroundFillType fill_type = static_cast<BackgroundFillType>(logical_value);

    switch(btn_id)
    {
        case IDC_BACKGROUND_COLOUR:
        {
            if(fill_type == BackgroundFillType::SolidColour)
            {
                return (LRESULT)cfg_background_colour.get_brush_handle();
            }
        }
        break;

        case IDC_BACKGROUND_GRADIENT_TL:
        {
            if(fill_type == BackgroundFillType::Gradient)
            {
                return (LRESULT)cfg_background_gradient_TL.get_brush_handle();
            }
        }
        break;

        case IDC_BACKGROUND_GRADIENT_TR:
        {
            if(fill_type == BackgroundFillType::Gradient)
            {
                return (LRESULT)cfg_background_gradient_TR.get_brush_handle();
            }
        }
        break;

        case IDC_BACKGROUND_GRADIENT_BL:
        {
            if(fill_type == BackgroundFillType::Gradient)
            {
                return (LRESULT)cfg_background_gradient_BL.get_brush_handle();
            }
        }
        break;

        case IDC_BACKGROUND_GRADIENT_BR:
        {
            if(fill_type == BackgroundFillType::Gradient)
            {
                return (LRESULT)cfg_background_gradient_BR.get_brush_handle();
            }
        }
        break;
    }

    return FALSE;
}

void PreferencesDisplayBg::RepaintColours()
{
    int ids_to_repaint[] = { IDC_BACKGROUND_COLOUR,
                             IDC_BACKGROUND_GRADIENT_TL,
                             IDC_BACKGROUND_GRADIENT_TR,
                             IDC_BACKGROUND_GRADIENT_BL,
                             IDC_BACKGROUND_GRADIENT_BR };
    for(int id : ids_to_repaint)
    {
        CWindow handle = GetDlgItem(id);
        if(handle != nullptr)
        {
            handle.RedrawWindow(nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
        }
    }
}

void PreferencesDisplayBg::SetImageFieldsEnabled()
{
    // NOTE: the auto-combo config sets item-data to the integral representation of that option's enum value
    const LRESULT ui_index = SendDlgItemMessage(IDC_BACKGROUND_IMAGE_TYPE, CB_GETCURSEL, 0, 0);
    const LRESULT logical_value = SendDlgItemMessage(IDC_BACKGROUND_IMAGE_TYPE, CB_GETITEMDATA, ui_index, 0);
    assert(logical_value != CB_ERR);
    const BackgroundImageType img_type = static_cast<BackgroundImageType>(logical_value);

    const bool has_img = (img_type != BackgroundImageType::None);
    const bool is_img_custom = (img_type == BackgroundImageType::CustomImage);

    GetDlgItem(IDC_BACKGROUND_MAINTAIN_IMG_ASPECT_RATIO).EnableWindow(has_img);
    GetDlgItem(IDC_BACKGROUND_BLUR_SLIDER).EnableWindow(has_img);
    GetDlgItem(IDC_BACKGROUND_IMG_OPACITY).EnableWindow(has_img);
    GetDlgItem(IDC_BACKGROUND_CUSTOM_IMG_PATH).EnableWindow(is_img_custom);
    GetDlgItem(IDC_BACKGROUND_CUSTOM_IMG_BROWSE).EnableWindow(is_img_custom);
}

class PreferencesDisplayBgImpl : public preferences_page_impl<PreferencesDisplayBg>
{
public:
    const char* get_name() final
    {
        return "Background";
    }
    GUID get_guid() final
    {
        return GUID_PREFERENCES_PAGE_DISPLAY_BACKGROUND;
    }
    GUID get_parent_guid() final
    {
        return GUID_PREFERENCES_PAGE_ROOT;
    }
};
static preferences_page_factory_t<PreferencesDisplayBgImpl> g_preferences_page_display_factory;
