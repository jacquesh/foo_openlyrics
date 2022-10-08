#pragma once
#include "win32_op.h"

namespace PP {
	// Recreate std control with a different style, keep position, title, font
	template<typename CWindowT>
	void reStyleCtrl(CWindowT& btn, DWORD winStyle, DWORD winStyleEx) {
		CWindowT btnNew;
		CString title;
		CWindow parent = btn.GetParent();
		btn.GetWindowText(title);
		CRect rc; WIN32_OP_D(btn.GetWindowRect(rc)); WIN32_OP_D(parent.ScreenToClient(rc));
		CWindow wndPrev = parent.GetNextDlgTabItem(btn, TRUE);
		auto ctrlID = btn.GetDlgCtrlID();
		auto font = btn.GetFont();
		btn.DestroyWindow();
		WIN32_OP_D(btnNew.Create(parent, rc, title, winStyle | WS_CLIPSIBLINGS, winStyleEx, ctrlID));
		if (wndPrev != NULL) btnNew.SetWindowPos(wndPrev, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		btnNew.SetFont(font);
		btn = btnNew;
	}
}