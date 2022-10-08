#include "ui_extension.h"

namespace uie {

HWND container_window_v3::create(HWND wnd_parent, int x, int y, int cx, int cy)
{
    if (!s_registered_classes.contains(m_config.class_name)) {
        register_class();
    }

    m_wnd = CreateWindowEx(m_config.extended_window_styles, m_config.class_name, m_config.window_title,
        m_config.window_styles, x, y, cx, cy, wnd_parent, nullptr, core_api::get_my_instance(), this);

    if (!m_wnd) {
        pfc::string8 message;
        uFormatMessage(GetLastError(), message);

        console::formatter formatter;
        formatter << "CreateWindowEx failed: " << message;
    }

    return m_wnd;
}

HWND container_window_v3::create(HWND wnd_parent)
{
    return create(wnd_parent, 0, 0, 0, 0);
}

void container_window_v3::destroy() const
{
    DestroyWindow(m_wnd);
    if (s_window_count[m_config.class_name] == 0) {
        deregister_class();
    }
}

LRESULT container_window_v3::s_on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    container_window_v3* self{};

    if (msg == WM_NCCREATE) {
        const auto lpcs = reinterpret_cast<LPCREATESTRUCT>(lp);
        self = static_cast<container_window_v3*>(lpcs->lpCreateParams);
        SetWindowLongPtr(wnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(self));
    } else
        self = reinterpret_cast<container_window_v3*>(GetWindowLongPtr(wnd, GWLP_USERDATA));

    if (msg == WM_NCDESTROY)
        SetWindowLongPtr(wnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(nullptr));

    return self ? self->on_message(wnd, msg, wp, lp) : DefWindowProc(wnd, msg, wp, lp);
}

LRESULT container_window_v3::on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_NCCREATE: {
        m_wnd = wnd;
        const auto [item, _] = s_window_count.try_emplace(m_config.class_name, 0);
        ++item->second;
        break;
    }
    case WM_NCDESTROY:
        --s_window_count[m_config.class_name];
        m_wnd = nullptr;
        break;
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_TIMECHANGE:
        if (msg != WM_SETTINGCHANGE || m_config.forward_wm_settingchange)
            win32_helpers::send_message_to_direct_children(wnd, msg, wp, lp);
        break;
    case WM_ERASEBKGND:
        if (m_config.use_transparent_background) {
            return win32::paint_background_using_parent(wnd, reinterpret_cast<HDC>(wp), false);
        }
        break;
    case WM_PRINTCLIENT:
        if (m_config.use_transparent_background && (lp & PRF_ERASEBKGND)) {
            return win32::paint_background_using_parent(wnd, reinterpret_cast<HDC>(wp), true);
        }
        break;
    case WM_WINDOWPOSCHANGED:
        if (m_config.use_transparent_background || m_config.invalidate_children_on_move_or_resize) {
            const auto lpwp = reinterpret_cast<LPWINDOWPOS>(lp);

            if (!(lpwp->flags & SWP_NOSIZE) || !(lpwp->flags & SWP_NOMOVE) || (lpwp->flags & SWP_FRAMECHANGED)) {
                auto flags = RDW_ERASE | RDW_INVALIDATE;

                if (m_config.invalidate_children_on_move_or_resize)
                    flags |= RDW_ALLCHILDREN;

                RedrawWindow(wnd, nullptr, nullptr, flags);
            }
        }
        break;
    }

    return m_on_message ? m_on_message(wnd, msg, wp, lp) : DefWindowProc(wnd, msg, wp, lp);
}

void container_window_v3::register_class() const
{
    auto wc = WNDCLASS{};

    wc.lpfnWndProc = &s_on_message;
    wc.hInstance = core_api::get_my_instance();
    wc.hCursor = LoadCursor(nullptr, m_config.class_cursor);
    wc.hbrBackground = m_config.class_background;
    wc.lpszClassName = m_config.class_name;
    wc.style = m_config.class_styles;
    wc.cbWndExtra = m_config.class_extra_wnd_bytes;

    if (!RegisterClass(&wc)) {
        pfc::string8 message;
        uFormatMessage(GetLastError(), message);

        console::formatter formatter;
        formatter << "RegisterClass failed: " << message;
    }

    s_registered_classes.insert(m_config.class_name);
}

void container_window_v3::deregister_class() const
{
    const auto deregistered = UnregisterClass(m_config.class_name, core_api::get_my_instance());

    if (deregistered) {
        s_registered_classes.erase(m_config.class_name);
    } else {
        pfc::string8 message;
        uFormatMessage(GetLastError(), message);

        console::formatter formatter;
        formatter << "UnregisterClass failed: " << message;
    }
}

} // namespace uie
