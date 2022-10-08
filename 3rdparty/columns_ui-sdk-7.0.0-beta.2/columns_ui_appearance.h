#ifndef _COLUMNS_UI_APPEARANCE_
#define _COLUMNS_UI_APPEARANCE_

namespace cui {
namespace colours {
enum colour_identifier_t {
    colour_text,
    colour_selection_text,
    colour_inactive_selection_text,
    colour_background,
    colour_selection_background,
    colour_inactive_selection_background,
    colour_active_item_frame,
    /** Reserved */
    colour_group_foreground,
    /** Reserved */
    colour_group_background,
};

enum colour_flag_t {
    colour_flag_text = 1 << colour_text,
    colour_flag_selection_text = 1 << colour_selection_text,
    colour_flag_inactive_selection_text = 1 << colour_inactive_selection_text,
    colour_flag_background = 1 << colour_background,
    colour_flag_selection_background = 1 << colour_selection_background,
    colour_flag_inactive_selection_background = 1 << colour_inactive_selection_background,
    colour_flag_active_item_frame = 1 << colour_active_item_frame,
    colour_flag_group_foreground = 1 << colour_group_foreground,
    colour_flag_group_background = 1 << colour_group_background,

    colour_flag_all = (1 << colour_text) | (1 << colour_selection_text) | (1 << colour_inactive_selection_text)
        | (1 << colour_background) | (1 << colour_selection_background) | (1 << colour_inactive_selection_background)
        | (1 << colour_active_item_frame) | (1 << colour_group_foreground) | (1 << colour_group_background),

};

enum bool_identifier_t {
    bool_use_custom_active_item_frame,
    /**
     * Implemented in Columns UI 2.0. Always false on older versions.
     *
     * \see helper for more details
     */
    bool_dark_mode_enabled,
};

enum bool_flag_t {
    bool_flag_use_custom_active_item_frame = (1 << bool_use_custom_active_item_frame),
    bool_flag_dark_mode_enabled = (1 << bool_dark_mode_enabled),
};

static COLORREF g_get_system_color(const colour_identifier_t p_identifier)
{
    switch (p_identifier) {
    case colour_text:
        return GetSysColor(COLOR_WINDOWTEXT);
    case colour_selection_text:
        return GetSysColor(COLOR_HIGHLIGHTTEXT);
    case colour_background:
        return GetSysColor(COLOR_WINDOW);
    case colour_selection_background:
        return GetSysColor(COLOR_HIGHLIGHT);
    case colour_inactive_selection_text:
        return GetSysColor(COLOR_BTNTEXT);
    case colour_inactive_selection_background:
        return GetSysColor(COLOR_BTNFACE);
    case colour_active_item_frame:
        return GetSysColor(COLOR_WINDOWFRAME);
    default:
        return 0;
    }
}

/** One implementation in Columns UI - do not reimplement! */
class NOVTABLE manager_instance : public service_base {
public:
    /** \brief Get the specified colour */
    virtual COLORREF get_colour(const colour_identifier_t& p_identifier) const = 0;
    /** \brief Get the specified colour */
    virtual bool get_bool(const bool_identifier_t& p_identifier) const = 0;

    /**
     * Only returns true if your client::get_themes_supported() method does.
     * Indicates selected items should be drawn using Theme API.
     */
    virtual bool get_themed() const = 0;

    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(manager_instance);
};

/**
 * Use this class if you wish to use the global colours only rather than implementing the client class
 */
class NOVTABLE common_callback {
public:
    virtual void on_colour_changed(uint32_t changed_items_mask) const = 0;
    virtual void on_bool_changed(uint32_t changed_items_mask) const = 0;
};

/**
 * One implementation in Columns UI - do not reimplement!
 *
 * It is not recommended to use this class directly - use the helper class instead.
 */
class NOVTABLE manager : public service_base {
public:
    /** \brief Creates a manager_instance for the given client (null GUID implies global settings). */
    virtual void create_instance(const GUID& p_client_guid, cui::colours::manager_instance::ptr& p_out) = 0;

    virtual void register_common_callback(common_callback* p_callback){};
    virtual void deregister_common_callback(common_callback* p_callback){};

    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(manager);
};

/** Helper to simplify retrieving colours. */
class helper {
public:
    COLORREF get_colour(const colour_identifier_t& p_identifier) const
    {
        if (m_api.is_valid()) {
            return m_api->get_colour(p_identifier);
        }
        return g_get_system_color(p_identifier);
    }

    bool get_bool(const bool_identifier_t& p_identifier) const
    {
        if (m_api.is_valid()) {
            return m_api->get_bool(p_identifier);
        }
        return false;
    }

    bool get_themed() const
    {
        if (m_api.is_valid()) {
            return m_api->get_themed();
        }
        return false;
    }

    /**
     * \brief Get whether the UI-wide dark mode is currently active.
     *
     * Implemented in Columns UI 2.0. Always false on older versions.
     *
     * There is only one global value of this flag; it does not vary between colour clients.
     *
     * If your window contains a scroll bar, you should call SetWindowTheme based on the value
     * of this flag as follows:
     *
     * \code{.cpp}
     * const auto dark_mode_active = cui::colours::is_dark_mode_active().
     * SetWindowTheme(wnd, dark_mode_active ? L"DarkMode_Explorer" : nullptr, nullptr);
     * \endcode
     *
     * You should also do this when the client::on_bool_changed() method of your client is
     * called with the #bool_flag_dark_mode_enabled bit set.
     */
    bool is_dark_mode_active() const
    {
        if (m_api.is_valid()) {
            return m_api->get_bool(bool_dark_mode_enabled);
        }
        return false;
    }

    /** You can omit guid for the global colours */
    helper(GUID guid = GUID{})
    {
        try {
            manager::ptr api;
            standard_api_create_t(api);
            api->create_instance(guid, m_api);
        } catch (const exception_service_not_found&) {
        }
    }

private:
    service_ptr_t<manager_instance> m_api;
};

/**
 * \brief Get whether the UI-wide dark mode is currently active.
 *
 * Convenience method to avoid having to instantiate a helper instance.
 *
 * \see helper::is_dark_mode_active() for more details.
 */
bool is_dark_mode_active();

class NOVTABLE client : public service_base {
public:
    virtual const GUID& get_client_guid() const = 0;
    virtual void get_name(pfc::string_base& p_out) const = 0;

    virtual uint32_t get_supported_colours() const
    {
        return cui::colours::colour_flag_all
            & ~(cui::colours::colour_flag_group_foreground | cui::colours::colour_flag_group_background);
    }

    /**
     * Return a combination of bool_flag_t to indicate which boolean flags are supported.
     *
     * If dark mode is supported by your panel, you should set the #bool_flag_dark_mode_enabled bit.
     */
    virtual uint32_t get_supported_bools() const = 0; // bit-mask
    /** Indicates whether you are Theme API aware and can draw selected items using Theme API */
    virtual bool get_themes_supported() const = 0;

    virtual void on_colour_changed(uint32_t changed_items_mask) const = 0;

    /**
     * Called whenever a supported boolean flag changes. Support for a flag is determined using the
     * get_supported_bools() method.
     *
     * \param [in] changed_items_mask  a combination of bool_flag_t indicating the flags that have
     *                                 changed. (Only indicates which flags have changed, not the
     *                                 new values.)
     *
     * \note Only #bool_flag_dark_mode_enabled is currently supported. Ensure you inspect
     * <code>changed_items_mask</code> to check which flags have changed.
     *
     * Example implementation:
     *
     * \code{.cpp}
     * void on_bool_changed(uint32_t changed_items_mask) const override
     * {
     *     if (changed_items_mask & colours::bool_flag_dark_mode_enabled) {
     *         const auto is_dark = cui::colours::is_dark_mode_active();
     *         // Handle dark mode change
     *     }
     * }
     * \endcode
     */
    virtual void on_bool_changed(uint32_t changed_items_mask) const = 0;

    template <class tClass>
    class factory : public service_factory_t<tClass> {
    };

    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(client);
};

/**
 * Helper for receiving notifications when the global dark mode status changes.
 *
 * This is mainly used by non-panel parts of the UI. Panels would normally receive
 * this notification through the on_bool_changed method of their client instance.
 */
class dark_mode_notifier : common_callback {
public:
    dark_mode_notifier(std::function<void()> callback) : m_callback(std::move(callback))
    {
        if (fb2k::std_api_try_get(m_api)) {
            m_api->register_common_callback(this);
        }
    }

    ~dark_mode_notifier()
    {
        if (m_api.is_valid()) {
            m_api->deregister_common_callback(this);
        }
    }

    void on_colour_changed(uint32_t changed_items_mask) const override {}
    void on_bool_changed(uint32_t changed_items_mask) const override
    {
        if (changed_items_mask & bool_flag_dark_mode_enabled)
            m_callback();
    }

private:
    std::function<void()> m_callback;
    manager::ptr m_api;
};

} // namespace colours

namespace fonts {
enum font_mode_t {
    font_mode_common_items,
    font_mode_common_labels,
    font_mode_custom,
    font_mode_system,
};

enum font_type_t {
    font_type_items,
    font_type_labels,
};
enum font_type_flag_t {
    font_type_flag_items = (1 << 0),
    font_type_flag_labels = (1 << 1),
};

/**
 * Use this class if you wish to use the common fonts rather than implementing client
 */
class NOVTABLE common_callback {
public:
    virtual void on_font_changed(uint32_t changed_items_mask) const = 0;
};

/** One implementation in Columns UI - do not reimplement! */
class NOVTABLE manager : public service_base {
public:
    /** \brief Retrieves the font for the given client */
    virtual void get_font(const GUID& p_guid, LOGFONT& p_out) const = 0;

    /** \brief Retrieves common fonts. */
    virtual void get_font(const font_type_t p_type, LOGFONT& p_out) const = 0;
    /** \brief Sets your font as 'Custom' and to p_font */
    virtual void set_font(const GUID& p_guid, const LOGFONT& p_font) = 0;

    virtual void register_common_callback(common_callback* p_callback) = 0;
    virtual void deregister_common_callback(common_callback* p_callback) = 0;

    /** Helper */
    HFONT get_font(const GUID& p_guid) const
    {
        LOGFONT lf;
        memset(&lf, 0, sizeof(LOGFONT));
        get_font(p_guid, lf);
        return CreateFontIndirect(&lf);
    }

    /** Helper */
    HFONT get_font(const font_type_t p_type) const
    {
        LOGFONT lf;
        memset(&lf, 0, sizeof(LOGFONT));
        get_font(p_type, lf);
        return CreateFontIndirect(&lf);
    }

    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(manager);
};

/**
 * Experimental version of the font management API with custom DPI support.
 *
 * One implementation in Columns UI - do not reimplement!
 */
class NOVTABLE manager_v2 : public service_base {
public:
    /** \brief Retrieve the font for the given client */
    virtual LOGFONT get_client_font(GUID guid, unsigned dpi = USER_DEFAULT_SCREEN_DPI) const = 0;

    /** \brief Retrieve a common font. */
    virtual LOGFONT get_common_font(font_type_t type, unsigned dpi = USER_DEFAULT_SCREEN_DPI) const = 0;

    /** \brief Set your font as 'Custom' and to the specified font. */
    virtual void set_client_font(GUID guid, const LOGFONT& font, int point_size_tenths) = 0;

    virtual void register_common_callback(common_callback* callback) = 0;
    virtual void deregister_common_callback(common_callback* callback) = 0;

    HFONT get_client_font_handle(GUID guid, unsigned dpi = USER_DEFAULT_SCREEN_DPI) const
    {
        LOGFONT lf = get_client_font(guid, dpi);
        return CreateFontIndirect(&lf);
    }

    HFONT get_common_font_handle(const font_type_t type, unsigned dpi = USER_DEFAULT_SCREEN_DPI) const
    {
        LOGFONT lf = get_common_font(type, dpi);
        return CreateFontIndirect(&lf);
    }

    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(manager_v2);
};

/** Helper to simplify retrieving the font for a specific client. */
class helper {
public:
    void get_font(LOGFONT& p_out) const
    {
        if (m_api.is_valid()) {
            m_api->get_font(m_guid, p_out);
        } else
            uGetIconFont(&p_out);
    }
    HFONT get_font() const
    {
        if (m_api.is_valid()) {
            return m_api->get_font(m_guid);
        }
        return uCreateIconFont();
    }
    helper(GUID p_guid) : m_guid(p_guid)
    {
        try {
            standard_api_create_t(m_api);
        } catch (const exception_service_not_found&) {
        };
    }

private:
    service_ptr_t<manager> m_api;
    GUID m_guid;
};
class NOVTABLE client : public service_base {
public:
    virtual const GUID& get_client_guid() const = 0;
    virtual void get_name(pfc::string_base& p_out) const = 0;

    virtual font_type_t get_default_font_type() const = 0;

    virtual void on_font_changed() const = 0;

    template <class tClass>
    class factory : public service_factory_t<tClass> {
    };

    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(client);

public:
    static bool create_by_guid(const GUID& p_guid, ptr& p_out);
};
}; // namespace fonts
}; // namespace cui

#endif
