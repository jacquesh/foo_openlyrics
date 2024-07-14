#include "stdafx.h"
#include <vsstyle.h>
#include "Controls.h"
#include "PaintUtils.h"
#include "HyperLinkCtrl.h"

void CStaticSeparator::OnPaint(CDCHandle) {
	PaintUtils::PaintSeparatorControl(*this);
}

void CSeparator::OnPaint(CDCHandle dc) {
	PaintUtils::PaintSeparatorControl(*this);
}

#if 0 // BROKEN WITH DARK MODE, DO NOT USE
CStaticMainInstruction::CStaticMainInstruction() { 
	SetThemePart(TEXT_MAININSTRUCTION); 
}

void CStaticThemed::OnPaint(CDCHandle) {
	if (m_fallback) {
		SetMsgHandled(FALSE); return;
	}
	if (m_theme == NULL) {
		m_theme.OpenThemeData(*this, L"TextStyle");
		if (m_theme == NULL) {
			m_fallback = true; SetMsgHandled(FALSE); return;
		}
	}
	CPaintDC dc(*this);
	TCHAR buffer[512] = {};
	GetWindowText(buffer, _countof(buffer));
	const int txLen = (int) pfc::strlen_max_t(buffer, _countof(buffer));
	CRect contentRect;
	WIN32_OP_D(GetClientRect(contentRect));
	SelectObjectScope scopeFont(dc, GetFont());
	dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
	dc.SetBkMode(TRANSPARENT);

	if (txLen > 0) {
		CRect rcText(contentRect);
		DWORD flags = 0;
		DWORD style = GetStyle();
		if (style & SS_LEFT) flags |= DT_LEFT;
		else if (style & SS_RIGHT) flags |= DT_RIGHT;
		else if (style & SS_CENTER) flags |= DT_CENTER;
		if (style & SS_ENDELLIPSIS) flags |= DT_END_ELLIPSIS;

		HRESULT retval = DrawThemeText(m_theme, dc, m_id, 0, buffer, txLen, flags, 0, rcText);
		PFC_ASSERT(SUCCEEDED(retval));
	}
}
#endif


#include "DarkMode-CHyperLink.h"
#include "windowLifetime.h"

void PP::createHyperLink(HWND wndReplaceMe) {
	auto obj = PP::subclassThisWindow<DarkMode::CHyperLink>(wndReplaceMe);
	obj->SetHyperLinkExtendedStyle(HLINK_NOTIFYBUTTON);
}

namespace {
	class CHyperLinkLambda : public DarkMode::CHyperLinkImpl<CHyperLinkLambda> {
	public:
		std::function<void ()> f;
		bool Navigate() {
			f();
			return true;
		}
	};
}
void PP::createHyperLink(HWND wndReplaceMe, std::function<void ()> handler) {
	auto obj = PP::subclassThisWindow<CHyperLinkLambda>(wndReplaceMe);
	obj->f = handler;
}

void PP::createHyperLink(HWND wndReplaceMe, const wchar_t* openURL) {
	auto obj = PP::subclassThisWindow<DarkMode::CHyperLink>(wndReplaceMe);
	obj->SetHyperLink(openURL);
}
