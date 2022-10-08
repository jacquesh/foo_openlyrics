#pragma once

namespace uie {

/**
 * \brief Window and window class styles, names and other parameters
 * for a container_window_v3.
 */
struct container_window_v3_config {
    const wchar_t* class_name{};

    /**
     * Whether to use the parent window's background for this window.
     *
     * If true, on_message() will not be called when the WM_ERASEBKGND message is received.
     * The window (but not its children) will also be invalidated on resize or move.
     *
     * You can also set this to false and use uie::win32::paint_background_using_parent()
     * in your on_message() implementation for more flexibility.
     *
     * If set to false, you should ensure a background is painted for this window.
     */
    bool use_transparent_background{true};
    bool invalidate_children_on_move_or_resize{};

    /**
     * Whether to forward WM_SETTINGCHANGE messages to direct child windows.
     *
     * This should be set to false if a toolbar control is a direct child window,
     * as they can misbehave when handling WM_SETTINGCHANGE.
     */
    bool forward_wm_settingchange{true};
    unsigned window_styles{WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS};
    unsigned extended_window_styles{WS_EX_CONTROLPARENT};
    unsigned class_styles{};
    LPWSTR class_cursor{IDC_ARROW};
    HBRUSH class_background{};
    const wchar_t* window_title{L""};
    int class_extra_wnd_bytes{};

    container_window_v3_config(
        const wchar_t* class_name, bool use_transparent_background = true, unsigned class_styles = 0)
        : class_name(class_name)
        , use_transparent_background(use_transparent_background)
        , class_styles(class_styles)
    {
    }
};

/**
 * \brief Implements a window that serves either as an empty container for other windows, or as window for a custom
 * control.
 */
class container_window_v3 {
public:
    container_window_v3(container_window_v3_config config,
        std::function<LRESULT(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)> on_message = nullptr)
        : m_config{config}
        , m_on_message{std::move(on_message)}
    {
    }

    container_window_v3(const container_window_v3& p_source) = delete;
    container_window_v3& operator=(const container_window_v3& p_source) = delete;

    HWND create(HWND wnd_parent, int x, int y, int cx, int cy);
    HWND create(HWND wnd_parent);

    /**
     * Destroy the window.
     *
     * If this is the last instance of this window class, the window class will also be deregistered.
     */
    void destroy() const;

    [[nodiscard]] HWND get_wnd() const { return m_wnd; }

    /**
     * Deregister the window class.
     *
     * If not using #destroy() to destryoy the window, call this to deregister the window class when
     * all windows belonging to the class have ben destroyed.
     */
    void deregister_class() const;

private:
    static LRESULT WINAPI s_on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

    LRESULT on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
    void register_class() const;

    inline static std::unordered_map<std::wstring, size_t> s_window_count;
    inline static std::unordered_set<std::wstring> s_registered_classes;

    HWND m_wnd{};
    container_window_v3_config m_config;
    std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> m_on_message;
};

} // namespace uie
