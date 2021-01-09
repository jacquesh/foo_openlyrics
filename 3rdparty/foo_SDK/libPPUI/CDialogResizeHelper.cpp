#include "stdafx.h"
#include "CDialogResizeHelper.h"

#include "win32_op.h"

static BOOL GetChildWindowRect2(HWND wnd, HWND wndChild, RECT* child) {
	RECT temp;
	if (wndChild == NULL) return FALSE;
	if (!GetWindowRect(wndChild, &temp)) return FALSE;
	if (!MapWindowPoints(NULL, wnd, (POINT*)&temp, 2)) return FALSE;
	*child = temp;
	return TRUE;
}

static BOOL GetChildWindowRect(HWND wnd, UINT id, RECT* child) {
	return GetChildWindowRect2(wnd, GetDlgItem(wnd, id), child);
}


bool CDialogResizeHelper::EvalRect(UINT id, CRect & out) const {
	for( auto iter = m_runtime.begin(); iter != m_runtime.end(); ++ iter ) {
		if ( iter->initData.id == id ) {
			out = _EvalRect(*iter, this->CurrentSize() );
			return true;
		}
	}
	return false;
}

CRect CDialogResizeHelper::_EvalRect(runtime_t const & rt, CSize wndSize) const {
	CRect rc = rt.origRect;
	int delta_x = wndSize.cx - rt.origWindowSize.cx,
		delta_y = wndSize.cy - rt.origWindowSize.cy;

	const Param & e = rt.initData;
	rc.left += pfc::rint32(e.snapLeft * delta_x);
	rc.right += pfc::rint32(e.snapRight * delta_x);
	rc.top += pfc::rint32(e.snapTop * delta_y);
	rc.bottom += pfc::rint32(e.snapBottom * delta_y);

	return rc;
}

CWindow CDialogResizeHelper::ResolveWnd(runtime_t const & rt) const {
	if ( rt.userHWND != NULL ) return rt.userHWND;
	return m_thisWnd.GetDlgItem( rt.initData.id );
}

CSize CDialogResizeHelper::CurrentSize() const {
	CRect rc;
	WIN32_OP_D( m_thisWnd.GetClientRect(rc) );
	return rc.Size();
}

void CDialogResizeHelper::OnSize(UINT, CSize newSize)
{
	if (m_thisWnd != NULL) {
		HDWP hWinPosInfo = BeginDeferWindowPos((int)(m_runtime.size() + (m_sizeGrip != NULL ? 1 : 0)));
		for (auto iter = m_runtime.begin(); iter != m_runtime.end(); ++ iter) {
			CRect rc = _EvalRect(*iter, newSize);
			hWinPosInfo = DeferWindowPos(hWinPosInfo, ResolveWnd(*iter), 0, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
		}
		if (m_sizeGrip != NULL)
		{
			RECT rc, rc_grip;
			if (m_thisWnd.GetClientRect(&rc) && m_sizeGrip.GetWindowRect(&rc_grip)) {
				DWORD flags = SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOCOPYBITS;
				if (IsZoomed(m_thisWnd)) flags |= SWP_HIDEWINDOW;
				else flags |= SWP_SHOWWINDOW;
				hWinPosInfo = DeferWindowPos(hWinPosInfo, m_sizeGrip, NULL, rc.right - (rc_grip.right - rc_grip.left), rc.bottom - (rc_grip.bottom - rc_grip.top), 0, 0, flags);
			}
		}
		EndDeferWindowPos(hWinPosInfo);
		//RedrawWindow(m_thisWnd, NULL, NULL, RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
	SetMsgHandled(FALSE);
}

void CDialogResizeHelper::OnGetMinMaxInfo(LPMINMAXINFO info) const {
	CRect r;
	const DWORD dwStyle = m_thisWnd.GetWindowLong(GWL_STYLE);
	const DWORD dwExStyle = m_thisWnd.GetWindowLong(GWL_EXSTYLE);
	if (max_x && max_y)
	{
		r.left = 0; r.right = max_x;
		r.top = 0; r.bottom = max_y;
		MapDialogRect(m_thisWnd, &r);
		AdjustWindowRectEx(&r, dwStyle, FALSE, dwExStyle);
		info->ptMaxTrackSize.x = r.right - r.left;
		info->ptMaxTrackSize.y = r.bottom - r.top;
	}
	if (min_x && min_y)
	{
		r.left = 0; r.right = min_x;
		r.top = 0; r.bottom = min_y;
		MapDialogRect(m_thisWnd, &r);
		AdjustWindowRectEx(&r, dwStyle, FALSE, dwExStyle);
		info->ptMinTrackSize.x = r.right - r.left;
		info->ptMinTrackSize.y = r.bottom - r.top;
	}
}

void CDialogResizeHelper::OnInitDialog(CWindow thisWnd) {
	
	m_thisWnd = thisWnd;
	const auto origSize = CurrentSize();
	for( auto initIter = m_initData.begin(); initIter != m_initData.end(); ++ initIter ) {
		CRect rc;
		if (GetChildWindowRect(m_thisWnd, initIter->id, &rc)) {
			runtime_t rt;
			rt.origRect = rc;
			rt.initData = * initIter;
			rt.origWindowSize = origSize;
			m_runtime.push_back( std::move(rt) );
		}
	}
	AddSizeGrip();
	SetMsgHandled(FALSE);
}
void CDialogResizeHelper::OnDestroy() {
	m_runtime.clear();
	m_sizeGrip = NULL; m_thisWnd = NULL;
	SetMsgHandled(FALSE);
}

void CDialogResizeHelper::AddSizeGrip()
{
	if (m_thisWnd != NULL && m_sizeGrip == NULL)
	{
		if (m_thisWnd.GetWindowLong(GWL_STYLE) & WS_POPUP) {
			m_sizeGrip = CreateWindowEx(0, WC_SCROLLBAR, _T(""), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SBS_SIZEGRIP | SBS_SIZEBOXBOTTOMRIGHTALIGN,
				0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
				m_thisWnd, (HMENU)0, NULL, NULL);
			if (m_sizeGrip != NULL)
			{
				RECT rc, rc_grip;
				if (m_thisWnd.GetClientRect(&rc) && m_sizeGrip.GetWindowRect(&rc_grip)) {
					m_sizeGrip.SetWindowPos(NULL, rc.right - (rc_grip.right - rc_grip.left), rc.bottom - (rc_grip.bottom - rc_grip.top), 0, 0, SWP_NOZORDER | SWP_NOSIZE);
				}
			}
		}
	}
}


void CDialogResizeHelper::InitTable(const Param* table, size_t tableSize) {
	m_initData.assign( table, table+tableSize );
}
void CDialogResizeHelper::InitTable(const ParamOld * table, size_t tableSize) {
	m_initData.resize(tableSize);
	for (size_t walk = 0; walk < tableSize; ++walk) {
		const ParamOld in = table[walk];
		Param entry = {};
		entry.id = table[walk].id;
		if (in.flags & CDialogResizeHelperCompat::X_MOVE) entry.snapLeft = entry.snapRight = 1;
		else if (in.flags & CDialogResizeHelperCompat::X_SIZE) entry.snapRight = 1;
		if (in.flags & CDialogResizeHelperCompat::Y_MOVE) entry.snapTop = entry.snapBottom = 1;
		else if (in.flags & CDialogResizeHelperCompat::Y_SIZE) entry.snapBottom = 1;
		m_initData[walk] = entry;
	}
}
void CDialogResizeHelper::InitMinMax(const CRect & range) {
	min_x = range.left; min_y = range.top; max_x = range.right; max_y = range.bottom;
}

void CDialogResizeHelper::AddControl(Param const & initData, CWindow wnd) {
	CRect rc;
	if ( wnd == NULL ) {
		PFC_ASSERT( initData.id != 0 );
		WIN32_OP_D(GetChildWindowRect(m_thisWnd, initData.id, rc));
	} else {
		WIN32_OP_D(GetChildWindowRect2(m_thisWnd, wnd, rc));
	}
	
	runtime_t rt;
	rt.initData = initData;
	rt.userHWND = wnd;
	rt.origRect = rc;
	rt.origWindowSize = this->CurrentSize();
	m_runtime.push_back( std::move(rt) );
}

bool CDialogResizeHelper::RemoveControl(CWindow wnd) {
	return pfc::remove_if_t( m_runtime, [wnd] (runtime_t const & rt) {
		return rt.userHWND == wnd;
	} ) > 0;
}

bool CDialogResizeHelper::HaveControl(CWindow wnd) const {
	for( auto i = m_runtime.begin(); i != m_runtime.end(); ++ i ) {
		if ( i->userHWND == wnd ) return true;
	}
	return false;
}
