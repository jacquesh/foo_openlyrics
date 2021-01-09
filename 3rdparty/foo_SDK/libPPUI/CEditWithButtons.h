#pragma once

#include <list>
#include <string>
#include <functional>
#include <memory>
#include "WTL-PP.h"

#include "CButtonLite.h"

class CEditWithButtons : public CEditPPHooks {
public:
	CEditWithButtons(::ATL::CMessageMap * hookMM = nullptr, int hookMMID = 0) : CEditPPHooks(hookMM, hookMMID), m_fixedWidth() {}

	enum {
		MSG_CHECKCONDITIONS = WM_USER+13,
		MSG_CHECKCONDITIONS_MAGIC1 = 0xaec66f0c,
		MSG_CHECKCONDITIONS_MAGIC2 = 0x180c2f35,

	};

	BEGIN_MSG_MAP_EX(CEditWithButtons)
		MSG_WM_SETFONT(OnSetFont)
		MSG_WM_WINDOWPOSCHANGED(OnPosChanged)
		MSG_WM_CTLCOLORBTN(OnColorBtn)
		MESSAGE_HANDLER_EX(WM_GETDLGCODE, OnGetDlgCode)
		MSG_WM_KEYDOWN(OnKeyDown)
		MSG_WM_CHAR(OnChar)
		// MSG_WM_SETFOCUS( OnSetFocus )
		// MSG_WM_KILLFOCUS( OnKillFocus )
		MSG_WM_ENABLE(OnEnable)
		MSG_WM_SETTEXT( OnSetText )
		MESSAGE_HANDLER_EX(WM_PAINT, CheckConditionsTrigger)
		MESSAGE_HANDLER_EX(WM_CUT, CheckConditionsTrigger)
		MESSAGE_HANDLER_EX(WM_PASTE, CheckConditionsTrigger)
		MESSAGE_HANDLER_EX(MSG_CHECKCONDITIONS, OnCheckConditions)
		CHAIN_MSG_MAP(CEditPPHooks)
	END_MSG_MAP()

	void SubclassWindow( HWND wnd ) {
		CEditPPHooks::SubclassWindow( wnd );
		this->ModifyStyle(0, WS_CLIPCHILDREN);
		RefreshButtons();
	}
	typedef std::function<void () > handler_t;
	typedef std::function<bool (const wchar_t*) > condition_t;

	void AddMoreButton( std::function<void ()> f );
	void AddClearButton( const wchar_t * clearVal = L"", bool bHandleEsc = false);
	void AddButton( const wchar_t * str, handler_t handler, condition_t condition = nullptr, const wchar_t * drawAlternateText = nullptr );

	static unsigned DefaultFixedWidth() {return GetSystemMetrics(SM_CXVSCROLL) * 3 / 4;}
	void SetFixedWidth(unsigned fw = DefaultFixedWidth() ) {
		m_fixedWidth = fw;
		RefreshButtons();
	}
	CRect RectOfButton( const wchar_t * text );

	void Invalidate() {
		__super::Invalidate();
		for( auto i = m_buttons.begin(); i != m_buttons.end(); ++i ) {
			if (i->wnd != NULL) i->wnd.Invalidate();
		}
	}
	void SetShellFolderAutoComplete() {
		SetShellAutoComplete(SHACF_FILESYS_DIRS);
	}
	void SetShellFileAutoComplete() {
		SetShellAutoComplete(SHACF_FILESYS_ONLY);
	}
	void SetShellAutoComplete(DWORD flags) {
		SHAutoComplete(*this, flags);
		SetHasAutoComplete();
	}
	void SetHasAutoComplete(bool bValue = true) {
		m_hasAutoComplete = bValue;
	}
	void RefreshConditions(const wchar_t * newText = nullptr);
private:
	LRESULT OnCheckConditions( UINT msg, WPARAM wp, LPARAM lp ) {
		if ( msg == MSG_CHECKCONDITIONS && wp == MSG_CHECKCONDITIONS_MAGIC1 && lp == MSG_CHECKCONDITIONS_MAGIC2 ) {
			this->RefreshConditions();
		} else {
			SetMsgHandled(FALSE);
		}		
		return 0;
	}
	bool HaveConditions() {
		for( auto i = m_buttons.begin(); i != m_buttons.end(); ++i ) {
			if ( i->condition ) return true;
		}
		return false;
	}
	void PostCheckConditions() {
		if ( HaveConditions() ) {
			PostMessage( MSG_CHECKCONDITIONS, MSG_CHECKCONDITIONS_MAGIC1, MSG_CHECKCONDITIONS_MAGIC2 );
		}
	}
	LRESULT CheckConditionsTrigger( UINT, WPARAM, LPARAM ) {
		PostCheckConditions();
		SetMsgHandled(FALSE);
		return 0;
	}
	int OnSetText(LPCTSTR lpstrText) {
		PostCheckConditions();
		SetMsgHandled(FALSE);
		return 0;
	}
	void OnEnable(BOOL bEnable) {
		for( auto i = m_buttons.begin(); i != m_buttons.end(); ++ i ) {
			if ( i->wnd != NULL ) {
				i->wnd.EnableWindow( bEnable );
			}
		}
		SetMsgHandled(FALSE); 
	}
	void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) {
		if (nChar == VK_TAB ) {
			return;
		}
		PostCheckConditions();
		SetMsgHandled(FALSE);
	}
	void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
		if ( nChar == VK_TAB ) {
			return;
		}
		SetMsgHandled(FALSE);
	}
	bool canStealTab() {
		if (m_hasAutoComplete) return false;
		if (IsShiftPressed()) return false;
		if (m_buttons.size() == 0) return false;
		return true;
	}
	UINT OnGetDlgCode(UINT, WPARAM wp, LPARAM lp) {
		if ( wp == VK_TAB && canStealTab() ) {
			for (auto i = m_buttons.begin(); i != m_buttons.end(); ++ i ) {
				if ( i->visible ) {
					TabFocusThis(i->wnd);
					return DLGC_WANTTAB;
				}
			}
		}
		SetMsgHandled(FALSE); return 0;
	}
	void OnSetFocus(HWND) {
		this->ModifyStyleEx(0, WS_EX_CONTROLPARENT ); SetMsgHandled(FALSE);
	}
	void OnKillFocus(HWND) {
		this->ModifyStyleEx(WS_EX_CONTROLPARENT, 0 ); SetMsgHandled(FALSE);
	}
	HBRUSH OnColorBtn(CDCHandle dc, CButton button) {
		if ( (this->GetStyle() & ES_READONLY) != 0 || !this->IsWindowEnabled() ) {
			return (HBRUSH) GetParent().SendMessage( WM_CTLCOLORSTATIC, (WPARAM) dc.m_hDC, (LPARAM) m_hWnd );
		} else {
			return (HBRUSH) GetParent().SendMessage( WM_CTLCOLOREDIT, (WPARAM) dc.m_hDC, (LPARAM) m_hWnd );
		}
	}
	void OnPosChanged(LPWINDOWPOS lpWndPos) {
		Layout(); SetMsgHandled(FALSE);
	}
	
	struct Button_t {
		std::wstring title, titleDraw;
		handler_t handler;
		std::shared_ptr<CButtonLite> buttonImpl;
		CWindow wnd;
		bool visible;
		condition_t condition;
	};
	void OnSetFont(CFontHandle font, BOOL bRedraw) {
		CRect rc;
		if (GetClientRect(&rc)) {
			Layout(rc.Size(), font);
		}
		SetMsgHandled(FALSE);
	}

	void RefreshButtons() {
		if ( m_hWnd != NULL && m_buttons.size() > 0 ) {
			Layout();
		}
	}
	void Layout( ) {
		CRect rc;
		if (GetClientRect(&rc)) {
			Layout(rc.Size(), NULL);
		}
	}
	static bool IsShiftPressed() {
		return (GetKeyState(VK_SHIFT) & 0x8000) ? true : false;
	}
	CWindow FindDialog() {
		return GetParent(); 
	}
	void TabFocusThis(HWND wnd) {
		FindDialog().PostMessage(WM_NEXTDLGCTL, (WPARAM) wnd, TRUE );
	}
	void TabFocusPrevNext(bool bPrev) {
		FindDialog().PostMessage(WM_NEXTDLGCTL, bPrev ? TRUE : FALSE, FALSE);
	}
	void TabCycleButtons(HWND wnd);

	bool ButtonWantTab( HWND wnd );
	bool EvalCondition( Button_t & btn, const wchar_t * newText );
	void Layout(CSize size, CFontHandle fontSetMe);
	unsigned MeasureButton(Button_t const & button );

	unsigned m_fixedWidth;
	std::list< Button_t > m_buttons;
	bool m_hasAutoComplete = false;
};
