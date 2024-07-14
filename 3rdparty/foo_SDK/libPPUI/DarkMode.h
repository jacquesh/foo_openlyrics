#pragma once
#include <functional>
#include <list>

namespace DarkMode {
	// Is dark mode supported on this system or not?
	bool IsSupportedSystem();
	// Is system in dark mode or not?
	bool QueryUserOption();
	
	// Darken menus etc app-wide
	void SetAppDarkMode(bool bDark);
	// Darken window title bar
	void UpdateTitleBar(HWND wnd, bool bDark );
	void ApplyDarkThemeCtrl(HWND ctrl, bool bDark, const wchar_t * ThemeID = L"Explorer");
	void AllowDarkModeForWindow(HWND wnd, bool bDark);

	// One-shot version of darkening function for editboxes
	void DarkenEditLite(HWND ctrl);
	// One-shot version of darkening function for comboboxes
	void DarkenComboLite(HWND ctrl);

	// Returns if the dialog appears to be using dark mode (text brighter than background)
	bool IsDialogDark(HWND dlg, UINT msgSend = WM_CTLCOLORDLG);
	// Returns if the DC appears to be using dark mode (text brighter than background)
	bool IsDCDark(HDC dc);
	
	// Returns if these colors (text, background) look like dark theme
	bool IsThemeDark(COLORREF text, COLORREF background);

	COLORREF GetSysColor(int, bool bDark = true);

	LRESULT CustomDrawToolbar(NMHDR*);
	// Custom draw handler that paints registered darkened controls.
	// Can be used with NOTIFY_CODE_HANDLER() directly
	LRESULT OnCustomDraw(int, NMHDR*, BOOL & bHandled);

	// Handle WM_NCPAINT drawing dark frame
	void NCPaintDarkFrame(HWND ctrl, HRGN rgn);

	bool IsHighContrast();

	// msgSetDarkMode
	// return 1 if understood, 0 otherwise
	// WPARAM = 0, DISABLE dark mode
	// WPARAM = 1, ENABLE dark mode
	// WPARAM = -1, query if supported
	UINT msgSetDarkMode();

	//! CHooks class: entrypoint class for all Dark Mode hacks. \n
	//! Usage: Keep CHooks m_dark = detectDarkMode(); as a member of your window class, replacing detectDarkMode() with your own function returning dark mode on/off state. \n
	//! When initializing your window (WM_CREATE, WM_INITDIALOG), call m_dark.AddDialogWithControls(m_hWnd); \n
	//! AddDialogWithControls() walks all child windows of your window; call other individual methods to finetune handling of Dark Mode if that's not acceptable in your case.
	class CHooks {
	public:
		CHooks(bool enabled = false) : m_dark(enabled) {}
		CHooks(const CHooks&) = delete;
		void operator=(const CHooks&) = delete;

		void AddDialog(HWND);
		void AddTabCtrl(HWND);
		void AddComboBox(HWND);
		void AddComboBoxEx(HWND);
		void AddButton(HWND);
		void AddEditBox(HWND);
		void AddPopup(HWND);
		void AddStatusBar(HWND);
		void AddScrollBar(HWND);
		void AddToolBar(HWND, bool bExplorerTheme = true);
		void AddReBar(HWND);
		void AddStatic(HWND);
		void AddUpDown(HWND);
		void AddListBox(HWND);
		void AddListView(HWND);
		void AddTreeView(HWND);
		void AddPPListControl(HWND);


		// SetWindowTheme with DarkMode_Explorer <=> Explorer
		void AddGeneric(HWND, const wchar_t * name = L"explorer");
		// SetWindowTheme(wnd, L"", L"") for dark, Explorer theme for normal
		void AddClassic(HWND, const wchar_t* normalTheme = L"explorer");

		void AddCtrlAuto(HWND);
		void AddCtrlMsg(HWND);

		void AddDialogWithControls(HWND);
		void AddControls(HWND wndParent);

		void SetDark(bool v = true);
		bool IsDark() const { return m_dark; }
		operator bool() const { return m_dark; }

		~CHooks() { clear(); }
		void clear();

		void AddApp();
	private:
		template<typename obj_t> void addObj(obj_t* arg) { 
			m_apply.push_back([arg, this] { arg->SetDark(m_dark); });
			m_cleanup.push_back([arg] { delete arg; });
		}
		void addOp(std::function<void()> f) { f(); m_apply.push_back(f); }
		bool m_dark = false;
		std::list<std::function<void()> > m_apply;
		std::list<std::function<void()> > m_cleanup;

		void flushMoveToBack();
		std::list<HWND> m_lstMoveToBack;
	}; 
}
