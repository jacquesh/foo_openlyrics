#pragma once
#include "stdafx.h"

#include "ui_extension.h"

#define UIE_SHIM_PANEL_FACTORY(TypeName) ui_extension::window_factory<uie_shim_panel< TypeName >> g_uie_shim_panel##TypeName;

template<typename TPanel>
class uie_shim_colour_client : public columns_ui::colours::client
{
public:
    const GUID& get_client_guid() const override
    {
        static GUID guid = TPanel::g_get_guid();
        return guid;
    }

    void get_name(pfc::string_base& out) const override
    {
        TPanel::g_get_name(out);
    }

    uint32_t get_supported_bools() const override
    {
        return 0;
    }

    bool get_themes_supported() const override
    {
        return false;
    }

    void on_colour_changed(uint32_t mask) const override {}
    void on_bool_changed(uint32_t mask) const override {}
};

template<typename TPanel>
class uie_shim_font_client : public columns_ui::fonts::client
{
public:
    const GUID& get_client_guid() const override
    {
        static GUID guid = TPanel::g_get_guid();
        return guid;
    }

    void get_name(pfc::string_base& out) const override
    {
        TPanel::g_get_name(out);
    }

    columns_ui::fonts::font_type_t get_default_font_type() const override
    {
        return columns_ui::fonts::font_type_items;
    }

    void on_font_changed() const override
    {
        font_generation++;
    }

    uint64_t get_font_generation() const
    {
        return font_generation;
    }

private:
    mutable uint64_t font_generation = 1;
};

template<typename TPanel>
class uie_shim_instance_callback_impl : public ui_element_instance_callback_v3
{
public:
    uie_shim_instance_callback_impl() :
        colours(TPanel::g_get_guid()),
        font_uie(nullptr),
        font_generation(0)
    {
        standard_api_create_t(font_manager);
    }

    ~uie_shim_instance_callback_impl()
    {
        if(font_uie != nullptr) DeleteObject(font_uie);
    }

    void on_min_max_info_change() override {}
    void on_alt_pressed(bool p_state) override {}
    void request_replace(service_ptr_t<class ui_element_instance> p_item) override {}
    bool request_activation(service_ptr_t<class ui_element_instance> p_item) { return false; }
    bool is_edit_mode_enabled() { return false; }
    bool is_elem_visible(service_ptr_t<class ui_element_instance> elem) { return true; }
    t_size notify(ui_element_instance* source, const GUID& what, t_size param1, const void* param2, t_size param2size) { return 0; }

    bool query_color(const GUID& guid, t_ui_color& out_colour) override
    {
        if(guid == ui_color_text)
        {
            out_colour = colours.get_colour(columns_ui::colours::colour_text);
            return true;
        }
        else if(guid == ui_color_background)
        {
            out_colour = colours.get_colour(columns_ui::colours::colour_background);
            return true;
        }
        else if(guid == ui_color_highlight)
        {
            out_colour = colours.get_colour(columns_ui::colours::colour_inactive_selection_text);
            return true;
        }
        else if(guid == ui_color_selection)
        {
            out_colour = colours.get_colour(columns_ui::colours::colour_selection_text);
            return true;
        }

        return false;
    }

    t_ui_font query_font_ex(const GUID& /*guid*/) override
    {
        uint64_t factory_font_gen = s_font_client_factory.get_static_instance().get_font_generation();
        if(factory_font_gen > font_generation)
        {
            font_generation = factory_font_gen;

            if(font_uie != nullptr) DeleteObject(font_uie);
            font_uie = font_manager->get_font(TPanel::g_get_guid());
        }

        return font_uie;
    }

private:
    inline static service_factory_single_t<uie_shim_colour_client<TPanel>> s_colour_client_factory = {};
    inline static service_factory_single_t<uie_shim_font_client<TPanel>> s_font_client_factory = {};

    columns_ui::colours::helper colours;
    columns_ui::fonts::manager::ptr font_manager;

    HFONT font_uie;
    uint64_t font_generation;
};

template<typename TPanel>
class uie_shim_panel : public ui_extension::window
{
public:
    void get_category(pfc::string_base& out) const override { out = "Panels"; }
    unsigned int get_type() const override { return ui_extension::type_panel; }
    bool is_available(const ui_extension::window_host_ptr& p_host) const override { return true; }

    const GUID& get_extension_guid() const override
    {
        static GUID guid = TPanel::g_get_guid();
        return guid;
    }

    void get_name(pfc::string_base& out) const override
    {
        TPanel::g_get_name(out);
    }

    void set_config(stream_reader* reader, t_size size, abort_callback& abort) override
    {
        config = ui_element_config::g_create(TPanel::g_get_guid(), reader, size, abort);
    }

    void get_config(stream_writer* writer, abort_callback& abort) const override
    {
        if(config != nullptr)
        {
            writer->write(config->get_data(), config->get_data_size(), abort);
        }
    }

    HWND create_or_transfer_window(HWND wnd_parent,
                                   const ui_extension::window_host_ptr& p_host,
                                   const ui_helpers::window_position_t& p_position) override
    {
        if(instance != nullptr)
        {
            pfc::string8 name;
            get_name(name);
            LOG_ERROR("Attempt to create UIE panel '%s' when one already exists", name.c_str());
            return nullptr;
        }

        ui_element_config::ptr cfg = config;
        if(cfg == nullptr)
        {
            cfg = TPanel::g_get_default_configuration();
        }

        service_impl_single_t<TPanelImpl> impl;
        ui_element_instance_callback_ptr callback = new service_impl_t<uie_shim_instance_callback_impl<TPanel>>();
        instance = impl.instantiate(wnd_parent, cfg, callback);
        return instance->get_wnd();
    }

    void destroy_window() override
    {
        if(instance == nullptr)
        {
            pfc::string8 name;
            get_name(name);
            LOG_ERROR("Attempt to destroy UIE panel '%s' when none exists", name.c_str());
            return;
        }

        DestroyWindow(instance->get_wnd());
        instance.release();
    }

    HWND get_wnd() const override
    {
        if(instance == nullptr)
        {
            pfc::string8 name;
            get_name(name);
            LOG_ERROR("Attempt to get handle for UIE panel '%s' when no such panel exists", name.c_str());
            return {};
        }

        return instance->get_wnd();
    }

private:
    class TPanelImpl : public ui_element_impl_withpopup<TPanel> {};

    ui_element_instance::ptr instance = nullptr;
    ui_element_config::ptr config = nullptr;
};
