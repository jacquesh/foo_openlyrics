#pragma once

#include "win32_op.h"
#include "win32_utility.h"

class CMiddleDragOverlay : public CWindowImpl<CMiddleDragOverlay,CWindow, CWinTraits<WS_POPUP, WS_EX_LAYERED> > {
public:
	DECLARE_WND_CLASS_EX(TEXT("{61BFC7AD-C00F-4CEA-8E6A-EA22E01F43F9}"),0,(-1));

	enum {
		ColorKey = 0xc0ffee
	};

	BEGIN_MSG_MAP_EX(CMiddleDragOverlay)
		MESSAGE_HANDLER(WM_CREATE,OnCreate);
		MSG_WM_ERASEBKGND(OnEraseBkgnd)
		MSG_WM_PAINT(OnPaint)
	END_MSG_MAP()

	void ShowHere(CPoint pt);
private:
	LRESULT OnCreate(UINT,WPARAM,LPARAM,BOOL&) {
		::SetLayeredWindowAttributes(*this,ColorKey,0,LWA_COLORKEY);
		return 0;
	}
	BOOL OnEraseBkgnd(CDCHandle dc) {
		CRect rcClient;
		WIN32_OP_D(GetClientRect(rcClient));
		dc.FillSolidRect(rcClient, ColorKey);
		return TRUE;
	}
	void OnPaint(CDCHandle dc) {
		if (dc) {
			Paint(dc);
		} else {
			CPaintDC pdc(*this);
			Paint(pdc.m_hDC);
		}
	}
	void Paint(CDCHandle dc);
};
