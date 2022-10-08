#ifndef _COLUMNS_UI_SDK_MENU_H_
#define _COLUMNS_UI_SDK_MENU_H_

namespace uie {
/**
 * \brief Menu item interface class.
 *
 * \note Do not derive directly from this; instead derive from either
 * menu_node_command_t, menu_node_popup_t or menu_node_separator_t
 *
 * \remark Remember, its derived from pfc::refcounted_object_root. So instantiate like:
 * \code uie::mode_node_ptr = new menu_node_impl; \endcode
 */
class NOVTABLE menu_node_t : public pfc::refcounted_object_root {
public:
    /** State of the menu item */
    enum state_t : unsigned {
        state_checked = (1 << 0),
        state_disabled = (1 << 1),
        state_greyed = (1 << 2),
        state_disabled_grey = state_disabled | state_greyed,
        state_radio = (1 << 3), // do not use
        state_radiochecked = state_radio | state_checked,
    };

    /** Type of the menu item */
    enum type_t { type_popup, type_command, type_separator };

    /**
     * Retrieves the type of the menu item.
     *
     * \return                Type of the menu item.
     */
    virtual type_t get_type() const = 0;

    /**
     * Retrieves the number of child items.
     *
     * \pre                    May only return a non-zero value if your item is
     *                         of type type_popup.
     *
     * \return                Number of child items.
     */
    virtual t_size get_children_count() const = 0;

    /**
     * Retrieves child item.
     *
     * \param [in]    index    Index of the child item to retrieve
     * \param [out]   p_out    Receives pointer to the child item
     */
    virtual void get_child(t_size index, menu_node_ptr& p_out) const = 0;

    /**
     * Gets display data.
     *
     * \param [out]    p_out    Receives display text, utf-8 encoded.
     *                          Valid only if flag_separator is not specified
     * \param [out]    p_state  Receives display state, combination of state_t flags.
     *
     * \return                true iff the item should be displayed
     */
    virtual bool get_display_data(pfc::string_base& p_out, unsigned& p_state) const = 0;

    /**
     * Gets item description.
     *
     * \param [out]    p_out    Receives item description, utf-8 encoded.
     *
     * \return                true iff the item has a description and p_out was set to it
     */
    virtual bool get_description(pfc::string_base& p_out) const = 0;

    /**
     * Executes the command.
     * Applicable only for type_command items.
     */
    virtual void execute() = 0;
};

/** \brief Base class for command menu items */
class NOVTABLE menu_node_command_t : public menu_node_t {
public:
    type_t get_type() const override { return type_command; };
    t_size get_children_count() const override { return 0; }
    void get_child(t_size index, menu_node_ptr& p_out) const override {}
};

/** \brief Base class for popup menu items */
class NOVTABLE menu_node_popup_t : public menu_node_t {
public:
    type_t get_type() const override { return type_popup; };
    void execute() override{};
    bool get_description(pfc::string_base& p_out) const override { return false; }
};

/** \brief Implements menu_node_t as a separator item */
class menu_node_separator_t : public menu_node_t {
public:
    type_t get_type() const override { return type_separator; };
    void execute() override{};
    bool get_description(pfc::string_base& p_out) const override { return false; }
    t_size get_children_count() const override { return 0; }
    bool get_display_data(pfc::string_base& p_out, unsigned& p_displayflags) const override
    {
        p_out.reset();
        p_displayflags = 0;
        return true;
    }
    void get_child(t_size index, menu_node_ptr& p_out) const override {}
};

#if _MSC_VER >= 1800
/**
 * \brief Helper class to instantiate simple command menu nodes.
 */
class simple_command_menu_node : public menu_node_command_t {
public:
    simple_command_menu_node(
        const char* display_name, const char* description, uint32_t display_flags, std::function<void()> on_execute)
        : m_display_name{display_name}
        , m_description{description}
        , m_display_flags{display_flags}
        , m_on_execute{on_execute}
    {
    }

    bool get_display_data(pfc::string_base& p_out, unsigned& p_displayflags) const override
    {
        p_out = m_display_name;
        p_displayflags = m_display_flags;
        return true;
    }
    bool get_description(pfc::string_base& p_out) const override
    {
        p_out = m_description;
        return m_description.length();
    }
    void execute() override { m_on_execute(); }

private:
    pfc::string_simple m_display_name;
    pfc::string_simple m_description;
    uint32_t m_display_flags;
    std::function<void()> m_on_execute;
};
#endif

/** \brief Class that collects menu_node_t objects */
class NOVTABLE menu_hook_t {
public:
    virtual void add_node(const menu_node_ptr& p_node) = 0;
};

/**
 * \brief Standard implementation of menu_hook_t, also exposes menu_node_t interface
 * \remark Remember, its derived from pfc::refcounted_object_root. So instantiate like:
 * \code pfc::refcounted_ptr_t<uie::menu_hook_impl> = new uie::menu_hook_impl; \endcode
 */
class menu_hook_impl
    : public menu_hook_t
    , public menu_node_t {
    pfc::list_t<menu_node_ptr> m_nodes;
    unsigned m_base_id;
    unsigned m_max_id;
    static void fix_ampersand(const char* src, pfc::string_base& out);
    static unsigned flags_to_win32(unsigned flags);
    unsigned win32_build_menu_recur(HMENU menu, menu_node_ptr parent, unsigned base_id, unsigned max_id);
    unsigned execute_by_id_recur(menu_node_ptr parent, unsigned base_id, unsigned max_id, unsigned id_exec);

public:
    void add_node(const menu_node_ptr& p_node) override;
    t_size get_children_count() const override;
    void get_child(t_size p_index, menu_node_ptr& p_out) const override;
    type_t get_type() const override;

    bool get_display_data(pfc::string_base& p_out, unsigned& p_displayflags) const override;
    bool get_description(pfc::string_base& p_out) const override;
    void execute() override;

    void win32_build_menu(
        HMENU menu, unsigned base_id, unsigned max_id); // menu item identifiers are base_id<=N<base_id+max_id (if
                                                        // theres too many items, they will be clipped)
    void execute_by_id(unsigned id_exec);
};
} // namespace uie

#endif //_COLUMNS_UI_SDK_MENU_H_
