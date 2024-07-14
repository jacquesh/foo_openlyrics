#pragma once
#include "ImplementOnFinalMessage.h"
#include "win32_op.h"
#include <functional>

namespace PP {
	//! Create a new window object - obj_t derived from CWindowImpl<> etc, with automatic lifetime management, \n
	//! OnFinalMessage() is automatically overridden to delete the window object. \n
	//! Does NOT create the window itself, it's up to you to call Create() or SubclassWindow() with the returned object.
	template<typename obj_t, typename ... arg_t>
	obj_t* newWindowObj(arg_t && ... arg) {
		return new ImplementOnFinalMessage<obj_t>(std::forward<arg_t>(arg) ...);
	}

	//! Subclass HWND with window class object obj_t deriving from CWindowImpl<> etc, with automatic lifetime management of the object, \n
	//! OnFinalMessage() is automatically overridden to delete the window subclass object.
	template<typename obj_t, typename ... arg_t>
	obj_t* subclassThisWindow(HWND wnd, arg_t && ... arg) {
		PFC_ASSERT( wnd != NULL );
		auto ret = newWindowObj<obj_t>(std::forward<arg_t>(arg) ...);
		WIN32_OP_D(ret->SubclassWindow(wnd));
		return ret;
	}

	//! Creates a new window object, of ctrl_t typem with automatic lifetime management, 
	//! and replaces an existing control in a dialog.
	template<typename ctrl_t>
	ctrl_t * replaceDialogCtrl(CWindow wndDialog, UINT replaceControlID) {
		CWindow wndReplace = wndDialog.GetDlgItem(replaceControlID);
		ATLASSERT(wndReplace != NULL);
		CRect rc;
		CWindow wndPrev = wndDialog.GetNextDlgTabItem(wndReplace, TRUE);
		WIN32_OP_D(wndReplace.GetWindowRect(&rc));
		WIN32_OP_D(wndDialog.ScreenToClient(rc));
		CString text;
		wndReplace.GetWindowText(text);
		WIN32_OP_D(wndReplace.DestroyWindow());
		auto ctrl = newWindowObj<ctrl_t>();
		WIN32_OP_D(ctrl->Create(wndDialog, &rc, text, 0, 0, replaceControlID));
		if (wndPrev != NULL) ctrl->SetWindowPos(wndPrev, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		ctrl->SetFont(wndDialog.GetFont());
		return ctrl;
	}
}