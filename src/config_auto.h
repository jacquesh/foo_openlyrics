#pragma once
#include "stdafx.h"

struct cfg_auto_property
{
    virtual void Initialise(HWND container)
    {
        m_hWnd = container;
        ResetFromSaved();
    }
    virtual void ResetFromSaved() = 0;
    virtual void Apply() = 0;
    virtual bool HasChanged() = 0;

protected:
    HWND m_hWnd = nullptr;
};

struct cfg_auto_string : public cfg_string, public cfg_auto_property
{
    cfg_auto_string(const GUID& guid, int control_id, const char* default_value) :
        cfg_string(guid, default_value),
        m_control_id(control_id)
    {
    }

    void ResetFromSaved() override
    {
#ifdef UNICODE
        size_t wide_len = pfc::stringcvt::estimate_utf8_to_wide(c_str(), length());
        wchar_t* wide_buffer = new wchar_t[wide_len];
        size_t chars_converted = pfc::stringcvt::convert_utf8_to_wide(wide_buffer, wide_len, c_str(), length());
        SetDlgItemText(m_hWnd, m_control_id, wide_buffer);
        delete[] wide_buffer;
#else
        SetDlgItemText(m_control_id, c_str());
#endif // UNICODE
    }

    void Apply() override
    {
        LRESULT text_length = SendDlgItemMessage(m_hWnd, m_control_id, WM_GETTEXTLENGTH, 0, 0);
        if(text_length <= 0) return;
        TCHAR* text_buffer = new TCHAR[text_length+1]; // +1 for null-terminator
        UINT chars_copied = GetDlgItemText(m_hWnd, m_control_id, text_buffer, text_length+1);

#ifdef UNICODE
        size_t narrow_len = pfc::stringcvt::estimate_wide_to_utf8(text_buffer, text_length);
        char* narrow_buffer = new char[narrow_len];
        size_t chars_converted = pfc::stringcvt::convert_wide_to_utf8(narrow_buffer, narrow_len, text_buffer, text_length);
        set_string(narrow_buffer);
        delete[] narrow_buffer;
#else
        set_string(text_buffer);
#endif // UNICODE

        delete[] text_buffer;
    }

    bool HasChanged() override
    {
        LRESULT text_length = SendDlgItemMessage(m_hWnd, m_control_id, WM_GETTEXTLENGTH, 0, 0);
        if(text_length <= 0)
        {
            return (*this == "");
        }
        TCHAR* text_buffer = new TCHAR[text_length+1]; // +1 for null-terminator
        UINT chars_copied = GetDlgItemText(m_hWnd, m_control_id, text_buffer, text_length+1);

#ifdef UNICODE
        size_t narrow_len = pfc::stringcvt::estimate_wide_to_utf8(text_buffer, text_length);
        char* narrow_buffer = new char[narrow_len];
        size_t chars_converted = pfc::stringcvt::convert_wide_to_utf8(narrow_buffer, narrow_len, text_buffer, text_length);
        bool changed = (*this != pfc::string8(narrow_buffer, narrow_len));
        delete[] narrow_buffer;
        return changed;
#else
        return (*this != pfc::string8(text_buffer, text_length));
#endif // UNICODE

        delete[] text_buffer;
    }

private:
    int m_control_id;
};

template<typename TInt>
struct cfg_auto_int_t : public cfg_int_t<TInt>, public cfg_auto_property
{
    cfg_auto_int_t(const GUID& guid, int control_id, TInt default_value) :
        cfg_int_t<TInt>(guid, default_value),
        m_control_id(control_id)
    {
    }

    void ResetFromSaved() override
    {
        SetDlgItemInt(m_hWnd, m_control_id, cfg_int_t<TInt>::get_value(), std::is_signed<TInt>());
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
};

typedef cfg_auto_int_t<t_int32> cfg_auto_int;
typedef cfg_auto_int_t<t_uint32> cfg_auto_uint;
typedef cfg_auto_int_t<float> cfg_auto_float;
typedef cfg_auto_int_t<double> cfg_auto_double;

struct cfg_auto_bool : public cfg_int_t<bool>, public cfg_auto_property
{
    cfg_auto_bool(const GUID& guid, int control_id, bool default_value) :
        cfg_int_t<bool>(guid, default_value),
        m_control_id(control_id)
    {
    }

    void ResetFromSaved() override
    {
        CheckDlgButton(m_hWnd, m_control_id, cfg_int_t<bool>::get_value() ? BST_CHECKED : BST_UNCHECKED);
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
};

template<typename TEnum>
struct cfg_auto_combo_option
{
    const TCHAR* display_string;
    TEnum config_value;
};

template<typename TEnum, int IOptionCount>
struct cfg_auto_combo : public cfg_int_t<int>, public cfg_auto_property
{
    cfg_auto_combo(const GUID& guid, int control_id, TEnum default_value, cfg_auto_combo_option<TEnum> (&options)[IOptionCount]) :
        cfg_int_t<int>(guid, static_cast<int>(default_value)),
        m_control_id(control_id),
        m_options(options)
    {
    }

    void Initialise(HWND container) override
    {
        for(int i=0; i<IOptionCount; i++)
        {
            cfg_auto_combo_option<TEnum>& option = m_options[i];
            LRESULT add_result = SendDlgItemMessage(container, m_control_id, CB_ADDSTRING, 0, (LPARAM)option.display_string);
            assert(add_result != CB_ERR);
            assert(add_result != CB_ERRSPACE);

            m_index_mapping[i].ui_value = add_result;
            m_index_mapping[i].logical_value = static_cast<int>(m_options[i].config_value);

            for(int j=0; j<i; j++)
            {
                if(m_index_mapping[j].ui_value >= add_result)
                {
                    m_index_mapping[j].ui_value++;
                }
            }
        }

        cfg_auto_property::Initialise(container);
    }

    void ResetFromSaved() override
    {
        int logical_value = cfg_int_t<int>::get_value();
        int ui_value = logical_to_ui_value(logical_value);
        LRESULT result = SendDlgItemMessage(m_hWnd, m_control_id, CB_SETCURSEL, ui_value, 0);
        assert(result != CB_ERR);
    }

    void Apply() override
    {
        LRESULT ui_value = SendDlgItemMessage(m_hWnd, m_control_id, CB_GETCURSEL, 0, 0);
        int logical_value = ui_to_logical_value(ui_value);
        cfg_int_t<int>::operator=(logical_value);
    }

    bool HasChanged() override
    {
        LRESULT ui_value = SendDlgItemMessage(m_hWnd, m_control_id, CB_GETCURSEL, 0, 0);
        int logical_value = ui_to_logical_value(ui_value);
        int stored_value = cfg_int_t<int>::get_value();
        return logical_value != stored_value;
    }

private:
    struct IndexMap
    {
        int logical_value;
        int ui_value;
    };

    int ui_to_logical_value(int ui_value)
    {
        assert((ui_value >= 0) && (ui_value < IOptionCount));
        for(IndexMap& map : m_index_mapping)
        {
            if(map.ui_value == ui_value)
            {
                return map.logical_value;
            }
        }
        assert(false);
        return 0;
    }

    int logical_to_ui_value(int logical_value)
    {
        for(IndexMap& map : m_index_mapping)
        {
            if(map.logical_value == logical_value)
            {
                assert((map.ui_value >= 0) && (map.ui_value < IOptionCount));
                return map.ui_value;
            }
        }
        assert(false);
        return 0;
    }

    int m_control_id;
    cfg_auto_combo_option<TEnum>* m_options;
    IndexMap m_index_mapping[IOptionCount]; // A mapping of logical/input values to UI values (so this is indexed by enum values and contains the integers used on the UI)
};
