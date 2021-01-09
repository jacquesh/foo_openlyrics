#include "stdafx.h"
#include <vsstyle.h>
#include "Controls.h"
#include "PaintUtils.h"

void CStaticSeparator::OnPaint(CDCHandle) {
	PaintUtils::PaintSeparatorControl(*this);
}

void CSeparator::OnPaint(CDCHandle dc) {
	PaintUtils::PaintSeparatorControl(*this);
}

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
