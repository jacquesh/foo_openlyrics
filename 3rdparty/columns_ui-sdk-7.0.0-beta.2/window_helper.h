#ifndef _UI_EXTENSION_WINDOW_H_
#define _UI_EXTENSION_WINDOW_H_

/**
 * \file window_helper.h
 * \brief Helpers to write a custom window class
 * \author musicmusic
 */

#define __implement_get_class_data(class_name, want_transparent_background)                                            \
    static ui_helpers::container_window::class_data my_class_data                                                      \
        = {class_name, _T(""), 0, false, want_transparent_background, 0, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, \
            WS_EX_CONTROLPARENT, 0, true, true, true, IDC_ARROW};                                                      \
    return my_class_data

#define __implement_get_class_data_child_ex(class_name, want_transparent_background, forward_system_setting_change) \
    static ui_helpers::container_window::class_data my_class_data = {class_name, _T(""), 0, false,                  \
        want_transparent_background, 0, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT, 0,       \
        forward_system_setting_change, forward_system_setting_change, forward_system_setting_change, IDC_ARROW};    \
    return my_class_data

#define __implement_get_class_data_child_ex2(                                                                 \
    class_name, want_transparent_background, forward_system_setting_change, cursor)                           \
    static ui_helpers::container_window::class_data my_class_data = {class_name, _T(""), 0, false,            \
        want_transparent_background, 0, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT, 0, \
        forward_system_setting_change, forward_system_setting_change, forward_system_setting_change, cursor}; \
    return my_class_data

#define __implement_get_class_data_child_ex3(                                                                          \
    class_name, want_transparent_background, forward_system_setting_change, classstyles, cursor)                       \
    static ui_helpers::container_window::class_data my_class_data                                                      \
        = {class_name, _T(""), 0, false, want_transparent_background, 0, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, \
            WS_EX_CONTROLPARENT, classstyles, forward_system_setting_change, forward_system_setting_change,            \
            forward_system_setting_change, cursor};                                                                    \
    return my_class_data

#define __implement_get_class_data_ex(                                                                             \
    class_name, window_title, want_transparent_background, extra_wnd_bytes, styles, stylesex, classstyles)         \
    static ui_helpers::container_window::class_data my_class_data = {class_name, window_title, 0, false,           \
        want_transparent_background, extra_wnd_bytes, styles, stylesex, classstyles, true, true, true, IDC_ARROW}; \
    return my_class_data

#define __implement_get_class_data_ex2(class_name, window_title, want_transparent_background,                       \
    forward_system_setting_change, extra_wnd_bytes, styles, stylesex, classstyles, cursor)                          \
    static ui_helpers::container_window::class_data my_class_data = {class_name, window_title, 0, false,            \
        want_transparent_background, extra_wnd_bytes, styles, stylesex, classstyles, forward_system_setting_change, \
        forward_system_setting_change, forward_system_setting_change, cursor};                                      \
    return my_class_data
namespace ui_helpers {

/** \brief Implements a window that serves either as an empty container for either other windows, or as a custom control
 */
class [[deprecated("Use uie::container_window_v3")]] container_window
{
private:
    HWND wnd_host;
    container_window(const container_window& p_source) = delete;

public:
    struct class_data {
        LPCTSTR class_name;
        LPCTSTR window_title;
        long refcount;
        bool class_registered;
        bool want_transparent_background;
        int extra_wnd_bytes;
        DWORD styles;
        DWORD ex_styles;
        UINT class_styles;
        bool forward_system_settings_change;
        bool forward_system_colours_change;
        bool forward_system_time_change;
        LPWSTR cursor;
    };

    /**
     * Gets window class data.
     *
     * \return                    Reference to class_data
     *
     * \par Sample implementation:
     * \code
     * virtual class_data & get_class_data() const
     * {
     * __implement_get_class_data(
     * "My Window Class", //window class name
     * true); //want transparent background (i.e. for toolbar controls)
     * }
     * \endcode
     * \see __implement_get_class_data, __implement_get_class_data_ex
     */
    virtual class_data& get_class_data() const = 0;

    container_window();

    HWND create(HWND wnd_parent, LPVOID create_param = 0,
        const ui_helpers::window_position_t& p_window_position = ui_helpers::window_position_null);

    HWND create_in_dialog_units(
        HWND wnd_dialog, const ui_helpers::window_position_t& p_window_position, LPVOID create_param = NULL);

    bool ensure_class_registered();
    bool class_release();

    void destroy(); // if destroying someother way, you should make sure you call class_release() after destroying the
                    // window (e.g. call it on app-shutdown)

    static LRESULT WINAPI window_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

    HWND get_wnd() const;

    // override me
    // you won't get called for WM_ERASEBKGRND if you specify want_transparent_background
    virtual LRESULT on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) = 0;
};

/** \brief    Implements a container_window that cleans up on quit.
 *            If you use this, you need to call destroy() in your WM_CLOSE handler (and be sure not to call
 * DefWindowProc)
 */
#pragma warning(suppress : 4996)
class [[deprecated("Use uie::container_window_v3")]] container_window_release_t : public container_window
{
public:
    class initquit_t : public initquit {
    public:
        void register_window(container_window_release_t& ptr) { m_windows.add_item(&ptr); }
        void deregister_window(container_window_release_t& ptr) { m_windows.remove_item(&ptr); }

    private:
        void on_quit() override
        {
            t_size i = m_windows.get_count();
            for (; i; i--) {
#pragma warning(suppress : 4996)
                if (m_windows[i - 1]->get_wnd())
#pragma warning(suppress : 4996)
                    SendMessage(m_windows[i - 1]->get_wnd(), WM_CLOSE, 0, 0);
            }
        }
#pragma warning(suppress : 4996)
        pfc::list_t<container_window_release_t*> m_windows;
    };

    void register_initquit();
    void deregister_initquit();

public:
};

#pragma warning(suppress : 4996)
class [[deprecated("Use uie::container_window_v3")]] container_window_autorelease_t : public container_window_release_t
{
public:
    container_window_autorelease_t();
    ~container_window_autorelease_t();
};

}; // namespace ui_helpers

// for multiple instance extensions only

namespace uie {

/** \brief Wraps ui_helpers::container_window into a panel */
template <class W = ui_helpers::container_window, class T = window>
class [[deprecated("Use uie::container_uie_window_v3_t")]] container_ui_extension_t
    : public W
    , public T
{
    window_host_ptr p_host;

public:
    HWND create_or_transfer_window(
        HWND parent, const window_host_ptr& host, const ui_helpers::window_position_t& p_position)
    {
        if (W::get_wnd()) {
            ShowWindow(W::get_wnd(), SW_HIDE);
            SetParent(W::get_wnd(), parent);
            p_host->relinquish_ownership(W::get_wnd());
            p_host = host;

            SetWindowPos(get_wnd(), NULL, p_position.x, p_position.y, p_position.cx, p_position.cy, SWP_NOZORDER);
        } else {
            p_host = host; // store interface to host
            this->create(parent, get_create_param(), p_position);
        }

        return W::get_wnd();
    }
    virtual void destroy_window()
    {
        this->destroy();
        p_host.release();
    }

    virtual bool is_available(const window_host_ptr& p) const { return true; }
    const window_host_ptr& get_host() const { return p_host; }
    virtual HWND get_wnd() const { return W::get_wnd(); }

    // lpCreateParams in CREATESTRUCT struct in WM_NCCREATE/WM_CREATE is a pointer to an array of LPVOIDs. This is the
    // second LPVOID in the array
    virtual LPVOID get_create_param() { return this; }
};

[[deprecated(
#pragma warning(suppress : 4996)
    "Use uie::container_uie_window_v3")]] typedef container_ui_extension_t<ui_helpers::container_window, uie::window>
    container_ui_extension;

#pragma warning(suppress : 4996)
[[deprecated("Use uie::container_uie_window_v3_t")]] typedef container_ui_extension_t<ui_helpers::container_window,
    uie::menu_window>
    container_menu_ui_extension;

#if _MSC_VER >= 1800
template <class base_window_class = uie::window>
using container_uie_window_t [[deprecated("Use uie::container_uie_window_v3_t")]]
#pragma warning(suppress : 4996)
= container_ui_extension_t<ui_helpers::container_window, base_window_class>;
#endif

}; // namespace uie

#endif
