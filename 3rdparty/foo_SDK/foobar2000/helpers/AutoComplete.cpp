#include "stdafx.h"
#include "AutoComplete.h"
#include <ShlGuid.h> // CLSID_AutoComplete
#include "../helpers/COM_utils.h"
#include "../helpers/dropdown_helper.h"
#include <libPPUI/CEnumString.h>

using PP::CEnumString;

namespace {
	class CACList_History : public CEnumString {
	public:
		CACList_History(cfg_dropdown_history_mt * var) : m_var(var) { Reset(); }
		typedef ImplementCOMRefCounter<CACList_History> TImpl;

		HRESULT STDMETHODCALLTYPE Reset() {
			/*if (core_api::assert_main_thread())*/ {
				ResetStrings();
				pfc::string8 state; m_var->get_state(state);
				for (const char * walk = state;;) {
					const char * next = strchr(walk, cfg_dropdown_history_mt::separator);
					t_size len = (next != NULL) ? next - walk : ~0;
					AddStringU(walk, len);
					if (next == NULL) break;
					walk = next + 1;
				}
			}
			return __super::Reset();
		}

		HRESULT STDMETHODCALLTYPE Clone(IEnumString **ppenum) {
			*ppenum = new TImpl(*this); return S_OK;
		}

	private:
		cfg_dropdown_history_mt * const m_var;
	};
}

HRESULT InitializeDropdownAC(HWND comboBox, cfg_dropdown_history_mt & var, const char * initVal) {
	var.on_init(comboBox, initVal);
	COMBOBOXINFO ci = {}; ci.cbSize = sizeof(ci);
	if (!GetComboBoxInfo(comboBox, &ci)) {
		PFC_ASSERT(!"Should not get here - GetComboBoxInfo fail!");
		return E_FAIL;
	}
	pfc::com_ptr_t<IUnknown> acl = new CACList_History::TImpl(&var);
	return InitializeSimpleAC(ci.hwndItem, acl.get_ptr(), ACO_AUTOAPPEND|ACO_AUTOSUGGEST);
}
