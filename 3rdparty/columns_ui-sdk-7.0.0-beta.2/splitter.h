#ifndef _COLUMNS_API_SPLITTER_H_
#define _COLUMNS_API_SPLITTER_H_

/**
 * \file splitter.h
 * \brief Splitter Window API
 */

namespace uie {

/**
 * \brief Holds data about a splitter item
 *
 * Derive from here and also store your other stuff (show_caption..)
 * Functions as data container only!
 */
class NOVTABLE splitter_item_t {
public:
    virtual const GUID& get_panel_guid() const = 0;
    /** Setting GUID deletes panel config and window ptr (i.e. do it first)*/
    virtual void set_panel_guid(const GUID& p_guid) = 0;

    virtual void get_panel_config(stream_writer* p_out) const = 0;
    virtual void set_panel_config(stream_reader* p_reader, t_size p_size) = 0;

    virtual const window_ptr& get_window_ptr() const = 0;

    virtual bool query(const GUID& p_guid) const { return false; }
    virtual ~splitter_item_t(){};

    template <typename t_class>
    bool query(const t_class*& p_out) const
    {
        if (query(t_class::get_class_guid())) {
            p_out = static_cast<const t_class*>(this);
            return true;
        }
        return false;
    }

    template <typename t_class>
    bool query(t_class*& p_out)
    {
        if (query(t_class::get_class_guid())) {
            p_out = static_cast<t_class*>(this);
            return true;
        }
        return false;
    }

    void get_panel_config_to_array(pfc::array_t<uint8_t>& p_data, bool reset = false, bool refresh = false) const
    {
        const auto window = get_window_ptr();

        if (refresh && window.is_valid()) {
            window->get_config_to_array(p_data, fb2k::noAbort, reset);
        } else {
            stream_writer_memblock_ref writer(p_data, reset);
            get_panel_config(&writer);
        }
    }

    pfc::array_t<uint8_t> get_panel_config_to_array(bool refresh = false) const
    {
        pfc::array_t<uint8_t> data;
        get_panel_config_to_array(data, false, refresh);
        return data;
    }

    void set_panel_config_from_ptr(const void* p_data, t_size p_size)
    {
        stream_reader_memblock_ref reader(p_data, p_size);
        return set_panel_config(&reader, p_size);
    }
};

typedef pfc::ptrholder_t<splitter_item_t> splitter_item_ptr;

/** \brief Implements splitter_item_t with the standard set of data stored */
template <class t_base>
class splitter_item_simple : public t_base {
public:
    virtual const GUID& get_panel_guid() const { return m_guid; }
    virtual void get_panel_config(stream_writer* p_out) const
    {
        abort_callback_impl p_abort;
        p_out->write(m_data.get_ptr(), m_data.get_size(), p_abort);
    }
    virtual void set_panel_guid(const GUID& p_guid)
    {
        m_guid = p_guid;
        m_data.set_size(0);
        m_ptr.release();
    }
    virtual void set_panel_config(stream_reader* p_reader, t_size p_size)
    {
        abort_callback_impl p_abort;
        m_data.set_size(p_size);
        p_reader->read(m_data.get_ptr(), m_data.get_size(), p_abort);
    }
    virtual const window_ptr& get_window_ptr() const { return m_ptr; }
    void set_window_ptr(const window_ptr& p_source) { m_ptr = p_source; }

protected:
    GUID m_guid{};
    pfc::array_t<t_uint8> m_data;
    window_ptr m_ptr;
};

typedef splitter_item_simple<splitter_item_t> splitter_item_simple_t;

/** \brief Implements splitter_item_t with a full set of data stored */
class splitter_item_full_t : public splitter_item_t {
public:
    uint32_t m_caption_orientation{};
    bool m_locked{};
    bool m_hidden{};
    bool m_autohide{};
    bool m_show_caption{};
    uint32_t m_size{};
    bool m_show_toggle_area{};
    bool m_custom_title{};

    virtual void get_title(pfc::string_base& p_out) const = 0;
    virtual void set_title(const char* p_title, t_size length) = 0;

    static const GUID& get_class_guid()
    {
        // {57C8EEAE-0D4F-486a-8769-FFFCB4B4349B}
        static const GUID rv = {0x57c8eeae, 0xd4f, 0x486a, {0x87, 0x69, 0xff, 0xfc, 0xb4, 0xb4, 0x34, 0x9b}};
        return rv;
    }

    bool query(const GUID& p_guid) const override { return p_guid == get_class_guid(); }
};

class splitter_item_full_v2_t : public splitter_item_full_t {
public:
    uint32_t m_size_v2{};
    uint32_t m_size_v2_dpi{};

    static const GUID& get_class_guid()
    {
        // {4C0BAD6E-A0BE-4F57-981B-F94EBBEE57EF}
        static const GUID rv = {0x4c0bad6e, 0xa0be, 0x4f57, {0x98, 0x1b, 0xf9, 0x4e, 0xbb, 0xee, 0x57, 0xef}};
        return rv;
    }

    bool query(const GUID& p_guid) const override
    {
        return p_guid == get_class_guid() || splitter_item_full_t::query(p_guid);
    }
};

/**
 * \brief Splitter item implementing support for additional data
 *
 * Use this when your splitter window needs to store additional data
 * for each child panel that's not covered by the standard variables.
 *
 * \note You can use splitter_item_full_v3_impl_t rather than implementing
 *       this class. Alternatively, you can derive from
 *       splitter_item_full_v3_base_t.
 */
class splitter_item_full_v3_t : public splitter_item_full_v2_t {
public:
    /**
     * \brief Gets the additional data associated with this splitter item
     *
     * \note
     * Check that get_extra_data_format_id() matches your format ID before
     * calling this, as splitter items from other splitter windows may be
     * inserted into your window.
     *
     * \note
     * The data returned by this function may be serialised and passed
     * between foobar2000 instances via the clipboard. And, at some point, you
     * may find that you need to change the structure of the data. Make sure
     * that your code handles such changes gracefully.
     *
     * \param writer Stream that receives the additional data.
     */
    virtual void get_extra_data(stream_writer* writer) const = 0;

    /**
     * \brief Gets a GUID to identify the format of the data returned by
     *        get_extra_data()
     * \return The format identifier
     */
    virtual GUID get_extra_data_format_id() const = 0;

    static const GUID& get_class_guid()
    {
        // {9B0FB084-0580-4BA7-B7D9-AA2255EEDEA8}
        static const GUID rv = {0x9b0fb084, 0x580, 0x4ba7, {0xb7, 0xd9, 0xaa, 0x22, 0x55, 0xee, 0xde, 0xa8}};
        return rv;
    }

    bool query(const GUID& p_guid) const override
    {
        return p_guid == get_class_guid() || splitter_item_full_v2_t::query(p_guid);
    }
};

template <class TBase>
class splitter_item_full_impl_base_t : public splitter_item_simple<TBase> {
    pfc::string8 m_title;

public:
    virtual void get_title(pfc::string_base& p_out) const { p_out = m_title; }
    virtual void set_title(const char* p_title, t_size length) { m_title.set_string(p_title, length); }
};

using splitter_item_full_impl_t = splitter_item_full_impl_base_t<splitter_item_full_t>;
using splitter_item_full_v2_impl_t = splitter_item_full_impl_base_t<splitter_item_full_v2_t>;
using splitter_item_full_v3_base_t = splitter_item_full_impl_base_t<splitter_item_full_v3_t>;

/**
 * \brief Implements splitter_item_full_v3_t
 */
class splitter_item_full_v3_impl_t : public uie::splitter_item_full_v3_base_t {
public:
    pfc::array_t<t_uint8> m_extra_data;
    GUID m_extra_data_format_id{};

    void get_extra_data(stream_writer* writer) const override
    {
        abort_callback_dummy aborter;
        writer->write(m_extra_data.get_ptr(), m_extra_data.get_size(), aborter);
    }

    GUID get_extra_data_format_id() const override { return m_extra_data_format_id; }
};

class stream_writer_fixedbuffer : public stream_writer {
public:
    void write(const void* p_buffer, t_size p_bytes, abort_callback& p_abort) override
    {
        if (p_bytes > 0) {
            if (p_bytes > m_bytes - m_bytes_read)
                throw pfc::exception_overflow();
            memcpy((t_uint8*)m_out, p_buffer, p_bytes);
            m_bytes_read += p_bytes;
        }
    }
    stream_writer_fixedbuffer(void* p_out, t_size p_bytes, t_size& p_bytes_read)
        : m_out(p_out)
        , m_bytes(p_bytes)
        , m_bytes_read(p_bytes_read)
    {
        m_bytes_read = 0;
    }

private:
    void* m_out;
    t_size m_bytes;
    t_size& m_bytes_read;
};

#ifndef USER_DEFAULT_SCREEN_DPI
#define CUI_SDK_DEFAULT_DPI 96
#else
#define CUI_SDK_DEFAULT_DPI USER_DEFAULT_SCREEN_DPI
#endif

struct size_and_dpi {
    uint32_t size;
    uint32_t dpi;

    size_and_dpi() : size(0), dpi(CUI_SDK_DEFAULT_DPI) {}
    size_and_dpi(uint32_t size_, uint32_t dpi_ = CUI_SDK_DEFAULT_DPI) : size(size_), dpi(dpi_) {}
};

/**
 * \brief Subclass of uie::window, specifically for splitters.
 *
 * Splitter classes must support multiple instances
 */
class NOVTABLE splitter_window : public window {
public:
    static const GUID bool_show_caption;
    static const GUID bool_hidden;
    static const GUID bool_autohide;
    static const GUID bool_locked;
    static const GUID uint32_orientation;
    static const GUID bool_show_toggle_area;
    static const GUID uint32_size;
    static const GUID bool_use_custom_title;
    static const GUID string_custom_title;
    static const GUID size_and_dpi;

    /**
     * \brief Get config item supported
     *
     * \return count
     */
    virtual bool get_config_item_supported(t_size p_index, const GUID& p_type) const { return false; }

    /**
     * \brief Creates non-modal child configuration dialog.
     * Since its non-modal, remember to keep a refcounted reference to yourself.
     * Use WS_EX_CONTROLPARENT
     */
    // virtual HWND create_config_window(HWND wnd_parent, const container_window::window_position_t & p_placement)
    // {return 0;}

    // this config system isn't great. it may be changed.
    // write in native-endianess (i.e. use write_object_t)
    virtual bool get_config_item(t_size index, const GUID& p_type, stream_writer* p_out, abort_callback& p_abort) const
    {
        return false;
    };

    bool get_config_item(t_size index, const GUID& p_type, stream_writer* p_out) const
    {
        abort_callback_dummy p_abort;
        return get_config_item(index, p_type, p_out, p_abort);
    }
    virtual bool set_config_item(t_size index, const GUID& p_type, stream_reader* p_source, abort_callback& p_abort)
    {
        return false;
    };
    template <typename class_t>
    bool set_config_item_t(t_size index, const GUID& p_type, const class_t& p_val, abort_callback& p_abort)
    {
        stream_reader_memblock_ref reader(&p_val, sizeof(class_t));
        return set_config_item(index, p_type, &reader, p_abort);
    };

    template <class T>
    bool get_config_item(t_size p_index, const GUID& p_type, T& p_out, abort_callback& p_abort) const
    {
        t_size written;
        stream_writer_fixedbuffer writer(&p_out, sizeof(T), written);
        return get_config_item(p_index, p_type, &writer, p_abort);
    }

    template <class T>
    bool get_config_item(t_size p_index, const GUID& p_type, T& p_out) const
    {
        abort_callback_dummy abort_callback;
        return get_config_item(p_index, p_type, p_out, abort_callback);
    }
    /** This method may be called on both active and inactive (i.e. no window) instances */
    virtual void insert_panel(t_size index, const splitter_item_t* p_item) = 0;
    /** This method may be called on both active and inactive (i.e. no window) instances */
    virtual void remove_panel(t_size index) = 0;
    /** This method may be called on both active and inactive (i.e. no window) instances */
    virtual void replace_panel(t_size index, const splitter_item_t* p_item) = 0;
    virtual t_size get_panel_count() const = 0;
    virtual t_size get_maximum_panel_count() const { return pfc_infinite; };

    /** Reserved for future use */
    virtual void register_callback(class splitter_callback* p_callback){};
    /** Reserved for future use */
    virtual void deregister_callback(class splitter_callback* p_callback){};

protected:
    /**
     * Return value needs deleting!! Use pfc::ptrholder_t
     * This method may be called on both active and inactive (i.e. no window) instances
     */
    virtual splitter_item_t* get_panel(t_size index) const = 0;

public:
    void get_panel(t_size index, pfc::ptrholder_t<splitter_item_t>& p_out) const { p_out = get_panel(index); }

    t_size add_panel(const splitter_item_t* p_item)
    {
        t_size count = get_panel_count();
        insert_panel(count, p_item);
        return count;
    }

    // helpers
    inline void swap_items(t_size p_item1, t_size p_item2)
    {
        splitter_item_ptr p1, p2;
        get_panel(p_item1, p1);
        get_panel(p_item2, p2);
        replace_panel(p_item1, p2.get_ptr());
        replace_panel(p_item2, p1.get_ptr());
    }

    inline bool move_up(t_size p_index)
    {
        t_size count = get_panel_count();
        if (p_index > 0 && p_index < count) {
            swap_items(p_index, p_index - 1);
            return true;
        }
        return false;
    }
    inline bool move_down(t_size p_index)
    {
        t_size count = get_panel_count();
        if (p_index < (count - 1)) {
            swap_items(p_index, p_index + 1);
            return true;
        }
        return false;
    }
    bool find_by_ptr(const uie::window::ptr& window, t_size& p_index)
    {
        t_size i, count = get_panel_count();
        for (i = 0; i < count; i++) {
            splitter_item_ptr si;
            get_panel(i, si);
            if (si->get_window_ptr() == window) {
                p_index = i;
                return true;
            }
        }
        return false;
    }
    void remove_panel(const uie::window::ptr& window)
    {
        t_size i, count = get_panel_count();
        for (i = 0; i < count; i++) {
            splitter_item_ptr si;
            get_panel(i, si);
            if (si->get_window_ptr() == window) {
                remove_panel(i);
                return;
            }
        }
    }
    bool set_config_item(t_size index, const GUID& p_type, const void* p_data, t_size p_size, abort_callback& p_abort)
    {
        stream_reader_memblock_ref reader(p_data, p_size);
        return set_config_item(index, p_type, &reader, p_abort);
    }

    FB2K_MAKE_SERVICE_INTERFACE(splitter_window, window);
};

/**
 * \brief    Extends uie::splitter_window, providing additional methods used for live editing.
 *
 * New in SDK version 6.5.
 */
class NOVTABLE splitter_window_v2 : public splitter_window {
public:
    /**
     * \brief Checks if a point is within this splitter window. Used for live layout editing.
     *
     * \param  [in]     wnd_point        The window the original mouse message was being sent to.
     * \param  [in]     pt_screen        The point being checked.
     * \param  [out]    p_hierarchy      Receives the hierarchy of windows leading to the point including this window.
     *
     * If the point is within your window (including any child windows), append yourself to p_hierarchy. If it is in a
     * non-splitter child window, additionally append the child window to the list. If the child window is a splitter
     * window, call its is_point_ours to complete the hierarchy.
     *
     * \return  True if the point is window the window; otherwise false.
     */
    virtual bool is_point_ours(HWND wnd_point, const POINT& pt_screen, pfc::list_base_t<uie::window::ptr>& p_hierarchy)
    {
        return false;
    };

    /**
     * \brief Checks if windows can be inserted into this splitter. Used for live editing.
     *
     * Implement this by calling uie::window::is_available on each window.
     *
     * \param  [in]    p_windows                List of windows to check.
     * \param  [out]    p_mask_unsupported        A bit array the same size as the number of windows in p_windows.
     *                                        Receives values indicating whether each window can be inserted.
     *                                        A set bit indicates the respective window cannot be inserted.
     */
    virtual void get_supported_panels(
        const pfc::list_base_const_t<uie::window::ptr>& p_windows, bit_array_var& p_mask_unsupported){};

    FB2K_MAKE_SERVICE_INTERFACE(splitter_window_v2, splitter_window);
};
} // namespace uie
#endif //_COLUMNS_API_SPLITTER_H_
