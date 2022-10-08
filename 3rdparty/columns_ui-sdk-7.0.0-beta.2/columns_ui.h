#ifndef _COLUMNS_API_COLUMNS_H_
#define _COLUMNS_API_COLUMNS_H_

/**
 * \file columns_ui.h
 * \brief Columns UI Definitions
 */

/** \brief Namespace containing all Columns UI definitions */
namespace cui {
/** \brief Namespace containing standard Columns UI toolbar GUIDs */
namespace toolbars {
constexpr GUID guid_buttons{0xd8e65660, 0x64ed, 0x42e7, {0x85, 0xb, 0x31, 0xd8, 0x28, 0xc2, 0x52, 0x94}};

constexpr GUID guid_menu{0x76e6db50, 0xde3, 0x4f30, {0xa7, 0xe4, 0x93, 0xfd, 0x62, 0x8b, 0x14, 0x1}};

constexpr GUID guid_playback_order{0xaba09e7e, 0x9c95, 0x443e, {0xbd, 0xfc, 0x4, 0x9d, 0x66, 0xb3, 0x24, 0xa0}};

constexpr GUID guid_spectrum_analyser{0xd947777c, 0x94c7, 0x409a, {0xb0, 0x2c, 0x9b, 0xe, 0xb9, 0xe3, 0x74, 0xfa}};

constexpr GUID guid_seek_bar{0x678fe380, 0xabbb, 0x4c72, {0xa0, 0xb3, 0x72, 0xe7, 0x69, 0x67, 0x11, 0x25}};

constexpr GUID guid_volume_control{0xb3259290, 0xcb68, 0x4d37, {0xb0, 0xf1, 0x80, 0x94, 0x86, 0x2a, 0x95, 0x24}};

constexpr GUID guid_filter_search_bar{0x6e3b8b17, 0xaebd, 0x40d2, {0xa1, 0xf, 0x9d, 0x3a, 0xcf, 0x74, 0xf0, 0x91}};
} // namespace toolbars
/** \brief Namespace containing standard Columns UI panel GUIDs */
namespace panels {
constexpr GUID guid_playlist_switcher{0xc2cf9425, 0x540, 0x4579, {0xab, 0x3f, 0x13, 0xe2, 0x17, 0x66, 0x3d, 0x9b}};

constexpr GUID guid_playlist_tabs{0xabb72d0d, 0xdbf0, 0x4bba, {0x8c, 0x68, 0x33, 0x57, 0xeb, 0xe0, 0x7a, 0x4d}};

[[deprecated]] constexpr GUID guid_playlist_view{
    0xf20bed8f, 0x225b, 0x46c3, {0x9f, 0xc7, 0x45, 0x4c, 0xed, 0xb6, 0xcd, 0xad}};

constexpr GUID guid_vertical_splitter{0x77653a44, 0x66d1, 0x49e0, {0x9a, 0x7a, 0x1c, 0x71, 0x89, 0x8c, 0x4, 0x41}};

constexpr GUID guid_horizontal_splitter{0x8fa0bc24, 0x882a, 0x4fff, {0x8a, 0x3b, 0x21, 0x5e, 0xa7, 0xfb, 0xd0, 0x7f}};

constexpr GUID guid_filter{0xfb059406, 0xdddd, 0x4bd0, {0x8a, 0x11, 0x42, 0x42, 0x85, 0x4c, 0xbb, 0xa5}};

constexpr GUID guid_artwork_view{0xdeead6ec, 0xf0b9, 0x4919, {0xb1, 0x6d, 0x28, 0xa, 0xed, 0xde, 0x73, 0x43}};

constexpr GUID guid_playlist_view_v2{0xfb059406, 0x5f14, 0x4bd0, {0x8a, 0x11, 0x42, 0x42, 0x85, 0x4c, 0xbb, 0xa5}};

constexpr GUID guid_item_details{0x59b4f428, 0x26a5, 0x4a51, {0x89, 0xe5, 0x39, 0x45, 0xd3, 0x27, 0xb4, 0xcb}};

constexpr GUID guid_item_properties{0x8f6069cd, 0x2e36, 0x4ead, {0xb1, 0x71, 0x93, 0xf3, 0xdf, 0xf0, 0x7, 0x3a}};
} // namespace panels
/** \brief Namespace containing standard Columns UI visualisation GUIDs */
namespace visualisations {
constexpr GUID guid_spectrum_analyser{0xd947777c, 0x94c7, 0x409a, {0xb0, 0x2c, 0x9b, 0xe, 0xb9, 0xe3, 0x74, 0xfa}};
}

/** \brief Namespace containing Columns UI string GUIDs */
namespace strings {
constexpr GUID guid_global_variables{0x493d419a, 0xcbb3, 0x4b8a, {0x8f, 0xb8, 0x28, 0xde, 0x2a, 0xe2, 0xf3, 0x6f}};
}

/**
 * \brief Namespace containing Columns UI config_object GUIDs and
 * related helper functions
 *
 * \see See config_object, config_object_notify and config_object_notify_impl_simple
 */
namespace config_objects {
constexpr GUID guid_bool_locked_panel_resizing_allowed{
    0x3a0ef00a, 0xd538, 0x4470, {0x9a, 0x18, 0xdc, 0xf8, 0x22, 0xcc, 0x96, 0x73}};

/**
 * \brief Gets whether resizing of locked panels should be allowed.
 *
 * \remarks
 * - In Columns UI 0.5.1 and older, this always returns true.
 *
 * \return Current value of 'Allow locked panel resizing' setting.
 */
inline bool get_locked_panel_resizing_allowed()
{
    return config_object::g_get_data_bool_simple(guid_bool_locked_panel_resizing_allowed, true);
}
} // namespace config_objects

/**
 * \brief Service exposing Columns UI control methods
 * \remarks
 * - One implementation in Columns UI, do not reimplement.
 * - Call from main thread only
 */
class NOVTABLE control : public service_base {
public:
    /** \brief Retrieves a string from Columns UI */
    virtual bool get_string(const GUID& p_guid, pfc::string_base& p_out) const = 0;

    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(control);
};

/**
 * \brief Pointer to the columns_ui::control service, initialised in its constructor
 *
 * If you use the control service, you should make sure you handle the cases where the
 * service is not available (no Columns UI installed / incompatible Columns UI installed)
 * by handling the exceptions below.
 *
 * \throw
 * The constructor for this class may throw:
 * - exception_service_not_found if the service was not found
 * - exception_service_duplicated if the service was duplicated
 */
typedef static_api_ptr_t<control> static_control_ptr;

/**
 * \brief Holds a pointer to the columns_ui::control service
 */
typedef service_ptr_t<control> control_ptr;

class global_variable {
public:
    global_variable(const char* p_name, t_size p_name_length, const char* p_value, t_size p_value_length)
        : m_name(p_name, p_name_length)
        , m_value(p_value, p_value_length)
    {
    }
    inline const char* get_name() const { return m_name; }
    inline const char* get_value() const { return m_value; }

private:
    pfc::string_simple m_name, m_value;
};

class global_variable_list : public pfc::ptr_list_t<global_variable> {
public:
    const char* find_by_name(const char* p_name, t_size length)
    {
        size_t count = get_count();
        for (size_t n = 0; n < count; n++) {
            const char* ptr = get_item(n)->get_name();
            if (!stricmp_utf8_ex(p_name, length, ptr, pfc_infinite))
                return get_item(n)->get_value();
        }
        return 0;
    }
    void add_item(const char* p_name, t_size p_name_length, const char* p_value, t_size p_value_length)
    {
        global_variable* var = new global_variable(p_name, p_name_length, p_value, p_value_length);
        pfc::ptr_list_t<global_variable>::add_item(var);
    }
    ~global_variable_list() { delete_all(); }
};

template <bool set = true, bool get = true>
class titleformat_hook_global_variables : public titleformat_hook {
    global_variable_list& p_vars;

public:
    bool process_field(
        titleformat_text_out* p_out, const char* p_name, size_t p_name_length, bool& p_found_flag) override
    {
        p_found_flag = false;
        return false;
    }

    bool process_function(titleformat_text_out* p_out, const char* p_name, size_t p_name_length,
        titleformat_hook_function_params* p_params, bool& p_found_flag) override
    {
        p_found_flag = false;
        if (set && !stricmp_utf8_ex(p_name, p_name_length, "set_global", pfc_infinite)) {
            switch (p_params->get_param_count()) {
            case 2: {
                const char* name;
                const char* value;
                t_size name_length, value_length;
                p_params->get_param(0, name, name_length);
                p_params->get_param(1, value, value_length);
                p_vars.add_item(name, name_length, value, value_length);
                p_found_flag = true;
                return true;
            }
            default:
                return false;
            }
        } else if (get && !stricmp_utf8_ex(p_name, p_name_length, "get_global", pfc_infinite)) {
            switch (p_params->get_param_count()) {
            case 1: {
                const char* name;
                t_size name_length;
                p_params->get_param(0, name, name_length);
                const char* ptr = p_vars.find_by_name(name, name_length);
                if (ptr) {
                    p_out->write(titleformat_inputtypes::meta, ptr, pfc_infinite);
                    p_found_flag = true;
                } else
                    p_out->write(titleformat_inputtypes::meta, "[unknown variable]", pfc_infinite);
                return true;
            }
            default:
                return false;
            }
        } else
            return false;
    }
    inline titleformat_hook_global_variables(global_variable_list& vars) : p_vars(vars){};
};

namespace fcl {
PFC_DECLARE_EXCEPTION(exception_missing_panel, pfc::exception, "Missing panel.");
/** Namespace containing standard FCL group identifiers. */
namespace groups {
constexpr GUID layout{0x1979b677, 0x17ef, 0x4423, {0x94, 0x69, 0x11, 0x39, 0xa1, 0x1b, 0xd6, 0x87}};

constexpr GUID toolbars{0xf1181b34, 0x8848, 0x43d0, {0x92, 0x96, 0x24, 0x48, 0x4c, 0x1f, 0x5b, 0xf1}};

constexpr GUID colours_and_fonts{0xdd5158ae, 0xc8ed, 0x42d0, {0x89, 0xe3, 0xef, 0x1b, 0x19, 0x7f, 0xfc, 0xaf}};

constexpr GUID title_scripts{0x2a8e63a4, 0xf8e, 0x459d, {0xb7, 0x52, 0x87, 0x4e, 0x38, 0x65, 0x8a, 0x6c}};
} // namespace groups
enum t_fcl_type {
    type_public = 0, // use export_config/import_config on panels
    type_private = 1, // use set_config/get_config on panels
};

class NOVTABLE t_import_feedback {
public:
    /**
     * Specifies any panels that you are dependent on that are not installed. You must specify only missing panels.
     *
     * \param [in]	name	Unused. Pass a null-terminiated empty string.
     * \param [in]	guid	GUID of panel.
     */
    virtual void add_required_panel(const char* name, const GUID& guid) = 0;
};
class NOVTABLE t_export_feedback {
public:
    /**
     * Specifies panels that you are dependent on. You must specify all dependent panels.
     *
     * \param [in]	panels	GUIDs of panels.
     */
    virtual void add_required_panels(const pfc::list_base_const_t<GUID>& panels) = 0;
    void add_required_panel(GUID guid) { add_required_panels(pfc::list_single_ref_t<GUID>(guid)); }
};

/** Groups are nodes in the export tree. Each group can have multiple datasets. */
class NOVTABLE group : public service_base {
public:
    virtual void get_name(pfc::string_base& p_out) const = 0;
    virtual void get_description(pfc::string_base& p_out) const = 0;
    virtual const GUID& get_guid() const = 0;

    /** NULL guid <=> root node */
    virtual const GUID& get_parent_guid() const { return pfc::guid_null; }

    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(group);
};

/** Helper. Use group_impl_factory below. */
class group_impl : public group {
public:
    void get_name(pfc::string_base& p_out) const override { p_out = m_name; }
    void get_description(pfc::string_base& p_out) const override { p_out = m_desc; }
    const GUID& get_guid() const override { return m_guid; }
    const GUID& get_parent_guid() const override { return m_parent_guid; }

    GUID m_guid, m_parent_guid;
    pfc::string8 m_name, m_desc;

    group_impl(const GUID& pguid, const char* pname, const char* pdesc, const GUID& pguidparent = pfc::guid_null)
        : m_guid(pguid)
        , m_parent_guid(pguidparent)
        , m_name(pname)
        , m_desc(pdesc){};
};

/** Helper. */
class group_impl_factory : public service_factory_single_t<group_impl> {
public:
    group_impl_factory(
        const GUID& pguid, const char* pname, const char* pdesc, const GUID& pguidparent = pfc::guid_null)
        : service_factory_single_t<group_impl>(pguid, pname, pdesc, pguidparent){};
};

class NOVTABLE dataset : public service_base {
public:
    /** User-friendly fully qualified (unambiguous) name. */
    virtual void get_name(pfc::string_base& p_out) const = 0;
    /** Unique identifier of the dataset. */
    virtual const GUID& get_guid() const = 0;
    /** The identifier of the group you belong to. */
    virtual const GUID& get_group() const = 0;
    /**
     * Retrieves your data for an export.
     *
     * \param [in]	type	Specifies export mode. See t_fcl_type.
     */
    virtual void get_data(
        stream_writer* p_writer, t_uint32 type, t_export_feedback& feedback, abort_callback& p_abort) const = 0;
    /**
     * Sets your data for an import.
     *
     * \param [in]	type	Specifies export mode. See t_fcl_type.
     */
    virtual void set_data(
        stream_reader* p_reader, t_size size, t_uint32 type, t_import_feedback& feedback, abort_callback& p_abort)
        = 0;

    /**
     * Helper function. Retrieves your data for an export.
     *
     * \see get_data
     *
     * \param [in]	type	Specifies export mode. See t_fcl_type.
     */
    void get_data_to_array(pfc::array_t<uint8_t>& p_data, t_uint32 type, t_export_feedback& feedback,
        abort_callback& p_abort, bool b_reset = false) const;

    /**
     * Helper function. Sets your data for an import.
     *
     * \see set_data
     *
     * \param [in]	type	Specifies export mode. See t_fcl_type.
     */
    void set_data_from_ptr(
        const void* p_data, t_size size, t_uint32 type, t_import_feedback& feedback, abort_callback& p_abort);

    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(dataset);
};

class NOVTABLE dataset_v2 : public dataset {
public:
    /**
     * Determines the order in which data sets are imported when an FCL file
     * is being imported.
     *
     * New in Columns UI 1.1.
     *
     * Data sets with a higher priority value are imported first.
     *
     * This can be used when there are dependencies between global configuration
     * data and panel instance data. Columns UI uses this internally to deprioritise
     * the toolbar and layout data sets and you will not generally need to override
     * this.
     */
    virtual double get_import_priority() const { return 0.0; }

    FB2K_MAKE_SERVICE_INTERFACE(dataset_v2, dataset);
};

typedef service_ptr_t<dataset> dataset_ptr;
typedef service_ptr_t<group> group_ptr;

template <class T>
class dataset_factory : public service_factory_single_t<T> {
};

template <class T>
class group_factory : public service_factory_single_t<T> {
};

/** Helper. */
template <class t_service, class t_service_ptr = service_ptr_t<t_service>>
class service_list_auto_t : public pfc::list_t<t_service_ptr> {
public:
    service_list_auto_t()
    {
        service_enum_t<t_service> export_enum;
        t_service_ptr ptr;
        while (export_enum.next(ptr))
            this->add_item(ptr);
    };
    bool find_by_guid(const GUID& guid, t_service_ptr& p_out)
    {
        t_size i, count = this->get_count();
        for (i = 0; i < count; i++) {
            if (this->get_item(i)->get_guid() == guid) {
                p_out = this->get_item(i);
                return true;
            }
        }
        return false;
    }
    void remove_by_guid(const GUID& guid)
    {
        t_size count = this->get_count();
        for (; count; count--) {
            if (this->get_item(count - 1)->get_guid() == guid) {
                this->remove_by_idx(count - 1);
            }
        }
    }
    static int g_compare_name(const t_service_ptr& i1, const t_service_ptr& i2)
    {
        pfc::string8 n1, n2;
        i1->get_name(n1);
        i2->get_name(n2);
        return StrCmpLogicalW(pfc::stringcvt::string_os_from_utf8(n1), pfc::stringcvt::string_os_from_utf8(n2));
    }
    void sort_by_name() { this->sort_t(g_compare_name); }
};

class dataset_list : public service_list_auto_t<dataset> {
public:
    dataset_list()
    {
        sort_t([](dataset::ptr& left, dataset::ptr& right) {
            dataset_v2::ptr left_v2;
            dataset_v2::ptr right_v2;

            left->service_query_t(left_v2);
            right->service_query_t(right_v2);

            const auto priority_left = left_v2.is_valid() ? left_v2->get_import_priority() : 0.0;
            const auto priority_right = right_v2.is_valid() ? right_v2->get_import_priority() : 0.0;

            // Descending sort, so higher priorities are imported earlier
            return pfc::compare_t<>(priority_right, priority_left);
        });
    }
};
typedef service_list_auto_t<group> group_list;

/** Helper. */
class group_list_filtered : public pfc::list_t<group_ptr> {
    pfc::list_t<t_size> m_original_indices;

public:
    t_size get_original_index(t_size index) const { return m_original_indices[index]; }
    group_list_filtered(const pfc::list_base_const_t<group_ptr>& list, const GUID& guid)
    {
        t_size i, count = list.get_count();
        for (i = 0; i < count; i++) {
            if (list[i]->get_parent_guid() == guid) {
                add_item(list[i]);
                m_original_indices.add_item(i);
            }
        }
    }
};
} // namespace fcl
} // namespace cui

#endif //_COLUMNS_API_COLUMNS_H_
