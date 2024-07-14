#include "stdafx.h"
#include "EditBoxFixes.h"
#include "wtl-pp.h"
#include "windowLifetime.h"

namespace PP {
	void editBoxFix(HWND wndEdit) {
		PFC_ASSERT( IsWindow(wndEdit) );
		PP::subclassThisWindow<CEditPPHooks>(wndEdit);
	}
	void comboBoxFix(HWND wndCombo) {
		PFC_ASSERT( IsWindow(wndCombo) );
		CComboBox combo = wndCombo;
		COMBOBOXINFO info = { sizeof(info) };
		if (combo.GetComboBoxInfo(&info)) {
			if ( info.hwndItem != NULL ) editBoxFix( info.hwndItem );
		}
	}
}