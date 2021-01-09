#pragma once

#pragma comment(lib, "uxtheme.lib")

#include "wtl-pp.h"
#include "win32_op.h"

// Separator-in-dialog tool: subclass a static control on init
class CStaticSeparator : public CContainedWindowT<CStatic>, private CMessageMap {
public:
	CStaticSeparator() : CContainedWindowT<CStatic>(this, 0) {}
	BEGIN_MSG_MAP_EX(CSeparator)
		MSG_WM_PAINT(OnPaint)
		MSG_WM_SETTEXT(OnSetText)
	END_MSG_MAP()
private:
	int OnSetText(LPCTSTR lpstrText) {
		Invalidate();
		SetMsgHandled(FALSE);
		return 0;
	}
	void OnPaint(CDCHandle);
};

// CWindowRegistered with font & text functionality, for creating custom text label classes
template<typename TClass>
class CTextControl : public CWindowRegisteredT<TClass> {
public:
	BEGIN_MSG_MAP_EX(CTextControl)
		MSG_WM_SETFONT(OnSetFont)
		MSG_WM_GETFONT(OnGetFont)
		MSG_WM_SETTEXT(OnSetText)
		CHAIN_MSG_MAP(__super)
	END_MSG_MAP()
private:
	HFONT OnGetFont() {
		return m_font;
	}
	void OnSetFont(HFONT font, BOOL bRedraw) {
		m_font = font;
		if (bRedraw) this->Invalidate();
	}
	int OnSetText(LPCTSTR lpstrText) {
		this->Invalidate();this->SetMsgHandled(FALSE); return 0;
	}
	CFontHandle m_font;
};


// Static control subclass with override for theme part used for rendering
class CStaticThemed : public CContainedWindowT<CStatic>, private CMessageMap {
public:
	CStaticThemed() : CContainedWindowT<CStatic>(this, 0), m_id(), m_fallback() {}
	BEGIN_MSG_MAP_EX(CStaticThemed)
		MSG_WM_PAINT(OnPaint)
		MSG_WM_THEMECHANGED(OnThemeChanged)
		MSG_WM_SETTEXT(OnSetText)
	END_MSG_MAP()

	void SetThemePart(int id) {m_id = id; if (m_hWnd != NULL) Invalidate();}
private:
	int OnSetText(LPCTSTR lpstrText) {
		Invalidate();
		SetMsgHandled(FALSE);
		return 0;
	}
	void OnThemeChanged() {
		m_theme.Release();
		m_fallback = false;
	}
	void OnPaint(CDCHandle);
	int m_id;
	CTheme m_theme;
	bool m_fallback;
};

class CStaticMainInstruction : public CStaticThemed {
public:
	CStaticMainInstruction();
};



class CSeparator : public CTextControl<CSeparator> {
public:
	BEGIN_MSG_MAP_EX(CSeparator)
		MSG_WM_PAINT(OnPaint)
		MSG_WM_ENABLE(OnEnable)
		CHAIN_MSG_MAP(__super)
	END_MSG_MAP()

	static const TCHAR * GetClassName() {
		return _T("foobar2000:separator");
	}
private:
	void OnEnable(BOOL bEnable) {
		Invalidate();
	}
	void OnPaint(CDCHandle dc);
};

