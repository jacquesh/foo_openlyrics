#pragma once
#include "stdafx.h"

struct cfg_auto_property
{
    void Initialise(HWND container)
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

