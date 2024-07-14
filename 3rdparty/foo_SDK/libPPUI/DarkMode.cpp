#include "stdafx.h"
#include "DarkMode.h"
#include "DarkModeEx.h"
#include "win32_utility.h"
#include "win32_op.h"
#include "PaintUtils.h"
#include "ImplementOnFinalMessage.h"
#include <vsstyle.h>
#include "GDIUtils.h"
#include "CListControl.h"
#include "CListControl-Subst.h"
#include "ReStyleWnd.h"
#include <map>

// Allow scary undocumented ordinal-dll-export functions?
#define DARKMODE_ALLOW_HAX 1

#define DARKMODE_DEBUG 0

#if DARKMODE_DEBUG
#define DARKMODE_DEBUG_PRINT(...) PFC_DEBUG_PRINT("DarkMode: ", __VA_ARGS__)
#else
#define DARKMODE_DEBUG_PRINT(...)
#endif


#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

/*

==== DARK MODE KNOWLEDGE BASE ====

== Generic window ==
UpdateTitleBar() to set title bar to black

== Dialog box ==
Use WM_CTLCOLORDLG, background of 0x383838
UpdateTitleBar() to set title bar to black

== Edit box ==
Use WM_CTLCOLOREDIT, background of 0x383838
ApplyDarkThemeCtrl() with "Explorer"

== Drop list combo ==
Method #1: ::SetWindowTheme(wnd, L"DarkMode_CFD", nullptr);
Method #2: ::SetWindowTheme(wnd, L"", L""); to obey WM_CTLCOLOR* but leaves oldstyle classic-colors button and breaks comboboxex

== Button ==
Use WM_CTLCOLORBTN, background of 0x383838
ApplyDarkThemeCtrl() with "Explorer"

== Scroll bar ==
ApplyDarkThemeCtrl() with "Explorer"
^^ on either scrollbar or the window that implicitly creates scrollbars

== Header ==
ApplyDarkThemeCtrl() with "ItemsView"
Handle custom draw, override text color

== Tree view ==
ApplyDarkThemeCtrl() with "Explorer"
Set text/bk colors explicitly

Text color: 0xdedede
Background: 0x191919

Label-editing:
Pass WM_CTLCOLOR* to parent, shim TVM_EDITLABEL to pass theme to the editbox (not really necessary tho)


== Rebar ==
Can be beaten into working to some extent with a combination of:
* ::SetWindowTheme(toolbar, L"", L""); or else RB_SETTEXTCOLOR & REBARBANDINFO colors are disregarded
* Use RB_SETTEXTCOLOR / SetTextColor() + RB_SETBKCOLOR / SetBkColor() to override text/background colors
* Override WM_ERASEBKGND to draw band frames, RB_SETBKCOLOR doesn't seem to be thorough
* NM_CUSTOMDRAW on parent window to paint band labels & grippers without annoying glitches
NM_CUSTOMDRAW is buggy, doesn't hand you band indexes to query text, have to use hit tests to know what text to render

Solution: full custom draw


== Toolbar ==
Source: https://stackoverflow.com/questions/61271578/winapi-toolbar-text-color
Respects background color of its parent
Override text:
::SetWindowTheme(toolbar, L"", L""); or else NM_CUSTOMDRAW color is disregarded
NM_CUSTOMDRAW handler: 
switch (cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT: return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT: cd->clrText = DarkMode::GetSysColor(COLOR_WINDOWTEXT); return CDRF_DODEFAULT;
}

== Tab control ==
Full custom draw, see CTabsHook

== List View ==
Dark scrollbars are shown only if using "Explorer" theme, but other stuff needs "ItemsView" theme???
Other projects shim Windows functions to bypass the above.
Avoid using List View, use libPPUI CListControl instead.

== List Box ==
Use WM_CTLCOLOR*

== Status Bar ==
Full custom draw

== Check box, radio button ==
SetWindowTheme(wnd, L"", L""); works but not 100% pretty, disabled text ugly in particular
Full custom draw preferred

== Group box ===
SetWindowTheme(wnd, L"", L""); works but not 100% pretty, disabled text ugly in particular
Full custom draw preferred (we don't do this).
Avoid disabling groupboxes / use something else.

==== NOTES ====
AllowDarkModeForWindow() needs SetPreferredAppMode() to take effect, hence we implicitly call it
AllowDarkModeForWindow() must be called BEFORE SetWindowTheme() to take effect

It seems it is interchangeable to do:
AllowDarkModeForWindow(wnd, true); SetWindowTheme(wnd, L"foo", nullptr);
vs
SetWindowTheme(wnd, L"DarkMode_foo", nullptr)
But the latter doesn't require undocumented function calls and doesn't infect all menus with dark mode
*/

namespace {
	enum class PreferredAppMode
	{
		Default,
		AllowDark,
		ForceDark,
		ForceLight,
		Max
	};

	enum WINDOWCOMPOSITIONATTRIB
	{
		WCA_UNDEFINED = 0,
		WCA_NCRENDERING_ENABLED = 1,
		WCA_NCRENDERING_POLICY = 2,
		WCA_TRANSITIONS_FORCEDISABLED = 3,
		WCA_ALLOW_NCPAINT = 4,
		WCA_CAPTION_BUTTON_BOUNDS = 5,
		WCA_NONCLIENT_RTL_LAYOUT = 6,
		WCA_FORCE_ICONIC_REPRESENTATION = 7,
		WCA_EXTENDED_FRAME_BOUNDS = 8,
		WCA_HAS_ICONIC_BITMAP = 9,
		WCA_THEME_ATTRIBUTES = 10,
		WCA_NCRENDERING_EXILED = 11,
		WCA_NCADORNMENTINFO = 12,
		WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
		WCA_VIDEO_OVERLAY_ACTIVE = 14,
		WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
		WCA_DISALLOW_PEEK = 16,
		WCA_CLOAK = 17,
		WCA_CLOAKED = 18,
		WCA_ACCENT_POLICY = 19,
		WCA_FREEZE_REPRESENTATION = 20,
		WCA_EVER_UNCLOAKED = 21,
		WCA_VISUAL_OWNER = 22,
		WCA_HOLOGRAPHIC = 23,
		WCA_EXCLUDED_FROM_DDA = 24,
		WCA_PASSIVEUPDATEMODE = 25,
		WCA_USEDARKMODECOLORS = 26,
		WCA_LAST = 27
	};

	struct WINDOWCOMPOSITIONATTRIBDATA
	{
		WINDOWCOMPOSITIONATTRIB Attrib;
		PVOID pvData;
		SIZE_T cbData;
	};
#if DARKMODE_ALLOW_HAX
	using fnAllowDarkModeForWindow = bool (WINAPI*)(HWND hWnd, bool allow); // ordinal 133
	using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode appMode); // ordinal 135, since 1809
	using fnFlushMenuThemes = void (WINAPI*)(); // ordinal 136
	fnAllowDarkModeForWindow _AllowDarkModeForWindow = nullptr;
	fnSetPreferredAppMode _SetPreferredAppMode = nullptr;
	fnFlushMenuThemes _FlushMenuThemes = nullptr;

	bool ImportsInited = false;

	void InitImports() {
		if (ImportsInited) return;
		if (DarkMode::IsSupportedSystem()) {
			HMODULE hUxtheme = LoadLibraryEx(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
			if (hUxtheme) {
				_AllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));
				_SetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135)));
				_FlushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136)));
			}
		}
		ImportsInited = true;
	}
#endif
}



namespace DarkMode {
	UINT msgSetDarkMode() {
		// No need to threadguard this, should be main thread only, not much harm even if it's not
		static UINT val = 0;
		if (val == 0) val = RegisterWindowMessage(L"libPPUI:msgSetDarkMode");
		return val;
	}
	bool IsSupportedSystem() {
		return Win10BuildNumber() >= 17763; // require at least Win10 1809 / Server 2019
	}
	bool IsWindows11() {
		return Win10BuildNumber() >= 22000;
	}
	bool QueryUserOption() {
		DWORD v = 0;
		DWORD cb = sizeof(v);
		DWORD type = 0;
		if (RegGetValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", RRF_RT_REG_DWORD, &type, &v, &cb) == 0) {
			if (type == REG_DWORD) {
				return v == 0;
			}
		}
		return false;
	}
	void UpdateTitleBar(HWND hWnd, bool bDark) {
		if (!IsSupportedSystem()) return;

		CWindow wnd(hWnd);
		DWORD style = wnd.GetStyle();
		if ((style & WS_CAPTION) != WS_CAPTION) return;

#if 0
		// Some apps do this - no idea why, doesn't work
		// Kept for future reference
		AllowDarkModeForWindow(hWnd, bDark);
		SetProp(hWnd, L"UseImmersiveDarkModeColors", (HANDLE)(INT_PTR)(bDark ? TRUE : FALSE));
#endif

		if (IsWindows11()) {
			// DwmSetWindowAttribute()
			// Windows 11 : works
			// Windows 10 @ late 2021 : doesn't work
			// Server 2019 : as good as SetWindowCompositionAttribute(), needs ModifyStyle() hack for full effect
			BOOL arg = !!bDark;
			DwmSetWindowAttribute(hWnd, 19 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &arg, sizeof(arg));
		} else {
			// Windows 10 mode
			using fnSetWindowCompositionAttribute = BOOL(WINAPI*)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
			static fnSetWindowCompositionAttribute _SetWindowCompositionAttribute = reinterpret_cast<fnSetWindowCompositionAttribute>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));
			if (_SetWindowCompositionAttribute != nullptr) {
				BOOL dark = !!bDark;
				WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
				_SetWindowCompositionAttribute(hWnd, &data);

				// Neither of these fixes stuck titlebar (kept in here for future reference)
				// ::RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_UPDATENOW);
				// ::SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_DRAWFRAME);

				// Apparently the least painful way to reliably fix stuck titlebar
				// 2x SWP_FRAMECHANGED needed with actual style changes

				if (style & WS_VISIBLE) { // Only do this if visible
					wnd.ModifyStyle(WS_BORDER, 0, SWP_FRAMECHANGED);
					wnd.ModifyStyle(0, WS_BORDER, SWP_FRAMECHANGED);
				}

			}
		}
	}

	void ApplyDarkThemeCtrl(HWND ctrl, bool bDark, const wchar_t* ThemeID) {
		if ( ctrl == NULL ) return;
		// Both ways work
		// DarkMode_Theme approach doesn't require evil undocumented MS API calls though
		AllowDarkModeForWindow(ctrl, bDark);
		if (bDark && IsSupportedSystem()) {
			std::wstring temp = L"DarkMode_"; temp += ThemeID;
			::SetWindowTheme(ctrl, temp.c_str(), NULL);
		} else {
			::SetWindowTheme(ctrl, ThemeID, NULL);
		}
	}

	void DarkenEditLite(HWND ctrl) {
		if (IsSupportedSystem()) {
			::SetWindowTheme(ctrl, L"DarkMode_Explorer", NULL);
		}
	}

	void DarkenComboLite(HWND ctrl) {
		if (IsSupportedSystem()) {
			::SetWindowTheme(ctrl, L"DarkMode_CFD", NULL);
		}
	}

	bool IsDCDark(HDC dc_) {
		CDCHandle dc(dc_);
		return IsThemeDark(dc.GetTextColor(), dc.GetBkColor());
	}
	bool IsDialogDark(HWND dlg, UINT msgSend) {
		CWindowDC dc(dlg);
		dc.SetTextColor(0x000000);
		dc.SetBkColor(0xFFFFFF);
		::SendMessage(dlg, msgSend, (WPARAM)dc.m_hDC, (LPARAM)dlg);
		return IsDCDark(dc);
	}

	COLORREF GetSysColor(int idx, bool bDark) {
		if (!bDark) return ::GetSysColor(idx);
		switch (idx) {
		case COLOR_MENU:
		case COLOR_BTNFACE:
		case COLOR_WINDOW:
		case COLOR_MENUBAR:
			// Explorer:
			// return 0x383838;
			return 0x202020;
		case COLOR_BTNSHADOW:
			return 0;
		case COLOR_WINDOWTEXT:
		case COLOR_MENUTEXT:
		case COLOR_BTNTEXT:
		case COLOR_CAPTIONTEXT:
			// Explorer:
			// return 0xdedede;
			return 0xC0C0C0;
		case COLOR_BTNHIGHLIGHT:
		case COLOR_MENUHILIGHT:
			return 0x383838;
		case COLOR_HIGHLIGHT:
			return 0x777777;
		case COLOR_HIGHLIGHTTEXT:
			return 0x101010;
		case COLOR_GRAYTEXT:
			return 0x777777;
		case COLOR_HOTLIGHT:
			return 0xd69c56;
		default:
			return ::GetSysColor(idx);
		}
	}
#if DARKMODE_ALLOW_HAX
	void SetAppDarkMode(bool bDark) {
		InitImports();

		if (_SetPreferredAppMode != nullptr) {
			static PreferredAppMode lastMode = PreferredAppMode::Default;
			PreferredAppMode wantMode = bDark ? PreferredAppMode::ForceDark : PreferredAppMode::ForceLight;
			if (lastMode != wantMode) {
				_SetPreferredAppMode(wantMode);
				lastMode = wantMode;
				if (_FlushMenuThemes) _FlushMenuThemes();
			}
		}
	}
#else
	void SetAppDarkMode(bool) {}
#endif
#if DARKMODE_ALLOW_HAX
	void AllowDarkModeForWindow(HWND wnd, bool bDark) {
		InitImports();

		if (_AllowDarkModeForWindow) _AllowDarkModeForWindow(wnd, bDark);
	}
#else
	void AllowDarkModeForWindow(HWND, bool) {}
#endif

	bool IsThemeDark(COLORREF text, COLORREF background) {
		if (!IsSupportedSystem() || IsHighContrast()) return false;
		auto l_text = PaintUtils::Luminance(text);
		auto l_bk = PaintUtils::Luminance(background);
		if (l_text > l_bk) {
			if (l_bk <= PaintUtils::Luminance(GetSysColor(COLOR_BTNFACE))) {
				return true;
			}
		}
		return false;
	}

	bool IsHighContrast() {
		HIGHCONTRASTW highContrast = { sizeof(highContrast) };
		if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, FALSE))
			return (highContrast.dwFlags & HCF_HIGHCONTRASTON) != 0;
		return false;
	}

	static void DrawTab(CTabCtrl& tabs, CDCHandle dc, int iTab, bool selected, bool focused, const RECT * rcPaint) {
		(void)focused;
		PFC_ASSERT((tabs.GetStyle() & TCS_VERTICAL) == 0);

		CRect rc;
		if (!tabs.GetItemRect(iTab, rc)) return;

		if ( rcPaint != nullptr ) {
			CRect foo;
			if (!foo.IntersectRect(rc, rcPaint)) return;
		}
		const int edgeCX = MulDiv(1, QueryScreenDPI_X(tabs), 120); // note: MulDiv() rounds up from +0.5, this will
		const auto colorBackground = GetSysColor(selected ? COLOR_HIGHLIGHT : COLOR_BTNFACE);
		const auto colorFrame = GetSysColor(COLOR_WINDOWFRAME);
		dc.SetDCBrushColor(colorBackground);
		dc.FillSolidRect(rc, colorBackground);

		{
			CPen pen;
			WIN32_OP_D(pen.CreatePen(PS_SOLID, edgeCX, colorFrame));
			SelectObjectScope scope(dc, pen);
			dc.MoveTo(rc.left, rc.bottom);
			dc.LineTo(rc.left, rc.top);
			dc.LineTo(rc.right, rc.top);
			dc.LineTo(rc.right, rc.bottom);
		}

		wchar_t text[512] = {};
		TCITEM item = {};
		item.mask = TCIF_TEXT;
		item.pszText = text;
		item.cchTextMax = (int)(std::size(text) - 1);
		if (tabs.GetItem(iTab, &item)) {
			SelectObjectScope fontScope(dc, tabs.GetFont());
			dc.SetBkMode(TRANSPARENT);
			dc.SetTextColor(GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
			dc.DrawText(text, (int)wcslen(text), rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		}
	}

	void PaintTabs(CTabCtrl tabs, CDCHandle dc, const RECT * rcPaint) {
		CRect rcClient; tabs.GetClientRect(rcClient); 
		CRect rcArea = rcClient; tabs.AdjustRect(FALSE, rcArea);
		int dx = rcClient.bottom - rcArea.bottom;
		int dy = rcClient.right - rcArea.right;
		CRect rcFrame = rcArea; rcFrame.InflateRect(dx/2, dy/2);
		dc.SetDCBrushColor(GetSysColor(COLOR_WINDOWFRAME));
		dc.FrameRect(rcFrame, (HBRUSH)GetStockObject(DC_BRUSH));
		const int tabCount = tabs.GetItemCount();
		const int tabSelected = tabs.GetCurSel();
		const int tabFocused = tabs.GetCurFocus();
		for (int iTab = 0; iTab < tabCount; ++iTab) {
			if (iTab != tabSelected) DrawTab(tabs, dc, iTab, false, iTab == tabFocused, rcPaint);
		}
		if (tabSelected >= 0) DrawTab(tabs, dc, tabSelected, true, tabSelected == tabFocused, rcPaint);
	}

	void PaintTabsErase(CTabCtrl tabs, CDCHandle dc) {
		CRect rcClient; WIN32_OP_D(tabs.GetClientRect(rcClient));
		dc.FillSolidRect(&rcClient, GetSysColor(COLOR_BTNFACE));
	}


	// =================================================
	// NM_CUSTOMDRAW handlers
	// =================================================

	// We keep a global list of HWNDs that require dark rendering, so dialogs can call DarkMode::OnCustomDraw() which deals with this nonsense behind the scenes
	// This way there's no need to subclass parent windows at random

	enum class whichDark_t {
		none, toolbar
	};

	// readWriteLock used in case someone uses off-main-thread UI, though it should not really happen in real life
	static pfc::readWriteLock lstDarkGuard;
	static std::map<HWND, whichDark_t> lstDark;
	static whichDark_t lstDark_query(HWND w) {
		PFC_INSYNC_READ(lstDarkGuard);
		auto iter = lstDark.find(w);
		if (iter == lstDark.end()) return whichDark_t::none;
		return iter->second;
	}
	static void lstDark_set(HWND w, whichDark_t which) {
		PFC_INSYNC_WRITE(lstDarkGuard);
		lstDark[w] = which;
	}
	static void lstDark_clear(HWND w) {
		PFC_INSYNC_WRITE(lstDarkGuard);
		lstDark.erase(w);
	}

	LRESULT CustomDrawToolbar(NMHDR* hdr) {
		LPNMTBCUSTOMDRAW cd = reinterpret_cast<LPNMTBCUSTOMDRAW>(hdr);
		switch (cd->nmcd.dwDrawStage) {
		case CDDS_PREPAINT: return CDRF_NOTIFYITEMDRAW;
		case CDDS_ITEMPREPAINT:
			cd->clrText = DarkMode::GetSysColor(COLOR_WINDOWTEXT);
			cd->clrBtnFace = DarkMode::GetSysColor(COLOR_BTNFACE);
			cd->clrBtnHighlight = DarkMode::GetSysColor(COLOR_BTNHIGHLIGHT);
			return CDRF_DODEFAULT;
		default:
			return CDRF_DODEFAULT;
		}
	}

	LRESULT OnCustomDraw(int,NMHDR* hdr, BOOL& bHandled) {
		switch (lstDark_query(hdr->hwndFrom)) {
		case whichDark_t::toolbar:
			bHandled = TRUE;
			return CustomDrawToolbar(hdr);
		default:
			bHandled = FALSE; return 0;
		}
	}

	namespace {

		class CToolbarHook {
			bool m_dark = false;
			const bool m_explorerTheme;
			CToolBarCtrl m_wnd;
		public:
			CToolbarHook(HWND wnd, bool initial, bool bExplorerTheme) : m_wnd(wnd), m_explorerTheme(bExplorerTheme) {
				SetDark(initial);
			}
			
			void SetDark(bool v) {
				if (m_dark == v) return;
				m_dark = v;
				if (v) {
					lstDark_set(m_wnd, whichDark_t::toolbar);
					if (m_explorerTheme) ::SetWindowTheme(m_wnd, L"", L""); // if we don't do this, NM_CUSTOMDRAW color overrides get disregarded
				} else {
					lstDark_clear(m_wnd);
					if (m_explorerTheme) ::SetWindowTheme(m_wnd, L"Explorer", NULL);
				}
				m_wnd.Invalidate();

				ApplyDarkThemeCtrl(m_wnd.GetToolTips(), v);
			}
			~CToolbarHook() {
				if (m_dark) lstDark_clear(m_wnd);
			}

		};

		class CTabsHook : public CWindowImpl<CTabsHook, CTabCtrl> {
		public:
			CTabsHook(bool bDark = false) : m_dark(bDark) {}
			BEGIN_MSG_MAP_EX(CTabsHook)
				MSG_WM_PAINT(OnPaint)
				MSG_WM_ERASEBKGND(OnEraseBkgnd)
				MESSAGE_HANDLER_EX(msgSetDarkMode(), OnSetDarkMode)
			END_MSG_MAP()

			void SetDark(bool v = false);
		private:
			void OnPaint(CDCHandle);
			BOOL OnEraseBkgnd(CDCHandle);

			LRESULT OnSetDarkMode(UINT, WPARAM wp, LPARAM) {
				switch (wp) {
				case 0: SetDark(false); break;
				case 1: SetDark(true); break;
				}
				return 1;
			}

			bool m_dark = false;
		};

		void CTabsHook::OnPaint(CDCHandle target) {
			if (!m_dark) { SetMsgHandled(FALSE); return; }
			if (target) {
				PaintTabs(m_hWnd, target);
			} else {
				CPaintDC dc(*this);
				PaintTabs(m_hWnd, dc.m_hDC, &dc.m_ps.rcPaint);
			}
		}
		BOOL CTabsHook::OnEraseBkgnd(CDCHandle dc) {
			if (m_dark) {
				PaintTabsErase(*this, dc);
				return TRUE;
			}
			SetMsgHandled(FALSE);
			return FALSE;
		}

		void CTabsHook::SetDark(bool v) {
			m_dark = v;
			if (m_hWnd != NULL) Invalidate();

			ApplyDarkThemeCtrl(GetToolTips(), v);
		}

		class CTreeViewHook : public CWindowImpl<CTreeViewHook, CTreeViewCtrl> {
			bool m_dark;
		public:
			CTreeViewHook(bool v) : m_dark(v) {}

			BEGIN_MSG_MAP_EX(CTreeViewHook)
				MESSAGE_RANGE_HANDLER_EX(WM_CTLCOLORMSGBOX, WM_CTLCOLORSTATIC, OnCtlColor)
				MESSAGE_HANDLER_EX(msgSetDarkMode(), OnSetDarkMode)
				MESSAGE_HANDLER_EX(TVM_EDITLABEL, OnEditLabel)
			END_MSG_MAP()

			LRESULT OnCtlColor(UINT uMsg, WPARAM wParam, LPARAM lParam) {
				return GetParent().SendMessage(uMsg, wParam, lParam);
			}
			LRESULT OnEditLabel(UINT, WPARAM, LPARAM) {
				LRESULT ret = DefWindowProc();
				if (ret != 0) {
					HWND edit = (HWND) ret;
					PFC_ASSERT( ::IsWindow(edit) );
					ApplyDarkThemeCtrl( edit, m_dark );
				}
				return ret;
			}
			void SetDark(bool v) { 
				if (m_dark == v) return;
				m_dark = v;
				ApplyDark();
			}
			void ApplyDark() {
				ApplyDarkThemeCtrl(m_hWnd, m_dark);
				COLORREF bk = m_dark ? GetSysColor(COLOR_WINDOW) : (COLORREF)(-1);
				COLORREF tx = m_dark ? GetSysColor(COLOR_WINDOWTEXT) : (COLORREF)(-1);
				this->SetTextColor(tx); this->SetLineColor(tx);
				this->SetBkColor(bk);

				ApplyDarkThemeCtrl(GetToolTips(), m_dark);
			}

			void SubclassWindow(HWND wnd) {
				WIN32_OP_D( __super::SubclassWindow(wnd) );
				this->ApplyDark();
			}

			LRESULT OnSetDarkMode(UINT, WPARAM wp, LPARAM) {
				switch (wp) {
				case 0: SetDark(false); break;
				case 1: SetDark(true); break;
				}
				return 1;
			}
		};

		class CDialogHook : public CWindowImpl<CDialogHook> {
			bool m_enabled;
		public:
			CDialogHook(bool v) : m_enabled(v) {}

			void SetDark(bool v) { 
				if (m_enabled == v) return;

				// Important: PostMessage()'ing this caused bugs
				SendMessage(WM_THEMECHANGED); 
				
				m_enabled = v; 
				
				// Ensure menu bar redraw with RDW_FRAME
				RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME);
			}

			// Undocumented Windows menu drawing API
			// Source: https://github.com/adzm/win32-custom-menubar-aero-theme

			static constexpr unsigned WM_UAHDRAWMENU = 0x0091;
			static constexpr unsigned WM_UAHDRAWMENUITEM = 0x0092;

			typedef union tagUAHMENUITEMMETRICS
			{
				struct {
					DWORD cx;
					DWORD cy;
				} rgsizeBar[2];
				struct {
					DWORD cx;
					DWORD cy;
				} rgsizePopup[4];
			} UAHMENUITEMMETRICS;

			typedef struct tagUAHMENUPOPUPMETRICS
			{
				DWORD rgcx[4];
				DWORD fUpdateMaxWidths : 2;
			} UAHMENUPOPUPMETRICS;

			typedef struct tagUAHMENU
			{
				HMENU hmenu;
				HDC hdc;
				DWORD dwFlags;
			} UAHMENU;

			typedef struct tagUAHMENUITEM
			{
				int iPosition;
				UAHMENUITEMMETRICS umim;
				UAHMENUPOPUPMETRICS umpm;
			} UAHMENUITEM;

			typedef struct UAHDRAWMENUITEM
			{
				DRAWITEMSTRUCT dis;
				UAHMENU um;
				UAHMENUITEM umi;
			} UAHDRAWMENUITEM;


			BEGIN_MSG_MAP_EX(CDialogHook)
				MSG_WM_CTLCOLORDLG(ctlColorCommon)
				MSG_WM_CTLCOLORSTATIC(ctlColorCommon)
				MSG_WM_CTLCOLOREDIT(ctlColorCommon)
				MSG_WM_CTLCOLORBTN(ctlColorCommon)
				MSG_WM_CTLCOLORLISTBOX(ctlColorCommon)
				MSG_WM_CTLCOLORSCROLLBAR(ctlColorCommon)
				NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, DarkMode::OnCustomDraw)
				MESSAGE_HANDLER_EX(WM_UAHDRAWMENU, Handle_WM_UAHDRAWMENU)
				MESSAGE_HANDLER_EX(WM_UAHDRAWMENUITEM, Handle_WM_UAHDRAWMENUITEM)
				MESSAGE_HANDLER_EX(msgSetDarkMode(), OnSetDarkMode)
			END_MSG_MAP()

			LRESULT OnSetDarkMode(UINT, WPARAM wp, LPARAM) {
				switch (wp) {
				case 0: SetDark(false); break;
				case 1: SetDark(true); break;
				}
				return 1;
			}

			static COLORREF GetBkColor() { return DarkMode::GetSysColor(COLOR_WINDOW); }
			static COLORREF GetTextColor() { return DarkMode::GetSysColor(COLOR_WINDOWTEXT); }

			HBRUSH ctlColorDlg(CDCHandle dc, CWindow wnd) {
				if (m_enabled && ::IsThemeDialogTextureEnabled(*this)) {
					auto bkColor = DarkMode::GetSysColor(COLOR_HIGHLIGHT);
					auto txColor = GetTextColor();

					dc.SetTextColor(txColor);
					dc.SetBkColor(bkColor);
					dc.SetDCBrushColor(bkColor);
					return (HBRUSH)GetStockObject(DC_BRUSH);
				}
				return ctlColorCommon(dc, wnd);
			}

			HBRUSH ctlColorCommon(CDCHandle dc, CWindow wnd) {
				(void)wnd;
				if (m_enabled) {
					auto bkColor = GetBkColor();
					auto txColor = GetTextColor();

					dc.SetTextColor(txColor);
					dc.SetBkColor(bkColor);
					dc.SetDCBrushColor(bkColor);
					return (HBRUSH)GetStockObject(DC_BRUSH);
				}
				SetMsgHandled(FALSE);
				return NULL;
			}
			LRESULT Handle_WM_UAHDRAWMENU(UINT, WPARAM wParam, LPARAM lParam) {
				if (!m_enabled) {
					SetMsgHandled(FALSE);
					return 0;
				}
				UAHMENU* pUDM = (UAHMENU*)lParam;
				CRect rc;

				MENUBARINFO mbi = { sizeof(mbi) };
				WIN32_OP_D(GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi));

				CRect rcWindow;
				WIN32_OP_D(GetWindowRect(rcWindow));

				rc = mbi.rcBar;
				OffsetRect(&rc, -rcWindow.left, -rcWindow.top);

				rc.top -= 1;

				CDCHandle dc(pUDM->hdc);
				dc.FillSolidRect(rc, DarkMode::GetSysColor(COLOR_MENUBAR));
				return 0;
			}
			LRESULT Handle_WM_UAHDRAWMENUITEM(UINT, WPARAM wParam, LPARAM lParam) {
				if (!m_enabled) {
					SetMsgHandled(FALSE);
					return 0;
				}

				UAHDRAWMENUITEM* pUDMI = (UAHDRAWMENUITEM*)lParam;
				CMenuHandle hMenu = pUDMI->um.hmenu;

				CString menuString;
				WIN32_OP_D(hMenu.GetMenuString(pUDMI->umi.iPosition, menuString, MF_BYPOSITION) > 0);

				DWORD drawTextFlags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;

				int iTextStateID = MPI_NORMAL;
				int iBackgroundStateID = MPI_NORMAL;
				if ((pUDMI->dis.itemState & ODS_INACTIVE) | (pUDMI->dis.itemState & ODS_DEFAULT)) {
					iTextStateID = MPI_NORMAL;
					iBackgroundStateID = MPI_NORMAL;
				}
				if (pUDMI->dis.itemState & (ODS_HOTLIGHT|ODS_SELECTED)) {
					iTextStateID = MPI_HOT;
					iBackgroundStateID = MPI_HOT;
				}
				if (pUDMI->dis.itemState & (ODS_GRAYED|ODS_DISABLED)) {
					iTextStateID = MPI_DISABLED;
					iBackgroundStateID = MPI_DISABLED;
				}
				if (pUDMI->dis.itemState & ODS_NOACCEL) {
					drawTextFlags |= DT_HIDEPREFIX;
				}

				if (m_menuTheme == NULL) {
					m_menuTheme.OpenThemeData(m_hWnd, L"Menu");
				}

				CDCHandle dc(pUDMI->um.hdc);
				switch (iBackgroundStateID) {
				case MPI_NORMAL:
				case MPI_DISABLED:
					dc.FillSolidRect(&pUDMI->dis.rcItem, DarkMode::GetSysColor(COLOR_MENUBAR));
					break;
				case MPI_HOT:
				case MPI_DISABLEDHOT:
					dc.FillSolidRect(&pUDMI->dis.rcItem, DarkMode::GetSysColor(COLOR_MENUHILIGHT));
					break;
				default:
					DrawThemeBackground(m_menuTheme, pUDMI->um.hdc, MENU_POPUPITEM, iBackgroundStateID, &pUDMI->dis.rcItem, nullptr);
					break;
				}
				DTTOPTS dttopts = { sizeof(dttopts) };
				if (iTextStateID == MPI_NORMAL || iTextStateID == MPI_HOT)
				{
					dttopts.dwFlags |= DTT_TEXTCOLOR;
					dttopts.crText = DarkMode::GetSysColor(COLOR_WINDOWTEXT);
				}
				DrawThemeTextEx(m_menuTheme, dc, MENU_POPUPITEM, iTextStateID, menuString, menuString.GetLength(), drawTextFlags, &pUDMI->dis.rcItem, &dttopts);

				return 0;
			}
			CTheme m_menuTheme;
		};


		class CStatusBarHook : public CWindowImpl<CStatusBarHook, CStatusBarCtrl> {
		public:
			CStatusBarHook(bool v = false) : m_dark(v) {}

			BEGIN_MSG_MAP_EX(CStatusBarHook)
				MSG_WM_ERASEBKGND(OnEraseBkgnd)
				MSG_WM_PAINT(OnPaint)
				MESSAGE_HANDLER_EX(SB_SETTEXT, OnSetText)
				MESSAGE_HANDLER_EX(SB_SETICON, OnSetIcon)
				MESSAGE_HANDLER_EX(msgSetDarkMode(), OnSetDarkMode)
			END_MSG_MAP()

			LRESULT OnSetDarkMode(UINT, WPARAM wp, LPARAM) {
				switch (wp) {
				case 0: SetDark(false); break;
				case 1: SetDark(true); break;
				}
				return 1;
			}

			void SetDark(bool v = true) {
				if (m_dark != v) {
					m_dark = v;
					Invalidate();
					ApplyDarkThemeCtrl(m_hWnd, v);
				}
			}

			void SubclassWindow(HWND wnd) {
				WIN32_OP_D(__super::SubclassWindow(wnd));
				Invalidate();
				ApplyDarkThemeCtrl(m_hWnd, m_dark);
			}
			LRESULT OnSetIcon(UINT, WPARAM wp, LPARAM lp) {
				unsigned idx = (unsigned)wp;
				if (idx < 32) {
					CSize sz;
					if (lp != 0) sz = GetIconSize((HICON)lp);
					m_iconSizeCache[idx] = sz;
				}
				SetMsgHandled(FALSE);
				return 0;
			}
			LRESULT OnSetText(UINT, WPARAM wp, LPARAM) {
				// Status bar won't tell us about ownerdraw from GetText()
				// Have to listen to relevant messages to know
				unsigned idx = (unsigned)(wp & 0xFF);
				if (idx < 32) {
					uint32_t flag = 1 << idx;
					if (wp & SBT_OWNERDRAW) {
						m_ownerDrawMask |= flag;
					} else {
						m_ownerDrawMask &= ~flag;
					}
				}

				SetMsgHandled(FALSE);
				return 0;
			}

			void Paint(CDCHandle dc) {
				CRect rcClient; WIN32_OP_D(GetClientRect(rcClient));
				dc.FillSolidRect(rcClient, GetSysColor(COLOR_BTNFACE)); // Wine seems to not call our WM_ERASEBKGND handler, fill the background here too

				dc.SelectFont(GetFont());
				dc.SetBkMode(TRANSPARENT);
				dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
				CPen pen; pen.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNHIGHLIGHT));
				dc.SelectPen(pen);
				int count = this->GetParts(0, nullptr);
				for (int iPart = 0; iPart < count; ++iPart) {
					CRect rcPart;
					this->GetRect(iPart, rcPart);
					if (rcPart.left > 0) {
						dc.MoveTo(rcPart.left, rcPart.top);
						dc.LineTo(rcPart.left, rcPart.bottom);
					}
					int type = 0;
					CString text;
					this->GetText(iPart, text, &type);

					HICON icon = this->GetIcon(iPart);
					int iconMargin = 0;
					if (icon != NULL && (unsigned)iPart < std::size(m_iconSizeCache)) {

						auto size = m_iconSizeCache[iPart];

						dc.DrawIconEx(rcPart.left + size.cx / 4, (rcPart.top + rcPart.bottom) / 2 - size.cy / 2, icon, size.cx, size.cy);
						iconMargin = MulDiv(size.cx, 3, 2);
					}

					if (m_ownerDrawMask & (1 << iPart)) { // statusbar won't tell us about ownerdraw from GetText()
						DRAWITEMSTRUCT ds = {};
						ds.CtlType = ODT_STATIC;
						ds.CtlID = this->GetDlgCtrlID();
						ds.itemID = iPart;
						ds.hwndItem = m_hWnd;
						ds.hDC = dc;
						ds.rcItem = rcPart;

						DCStateScope scope(dc);
						GetParent().SendMessage(WM_DRAWITEM, GetDlgCtrlID(), (LPARAM)&ds);
					} else {
						CRect rcText = rcPart;
						int defMargin = rcText.Height() / 4;
						int l = iconMargin > 0 ? iconMargin : defMargin;
						int r = defMargin;
						rcText.DeflateRect(l, 0, r, 0);
						dc.DrawText(text, text.GetLength(), rcText, DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
					}

					if (GetStyle() & SBARS_SIZEGRIP) {
						CSize size;
						auto theme = OpenThemeData(*this, L"status");
						PFC_ASSERT(theme != NULL);
						GetThemePartSize(theme, dc, SP_GRIPPER, 0, &rcClient, TS_DRAW, &size);
						auto rc = rcClient;
						rc.left = rc.right - size.cx;
						rc.top = rc.bottom - size.cy;
						DrawThemeBackground(theme, dc, SP_GRIPPER, 0, &rc, nullptr);
						CloseThemeData(theme);
					}
				}
			}

			void OnPaint(CDCHandle target) {
				if (!m_dark) { SetMsgHandled(FALSE); return; }
				if (target) {
					Paint(target);
				} else {
					CPaintDC dc(*this);
					Paint(dc.m_hDC);
				}
			}

			BOOL OnEraseBkgnd(CDCHandle dc) {
				if (m_dark) {
					CRect rc; WIN32_OP_D(GetClientRect(rc)); dc.FillSolidRect(rc, DarkMode::GetSysColor(COLOR_BTNFACE)); return TRUE;
				}
				SetMsgHandled(FALSE); return FALSE;			
			}

			bool m_dark = false;

			uint32_t m_ownerDrawMask = 0;
			CSize m_iconSizeCache[32];
		};

		class CCheckBoxHook : public CWindowImpl<CCheckBoxHook, CButton> {
		public:
			CCheckBoxHook(bool v = false) : m_dark(v) {}

			BEGIN_MSG_MAP_EX(CCheckBoxHook)
				MSG_WM_PAINT(OnPaint)
				MSG_WM_PRINTCLIENT(OnPaint)
				MSG_WM_ERASEBKGND(OnEraseBkgnd)
				MSG_WM_UPDATEUISTATE(OnUpdateUIState)

				// Note that checkbox implementation likes to paint on its own in response to events 
				// instead of invalidating and handling WM_PAINT
				// We have to specifically trigger WM_PAINT to override their rendering with ours
				MESSAGE_HANDLER_EX(WM_SETFOCUS, OnMsgRedraw)
				MESSAGE_HANDLER_EX(WM_KILLFOCUS, OnMsgRedraw)
				MESSAGE_HANDLER_EX(WM_ENABLE, OnMsgRedraw)
				MESSAGE_HANDLER_EX(WM_SETTEXT, OnMsgRedraw)

				MESSAGE_HANDLER_EX(msgSetDarkMode(), OnSetDarkMode)
			END_MSG_MAP()

			LRESULT OnSetDarkMode(UINT, WPARAM wp, LPARAM) {
				switch (wp) {
				case 0: SetDark(false); break;
				case 1: SetDark(true); break;
				}
				return 1;
			}

			LRESULT OnMsgRedraw(UINT, WPARAM, LPARAM) {
				if ( m_dark ) {
					// PROBLEM: 
					// Can't invalidate prior to their handling of the message
					// Causes bugs with specific chains of events - EnableWindow() followed immediately SetWindowText()
					LRESULT ret = DefWindowProc();
					Invalidate();
					return ret;
				}
				SetMsgHandled(FALSE); return 0;
			}

			void OnUpdateUIState(WORD nAction, WORD nState) {
				(void)nAction;
				if ( m_dark && (nState & (UISF_HIDEACCEL | UISF_HIDEFOCUS)) != 0) {
					// PROBLEM: 
					// Can't invalidate prior to their handling of the message
					// Causes bugs with specific chains of events - EnableWindow() followed immediately SetWindowText()
					DefWindowProc();
					Invalidate();
					return;
				}
				SetMsgHandled(FALSE);
			}
			void PaintHandler(CDCHandle dc) {
				CRect rcClient; WIN32_OP_D(GetClientRect(rcClient));

				const bool bDisabled = !this->IsWindowEnabled();

				dc.SetTextColor(DarkMode::GetSysColor(COLOR_BTNTEXT));
				dc.SetBkColor(DarkMode::GetSysColor(COLOR_BTNFACE));
				dc.SetBkMode(OPAQUE);
				dc.SelectFont(GetFont());
				GetParent().SendMessage(WM_CTLCOLORBTN, (WPARAM)dc.m_hDC, (LPARAM)m_hWnd);
				if (bDisabled) dc.SetTextColor(DarkMode::GetSysColor(COLOR_GRAYTEXT)); // override WM_CTLCOLORBTN

				const DWORD btnStyle = GetStyle();
				const DWORD btnType = btnStyle & BS_TYPEMASK;
				const bool bRadio = (btnType == BS_RADIOBUTTON || btnType == BS_AUTORADIOBUTTON);
				const int part = bRadio ? BP_RADIOBUTTON : BP_CHECKBOX;

				const auto ctrlState = GetState();
				const DWORD uiState = (DWORD)SendMessage(WM_QUERYUISTATE);

				
				const bool bChecked = (ctrlState & BST_CHECKED) != 0;
				const bool bMixed = (ctrlState & BST_INDETERMINATE) != 0;
				const bool bHot = (ctrlState & BST_HOT) != 0;
				const bool bFocus = (ctrlState & BST_FOCUS) != 0 && (uiState & UISF_HIDEFOCUS) == 0;

				HTHEME theme = OpenThemeData(m_hWnd, L"button");

				CRect rcCheckBox = rcClient;
				bool bDrawn = false;
				int margin = 0;
				if (theme != NULL && IsThemePartDefined(theme, part, 0)) {
					int state = 0;
					if (bDisabled) {
						if ( bChecked ) state = CBS_CHECKEDDISABLED;
						else if ( bMixed ) state = CBS_MIXEDDISABLED;
						else state = CBS_UNCHECKEDDISABLED;
					} else if (bHot) {
						if ( bChecked ) state = CBS_CHECKEDHOT;
						else if ( bMixed ) state = CBS_MIXEDHOT;
						else state = CBS_UNCHECKEDNORMAL;
					} else {
						if ( bChecked ) state = CBS_CHECKEDNORMAL;
						else if ( bMixed ) state = CBS_MIXEDNORMAL;
						else state = CBS_UNCHECKEDNORMAL;
					}

					CSize size;
					if (SUCCEEDED(GetThemePartSize(theme, dc, part, state, rcCheckBox, TS_TRUE, &size))) {
						if (size.cx <= rcCheckBox.Width() && size.cy <= rcCheckBox.Height()) {
							CRect rc = rcCheckBox;
							margin = MulDiv(size.cx, 5, 4);
							// rc.left += (rc.Width() - size.cx) / 2;
							rc.top += (rc.Height() - size.cy) / 2;
							rc.right = rc.left + size.cx;
							rc.bottom = rc.top + size.cy;
							DrawThemeBackground(theme, dc, part, state, rc, &rc);
							bDrawn = true;
						}
					}
				}
				if (theme != NULL) CloseThemeData(theme);
				if (!bDrawn) {
					int stateEx = bRadio ? DFCS_BUTTONRADIO : DFCS_BUTTONCHECK;
					if (bChecked) stateEx |= DFCS_CHECKED;
					// FIX ME bMixed ?
					if (bDisabled) stateEx |= DFCS_INACTIVE;
					else if (bHot) stateEx |= DFCS_HOT;

					const int dpi = GetDeviceCaps(dc, LOGPIXELSX);
					int w = MulDiv(16, dpi, 96);

					CRect rc = rcCheckBox; 
					if (rc.Width() > w) rc.right = rc.left + w;;

					DrawFrameControl(dc, rc, DFC_BUTTON, stateEx);
					margin = MulDiv(20, dpi, 96);
				}

				CString text;
				if (margin < rcClient.Width()) GetWindowText(text);
				if (!text.IsEmpty()) {
					CRect rcText = rcClient;
					rcText.left += margin;
					UINT dtFlags = DT_VCENTER;
					if (btnStyle & BS_MULTILINE) {
						dtFlags |= DT_WORDBREAK;
					} else {
						dtFlags |= DT_END_ELLIPSIS | DT_SINGLELINE;
					}
					dc.DrawText(text, text.GetLength(), rcText, dtFlags);
					if (bFocus) {
						dc.DrawText(text, text.GetLength(), rcText, DT_CALCRECT | dtFlags);
						dc.DrawFocusRect(rcText);
					}
				} else if (bFocus) {
					dc.DrawFocusRect(rcClient);
				}
			}
			void OnPaint(CDCHandle userDC, UINT flags = 0) {
				(void)flags;
				if (!m_dark) { SetMsgHandled(FALSE); return; }
				if (userDC) {
					PaintHandler(userDC);
				} else {
					CPaintDC dc(*this);
					PaintHandler(dc.m_hDC);
				}
			}
			BOOL OnEraseBkgnd(CDCHandle dc) {
				if (m_dark) {
					CRect rc; WIN32_OP_D(GetClientRect(rc));

					dc.SetTextColor(DarkMode::GetSysColor(COLOR_BTNTEXT));
					dc.SetBkColor(DarkMode::GetSysColor(COLOR_BTNFACE));
					auto br = (HBRUSH)GetParent().SendMessage(WM_CTLCOLORSTATIC, (WPARAM)dc.m_hDC, (LPARAM)m_hWnd);
					if (br != NULL) {
						dc.FillRect(rc, br);
					} else {
						dc.FillSolidRect(rc, DarkMode::GetSysColor(COLOR_BTNFACE));
					}
					return TRUE;
				}
				SetMsgHandled(FALSE); return FALSE;
			}

			void SetDark(bool v = true) {
				if (v != m_dark) {
					m_dark = v;
					Invalidate();
					ApplyDarkThemeCtrl(m_hWnd, m_dark);
				}
			}

			void SubclassWindow(HWND wnd) {
				WIN32_OP_D(__super::SubclassWindow(wnd));
				ApplyDarkThemeCtrl(m_hWnd, m_dark);
			}

			bool m_dark = false;
		};

		class CGripperHook : public CWindowImpl<CGripperHook> {
		public:
			CGripperHook(bool v) : m_dark(v) {}

			BEGIN_MSG_MAP_EX(CGRipperHook)
				MSG_WM_ERASEBKGND(OnEraseBkgnd)
				MSG_WM_PAINT(OnPaint)
				MESSAGE_HANDLER_EX(msgSetDarkMode(), OnSetDarkMode)
			END_MSG_MAP()

			LRESULT OnSetDarkMode(UINT, WPARAM wp, LPARAM) {
				switch (wp) {
				case 0: SetDark(false); break;
				case 1: SetDark(true); break;
				}
				return 1;
			}

			void SetDark(bool v) {
				if (v != m_dark) {
					m_dark = v;
					ApplyDarkThemeCtrl(*this, m_dark);
				}
			}

			void SubclassWindow(HWND wnd) {
				WIN32_OP_D(__super::SubclassWindow(wnd));
				ApplyDarkThemeCtrl(m_hWnd, m_dark);
			}

			void PaintGripper(CDCHandle dc) {
				CRect rcClient; WIN32_OP_D(GetClientRect(rcClient));
				CSize size;
				auto theme = OpenThemeData(*this, L"status");
				PFC_ASSERT(theme != NULL);
				GetThemePartSize(theme, dc, SP_GRIPPER, 0, &rcClient, TS_DRAW, &size);
				auto rc = rcClient;
				rc.left = rc.right - size.cx;
				rc.top = rc.bottom - size.cy;
				DrawThemeBackground(theme, dc, SP_GRIPPER, 0, &rc, nullptr);
				CloseThemeData(theme);
			}

			void OnPaint(CDCHandle dc) {
				if (!m_dark) { SetMsgHandled(FALSE); return; }
				if (dc) PaintGripper(dc);
				else {CPaintDC pdc(*this); PaintGripper(pdc.m_hDC);}
			}

			BOOL OnEraseBkgnd(CDCHandle dc) {
				if (m_dark) {
					CRect rc; GetClientRect(rc);
					dc.FillSolidRect(rc, GetSysColor(COLOR_WINDOW));
					return TRUE;
				}
				SetMsgHandled(FALSE); return FALSE;
			}
			bool m_dark = false;
		};

		class CReBarHook : public CWindowImpl<CReBarHook, CReBarCtrl> {
			bool m_dark;
		public:
			CReBarHook(bool v) : m_dark(v) {}
			BEGIN_MSG_MAP_EX(CReBarHook)
				MSG_WM_ERASEBKGND(OnEraseBkgnd)
				MSG_WM_DESTROY(OnDestroy)
				MSG_WM_PAINT(OnPaint)
				MSG_WM_PRINTCLIENT(OnPaint)
				NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, DarkMode::OnCustomDraw)
				MESSAGE_HANDLER_EX(msgSetDarkMode(), OnSetDarkMode)
			END_MSG_MAP()

			LRESULT OnSetDarkMode(UINT, WPARAM wp, LPARAM) {
				switch (wp) {
				case 0: SetDark(false); break;
				case 1: SetDark(true); break;
				}
				return 1;
			}

			void OnPaint(CDCHandle target, unsigned flags = 0) {
				if (!m_dark) { SetMsgHandled(FALSE); return; }
				(void)flags;
				if (target) {
					HandlePaint(target);
				} else {
					CPaintDC pdc(*this);
					HandlePaint(pdc.m_hDC);
				}
			}

			void HandlePaint(CDCHandle dc) {
				const int total = this->GetBandCount();
				for (int iBand = 0; iBand < total; ++iBand) {
					CRect rc;
					WIN32_OP_D(this->GetRect(iBand, rc));

					wchar_t buffer[256] = {};
					REBARBANDINFO info = { sizeof(info) };
					info.fMask = RBBIM_TEXT | RBBIM_CHILD | RBBIM_STYLE;
					info.lpText = buffer; info.cch = (UINT)std::size(buffer);
					WIN32_OP_D(this->GetBandInfo(iBand, &info));

					HFONT useFont;
					// Sadly overriding the font breaks the layout
					// MS implementation disregards fonts too
					// useFont = this->GetFont();
					useFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
					SelectObjectScope fontScope(dc, useFont);
					dc.SetTextColor(DarkMode::GetSysColor(COLOR_BTNTEXT));
					dc.SetBkMode(TRANSPARENT);

					CRect rcText = rc;
					if ((info.fStyle & RBBS_NOGRIPPER) == 0) {
						auto color = PaintUtils::BlendColor(DarkMode::GetSysColor(COLOR_WINDOWFRAME), DarkMode::GetSysColor(COLOR_BTNFACE));
						dc.SetDCPenColor(color);
						SelectObjectScope penScope(dc, GetStockObject(DC_PEN));
						dc.MoveTo(rcText.TopLeft());
						dc.LineTo(CPoint(rcText.left, rcText.bottom));
						rcText.left += 6; // this should be DPI-scaled, only it's not because rebar layout isn't either
					} else {
						rcText.left += 2;
					}
					dc.DrawText(buffer, (int)wcslen(buffer), &rcText, DT_VCENTER | DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
				}
			}


			void OnDestroy() {
				SetMsgHandled(FALSE);
			}
			BOOL OnEraseBkgnd(CDCHandle dc) {
				if (m_dark) {
					CRect rc;
					WIN32_OP_D(GetClientRect(rc));
					dc.FillSolidRect(rc, DarkMode::GetSysColor(COLOR_BTNFACE));
					return TRUE;
				}
				SetMsgHandled(FALSE); return FALSE;
			}
			void SetDark(bool v) {
				if (v != m_dark) {
					m_dark = v; Apply();
				}
			}
			void Apply() {
				if (m_dark) {
					this->SetTextColor(DarkMode::GetSysColor(COLOR_WINDOWTEXT));
					this->SetBkColor(DarkMode::GetSysColor(COLOR_BTNFACE));
				} else {
					this->SetTextColor((COLORREF)-1);
					this->SetBkColor((COLORREF)-1);
				}
				Invalidate();
			}
			void SubclassWindow(HWND wnd) {
				WIN32_OP_D(__super::SubclassWindow(wnd));
				Apply();
			}
		};

		class CStaticHook : public CWindowImpl<CStaticHook, CStatic> {
			bool m_dark;
		public:
			CStaticHook(bool v) : m_dark(v) {}

			BEGIN_MSG_MAP_EX(CStaticHook)
				MSG_WM_PAINT(OnPaint)
				MESSAGE_HANDLER_EX(WM_ENABLE, OnMsgRedraw)
				MESSAGE_HANDLER_EX(msgSetDarkMode(), OnSetDarkMode)
			END_MSG_MAP()

			LRESULT OnSetDarkMode(UINT, WPARAM wp, LPARAM) {
				switch (wp) {
				case 0: SetDark(false); break;
				case 1: SetDark(true); break;
				}
				return 1;
			}
			
			LRESULT OnMsgRedraw(UINT, WPARAM, LPARAM) {
				Invalidate();
				SetMsgHandled(FALSE);
				return 0;
			}

			void SetDark(bool v) {
				if (m_dark != v) {
					m_dark = v;
					Invalidate();
				}
			}

			void OnPaint(CDCHandle dc) {
				// ONLY override the dark+disabled or dark+icon behavior
				if (!m_dark || (this->IsWindowEnabled() && this->GetIcon() == NULL) ) {
					SetMsgHandled(FALSE); return;
				}

				if (dc) HandlePaint(dc);
				else {
					CPaintDC pdc(*this); HandlePaint(pdc.m_hDC);
				}
			}
			void HandlePaint(CDCHandle dc) {
				CString str;
				CIconHandle icon = this->GetIcon();
				this->GetWindowTextW(str);

				CRect rcClient;
				WIN32_OP_D(GetClientRect(rcClient));
				
				const DWORD style = this->GetStyle();
				
				dc.SelectFont(GetFont());

				HBRUSH br = (HBRUSH) GetParent().SendMessage(WM_CTLCOLORSTATIC, (WPARAM)dc.m_hDC, (LPARAM)m_hWnd);;
				if (br == NULL) {
					dc.FillSolidRect(rcClient, DarkMode::GetSysColor(COLOR_WINDOW));
				} else {
					WIN32_OP_D(dc.FillRect(rcClient, br));
				}

				if (icon != NULL) {
					dc.DrawIcon(0, 0, icon);
				} else {
					DWORD flags = 0;
					if (style & SS_SIMPLE) flags |= DT_SINGLELINE | DT_WORD_ELLIPSIS;
					else flags |= DT_WORDBREAK;
					if (style & SS_RIGHT) flags |= DT_RIGHT;
					else if (style & SS_CENTER) flags |= DT_CENTER;

					dc.SetTextColor(DarkMode::GetSysColor(COLOR_GRAYTEXT));
					dc.SetBkMode(TRANSPARENT);
					dc.DrawText(str, str.GetLength(), rcClient, flags);
				}
			}
		};

		class CUpDownHook : public CWindowImpl<CUpDownHook, CUpDownCtrl> {
			bool m_dark;
		public:
			CUpDownHook(bool v) : m_dark(v) {}

			void SetDark(bool v) {
				if (v != m_dark) {
					m_dark = v; Invalidate();
				}
			}

			BEGIN_MSG_MAP_EX(CUpDownHook)
				MSG_WM_PAINT(OnPaint)
				MSG_WM_PRINTCLIENT(OnPaint)
				MSG_WM_MOUSEMOVE(OnMouseMove)
				MSG_WM_LBUTTONDOWN(OnMouseBtn)
				MSG_WM_LBUTTONUP(OnMouseBtn)
				MSG_WM_MOUSELEAVE(OnMouseLeave)
				MESSAGE_HANDLER_EX(msgSetDarkMode(), OnSetDarkMode)
			END_MSG_MAP()

		private:
			LRESULT OnSetDarkMode(UINT, WPARAM wp, LPARAM) {
				switch (wp) {
				case 0: SetDark(false); break;
				case 1: SetDark(true); break;
				}
				return 1;
			}

			struct layout_t {
				CRect whole, upper, lower;
				int yCenter;
			};
			layout_t Layout(CRect const & rcClient) {
				CRect rc = rcClient;
				rc.DeflateRect(1, 1);
				int yCenter = (rc.top + rc.bottom) / 2;
				layout_t ret;
				ret.yCenter = yCenter;
				ret.whole = rc;
				ret.upper = rc;
				ret.upper.bottom = yCenter;
				ret.lower = rc;
				ret.lower.top = yCenter;
				return ret;
			}
			layout_t Layout() {
				CRect rcClient; WIN32_OP_D(GetClientRect(rcClient)); return Layout(rcClient);
			}
			int m_hot = 0;
			bool m_btnDown = false;
			void SetHot(int iHot) {
				if (iHot != m_hot) {
					m_hot = iHot; Invalidate();
				}
			}
			void OnMouseLeave() {
				SetHot(0);
				SetMsgHandled(FALSE);
			}
			int HitTest(CPoint pt) {
				auto layout = Layout();
				if (layout.upper.PtInRect(pt)) return 1;
				else if (layout.lower.PtInRect(pt)) return 2;
				else return 0;
			}
			void OnMouseBtn(UINT flags, CPoint) {
				bool bDown = (flags & MK_LBUTTON) != 0;
				if (bDown != m_btnDown) {
					m_btnDown = bDown;
					Invalidate();
				}
				SetMsgHandled(FALSE);
			}
			void OnMouseMove(UINT, CPoint pt) {
				SetHot(HitTest(pt));
				SetMsgHandled(FALSE);
			}
			void OnPaint(CDCHandle target, unsigned flags = 0) {
				if (!m_dark) { SetMsgHandled(FALSE); return; }
				(void)flags;
				if (target) {
					HandlePaint(target);
				} else {
					CPaintDC pdc(*this);
					HandlePaint(pdc.m_hDC);
				}
			}
			void HandlePaint(CDCHandle dc) {
				// no corresponding getsyscolor values for this, hardcoded values taken from actual button control
				// + frame trying to fit edit control frame
				const COLORREF colorText = 0xFFFFFF;
				const COLORREF colorFrame = 0xFFFFFF;
				const COLORREF colorBk = 0x333333;
				const COLORREF colorHot = 0x454545;
				const COLORREF colorPressed = 0x666666;

				CRect rcClient; WIN32_OP_D(GetClientRect(rcClient));
				auto layout = Layout(rcClient);
				dc.FillRect(rcClient, MakeTempBrush(dc, colorBk));

				if (m_hot != 0) {
					auto color = m_btnDown ? colorPressed : colorHot;
					switch (m_hot) {
					case 1:
						dc.FillSolidRect(layout.upper, color);
						break;
					case 2:
						dc.FillSolidRect(layout.lower, color);
						break;
					}
				}

				dc.FrameRect(layout.whole, MakeTempBrush(dc, colorFrame));
				dc.SetDCPenColor(colorFrame);
				dc.SelectPen((HPEN)GetStockObject(DC_PEN));
				dc.MoveTo(layout.whole.left, layout.yCenter);
				dc.LineTo(layout.whole.right, layout.yCenter);
				
				CFontHandle f = GetFont();
				if (f == NULL) {
					auto buddy = this->GetBuddy();
					if ( buddy ) f = buddy.GetFont();
				}
				if (f == NULL) f = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
				PFC_ASSERT(f != NULL);
				CFont font2;
				CreateScaledFont(font2, f, 0.5);
				SelectObjectScope selectFont(dc, font2);
				dc.SetBkMode(TRANSPARENT);
				dc.SetTextColor(colorText);
				dc.DrawText(L"˄", 1, layout.upper, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);
				dc.DrawText(L"˅", 1, layout.lower, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);
			}
		};

		class CNCFrameHook : public CWindowImpl<CNCFrameHook, CWindow> { 
		public:
			CNCFrameHook(bool dark) : m_dark(dark) {}

			BEGIN_MSG_MAP_EX(CNCFrameHook)
				MESSAGE_HANDLER_EX(msgSetDarkMode(), OnSetDarkMode)
				MSG_WM_NCPAINT(OnNCPaint)
			END_MSG_MAP()

			void SetDark(bool v) {
				if (v != m_dark) {
					m_dark = v; ApplyDark();
				}
			}
			BOOL SubclassWindow(HWND wnd) {
				auto rv = __super::SubclassWindow(wnd);
				if (rv) {
					ApplyDark();
				}
				return rv;
			}
		private:
			void OnNCPaint(HRGN rgn) {
				if (m_dark) {
					NCPaintDarkFrame(m_hWnd, rgn);
					return;
				}
				SetMsgHandled(FALSE);
			}
			void ApplyDark() {
				ApplyDarkThemeCtrl(m_hWnd, m_dark);
				Invalidate();
			}
			LRESULT OnSetDarkMode(UINT, WPARAM wp, LPARAM) {
				switch (wp) {
				case 0: SetDark(false); break;
				case 1: SetDark(true); break;
				}
				return 1;
			}

			bool m_dark;
		};
	}

	void CHooks::AddPopup(HWND wnd) {
		addOp( [wnd, this] {
			UpdateTitleBar(wnd, m_dark);
		} );
	}

	void CHooks::AddDialogWithControls(HWND wnd) {
		AddDialog(wnd); AddControls(wnd);
	}

	void CHooks::AddDialog(HWND wnd) {

		{
			CWindow w(wnd);
			if ((w.GetStyle() & WS_CHILD) == 0) {
				AddPopup(wnd);
			}
		}

		auto hook = new ImplementOnFinalMessage< CDialogHook > (m_dark);
		hook->SubclassWindow(wnd);
		AddCtrlMsg(wnd);
	}
	void CHooks::AddTabCtrl(HWND wnd) {
		auto hook = new ImplementOnFinalMessage<CTabsHook>(m_dark);
		hook->SubclassWindow(wnd);
		AddCtrlMsg(wnd);
	}
	void CHooks::AddComboBox(HWND wnd) {
		{
			CComboBox combo = wnd;
			COMBOBOXINFO info = {sizeof(info)};
			WIN32_OP_D( combo.GetComboBoxInfo(&info) );
			if (info.hwndList != NULL) {
				AddListBox( info.hwndList );
			}
		}

		addOp([wnd, this] { 
			SetWindowTheme(wnd, m_dark ? L"DarkMode_CFD" : L"Explorer", NULL);
		});
	}
	void CHooks::AddComboBoxEx(HWND wnd) {
		this->AddControls(wnd); // recurse to add the combo box
	}
	void CHooks::AddEditBox(HWND wnd) { 
#if 0 // Experimental
		auto hook = new ImplementOnFinalMessage<CNCFrameHook>(m_dark);
		hook->SubclassWindow( wnd );
		AddCtrlMsg( wnd );
#else
		AddGeneric(wnd); 
#endif
	}
	void CHooks::AddButton(HWND wnd) { 
		CButton btn(wnd);
		auto style = btn.GetButtonStyle();
		auto type = style & BS_TYPEMASK;
		if ((type == BS_CHECKBOX || type == BS_AUTOCHECKBOX || type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON || type == BS_3STATE || type == BS_AUTO3STATE) && (style & BS_PUSHLIKE) == 0) {
			// MS checkbox implementation is terminally retarded and won't draw text in correct color
			// Subclass it and draw our own content
			// Other button types seem OK
			auto hook = new ImplementOnFinalMessage<CCheckBoxHook>(m_dark);
			hook->SubclassWindow(wnd);
			AddCtrlMsg(wnd);
		} else if (type == BS_GROUPBOX) {
			SetWindowTheme(wnd, L"", L"");
			// SNAFU: re-creation of other controls such as list boxes causes repaint bugs due to overlapping
			// Even though this is not a fault of the groupbox, fix it here - by defer-pushing all group boxes to the back
			// Can't move to back right away, breaks window enumeration
			m_lstMoveToBack.push_back(wnd);
		} else {
			AddGeneric(wnd);
		}
		
	}
	void CHooks::AddGeneric(HWND wnd, const wchar_t * name) {
		this->addOp([wnd, this, name] {ApplyDarkThemeCtrl(wnd, m_dark, name); });
	}
	void CHooks::AddClassic(HWND wnd, const wchar_t* normalTheme) {
		this->addOp([wnd, this, normalTheme] {
			if (m_dark) ::SetWindowTheme(wnd, L"", L"");
			else ::SetWindowTheme(wnd, normalTheme, nullptr);
		});
	}
	void CHooks::AddStatusBar(HWND wnd) {
		auto hook = new ImplementOnFinalMessage<CStatusBarHook>(m_dark);
		hook->SubclassWindow(wnd);
		this->AddCtrlMsg(wnd);
	}
	void CHooks::AddScrollBar(HWND wnd) {
		CWindow w(wnd);
		if (w.GetStyle() & SBS_SIZEGRIP) {
			auto hook = new ImplementOnFinalMessage<CGripperHook>(m_dark);
			hook->SubclassWindow(wnd);
			this->AddCtrlMsg(wnd);
		} else {
			AddGeneric(wnd);
		}
	}

	void CHooks::AddReBar(HWND wnd) {
		auto hook = new ImplementOnFinalMessage<CReBarHook>(m_dark);
		hook->SubclassWindow(wnd);
		this->AddCtrlMsg(wnd);
	}

	void CHooks::AddToolBar(HWND wnd, bool bExplorerTheme) {
		// Not a subclass
		addObj(new CToolbarHook(wnd, m_dark, bExplorerTheme));
	}

	void CHooks::AddStatic(HWND wnd) {
		auto hook = new ImplementOnFinalMessage<CStaticHook>(m_dark);
		hook->SubclassWindow(wnd);
		this->AddCtrlMsg(wnd);
	}

	void CHooks::AddUpDown(HWND wnd) {
		auto hook = new ImplementOnFinalMessage<CUpDownHook>(m_dark);
		hook->SubclassWindow(wnd);
		this->AddCtrlMsg(wnd);
	}

	void CHooks::AddTreeView(HWND wnd) {
		auto hook = new ImplementOnFinalMessage<CTreeViewHook>(m_dark);
		hook->SubclassWindow(wnd);
		this->AddCtrlMsg(wnd);
	}
	void CHooks::AddListBox(HWND wnd) {
		this->AddGeneric( wnd );
#if 0
		auto subst = CListControl_ReplaceListBox(wnd);
		if (subst) AddPPListControl(subst);
#endif
	}

	void CHooks::AddListView(HWND wnd) {
		auto subst = CListControl_ReplaceListView(wnd);
		if (subst) AddPPListControl(subst);
	}

	void CHooks::AddPPListControl(HWND wnd) {
		this->AddCtrlMsg(wnd);
		// this->addOp([this, wnd] { CListControl::wndSetDarkMode(wnd, m_dark); });
	}

	void CHooks::SetDark(bool v) {
		// Important: some handlers to ugly things if told to apply when no state change occurred - UpdateTitleBar() stuff in particular
		if (m_dark != v) {
			m_dark = v;
			for (auto& f : m_apply) f();
		}
	}
	void CHooks::flushMoveToBack() {
		for (auto w : m_lstMoveToBack) {
			::SetWindowPos(w, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
		m_lstMoveToBack.clear();
	}
	void CHooks::AddControls(HWND wndParent) {
		for (HWND walk = GetWindow(wndParent, GW_CHILD); walk != NULL; ) {
			HWND next = GetWindow(walk, GW_HWNDNEXT);
			AddCtrlAuto(walk);
			walk = next;
		}
		this->flushMoveToBack();
		// EnumChildWindows(wndParent, [this](HWND ctrl) {this->AddCtrlAuto(ctrl);});
	}

	void CHooks::AddCtrlMsg(HWND w) {
		this->addOp([this, w] {
			::SendMessage(w, msgSetDarkMode(), this->m_dark ? 1 : 0, 0);
		});
	}

	void CHooks::AddCtrlAuto(HWND wnd) {

		if (::SendMessage(wnd, msgSetDarkMode(), -1, -1)) {
			AddCtrlMsg(wnd); return;
		}

		wchar_t buffer[128] = {};

		::GetClassName(wnd, buffer, (int)(std::size(buffer) - 1));

		const wchar_t* cls = buffer;
		if (_wcsnicmp(cls, L"ATL:", 4) == 0) cls += 4;


		if (_wcsicmp(cls, CButton::GetWndClassName()) == 0) {
			AddButton(wnd);
		} else if (_wcsicmp(cls, CComboBox::GetWndClassName()) == 0) {
			AddComboBox(wnd);
		} else if (_wcsicmp(cls, CComboBoxEx::GetWndClassName()) == 0 ) {
			AddComboBoxEx(wnd);
		} else if (_wcsicmp(cls, CTabCtrl::GetWndClassName()) == 0) {
			AddTabCtrl(wnd);
		} else if (_wcsicmp(cls, CStatusBarCtrl::GetWndClassName()) == 0) {
			AddStatusBar(wnd);
		} else if (_wcsicmp(cls, CEdit::GetWndClassName()) == 0) {
			AddEditBox(wnd);
		} else if (_wcsicmp(cls, WC_SCROLLBAR) == 0) {
			AddScrollBar(wnd);
		} else if (_wcsicmp(cls, CToolBarCtrl::GetWndClassName()) == 0) {
			AddToolBar(wnd);
		} else if (_wcsicmp(cls, CTrackBarCtrl::GetWndClassName()) == 0) {
			AddGeneric(wnd);
		} else if (_wcsicmp(cls, CTreeViewCtrl::GetWndClassName()) == 0) {
			AddTreeView(wnd);
		} else if (_wcsicmp(cls, CStatic::GetWndClassName()) == 0) {
			AddStatic(wnd);
		} else if (_wcsicmp(cls, CUpDownCtrl::GetWndClassName()) == 0) {
			AddUpDown(wnd);
		} else if (_wcsicmp(cls, CListViewCtrl::GetWndClassName()) == 0) {
			AddListView(wnd);
		} else if (_wcsicmp(cls, CListBox::GetWndClassName()) == 0) {
			AddListBox(wnd);
		} else if (_wcsicmp(cls, CReBarCtrl::GetWndClassName()) == 0) {
			 AddReBar(wnd);
		} else {
#if PFC_DEBUG
			pfc::outputDebugLine(pfc::format("DarkMode: unknown class - ", buffer));
#endif
		}
	}

	void CHooks::clear() {
		m_apply.clear();
		for (auto v : m_cleanup) v();
		m_cleanup.clear();
	}

	void CHooks::AddApp() {
		addOp([this] {
			SetAppDarkMode(this->m_dark);
		});
	}

	void NCPaintDarkFrame(HWND wnd_, HRGN rgn_) {
		// rgn is in SCREEN COORDINATES, possibly (HRGN)1 to indicate no clipping / whole nonclient area redraw
		// we're working with SCREEN COORDINATES until actual DC painting
		CWindow wnd = wnd_;

		CRect rcWindow, rcClient;
		WIN32_OP_D( wnd.GetWindowRect(rcWindow) );
		WIN32_OP_D( wnd.GetClientRect(rcClient) );
		WIN32_OP_D( wnd.ClientToScreen( rcClient ) ); // transform all to same coordinate system

		CRgn rgnClip;
		WIN32_OP_D( rgnClip.CreateRectRgnIndirect(rcWindow) != NULL );
		if (rgn_ != NULL && rgn_ != (HRGN)1) {
			// we have a valid HRGN from caller?
			if (rgnClip.CombineRgn(rgn_, RGN_AND) == NULLREGION) return; // nothing to draw, exit early
		}

		{
			// Have scroll bars? Have DefWindowProc() them then exclude from our rgnClip.
			SCROLLBARINFO si = { sizeof(si) };
			if (::GetScrollBarInfo(wnd, OBJID_VSCROLL, &si) && (si.rgstate[0] & STATE_SYSTEM_INVISIBLE) == 0 && si.rcScrollBar.left < si.rcScrollBar.right) {
				CRect rc = si.rcScrollBar;
				// rcClient.right = rc.right;
				CRgn rgn; WIN32_OP_D( rgn.CreateRectRgnIndirect(rc) );
				int status = SIMPLEREGION;
				if (rgnClip) {
					status = rgn.CombineRgn(rgn, rgnClip, RGN_AND);
				}
				if (status != NULLREGION) {
					DefWindowProc(wnd, WM_NCPAINT, (WPARAM)rgn.m_hRgn, 0);
					rgnClip.CombineRgn(rgn, RGN_DIFF); // exclude from further drawing
				}
			}
			if (::GetScrollBarInfo(wnd, OBJID_HSCROLL, &si) && (si.rgstate[0] & STATE_SYSTEM_INVISIBLE) == 0 && si.rcScrollBar.top < si.rcScrollBar.bottom) {
				CRect rc = si.rcScrollBar;
				// rcClient.bottom = rc.bottom;
				CRgn rgn; WIN32_OP_D(rgn.CreateRectRgnIndirect(rc));
				int status = SIMPLEREGION;
				if (rgnClip) {
					status = rgn.CombineRgn(rgn, rgnClip, RGN_AND);
				}
				if (status != NULLREGION) {
					DefWindowProc(wnd, WM_NCPAINT, (WPARAM)rgn.m_hRgn, 0);
					rgnClip.CombineRgn(rgn, RGN_DIFF); // exclude from further drawing
				}
			}
		}

		const auto colorLight = DarkMode::GetSysColor(COLOR_BTNHIGHLIGHT);
		const auto colorDark = DarkMode::GetSysColor(COLOR_BTNSHADOW);

		CWindowDC dc( wnd );
		if (dc.IsNull()) {
			PFC_ASSERT(!"???");
			return;
		}


		// Window DC has (0,0) in upper-left corner of our window (not screen, not client)
		// Turn rcWindow to (0,0), (winWidth, winHeight)
		CPoint origin = rcWindow.TopLeft();
		rcWindow.OffsetRect(-origin);
		rcClient.OffsetRect(-origin);

		if (!rgnClip.IsNull()) {
			// rgnClip is still in screen coordinates, fix this here
			rgnClip.OffsetRgn(-origin);
			dc.SelectClipRgn(rgnClip);
		}

		// bottom
		dc.FillSolidRect(CRect(rcClient.left, rcClient.bottom, rcWindow.right, rcWindow.bottom), colorLight);
		// right
		dc.FillSolidRect(CRect(rcClient.right, rcWindow.top, rcWindow.right, rcClient.bottom), colorLight);
		// top
		dc.FillSolidRect(CRect(rcWindow.left, rcWindow.top, rcWindow.right, rcClient.top), colorDark);
		// left
		dc.FillSolidRect(CRect(rcWindow.left, rcClient.top, rcClient.left, rcWindow.bottom), colorDark);
	}
}
