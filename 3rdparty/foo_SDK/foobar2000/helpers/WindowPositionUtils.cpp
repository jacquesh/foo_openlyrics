#include "StdAfx.h"
#include "WindowPositionUtils.h"

namespace {
	static BOOL GetParentWndRect(CWindow wndParent, CRect& rc) {
		if (!wndParent.IsIconic()) {
			return wndParent.GetWindowRect(rc);
		}
		WINDOWPLACEMENT pl = { sizeof(pl) };
		if (!wndParent.GetWindowPlacement(&pl)) return FALSE;
		rc = pl.rcNormalPosition;
		return TRUE;
	}

	struct DeOverlapState {
		CWindow m_thisWnd;
		CPoint m_topLeft;
		bool m_match;
	};
	static BOOL CALLBACK MyEnumChildProc(HWND wnd, LPARAM param) {
		DeOverlapState* state = reinterpret_cast<DeOverlapState*>(param);
		if (wnd != state->m_thisWnd && IsWindowVisible(wnd)) {
			CRect rc;
			if (GetWindowRect(wnd, rc)) {
				if (rc.TopLeft() == state->m_topLeft) {
					state->m_match = true; return FALSE;
				}
			}
		}
		return TRUE;
	}
	static bool DeOverlapTest(CWindow wnd, CPoint topLeft) {
		DeOverlapState state = {};
		state.m_thisWnd = wnd; state.m_topLeft = topLeft; state.m_match = false;
		EnumThreadWindows(GetCurrentThreadId(), MyEnumChildProc, reinterpret_cast<LPARAM>(&state));
		return state.m_match;
	}
	static int DeOverlapDelta() {
		return pfc::max_t<int>(GetSystemMetrics(SM_CYCAPTION), 1);
	}
	static void DeOverlap(CWindow wnd, CRect& rc) {
		const int delta = DeOverlapDelta();
		for (;;) {
			if (!DeOverlapTest(wnd, rc.TopLeft())) break;
			rc.OffsetRect(delta, delta);
		}
	}
}

bool cfgDialogPositionData::grabFrom(CWindow wnd) {
	CRect rc;
	if (!GetClientRectAsSC(wnd, rc)) return false;
	const CSize DPI = QueryScreenDPIEx();
	m_dpiX = DPI.cx; m_dpiY = DPI.cy;
	m_width = rc.Width(); m_height = rc.Height();
	m_posX = m_posY = posInvalid;
	CWindow parent = wnd.GetParent();
	if (parent != NULL) {
		CRect rcParent;
		if (GetParentWndRect(parent, rcParent)) {
			m_posX = rc.left - rcParent.left;
			m_posY = rc.top - rcParent.top;
		}
	}
	return true;
}

cfgDialogPositionData cfgDialogPositionData::reDPI( CSize screenDPI ) const {
	cfgDialogPositionData v = *this;
	if (screenDPI.cx == 0 || screenDPI.cy == 0) {
		PFC_ASSERT(!"Should not get here - something seriously wrong with the OS");
		return v;
	}
	if (v.m_dpiX != dpiInvalid && v.m_dpiX != screenDPI.cx) {
		if (v.m_width != sizeInvalid) v.m_width = MulDiv(v.m_width, screenDPI.cx, v.m_dpiX);
		if (v.m_posX != posInvalid) v.m_posX = MulDiv(v.m_posX, screenDPI.cx, v.m_dpiX);
	}
	if (v.m_dpiY != dpiInvalid && v.m_dpiY != screenDPI.cy) {
		if (v.m_height != sizeInvalid) v.m_height = MulDiv(v.m_height, screenDPI.cy, v.m_dpiY);
		if (v.m_posY != posInvalid) v.m_posY = MulDiv(v.m_posY, screenDPI.cy, v.m_dpiY);
	}
	v.m_dpiX = screenDPI.cx;
	v.m_dpiY = screenDPI.cy;
	return v;
}


bool cfgDialogPositionData::overrideDefaultSize(t_uint32 width, t_uint32 height) {
	bool rv = false;
	if (m_width == sizeInvalid && m_height == sizeInvalid) {
		m_width = width; m_height = height; m_posX = m_posY = posInvalid;
		m_dpiX = m_dpiY = 96;
		rv = true;
	}
	return rv;
}

bool cfgDialogPositionData::applyTo(CWindow wnd) const {
	auto v = reDPI(QueryScreenDPIEx());
	CWindow wndParent = wnd.GetParent();
	UINT flags = SWP_NOACTIVATE | SWP_NOZORDER;
	CRect rc;
	if (!GetClientRectAsSC(wnd, rc)) return false;
	if (v.m_width != v.sizeInvalid && v.m_height != v.sizeInvalid && (wnd.GetWindowLong(GWL_STYLE) & WS_SIZEBOX) != 0) {
		rc.right = rc.left + v.m_width;
		rc.bottom = rc.top + v.m_height;
	} else {
		flags |= SWP_NOSIZE;
	}
	if (wndParent != NULL) {
		CRect rcParent;
		if (GetParentWndRect(wndParent, rcParent)) {
			if (v.m_posX != v.posInvalid && v.m_posY != v.posInvalid) {
				rc.MoveToXY(rcParent.TopLeft() + CPoint(v.m_posX, v.m_posY));
			} else {
				CPoint center = rcParent.CenterPoint();
				rc.MoveToXY(center.x - rc.Width() / 2, center.y - rc.Height() / 2);
			}
		}
	}
	if (!AdjustWindowRectHelper(wnd, rc)) return FALSE;

	DeOverlap(wnd, rc);

	{
		CRect rcAdjust(0, 0, 1, 1);
		if (wndParent != NULL) {
			CRect temp;
			if (wndParent.GetWindowRect(temp)) rcAdjust = temp;
		}
		AdjustRectToScreenArea(rc, rcAdjust);
	}


	return wnd.SetWindowPos(NULL, rc, flags);
}

void cfgDialogPosition::AddWindow(CWindow wnd) {
	auto data = this->get();
	data.applyTo(wnd);
}

void cfgDialogPosition::RemoveWindow(CWindow wnd) {
	cfgDialogPositionData data;
	if (data.grabFrom(wnd)) this->set(data);
}
