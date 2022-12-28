#pragma once

#ifdef FOOBAR2000_DESKTOP_WINDOWS

//DEPRECATED dialog helpers - kept only for compatibility with old code - do not use in new code, use WTL instead.

namespace dialog_helper
{
	
	class dialog
	{
	protected:
		
		dialog() : wnd(0), m_is_modal(false) {}
		~dialog() { }

		virtual BOOL on_message(UINT msg,WPARAM wp,LPARAM lp)=0;

		void end_dialog(int code);

	public:
		inline HWND get_wnd() {return wnd;}

		__declspec(deprecated) int run_modal(unsigned id,HWND parent);

		__declspec(deprecated) HWND run_modeless(unsigned id,HWND parent);
	private:
		HWND wnd;
		static INT_PTR CALLBACK DlgProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);

		bool m_is_modal;

		modal_dialog_scope m_modal_scope;
	};

	//! This class is meant to be instantiated on-stack, as a local variable. Using new/delete operators instead or even making this a member of another object works, but does not make much sense because of the way this works (single run() call).
	class dialog_modal
	{
	public:
		__declspec(deprecated) int run(unsigned p_id,HWND p_parent,HINSTANCE p_instance = core_api::get_my_instance());
	protected:
		virtual BOOL on_message(UINT msg,WPARAM wp,LPARAM lp)=0;

		inline dialog_modal() : m_wnd(0) {}
		void end_dialog(int p_code);
		inline HWND get_wnd() const {return m_wnd;}
	private:
		static INT_PTR CALLBACK DlgProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);

		HWND m_wnd;
		modal_dialog_scope m_modal_scope;
	};

};

//! Wrapper (provided mainly for old code), simplifies parameters compared to standard CreateDialog() by using core_api::get_my_instance().
HWND uCreateDialog(UINT id,HWND parent,DLGPROC proc,LPARAM param = 0);
//! Wrapper (provided mainly for old code), simplifies parameters compared to standard DialogBox() by using core_api::get_my_instance().
int uDialogBox(UINT id,HWND parent,DLGPROC proc,LPARAM param = 0);


#endif // FOOBAR2000_DESKTOP_WINDOWS