#pragma once
#include "stdafx.h"

#include "winstr_util.h"

struct cfg_auto_property
{
    virtual void Initialise(HWND container)
    {
        m_hWnd = container;
        ResetFromSaved();
    }
    virtual void ResetFromSaved() = 0;
    virtual void ResetToDefault() = 0;
    virtual void Apply() = 0;
    virtual bool HasChanged() = 0;

protected:
    HWND m_hWnd = nullptr;
};

struct cfg_auto_string : public cfg_string, public cfg_auto_property
{
    cfg_auto_string(const GUID& guid, int control_id, const char* default_value) :
        cfg_string(guid, default_value),
        m_control_id(control_id),
        m_default_value(default_value)
    {
    }

    void ResetFromSaved() override
    {
        std::tstring saved_value = to_tstring(*this);
        SetDlgItemText(m_hWnd, m_control_id, saved_value.c_str());
    }

    void ResetToDefault() override
    {
        std::tstring default_value = to_tstring(m_default_value);
        SetDlgItemText(m_hWnd, m_control_id, default_value.c_str());
    }

    void Apply() override
    {
        LRESULT text_length_result = SendDlgItemMessage(m_hWnd, m_control_id, WM_GETTEXTLENGTH, 0, 0);
        if(text_length_result < 0) return;
        size_t text_length = (size_t)text_length_result;
        TCHAR* text_buffer = new TCHAR[text_length+1]; // +1 for null-terminator
        UINT chars_copied = GetDlgItemText(m_hWnd, m_control_id, text_buffer, text_length+1);

        std::string ui_string = from_tstring(std::tstring_view{text_buffer, chars_copied});
        set_string(pfc::string8(ui_string.c_str(), ui_string.length()));

        delete[] text_buffer;
    }

    bool HasChanged() override
    {
        LRESULT text_length_result = SendDlgItemMessage(m_hWnd, m_control_id, WM_GETTEXTLENGTH, 0, 0);
        if(text_length_result < 0)
        {
            return true;
        }
        size_t text_length = (size_t)text_length_result;
        TCHAR* text_buffer = new TCHAR[text_length+1]; // +1 for null-terminator
        UINT chars_copied = GetDlgItemText(m_hWnd, m_control_id, text_buffer, text_length+1);

        std::string ui_string = from_tstring(std::tstring_view{text_buffer, chars_copied});
        bool changed = (ui_string != std::string_view{get_ptr(), get_length()});

        delete[] text_buffer;
        return changed;
    }

private:
    int m_control_id;
    std::string m_default_value;
};

template<typename TInt>
struct cfg_auto_int_t : public cfg_int_t<TInt>, public cfg_auto_property
{
    cfg_auto_int_t(const GUID& guid, int control_id, TInt default_value) :
        cfg_int_t<TInt>(guid, default_value),
        m_control_id(control_id),
        m_default_value(default_value)
    {
    }

    void ResetFromSaved() override
    {
        SetDlgItemInt(m_hWnd, m_control_id, cfg_int_t<TInt>::get_value(), std::is_signed<TInt>());
    }

    void ResetToDefault() override
    {
        SetDlgItemInt(m_hWnd, m_control_id, m_default_value, std::is_signed<TInt>());
    }

    void Apply() override
    {
        UINT value = GetDlgItemInt(m_hWnd, m_control_id, nullptr, std::is_signed<TInt>());
        cfg_int_t<TInt>::operator=(value);
    }

    bool HasChanged() override
    {
        UINT ui_value = GetDlgItemInt(m_hWnd, m_control_id, nullptr, std::is_signed<TInt>());
        TInt stored_value = cfg_int_t<TInt>::get_value();
        return ui_value != static_cast<UINT>(stored_value);
    }

private:
    int m_control_id;
    TInt m_default_value;
};

typedef cfg_auto_int_t<t_int32> cfg_auto_int;
typedef cfg_auto_int_t<t_uint32> cfg_auto_uint;
typedef cfg_auto_int_t<float> cfg_auto_float;
typedef cfg_auto_int_t<double> cfg_auto_double;

struct cfg_auto_bool : public cfg_int_t<bool>, public cfg_auto_property
{
    cfg_auto_bool(const GUID& guid, int control_id, bool default_value) :
        cfg_int_t<bool>(guid, default_value),
        m_control_id(control_id),
        m_default_value(default_value)
    {
    }

    void ResetFromSaved() override
    {
        CheckDlgButton(m_hWnd, m_control_id, cfg_int_t<bool>::get_value() ? BST_CHECKED : BST_UNCHECKED);
    }

    void ResetToDefault() override
    {
        CheckDlgButton(m_hWnd, m_control_id, m_default_value ? BST_CHECKED : BST_UNCHECKED);
    }

    void Apply() override
    {
        LRESULT checkResult = SendDlgItemMessage(m_hWnd, m_control_id, BM_GETCHECK, 0, 0);
        bool checked = (checkResult == BST_CHECKED);
        cfg_int_t<bool>::operator=(checked);
    }

    bool HasChanged() override
    {
        LRESULT checkResult = SendDlgItemMessage(m_hWnd, m_control_id, BM_GETCHECK, 0, 0);
        bool ui_value = (checkResult == BST_CHECKED);
        bool stored_value = cfg_int_t<bool>::get_value();
        return ui_value != stored_value;
    }

private:
    int m_control_id;
    bool m_default_value;
};

template<typename TEnum>
struct cfg_auto_combo_option
{
    const TCHAR* display_string;
    TEnum config_value;
};

template<typename TEnum, int IOptionCount>
struct cfg_auto_combo : private cfg_int_t<int>, public cfg_auto_property
{
    cfg_auto_combo(const GUID& guid, int control_id, TEnum default_value, cfg_auto_combo_option<TEnum> (&options)[IOptionCount]) :
        cfg_int_t<int>(guid, static_cast<int>(default_value)),
        m_control_id(control_id),
        m_options(options),
        m_default_value(default_value)
    {
    }

    TEnum get_value()
    {
        return (TEnum)cfg_int_t<int>::get_value();
    }

    void Initialise(HWND container) override
    {
        for(int i=0; i<IOptionCount; i++)
        {
            cfg_auto_combo_option<TEnum>& option = m_options[i];
            LRESULT add_result = SendDlgItemMessage(container, m_control_id, CB_ADDSTRING, 0, (LPARAM)option.display_string);
            assert(add_result != CB_ERR);
            assert(add_result != CB_ERRSPACE);

            static_assert(sizeof(LPARAM) >= sizeof(int), "LPARAM must have enough space to store the enum");
            LRESULT set_result = SendDlgItemMessage(container, m_control_id, CB_SETITEMDATA, add_result, (LPARAM)option.config_value);
            assert(set_result != CB_ERR);
        }

        cfg_auto_property::Initialise(container);
    }

    void ResetFromSaved() override
    {
        int saved_logical_value = cfg_int_t<int>::get_value();

        for(int ui_index=0; ui_index<IOptionCount; ui_index++)
        {
            LRESULT item_logical_value = SendDlgItemMessage(m_hWnd, m_control_id, CB_GETITEMDATA, ui_index, 0);
            assert(item_logical_value != CB_ERR);

            if(item_logical_value == saved_logical_value)
            {
                LRESULT result = SendDlgItemMessage(m_hWnd, m_control_id, CB_SETCURSEL, ui_index, 0);
                assert(result != CB_ERR);
                return;
            }
        }

        // If we get here then we've exhausted all valid options and not found the configured value
        ResetToDefault();
        Apply();
    }

    void ResetToDefault() override
    {
        for(int ui_index=0; ui_index<IOptionCount; ui_index++)
        {
            LRESULT item_logical_value = SendDlgItemMessage(m_hWnd, m_control_id, CB_GETITEMDATA, ui_index, 0);
            assert(item_logical_value != CB_ERR);

            if(item_logical_value == static_cast<int>(m_default_value))
            {
                LRESULT result = SendDlgItemMessage(m_hWnd, m_control_id, CB_SETCURSEL, ui_index, 0);
                assert(result != CB_ERR);
                return;
            }
        }
    }

    void Apply() override
    {
        LRESULT ui_index = SendDlgItemMessage(m_hWnd, m_control_id, CB_GETCURSEL, 0, 0);
        LRESULT logical_value = SendDlgItemMessage(m_hWnd, m_control_id, CB_GETITEMDATA, ui_index, 0);
        assert(logical_value != CB_ERR);

        cfg_int_t<int>::operator=(logical_value);
    }

    bool HasChanged() override
    {
        LRESULT ui_index = SendDlgItemMessage(m_hWnd, m_control_id, CB_GETCURSEL, 0, 0);
        LRESULT logical_value = SendDlgItemMessage(m_hWnd, m_control_id, CB_GETITEMDATA, ui_index, 0);
        assert(logical_value != CB_ERR);

        int stored_value = cfg_int_t<int>::get_value();
        return logical_value != stored_value;
    }

private:
    int m_control_id;
    cfg_auto_combo_option<TEnum>* m_options;
    TEnum m_default_value;
};

class auto_preferences_page_instance : public preferences_page_instance
{
public:
    template<int N>
    auto_preferences_page_instance(preferences_page_callback::ptr callback, cfg_auto_property* (&auto_props)[N]) :
        m_callback(callback)
    {
        m_auto_properties.reserve(N);
        for(cfg_auto_property* prop : auto_props)
        {
            m_auto_properties.push_back(prop);
        }
    }

    void init_auto_preferences()
    {
        HWND handle = get_wnd();
        for(cfg_auto_property* prop : m_auto_properties)
        {
            prop->Initialise(handle);
        }
    }

    // preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
    t_uint32 get_state() override
    {
        t_uint32 state = preferences_state::resettable;
        if (has_changed()) state |= preferences_state::changed;
        return state;
    }
    void apply() override
    {
        for(cfg_auto_property* prop : m_auto_properties)
        {
            prop->Apply();
        }
        on_ui_interaction(); // our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
    }
    void reset() override
    {
        for(cfg_auto_property* prop : m_auto_properties)
        {
            prop->ResetToDefault();
        }
        on_ui_interaction();
    }

    virtual bool has_changed()
    {
        for(cfg_auto_property* prop : m_auto_properties)
        {
            if(prop->HasChanged()) return true;
        }
        return false;
    }

    void on_ui_interaction()
    {
        //tell the host that our state has changed to enable/disable the apply button appropriately.
        m_callback->on_state_changed();
    }

private:
    const preferences_page_callback::ptr m_callback;
    std::vector<cfg_auto_property*> m_auto_properties;
};
