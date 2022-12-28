#include "stdafx.h"

#ifdef FOOBAR2000_DESKTOP_WINDOWS

#include "win32_misc.h"
#include "win32_dialog.h"

namespace dialog_helper {


	INT_PTR CALLBACK dialog::DlgProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		dialog * p_this;
		BOOL rv;
		if (msg==WM_INITDIALOG)
		{
			p_this = reinterpret_cast<dialog*>(lp);
			p_this->wnd = wnd;
			SetWindowLongPtr(wnd,DWLP_USER,lp);

			if (p_this->m_is_modal) p_this->m_modal_scope.initialize(wnd);
		}
		else p_this = reinterpret_cast<dialog*>(GetWindowLongPtr(wnd,DWLP_USER));

		rv = p_this ? p_this->on_message(msg,wp,lp) : FALSE;

		if (msg==WM_DESTROY && p_this)
		{
			SetWindowLongPtr(wnd,DWLP_USER,0);
//			p_this->wnd = 0;
		}

		return rv;
	}


	int dialog::run_modal(unsigned id,HWND parent)
	{
		assert(wnd == 0);
		if (wnd != 0) return -1;
		m_is_modal = true; 
		return uDialogBox(id,parent,DlgProc,reinterpret_cast<LPARAM>(this));
	}
	HWND dialog::run_modeless(unsigned id,HWND parent)
	{
		assert(wnd == 0);
		if (wnd != 0) return 0;
		m_is_modal = false; 
		return uCreateDialog(id,parent,DlgProc,reinterpret_cast<LPARAM>(this));
	}

	void dialog::end_dialog(int code)
	{
		assert(m_is_modal); 
		if (m_is_modal) uEndDialog(wnd,code);
	}










	int dialog_modal::run(unsigned p_id,HWND p_parent,HINSTANCE p_instance)
	{
		int status;

		// note: uDialogBox() has its own modal scope, we don't want that to trigger
		// if this is ever changed, move deinit to WM_DESTROY handler in DlgProc

		status = (int)DialogBoxParam(p_instance,MAKEINTRESOURCE(p_id),p_parent,DlgProc,reinterpret_cast<LPARAM>(this));

		m_modal_scope.deinitialize();

		return status;
	}
		
	void dialog_modal::end_dialog(int p_code)
	{
		EndDialog(m_wnd,p_code);
	}

		
	INT_PTR CALLBACK dialog_modal::DlgProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		dialog_modal * _this;
		if (msg==WM_INITDIALOG)
		{
			_this = reinterpret_cast<dialog_modal*>(lp);
			_this->m_wnd = wnd;
			SetWindowLongPtr(wnd,DWLP_USER,lp);

			_this->m_modal_scope.initialize(wnd);
		}
		else _this = reinterpret_cast<dialog_modal*>(GetWindowLongPtr(wnd,DWLP_USER));

		assert(_this == 0 || _this->m_wnd == wnd);

		return _this ? _this->on_message(msg,wp,lp) : FALSE;
	}

}

HWND uCreateDialog(UINT id,HWND parent,DLGPROC proc,LPARAM param)
{
	return CreateDialogParam(core_api::get_my_instance(),MAKEINTRESOURCE(id),parent,proc,param);
}

int uDialogBox(UINT id,HWND parent,DLGPROC proc,LPARAM param)
{
	return (int)DialogBoxParam(core_api::get_my_instance(),MAKEINTRESOURCE(id),parent,proc,param);
}

#endif // FOOBAR2000_DESKTOP_WINDOWS
