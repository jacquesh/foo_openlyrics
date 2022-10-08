#pragma once

#include <functional>
#include <vsstyle.h>
#include "wtl-pp.h"
#include "win32_op.h"
#include "DarkMode.h"

typedef CWinTraits<WS_CHILD|WS_TABSTOP,0> CButtonLiteTraits;

class CButtonLite : public CWindowImpl<CButtonLite, CWindow, CButtonLiteTraits > {
public:
	BEGIN_MSG_MAP_EX(CButtonLite)
		MESSAGE_RANGE_HANDLER_EX(WM_MOUSEFIRST, WM_MOUSELAST, MousePassThru)
		MSG_WM_MOUSELEAVE(OnMouseLeave)
		MSG_WM_SETTEXT(OnSetText)
		MSG_WM_PAINT( OnPaint )
		MSG_WM_MOUSEMOVE(OnMouseMove)
		MSG_WM_LBUTTONDOWN(OnLButtonDown)
		MSG_WM_SETFOCUS(OnSetFocus)
		MSG_WM_KILLFOCUS(OnKillFocus)
		MSG_WM_KEYDOWN(OnKeyDown)
		MSG_WM_KEYUP(OnKeyUp)
		MSG_WM_CHAR(OnChar)
		MSG_WM_ENABLE(OnEnable)
		MESSAGE_HANDLER_EX(WM_GETDLGCODE, OnGetDlgCode)
		MSG_WM_SETFONT(OnSetFont)
		MSG_WM_GETFONT(OnGetFont)
		MSG_WM_CREATE(OnCreate)
	END_MSG_MAP()
	std::function<void () > ClickHandler;

	unsigned Measure() const {
		auto font = myGetFont();
		LOGFONT lf;
		WIN32_OP_D(font.GetLogFont(lf));
		MakeBoldFont( lf );
		CFont bold;
		WIN32_OP_D(bold.CreateFontIndirect(&lf));
		CWindowDC dc(*this);
		auto oldFont = dc.SelectFont( bold );
		CSize size (0,0);

		{
			CString measure;
			measure = L"#";
			measure += m_textDrawMe;
			WIN32_OP_D(dc.GetTextExtent(measure, measure.GetLength(), &size));
		}

		dc.SelectFont( oldFont );

		return size.cx;
	}
	std::function< void (HWND) > TabCycleHandler;
	std::function< HBRUSH (CDCHandle) > CtlColorHandler;
	std::function< bool (HWND) > WantTabCheck;
	CWindow WndCtlColorTarget;

	// Rationale: sometimes you want a different text to be presented to accessibility APIs than actually drawn
	// For an example, a clear button looks best with a multiplication sign, but the narrator should say "clear" or so when focused
	void DrawAlternateText( const wchar_t * textDrawMe ) {
		m_textDrawMe = textDrawMe;
	}

protected:
	LRESULT MousePassThru(UINT cMsg, WPARAM cFlags, LPARAM lParam) {
		SetMsgHandled(FALSE);
		CPoint cPoint(lParam);
		const DWORD maskButtons = MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 | MK_XBUTTON2;
		if (cMsg == WM_MOUSEWHEEL || cMsg == WM_MOUSEHWHEEL || (cFlags & maskButtons) != 0) {
			ToggleHot(false);
		}
		if (cMsg == WM_MOUSEWHEEL || cMsg == WM_MOUSEHWHEEL) {
			TogglePressed(false);
		}
		if (cMsg == WM_LBUTTONUP) {
			bool wasPressed = m_pressed;
			TogglePressed(false);
			if (wasPressed) OnClicked();
			SetMsgHandled(TRUE);
		}
		return 0;
	}
	CFontHandle m_font;
	void OnSetFont(HFONT font, BOOL bRedraw) {
		m_font = font; if (bRedraw) Invalidate();
	}
	HFONT OnGetFont() {
		return m_font;
	}
	LRESULT OnGetDlgCode(UINT, WPARAM wp, LPARAM) {
		if ( wp == VK_TAB && TabCycleHandler != NULL) {
			if ( WantTabCheck == NULL || WantTabCheck(m_hWnd) ) {
				TabCycleHandler( m_hWnd );
				return DLGC_WANTTAB;
			}
		}
		SetMsgHandled(FALSE); return 0;
	}
	void OnChar(UINT, UINT, UINT) {
	}
	void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
		(void)nRepCnt; (void)nFlags;
		switch(nChar) {
		case VK_SPACE:
		case VK_RETURN:
			TogglePressed(true); break;
		}
	}
	void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) {
		(void)nRepCnt; (void)nFlags;
		switch(nChar) {
		case VK_SPACE:
		case VK_RETURN:
			TogglePressed(false);
			OnClicked(); 
			break;
		}
	}
	void OnSetFocus(CWindow) {
		m_focused = true; Invalidate();
	}
	void OnKillFocus(CWindow) {
		m_focused = false; Invalidate();
	}
	CFontHandle myGetFont() const {
		auto f = GetFont();
		if ( f == NULL ) f = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
		return f;
	}
	static void MakeBoldFont(LOGFONT & lf ) {
		lf.lfWeight += 300;
		if (lf.lfWeight > 1000 ) lf.lfWeight = 1000;
	}

	CWindow GetCtlColorTarget() {
		CWindow target = WndCtlColorTarget;
		if (target == NULL) target = GetParent();
		return target;
	}
	virtual void DrawBackground( CDCHandle dc, CRect rcClient ) {
		{
			HBRUSH brush = NULL;
			if (CtlColorHandler) {
				brush = CtlColorHandler(dc);
			} else {
				brush = (HBRUSH)GetCtlColorTarget().SendMessage(WM_CTLCOLORBTN, (WPARAM)dc.m_hDC, (LPARAM)this->m_hWnd);
			}
			if (brush != NULL) {
				dc.FillRect(rcClient, brush);
				dc.SetBkMode(TRANSPARENT);
			}
		}
		if ( IsPressed() ) {
			CTheme theme;
			if (theme.OpenThemeData(*this, L"BUTTON" )) {
				DrawThemeBackground(theme, dc, BP_PUSHBUTTON, PBS_PRESSED, rcClient, rcClient );
			} else {
				DrawFrameControl( dc, rcClient, DFC_BUTTON, DFCS_PUSHED );
			}
		} else if (m_hot) {
			CTheme theme;
			if (theme.OpenThemeData(*this, L"BUTTON")) {
				DrawThemeBackground(theme, dc, BP_PUSHBUTTON, PBS_HOT, rcClient, rcClient);
			} else {
				DrawFrameControl(dc, rcClient, DFC_BUTTON, DFCS_HOT);
			}
		}
	}

	virtual void OnPaint(CDCHandle) {
		CPaintDC pdc(*this);

		CRect rcClient;
		if (! GetClientRect( &rcClient ) ) return;

		auto font = myGetFont();
		/*
		CFont fontOverride;
		if ( IsPressed() ) {
			LOGFONT lf;
			font.GetLogFont( lf );
			MakeBoldFont( lf );
			fontOverride.CreateFontIndirect( & lf );
			font = fontOverride;
		}
		*/
		HFONT oldFont = pdc.SelectFont( font );

		DrawBackground( pdc.m_hDC, rcClient );

		pdc.SetBkMode( TRANSPARENT );
		if ( !IsWindowEnabled() ) {
			pdc.SetTextColor( DarkMode::GetSysColor(COLOR_GRAYTEXT) );
		} else if ( m_focused ) {
			pdc.SetTextColor( DarkMode::GetSysColor(COLOR_HIGHLIGHT) );
		}
		pdc.DrawText( m_textDrawMe, m_textDrawMe.GetLength(), &rcClient, DT_VCENTER | DT_CENTER | DT_SINGLELINE | DT_NOPREFIX );

		pdc.SelectFont( oldFont );
	}
	virtual void OnClicked() { 
		if ( ClickHandler ) {
			ClickHandler();
		} else {
			GetParent().PostMessage( WM_COMMAND, MAKEWPARAM( this->GetDlgCtrlID(), BN_CLICKED ), (LPARAM) m_hWnd );
		}
	}
	bool IsPressed() {return m_pressed; }
private:
	int OnCreate(LPCREATESTRUCT lpCreateStruct) {
		DarkMode::ApplyDarkThemeCtrl(*this, DarkMode::IsDialogDark(GetCtlColorTarget(), WM_CTLCOLORBTN));
		if ( lpCreateStruct->lpszName != nullptr ) this->m_textDrawMe = lpCreateStruct->lpszName;
		SetMsgHandled(FALSE); return 0;
	}
	void OnEnable(BOOL) {
		Invalidate(); SetMsgHandled(FALSE);
	}
	void ToggleHot( bool bHot ) {
		if ( bHot != m_hot ) {
			m_hot = bHot; Invalidate();
		}
	}
	void OnMouseMove(UINT nFlags, CPoint) {
		const DWORD maskButtons = MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 | MK_XBUTTON2;
		if ((nFlags & maskButtons) != 0) return;
		ToggleHot(true);
		TrackMouseLeave();
	}
	void OnLButtonDown(UINT nFlags, CPoint) {
		const DWORD maskButtons = MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 | MK_XBUTTON2;
		if ( ( nFlags & maskButtons ) != MK_LBUTTON ) return;
		TogglePressed( true );
		TrackMouseLeave();
	}
	void TogglePressed( bool bPressed ) {
		if ( bPressed != m_pressed ) {
			m_pressed = bPressed; Invalidate();
		}
	}
	int OnSetText(LPCTSTR lpstrText) {
		m_textDrawMe = lpstrText;
		Invalidate(); SetMsgHandled(FALSE);
		return 0;
	}
	void TrackMouseLeave() {
		TRACKMOUSEEVENT tme = { sizeof(tme) };
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_hWnd;
		TrackMouseEvent(&tme);
	}
	void OnMouseLeave() {
		ToggleHot(false); TogglePressed(false);
	}

	bool m_pressed = false, m_focused = false, m_hot = false;
	CString m_textDrawMe;
};
