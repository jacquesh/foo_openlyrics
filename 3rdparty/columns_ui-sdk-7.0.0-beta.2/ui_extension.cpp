#include "ui_extension.h"

#if (WINVER >= 0x0500)
#define _GetParent(wnd) GetAncestor(wnd, GA_PARENT)
#else
#define _GetParent(wnd) GetParent(wnd)
#endif

#ifdef _WIN64

const GUID uie::window_host::class_guid = {0x4f39d394, 0x6ef0, 0x41f4, {0xa2, 0x85, 0x8, 0xd2, 0xab, 0x17, 0x74, 0xe5}};

const GUID uie::window_host_ex::class_guid
    = {0xf290fe6d, 0xf6e, 0x4f10, {0x8e, 0x3c, 0x8a, 0x9d, 0x1f, 0x72, 0x82, 0xf9}};

const GUID uie::menu_window::class_guid
    = {0xb4d5ed80, 0xd1d5, 0x46fd, {0xbe, 0x41, 0xbc, 0x44, 0xd5, 0x94, 0xa5, 0x61}};

const GUID uie::menu_window_v2::class_guid
    = {0x7f06ae13, 0x484d, 0x4432, {0x9d, 0xd8, 0x32, 0xda, 0x3f, 0x36, 0x24, 0xcf}};

const GUID uie::window::class_guid = {0x7e537dfd, 0x436a, 0x452e, {0x9f, 0x5d, 0x8b, 0x5f, 0x60, 0xbb, 0x53, 0x47}};

const GUID uie::splitter_window::class_guid
    = {0x5c35e048, 0x59c3, 0x44d4, {0x98, 0x69, 0x34, 0xce, 0x92, 0x4f, 0xf9, 0xd6}};

const GUID uie::splitter_window_v2::class_guid
    = {0x25958730, 0x989d, 0x4634, {0xac, 0x4c, 0x5d, 0xef, 0x91, 0xdb, 0xc1, 0x37}};

const GUID uie::visualisation::class_guid
    = {0x40cf3946, 0xbf8d, 0x4c50, {0x85, 0x9d, 0x47, 0x07, 0xb2, 0x08, 0xf8, 0x71}};

const GUID uie::visualisation_host::class_guid
    = {0xb5086743, 0xef48, 0x401a, {0x88, 0xd4, 0x8f, 0x39, 0x1b, 0x98, 0x2e, 0x0c}};

const GUID uie::button::class_guid = {0xbb6d7db8, 0x0c55, 0x428a, {0xa0, 0x6e, 0xf6, 0x3c, 0x6a, 0x1f, 0x25, 0xb1}};

const GUID uie::button_v2::class_guid = {0xd985e946, 0x2ba8, 0x40c7, {0xb8, 0x4f, 0xb7, 0xc8, 0x4a, 0x14, 0x2c, 0x09}};

const GUID uie::custom_button::class_guid
    = {0xf0c21bf5, 0x2637, 0x4917, {0xa7, 0xe0, 0xf0, 0x3a, 0x33, 0x03, 0x69, 0xac}};

const GUID uie::menu_button::class_guid
    = {0xf042a6a4, 0x8e7b, 0x4af4, {0xbc, 0x4f, 0x42, 0xa6, 0x70, 0xeb, 0x70, 0x2e}};

const GUID uie::playlist_window::class_guid
    = {0xee5126ef, 0xd623, 0x4ff8, {0xa2, 0x1b, 0x98, 0x08, 0x76, 0x23, 0x44, 0xd1}};

const GUID uie::window_host_with_control::class_guid
    = {0x1ffe39bf, 0xa714, 0x48ea, {0x88, 0xf4, 0x8e, 0x91, 0xb8, 0x6b, 0x96, 0x0a}};

#else

const GUID uie::window_host::class_guid
    = {0x5e283800, 0xe682, 0x4120, {0xa1, 0xa8, 0xd9, 0xcd, 0xad, 0xdc, 0x89, 0x56}};

const GUID uie::window_host_ex::class_guid
    = {0x503ec86, 0xfcbb, 0x4643, {0x82, 0x74, 0x1a, 0x6a, 0x13, 0x5a, 0x40, 0x4d}};

const GUID uie::menu_window::class_guid
    = {0xa2a21b5f, 0x6280, 0x47bf, {0x85, 0x6f, 0xc5, 0xf2, 0xc1, 0x29, 0xc3, 0xea}};

const GUID uie::menu_window_v2::class_guid
    = {0xbd12fba8, 0xf6e, 0x45c8, {0x9b, 0x38, 0xd7, 0x42, 0x15, 0x13, 0xa1, 0xea}};

const GUID uie::window::class_guid = {0xf84c1030, 0x4407, 0x496c, {0xb0, 0x9a, 0x14, 0xe5, 0xec, 0x5c, 0xa1, 0xc3}};

const GUID uie::splitter_window::class_guid
    = {0x627c2f4, 0x3d09, 0x4c02, {0xa1, 0x86, 0xd6, 0xc2, 0xa1, 0x1d, 0x7a, 0xfc}};

const GUID uie::splitter_window_v2::class_guid
    = {0xb2f8e8d9, 0x3302, 0x481e, {0xb6, 0x15, 0x39, 0xd7, 0xa, 0xdf, 0x81, 0x8e}};

const GUID uie::visualisation::class_guid
    = {0x47d142fb, 0x27e3, 0x4a51, {0x98, 0x17, 0x10, 0xe2, 0xa1, 0x48, 0xe, 0x7d}};

const GUID uie::visualisation_host::class_guid
    = {0xe0472fc9, 0x8b4a, 0x4e50, {0x89, 0x95, 0x31, 0x4b, 0xda, 0x3d, 0xb5, 0xa2}};

const GUID uie::button::class_guid = {0xef05de6b, 0xd65e, 0x47b7, {0x9a, 0x45, 0x73, 0x10, 0xdb, 0xe5, 0x23, 0xe0}};

const GUID uie::button_v2::class_guid = {0x87fc6de2, 0x26e7, 0x40ec, {0x8c, 0xc6, 0x4d, 0x24, 0x19, 0x8b, 0x1e, 0xd5}};

const GUID uie::custom_button::class_guid
    = {0xc27a5b38, 0x97c4, 0x491a, {0x9b, 0x61, 0x59, 0x7b, 0xb8, 0x48, 0x37, 0xef}};

const GUID uie::menu_button::class_guid
    = {0x6ab8127d, 0xf3e2, 0x4ef6, {0xa5, 0x15, 0xd5, 0x4f, 0x66, 0xfd, 0xb7, 0xac}};

const GUID uie::playlist_window::class_guid
    = {0xe67bc90b, 0x40a8, 0x4f54, {0xa1, 0xc9, 0x16, 0x9a, 0x14, 0xe6, 0x39, 0xd0}};

const GUID uie::window_host_with_control::class_guid
    = {0x9e2eda65, 0xa107, 0x4a46, {0x83, 0x3e, 0x5a, 0x83, 0x83, 0xc1, 0xd, 0xf0}};

#endif

HWND uFindParentPopup(HWND wnd_child)
{
    HWND wnd_temp = _GetParent(wnd_child);

    while (wnd_temp && (GetWindowLong(wnd_temp, GWL_EXSTYLE) & WS_EX_CONTROLPARENT)) {
        if (GetWindowLong(wnd_temp, GWL_STYLE) & WS_POPUP)
            break;
        else
            wnd_temp = _GetParent(wnd_temp);
    }
    return wnd_temp;
}

HWND uie::window::g_on_tab(HWND wnd_focus)
{
    const HWND wnd_temp = GetAncestor(wnd_focus, GA_ROOT);

    if (!wnd_temp)
        return nullptr;

    // Protect against infinite loop in GetNextDlgTabItem when the currently focused window is one
    // that shouldn't have the keyboard focus
    if (GetWindowLongPtr(wnd_focus, GWL_EXSTYLE) & WS_EX_CONTROLPARENT)
        wnd_focus = wnd_temp;

    const HWND wnd_next = GetNextDlgTabItem(wnd_temp, wnd_focus, (GetKeyState(VK_SHIFT) & KF_UP) != 0);

    if (!wnd_next || wnd_next == wnd_focus)
        return nullptr;

    // Do nothing if a sensible window wasn't selected
    const auto next_window_has_tabstop_style = (GetWindowLongPtr(wnd_next, GWL_STYLE) & WS_TABSTOP) != 0;
    if (!next_window_has_tabstop_style)
        return nullptr;

    const auto flags = SendMessage(wnd_next, WM_GETDLGCODE, 0, 0);
    if (flags & DLGC_HASSETSEL)
        SendMessage(wnd_next, EM_SETSEL, 0, -1);
    SetFocus(wnd_next);

    return wnd_next;
};

void uie::extension_base::set_config_from_ptr(const void* p_data, t_size p_size, abort_callback& p_abort)
{
    stream_reader_memblock_ref reader(p_data, p_size);
    return set_config(&reader, p_size, p_abort);
}

void uie::extension_base::import_config_from_ptr(const void* p_data, t_size p_size, abort_callback& p_abort)
{
    stream_reader_memblock_ref reader(p_data, p_size);
    return import_config(&reader, p_size, p_abort);
}

void uie::extension_base::get_config_to_array(
    pfc::array_t<uint8_t>& p_data, abort_callback& p_abort, bool b_reset) const
{
    stream_writer_memblock_ref writer(p_data, b_reset);
    get_config(&writer, p_abort);
}

pfc::array_t<uint8_t> uie::extension_base::get_config_as_array(abort_callback& p_abort) const
{
    pfc::array_t<uint8_t> data;
    get_config_to_array(data, p_abort);
    return data;
}

void uie::extension_base::export_config_to_array(
    pfc::array_t<uint8_t>& p_data, abort_callback& p_abort, bool b_reset) const
{
    stream_writer_memblock_ref writer(p_data, b_reset);
    export_config(&writer, p_abort);
}

void uie::window_info_list_simple::get_name_by_guid(const GUID& in, pfc::string_base& out)
{
    size_t count = get_count();
    for (size_t n = 0; n < count; n++) {
        if (get_item(n).guid == in) {
            out = get_item(n).name;
            return;
        }
    }
}

void uie::menu_hook_impl::fix_ampersand(const char* src, pfc::string_base& out)
{
    unsigned ptr = 0;
    while (src[ptr]) {
        if (src[ptr] == '&') {
            out.add_string("&&");
            ptr++;
            while (src[ptr] == '&') {
                out.add_string("&&");
                ptr++;
            }
        } else
            out.add_byte(src[ptr++]);
    }
}

unsigned uie::menu_hook_impl::flags_to_win32(unsigned flags)
{
    unsigned ret = 0;
    if (flags & menu_node_t::state_checked)
        ret |= MF_CHECKED;
    if (flags & menu_node_t::state_disabled)
        ret |= MF_DISABLED;
    if (flags & menu_node_t::state_greyed)
        ret |= MF_GRAYED;
    return ret;
}

void set_menu_item_radio(HMENU menu, UINT id)
{
    MENUITEMINFO mii;
    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_FTYPE | MIIM_ID;
    mii.wID = id;
    GetMenuItemInfo(menu, id, FALSE, &mii);
    mii.fType |= MFT_RADIOCHECK;
    SetMenuItemInfo(menu, id, FALSE, &mii);
}
unsigned uie::menu_hook_impl::win32_build_menu_recur(HMENU menu, uie::menu_node_ptr parent, unsigned base_id,
    unsigned
        max_id) // menu item identifiers are base_id<=N<base_id+max_id (if theres too many items, they will be clipped)
{
    if (parent.is_valid() && parent->get_type() == menu_node_t::type_popup) {
        pfc::string8_fast_aggressive temp, name;
        temp.prealloc(32);
        name.prealloc(32);
        const auto child_num = parent->get_children_count();
        unsigned new_base = base_id;
        for (size_t child_idx = 0; child_idx < child_num; child_idx++) {
            menu_node_ptr child;
            parent->get_child(child_idx, child);
            if (child.is_valid()) {
                unsigned displayflags = 0;
                child->get_display_data(name, displayflags);
                if (strchr(name, '&')) {
                    fix_ampersand(name, temp);
                    name = temp;
                    temp.reset();
                }
                menu_node_t::type_t type = child->get_type();
                if (type == menu_node_t::type_popup) {
                    HMENU new_menu = CreatePopupMenu();
                    uAppendMenu(menu, MF_STRING | MF_POPUP | flags_to_win32(displayflags),
                        reinterpret_cast<UINT_PTR>(new_menu), name);
                    new_base = win32_build_menu_recur(new_menu, child, new_base, max_id);
                } else if (type == menu_node_t::type_separator) {
                    uAppendMenu(menu, MF_SEPARATOR, 0, 0);
                } else if (type == menu_node_t::type_command) {
                    unsigned id = new_base;
                    if (id >= 0 && (max_id < 0 || id < max_id)) {
                        uAppendMenu(menu, MF_STRING | flags_to_win32(displayflags), new_base, name);
                        if (displayflags & menu_node_t::state_radio)
                            set_menu_item_radio(menu, new_base);
                    }
                    new_base++;
                }
            }
        }
        return new_base;
    }
    return base_id;
}
unsigned uie::menu_hook_impl::execute_by_id_recur(uie::menu_node_ptr parent, unsigned base_id, unsigned max_id,
    unsigned
        id_exec) // menu item identifiers are base_id<=N<base_id+max_id (if theres too many items, they will be clipped)
{
    if (parent.is_valid() && parent->get_type() == menu_node_t::type_popup) {
        size_t child_num = parent->get_children_count();
        unsigned new_base = base_id;
        for (size_t child_idx = 0; child_idx < child_num; child_idx++) {
            menu_node_ptr child;
            parent->get_child(child_idx, child);
            if (child.is_valid()) {
                menu_node_t::type_t type = child->get_type();
                if (type == menu_node_t::type_popup) {
                    new_base = execute_by_id_recur(child, new_base, max_id, id_exec);
                } else if (type == menu_node_t::type_separator) {
                } else if (type == menu_node_t::type_command) {
                    unsigned id = new_base;
                    if (id >= 0 && (max_id < 0 || id < max_id)) {
                        if (new_base == id_exec)
                            child->execute();
                    }
                    new_base++;
                }
            }
        }
        return new_base;
    }
    return base_id;
}

void uie::menu_hook_impl::add_node(const uie::menu_node_ptr& p_node)
{
    m_nodes.add_item(p_node);
}
t_size uie::menu_hook_impl::get_children_count() const
{
    return m_nodes.get_count();
}
void uie::menu_hook_impl::get_child(t_size p_index, menu_node_ptr& p_out) const
{
    p_out = m_nodes[p_index];
}
uie::menu_node_t::type_t uie::menu_hook_impl::get_type() const
{
    return type_popup;
};

bool uie::menu_hook_impl::get_display_data(pfc::string_base& p_out, unsigned& p_displayflags) const
{
    return false;
};
bool uie::menu_hook_impl::get_description(pfc::string_base& p_out) const
{
    return false;
};
void uie::menu_hook_impl::execute(){};

void uie::menu_hook_impl::win32_build_menu(HMENU menu, unsigned base_id, unsigned max_id)
{
    m_base_id = base_id;
    m_max_id = max_id;
    win32_build_menu_recur(menu, this, base_id, max_id);
}
void uie::menu_hook_impl::execute_by_id(unsigned id_exec)
{
    execute_by_id_recur(this, m_base_id, m_max_id, id_exec);
}

/**Stoled from menu_manager.cpp */
bool test_key(unsigned k)
{
    return (GetKeyState(k) & 0x8000) ? true : false;
}

#define F_SHIFT (HOTKEYF_SHIFT << 8)
#define F_CTRL (HOTKEYF_CONTROL << 8)
#define F_ALT (HOTKEYF_ALT << 8)
#define F_WIN (HOTKEYF_EXT << 8)

t_uint32 get_key_code(WPARAM wp)
{
    t_uint32 code = (t_uint32)(wp & 0xFF);
    if (test_key(VK_CONTROL))
        code |= F_CTRL;
    if (test_key(VK_SHIFT))
        code |= F_SHIFT;
    if (test_key(VK_MENU))
        code |= F_ALT;
    if (test_key(VK_LWIN) || test_key(VK_RWIN))
        code |= F_WIN;
    return code;
}

bool uie::window::g_process_keydown_keyboard_shortcuts(WPARAM wp)
{
    return static_api_ptr_t<keyboard_shortcut_manager_v2>()->process_keydown_simple(get_key_code(wp));
}

namespace uie {

// {4673437D-1685-433f-A2CC-3864D609F4E2}
const GUID splitter_window::bool_show_caption
    = {0x4673437d, 0x1685, 0x433f, {0xa2, 0xcc, 0x38, 0x64, 0xd6, 0x9, 0xf4, 0xe2}};

// {35FA3514-8120-49e3-A56C-3EA1C8170A2E}
const GUID splitter_window::bool_hidden = {0x35fa3514, 0x8120, 0x49e3, {0xa5, 0x6c, 0x3e, 0xa1, 0xc8, 0x17, 0xa, 0x2e}};

// {40C95DFE-E5E9-4f11-90EC-E741BE887DDD}
const GUID splitter_window::bool_autohide
    = {0x40c95dfe, 0xe5e9, 0x4f11, {0x90, 0xec, 0xe7, 0x41, 0xbe, 0x88, 0x7d, 0xdd}};

// {3661A5E9-0FB4-4d2a-AC05-EF2F47D18AD9}
const GUID splitter_window::bool_locked = {0x3661a5e9, 0xfb4, 0x4d2a, {0xac, 0x5, 0xef, 0x2f, 0x47, 0xd1, 0x8a, 0xd9}};

// {709465DE-42CD-484d-BE8F-E737F01A6458}
const GUID splitter_window::uint32_orientation
    = {0x709465de, 0x42cd, 0x484d, {0xbe, 0x8f, 0xe7, 0x37, 0xf0, 0x1a, 0x64, 0x58}};

// {5CB327AB-34EB-409c-9B4E-10D0A3B04E8D}
const GUID splitter_window::uint32_size
    = {0x5cb327ab, 0x34eb, 0x409c, {0x9b, 0x4e, 0x10, 0xd0, 0xa3, 0xb0, 0x4e, 0x8d}};

// {5CE8945E-BBB4-4308-99C1-DFA6D10F9004}
const GUID splitter_window::bool_show_toggle_area
    = {0x5ce8945e, 0xbbb4, 0x4308, {0x99, 0xc1, 0xdf, 0xa6, 0xd1, 0xf, 0x90, 0x4}};

// {71BC1FBC-EDD1-429c-B262-74C2F00AB3D3}
const GUID splitter_window::bool_use_custom_title
    = {0x71bc1fbc, 0xedd1, 0x429c, {0xb2, 0x62, 0x74, 0xc2, 0xf0, 0xa, 0xb3, 0xd3}};

// {3B4DEDA5-493D-4c5c-B52C-036DE4CF43D9}
const GUID splitter_window::string_custom_title
    = {0x3b4deda5, 0x493d, 0x4c5c, {0xb5, 0x2c, 0x3, 0x6d, 0xe4, 0xcf, 0x43, 0xd9}};

// {443EEA36-E5F0-4ADD-BA0E-F31726B0BC45}
const GUID splitter_window::size_and_dpi
    = {0x443eea36, 0xe5f0, 0x4add, {0xba, 0xe, 0xf3, 0x17, 0x26, 0xb0, 0xbc, 0x45}};

}; // namespace uie

void stream_to_mem_block(
    stream_reader* p_source, pfc::array_t<t_uint8>& p_out, abort_callback& p_abort, size_t p_sizehint, bool b_reset)
{
    if (b_reset)
        p_out.set_size(0);

    constexpr size_t min_buffer = 256;
    const auto buffer_size = std::max(min_buffer, p_sizehint);
    pfc::array_t<t_uint8> buffer;
    buffer.set_size(buffer_size);

    for (;;) {
        size_t delta_done = 0;
        delta_done = p_source->read(buffer.get_ptr(), buffer_size, p_abort);
        p_out.append_fromptr(buffer.get_ptr(), delta_done);
        if (delta_done < buffer_size)
            break;
    }
}
