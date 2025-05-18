#include "stdafx.h"

#pragma warning(push, 0)
#include <foobar2000/SDK/metadb_info_container_impl.h>
#include <foobar2000/helpers/BumpableElem.h>
#include <libPPUI/win32_op.h>
#pragma warning(pop)

#include "logging.h"
#include "ui_hooks.h"
#include "ui_lyrics_panel.h"
#include "uie_shim_panel.h"

namespace
{
    // clang-format off: GUIDs should be one line
    static const GUID GUID_LYRICS_PANEL = { 0x6e24d0be, 0xad68, 0x4bc9,{ 0xa0, 0x62, 0x2e, 0xc7, 0xb3, 0x53, 0xd5, 0xbd } };
    // clang-format on

    // There is (as far as I'm aware) no way to access fb2k's default fonts & colours from outside
    // a ui_element instance. We need those values elsewhere though, so that we can do things like
    // set the editor dialog to use the correct font (even though the editor dialog is not itself
    // a UI element instance) or use the default colours in the preferences code to compute the
    // correct values if we need to blend from a custom colour to a default colour.
    // To achieve this we cache the UI-provided defaults here, in the lyric panel.  Technically
    // this means that if the user does not have an instance of the OpenLyrics display panel on
    // their UI and they open the lyric editor, then they'll get the default font regardless of
    // fb2k's config. If the fb2k SDK at some point adds the ability to get the font config from
    // anywhere then these variables (and the restriction above) can be removed.
    static t_ui_font g_defaultui_default_font = nullptr;
    static t_ui_font g_defaultui_console_font = nullptr;
    static t_ui_color g_defaultui_background_colour = {};
    static t_ui_color g_defaultui_text_colour = {};
    static t_ui_color g_defaultui_highlight_colour = {};

    class LyricPanelUiElement : public ui_element_instance, public LyricPanel
    {
    public:
        LyricPanelUiElement(ui_element_config::ptr, ui_element_instance_callback_ptr p_callback);

        HWND get_wnd() override;
        void initialize_window(HWND parent);
        void set_configuration(ui_element_config::ptr config) override;
        ui_element_config::ptr get_configuration() override;
        void ui_fonts_changed() override;
        void ui_colors_changed() override;

        bool is_panel_ui_in_edit_mode() override;

        static GUID g_get_guid();
        static GUID g_get_subclass();
        static void g_get_name(pfc::string_base& out);
        static ui_element_config::ptr g_get_default_configuration();
        static const char* g_get_description();

    private:
        ui_element_config::ptr m_config;

        void recompute_fonts();
        void recompute_colours();

    protected:
        // this must be declared as protected for ui_element_impl_withpopup<> to work.
        const ui_element_instance_callback_ptr m_callback;
    };

    HWND LyricPanelUiElement::get_wnd()
    {
        return *this;
    }
    void LyricPanelUiElement::set_configuration(ui_element_config::ptr config)
    {
        m_config = config;
    }
    ui_element_config::ptr LyricPanelUiElement::get_configuration()
    {
        return m_config;
    }

    GUID LyricPanelUiElement::g_get_guid()
    {
        return GUID_LYRICS_PANEL;
    }
    GUID LyricPanelUiElement::g_get_subclass()
    {
        return ui_element_subclass_utility;
    }
    ui_element_config::ptr LyricPanelUiElement::g_get_default_configuration()
    {
        return ui_element_config::g_create_empty(g_get_guid());
    }
    void LyricPanelUiElement::g_get_name(pfc::string_base& out)
    {
        out = "OpenLyrics Panel";
    }
    const char* LyricPanelUiElement::g_get_description()
    {
        return "Displays lyrics for the currently-playing track.";
    }

    LyricPanelUiElement::LyricPanelUiElement(ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback)
        : m_config(config)
        , m_callback(p_callback)
    {
        recompute_fonts();
        recompute_colours();
    }

    // Called from the fb2k-helper's atl-misc
    void LyricPanelUiElement::initialize_window(HWND parent)
    {
        const _U_RECT rect = nullptr;
        const TCHAR* window_name = nullptr;
        const DWORD style =
            WS_CHILD | /*WS_VISIBLE |*/ WS_CLIPCHILDREN
            | WS_CLIPSIBLINGS; // Copied from atlwin.h's CControlWinTraits, used because we're implement CWindowImpl<>

        // NOTE: We specifically need to exclude the WS_VISIBLE style (which causes the window
        //       to be created already-visible) because including it results in ColumnsUI
        //       logging a warning that the "panel was visible on creation".
        //       It would seem that even without this style and without us making the panel
        //       visible after creation, fb2k does this for us already.
        //       See: https://github.com/jacquesh/foo_openlyrics/issues/132

        WIN32_OP(Create(parent, rect, window_name, style) != NULL)
    }

    void LyricPanelUiElement::ui_fonts_changed()
    {
        // If the font changed then the previously-stored font handle will now be invalid, so we
        // need to re-store the (possibly new) font handle to avoid getting the default font.
        recompute_fonts();

        LyricPanel::ui_fonts_changed();

        // Repaint with the new fonts
        Invalidate();
    }

    void LyricPanelUiElement::ui_colors_changed()
    {
        // This callback executes when the fb2k UI colour config is changed (including toggling dark mode).
        recompute_colours();

        LyricPanel::ui_colors_changed();

        // Repaint with the new colours
        Invalidate();
    }

    bool LyricPanelUiElement::is_panel_ui_in_edit_mode()
    {
        return m_callback->is_edit_mode_enabled();
    }

    void LyricPanelUiElement::recompute_fonts()
    {
        core_api::ensure_main_thread();
        g_defaultui_default_font = m_callback->query_font_ex(ui_font_default);
        g_defaultui_console_font = m_callback->query_font_ex(ui_font_console);
    }

    void LyricPanelUiElement::recompute_colours()
    {
        core_api::ensure_main_thread();
        g_defaultui_background_colour = m_callback->query_std_color(ui_color_background);
        g_defaultui_text_colour = m_callback->query_std_color(ui_color_text);
        g_defaultui_highlight_colour = m_callback->query_std_color(ui_color_highlight);
    }

    // ui_element_impl_withpopup autogenerates standalone version of our component and proper menu commands.
    // Use ui_element_impl instead if you don't want that.
    class LyricPanelImpl : public ui_element_impl_withpopup<LyricPanelUiElement>
    {
    };
    FB2K_SERVICE_FACTORY(LyricPanelImpl)
    UIE_SHIM_PANEL_FACTORY(LyricPanelUiElement)

} // namespace

t_ui_font defaultui::default_font()
{
    return g_defaultui_default_font;
}

t_ui_font defaultui::console_font()
{
    return g_defaultui_console_font;
}

t_ui_color defaultui::background_colour()
{
    return g_defaultui_background_colour;
}

t_ui_color defaultui::text_colour()
{
    return g_defaultui_text_colour;
}

t_ui_color defaultui::highlight_colour()
{
    return g_defaultui_highlight_colour;
}
