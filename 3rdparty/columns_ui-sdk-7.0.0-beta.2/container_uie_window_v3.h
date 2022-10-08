#pragma once

namespace uie {

/**
 * A base implementation of uie::window using uie::container_window_v3
 */
template <class Base = window>
class container_uie_window_v3_t : public Base {
public:
    /**
     * Get window and window class styles, names and other parameters.
     */
    [[nodiscard]] virtual container_window_v3_config get_window_config() = 0;
    [[nodiscard]] virtual LRESULT on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) = 0;

    bool is_available(const window_host_ptr& p) const override { return true; }
    const window_host_ptr& get_host() const { return m_host; }

    [[nodiscard]] HWND get_wnd() const final { return m_window ? m_window->get_wnd() : nullptr; }

    HWND create_or_transfer_window(
        HWND parent, const window_host_ptr& host, const ui_helpers::window_position_t& position) final
    {
        if (get_wnd()) {
            ShowWindow(get_wnd(), SW_HIDE);
            SetParent(get_wnd(), parent);
            m_host->relinquish_ownership(get_wnd());
            m_host = host;

            SetWindowPos(get_wnd(), nullptr, position.x, position.y, position.cx, position.cy, SWP_NOZORDER);
        } else {
            m_host = host;
            m_window = std::make_unique<container_window_v3>(get_window_config(),
                [this, ptr = this](auto&&... args) { return on_message(std::forward<decltype(args)>(args)...); });
            m_window->create(parent, position.x, position.y, position.cx, position.cy);
        }

        return get_wnd();
    }

    void destroy_window() final
    {
        m_window->destroy();
        m_window.reset();
        m_host.release();
    }

private:
    std::unique_ptr<container_window_v3> m_window;
    window_host_ptr m_host;
};

using container_uie_window_v3 = container_uie_window_v3_t<>;

} // namespace uie
