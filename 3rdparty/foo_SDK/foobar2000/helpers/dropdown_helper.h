#pragma once

#include "../SDK/cfg_var.h"

class _cfg_dropdown_history_base
{
	const unsigned m_max;
	void build_list(pfc::ptr_list_t<char> & out);
	void parse_list(const pfc::ptr_list_t<char> & src);
public:
	enum {separator = '\n'};

	_cfg_dropdown_history_base(const GUID & varid, unsigned p_max = 10, const char * initial = "") : m_max(p_max), m_state(varid, initial) {}
#ifdef FOOBAR2000_DESKTOP_WINDOWS
    void on_init(HWND ctrl, const char * initVal) {
        add_item(initVal); setup_dropdown(ctrl); uSetWindowText(ctrl, initVal);
    }
	void setup_dropdown(HWND wnd);
	void setup_dropdown_set_value(HWND wnd);
	void setup_dropdown(HWND wnd,UINT id) {setup_dropdown(GetDlgItem(wnd,id));}
    bool on_context(HWND wnd,LPARAM coords); //returns true when the content has changed
    bool add_item(const char * item, HWND combobox); //immediately adds the item to the combobox
#endif
	bool add_item(const char * item); //returns true when the content has changed, false when not (the item was already on the list)
	bool is_empty();

	void set_state(const char* val) { m_state = val; }
	void get_state(pfc::string_base& out) { out = m_state.get(); }
private:
	cfg_string m_state;
};

typedef _cfg_dropdown_history_base cfg_dropdown_history;
typedef _cfg_dropdown_history_base cfg_dropdown_history_mt;

#ifdef FOOBAR2000_DESKTOP_WINDOWS
// ATL-compatible message map entry macro for installing dropdown list context menus.
#define DROPDOWN_HISTORY_HANDLER(ctrlID,var) \
	if(uMsg == WM_CONTEXTMENU) { \
		const HWND source = (HWND) wParam; \
		if (source != NULL && source == CWindow(hWnd).GetDlgItem(ctrlID)) { \
			var.on_context(source,lParam);	\
			lResult = 0;	\
			return TRUE;	\
		}	\
	}

#endif // FOOBAR2000_DESKTOP_WINDOWS
