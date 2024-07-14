#pragma once

#ifdef _WIN32

#include "win32_misc.h"
#include "../SDK/cfg_var.h"

static BOOL AdjustWindowRectHelper(CWindow wnd, CRect & rc) {
	const DWORD style = wnd.GetWindowLong(GWL_STYLE), exstyle = wnd.GetWindowLong(GWL_EXSTYLE);
	return AdjustWindowRectEx(&rc,style,(style & WS_POPUP) ? wnd.GetMenu() != NULL : FALSE, exstyle);
}

static void AdjustRectToScreenArea(CRect & rc, CRect rcParent) {
	HMONITOR monitor = MonitorFromRect(rcParent,MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi = {sizeof(MONITORINFO)};
	if (GetMonitorInfo(monitor,&mi)) {
		const CRect clip = mi.rcWork;
		if (rc.right > clip.right) rc.OffsetRect(clip.right - rc.right, 0);
		if (rc.bottom > clip.bottom) rc.OffsetRect(0, clip.bottom - rc.bottom);
		if (rc.left < clip.left) rc.OffsetRect(clip.left - rc.left, 0);
		if (rc.top < clip.top) rc.OffsetRect(0, clip.top - rc.top);
	}
}

static BOOL GetClientRectAsSC(CWindow wnd, CRect & rc) {
	CRect temp;
	if (!wnd.GetClientRect(temp)) return FALSE;
	if (temp.IsRectNull()) return FALSE;
	if (!wnd.ClientToScreen(temp)) return FALSE;
	rc = temp;
	return TRUE;
}


static BOOL CenterWindowGetRect(CWindow wnd, CWindow wndParent, CRect & out) {
	CRect parent, child;
	if (!wndParent.GetWindowRect(&parent) || !wnd.GetWindowRect(&child)) return FALSE;
	{
		CPoint origin = parent.CenterPoint();
		origin.Offset( - child.Width() / 2, - child.Height() / 2);
		child.OffsetRect( origin - child.TopLeft() );
	}
	AdjustRectToScreenArea(child, parent);
	out = child;
	return TRUE;
}

static BOOL CenterWindowAbove(CWindow wnd, CWindow wndParent) {
	CRect rc;
	if (!CenterWindowGetRect(wnd, wndParent, rc)) return FALSE;
	return wnd.SetWindowPos(NULL,rc,SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

static BOOL ShowWindowCentered(CWindow wnd,CWindow wndParent) {
	CRect rc;
	if (!CenterWindowGetRect(wnd, wndParent, rc)) return FALSE;
	return wnd.SetWindowPos(HWND_TOP,rc,SWP_NOSIZE | SWP_SHOWWINDOW);
}

typedef cfg_struct_t<SIZE> cfgWindowSize;

class cfgWindowSizeTracker {
public:
	cfgWindowSizeTracker(cfgWindowSize & p_var) : m_var(p_var) {}

	bool Apply(HWND p_wnd) {
		bool retVal = false;
		m_applied = false;
		auto s = m_var.get();
		if (s.cx > 0 && s.cy > 0) {
			CRect rect(0,0,s.cx,s.cy);
			if (AdjustWindowRectHelper(p_wnd, rect)) {
				SetWindowPos(p_wnd,NULL,0,0,rect.right-rect.left,rect.bottom-rect.top,SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
				retVal = true;
			}
		}
		m_applied = true;
		return retVal;
	}

	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT & lResult) {
		if (uMsg == WM_SIZE && m_applied) {
			if (lParam != 0) {
				m_var.set({ (short)LOWORD(lParam), (short)HIWORD(lParam) });
			}
		}
		return FALSE;
	}
private:
	cfgWindowSize & m_var;
	bool m_applied = false;
};

class cfgDialogSizeTracker : public cfgWindowSizeTracker {
public:
	cfgDialogSizeTracker(cfgWindowSize & p_var) : cfgWindowSizeTracker(p_var) {}
	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT & lResult) {
		if (cfgWindowSizeTracker::ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult)) return TRUE;
		if (uMsg == WM_INITDIALOG) Apply(hWnd);
		return FALSE;
	}
};

struct cfgDialogPositionData {
	static constexpr int32_t
		posInvalid = 0x80000000;
	static constexpr uint32_t 
		sizeInvalid = 0xFFFFFFFF,
		dpiInvalid = 0;

	uint32_t m_width = sizeInvalid, m_height = sizeInvalid;
	int32_t m_posX = posInvalid, m_posY = posInvalid;
	uint32_t m_dpiX = dpiInvalid, m_dpiY = dpiInvalid;

	cfgDialogPositionData reDPI(CSize) const;
	bool grabFrom(CWindow wnd);
	bool applyTo(CWindow wnd) const;

	bool overrideDefaultSize(t_uint32 width, t_uint32 height);

	pfc::string8 debug() const;
};

struct cfgWindowPositionData {
	WINDOWPLACEMENT m_wp = {};
	SIZE m_dpi = {};

	bool grabFrom(CWindow wnd);
	bool applyTo(CWindow wnd, bool allowHidden = false) const;
};

FB2K_STREAM_READER_OVERLOAD(cfgDialogPositionData) {
	stream >> value.m_width >> value.m_height;
	try {
		stream >> value.m_posX >> value.m_posY >> value.m_dpiX >> value.m_dpiY;
	} catch (exception_io_data) {
		value.m_posX = value.m_posY = cfgDialogPositionData::posInvalid;
		value.m_dpiX = value.m_dpiY = cfgDialogPositionData::dpiInvalid;
	}
	return stream;
}
FB2K_STREAM_WRITER_OVERLOAD(cfgDialogPositionData) {
	return stream << value.m_width << value.m_height << value.m_posX << value.m_posY << value.m_dpiX << value.m_dpiY;
}

class cfgDialogPosition : public cfg_struct_t<cfgDialogPositionData> {
public:
	cfgDialogPosition(const GUID& id) : cfg_struct_t(id) {}

	//! Read and save size data from HWND.
	void read_from_window(HWND);
	//! Apply saved size data to HWND.
	bool apply_to_window(HWND);

	void AddWindow(HWND wnd) { apply_to_window(wnd); }
	void RemoveWindow(HWND wnd) { read_from_window(wnd); }
};

class cfgWindowPosition : public cfg_struct_t<cfgWindowPositionData> {
public:
	cfgWindowPosition(const GUID & id) : cfg_struct_t(id) {}

	//! Read and save size data from HWND.
	void read_from_window(HWND);
	//! Apply saved size data to HWND.
	bool apply_to_window(HWND, bool allowHidden = false);
	//! New window created, show it with saved metrics.
	void windowCreated(HWND, bool allowHidden = false);
};

class cfgDialogPositionTracker {
public:
	cfgDialogPositionTracker(cfgDialogPosition & p_var) : m_var(p_var) {}
	~cfgDialogPositionTracker() {Cleanup();}

	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT & lResult) {
		if (uMsg == WM_CREATE || uMsg == WM_INITDIALOG) {
			Cleanup();
			m_wnd = hWnd;
			m_var.AddWindow(m_wnd);
		} else if (uMsg == WM_DESTROY) {
			PFC_ASSERT( hWnd == m_wnd );
			Cleanup();
		}
		return FALSE;
	}

private:
	void Cleanup() {
		if (m_wnd != NULL) {
			m_var.RemoveWindow(m_wnd);
			m_wnd = NULL;
		}
	}
	cfgDialogPosition & m_var;
	CWindow m_wnd;
};

//! DPI-safe window size var \n
//! Stores size in pixel and original DPI\n
//! Use with cfgWindowSizeTracker2
struct cfgWindowSize2_data {
	CSize m_size = CSize(0, 0), m_dpi = CSize(0, 0);
};

class cfgWindowSize2 : public cfg_struct_t< cfgWindowSize2_data > {
public:
	cfgWindowSize2(const GUID & p_guid) : cfg_struct_t(p_guid) {}

	bool is_valid() {
		auto v = cfg_struct_t::get();
		return v.m_size.cx > 0 && v.m_size.cy > 0;
	}

	CSize get( CSize forDPI ) {
		auto v = cfg_struct_t::get();
		if ( forDPI == v.m_dpi ) return v.m_size;

		CSize ret;
		ret.cx = MulDiv( v.m_size.cx, forDPI.cx, v.m_dpi.cx );
		ret.cy = MulDiv( v.m_size.cy, forDPI.cy, v.m_dpi.cy );
		return ret;
	}
};

//! Forward messages to this class to utilize cfgWindowSize2
class cfgWindowSizeTracker2 : public CMessageMap {
public:
	cfgWindowSizeTracker2( cfgWindowSize2 & var ) : m_var(var) {}

	BEGIN_MSG_MAP_EX(cfgWindowSizeTracker2)
		if (uMsg == WM_CREATE || uMsg == WM_INITDIALOG) {
			Apply(hWnd);
		}
		MSG_WM_SIZE( OnSize )
	END_MSG_MAP()

	bool Apply(HWND p_wnd) {
		bool retVal = false;
		m_applied = false;
		if (m_var.is_valid()) {
			CRect rect( CPoint(0,0), m_var.get( m_DPI ) );
			if (AdjustWindowRectHelper(p_wnd, rect)) {
				SetWindowPos(p_wnd,NULL,0,0,rect.right-rect.left,rect.bottom-rect.top,SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
				retVal = true;
			}
		}
		m_applied = true;
		return retVal;
	}

private:
	void OnSize(UINT nType, CSize size) {
		if ( m_applied && size.cx > 0 && size.cy > 0 ) {
			m_var.set( { size, m_DPI } );
		}
		SetMsgHandled(FALSE);
	}
	cfgWindowSize2 & m_var;
	bool m_applied = false;
	const CSize m_DPI = QueryScreenDPIEx();
};

#endif // _WIN32
