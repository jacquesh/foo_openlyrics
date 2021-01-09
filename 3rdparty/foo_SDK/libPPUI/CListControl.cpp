#include "stdafx.h"
#include "CListControl.h"
#include "PaintUtils.h"
#include "CListControlUserOptions.h"
#include "GDIUtils.h"

CListControlUserOptions * CListControlUserOptions::instance = nullptr;

CRect CListControlImpl::GetClientRectHook() const {
	CRect temp; if (!GetClientRect(temp)) temp.SetRectEmpty(); return temp;
}

bool CListControlImpl::UserEnabledSmoothScroll() const {
	auto i = CListControlUserOptions::instance;
	if ( i != nullptr ) return i->useSmoothScroll();
	return false;
}

LRESULT CListControlImpl::SetFocusPassThru(UINT,WPARAM,LPARAM,BOOL& bHandled) {
	SetFocus();
	bHandled = FALSE;
	return 0;
}

void CListControlImpl::EnsureVisibleRectAbs(const CRect & p_rect) {
	const CRect rcView = GetVisibleRectAbs();
	const CRect rcItem = p_rect;
	int deltaX = 0, deltaY = 0;

	const bool centerOnItem = m_ensureVisibleUser;

	if (rcItem.top < rcView.top || rcItem.bottom > rcView.bottom) {
		if (rcItem.Height() > rcView.Height()) {
			deltaY = rcItem.top - rcView.top;
		} else {
			if (centerOnItem) {
				deltaY = rcItem.CenterPoint().y - rcView.CenterPoint().y;
			} else {
				if (rcItem.bottom > rcView.bottom) deltaY = rcItem.bottom - rcView.bottom;
				else deltaY = rcItem.top - rcView.top;
				
			}
		}
	}
	if (rcItem.left < rcView.left || rcItem.right > rcView.right) {
		if (rcItem.Width() > rcView.Width()) {
			if (rcItem.left > rcView.left || rcItem.right < rcView.right) deltaX = rcItem.left - rcView.left;
		} else {
			if (centerOnItem) {
				deltaX = rcItem.CenterPoint().x - rcView.CenterPoint().x;
			} else {
				if (rcItem.right > rcView.right) deltaX = rcItem.right - rcView.right;
				else deltaX = rcItem.left - rcView.left;
			}
		}
	}

	if (deltaX != 0 || deltaY != 0) {
		MoveViewOriginDelta(CPoint(deltaX,deltaY));
	}
}
void CListControlImpl::EnsureItemVisible(t_size p_item, bool bUser) {
	m_ensureVisibleUser = bUser;
	EnsureVisibleRectAbs(GetItemRectAbs(p_item));
	m_ensureVisibleUser = false;
}
void CListControlImpl::EnsureHeaderVisible(int p_group) {
	CRect rect;
	if (GetGroupHeaderRectAbs(p_group,rect)) EnsureVisibleRectAbs(rect);
}

void CListControlImpl::RefreshSlider(bool p_vertical) {
	const CRect viewArea = GetViewAreaRectAbs();
	const CRect rcVisible = GetVisibleRectAbs();
	SCROLLINFO si = {};
	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE|SIF_RANGE|SIF_POS;


	if (AllowScrollbar(p_vertical)) {
		if (p_vertical) {
			si.nPage = rcVisible.Height();
			si.nMin = viewArea.top;
			si.nMax = viewArea.bottom - 1;
			si.nPos = rcVisible.top;
		} else {
			si.nPage = rcVisible.Width();
			si.nMin = viewArea.left;
			si.nMax = viewArea.right - 1;
			si.nPos = rcVisible.left;
		}	
	}

	SetScrollInfo(p_vertical ? SB_VERT : SB_HORZ, &si);
}

void CListControlImpl::RefreshSliders() {
	//PROBLEM: while lots of data can be reused across those, it has to be recalculated inbetween because view area etc may change when scroll info changes
	RefreshSlider(false); RefreshSlider(true);
}

int CListControlImpl::GetScrollThumbPos(int which) {
	SCROLLINFO si = {};
	si.cbSize = sizeof(si);
	si.fMask = SIF_TRACKPOS;
	GetScrollInfo(which,&si);
	return si.nTrackPos;
}

namespace {
	class ResolveGroupHelper {
	public:
		ResolveGroupHelper(const CListControlImpl & p_control) : m_control(p_control) {}
		int operator[](t_size p_index) const {return m_control.GetItemGroup(p_index);}
	private:
		const CListControlImpl & m_control;
	};
}

bool CListControlImpl::ResolveGroupRange(int p_id,t_size & p_base,t_size & p_count) const {

	return pfc::binarySearch<>::runGroup(ResolveGroupHelper(*this),0,GetItemCount(),p_id,p_base,p_count);


	//return pfc::bsearch_range_t(GetItemCount(),ResolveGroupHelper(*this),pfc::compare_t<int,int>,p_id,p_base,p_count);
}

static int HandleScroll(WORD p_code,int p_offset,int p_page, int p_line, int p_bottom, int p_thumbpos) {
	switch(p_code) {
	case SB_LINEUP:
		return p_offset - p_line;
	case SB_LINEDOWN:
		return p_offset + p_line;
	case SB_BOTTOM:
		return p_bottom - p_page;
	case SB_TOP:
		return 0;
	case SB_PAGEUP:
		return p_offset - p_page;
	case SB_PAGEDOWN:
		return p_offset + p_page;
	case SB_THUMBPOSITION:
		return p_thumbpos;
	case SB_THUMBTRACK:
		return p_thumbpos;
	default:
		return p_offset;
	}
}

static CPoint ClipPointToRect(CPoint const & p_pt,CRect const & p_rect) {
	return CPoint(pfc::clip_t(p_pt.x,p_rect.left,p_rect.right),pfc::clip_t(p_pt.y,p_rect.top,p_rect.bottom));
}

void CListControlImpl::MoveViewOriginNoClip(CPoint p_target) {
	UpdateWindow();
	const CPoint old = m_viewOrigin;
	m_viewOrigin = p_target;

	if (m_viewOrigin != old) {
		if (m_viewOrigin.x != old.x) SetScrollPos(SB_HORZ,m_viewOrigin.x);
		if (m_viewOrigin.y != old.y) SetScrollPos(SB_VERT,m_viewOrigin.y);

		const CPoint delta = old - m_viewOrigin;
		if (FixedOverlayPresent()) Invalidate();
		else {
			DWORD flags = SW_INVALIDATE | SW_ERASE;
			const DWORD smoothScrollMS = 50;
			if (this->UserEnabledSmoothScroll() && this->CanSmoothScroll()) {
				flags |= SW_SMOOTHSCROLL | (smoothScrollMS << 16);
			}

			ScrollWindowEx(delta.x,delta.y,GetClientRectHook(),NULL,0,0,flags );
		}

		OnViewOriginChange(m_viewOrigin - old);
	}
}

CPoint CListControlImpl::ClipViewOrigin(CPoint p_origin) const {
	return ClipPointToRect(p_origin,GetValidViewOriginArea());
}
void CListControlImpl::MoveViewOrigin(CPoint p_target) {
	MoveViewOriginNoClip(ClipViewOrigin(p_target));
}

#ifndef SPI_GETWHEELSCROLLCHARS
#define SPI_GETWHEELSCROLLCHARS   0x006C
#endif
int CListControlImpl::HandleWheel(int & p_accum,int p_delta, bool bHoriz) {
	if ( m_suppressMouseWheel ) return 0;
	UINT scrollLines = 1;
	SystemParametersInfo(bHoriz ? SPI_GETWHEELSCROLLCHARS : SPI_GETWHEELSCROLLLINES,0,&scrollLines,0);
	if (scrollLines == ~0) {
		p_accum = 0;
		int rv = -pfc::sgn_t(p_delta);
		CRect client = GetClientRectHook();
		if (bHoriz) rv *= client.Width();
		else rv *= client.Height();
		return rv;
	}

	const int itemHeight = GetItemHeight();
	const int extraScale = 10000;

	p_accum += p_delta * extraScale;
	if ((int)scrollLines < 1) scrollLines = 1;
	int multiplier = (WHEEL_DELTA * extraScale) / (scrollLines * itemHeight);
	if (multiplier<1) multiplier = 1;

	int delta = pfc::rint32( (double) p_accum / (double) multiplier );
	p_accum -= delta * multiplier;
	return -delta;

	/*
	if (p_accum<=-multiplier || p_accum>=multiplier) {
		int direction;
		int ov = p_accum;
		if (ov<0) {
			direction = -1;
			ov = -ov;
			p_accum = - ((-p_accum)%multiplier);
		} else {
			p_accum %= multiplier;
			direction = 1;
		}

		return  - (direction * (ov + multiplier - 1) ) / multiplier;
	} else {
		return 0;
	}
	*/
}

LRESULT CListControlImpl::OnVWheel(UINT,WPARAM p_wp,LPARAM p_lp,BOOL&) {
	const CRect client = GetClientRectHook(), view = this->GetViewAreaRectAbs();
	int deltaPixels = HandleWheel(m_wheelAccumY,(short)HIWORD(p_wp), false);

	const bool canVScroll = client.Height() < view.Height();
	const bool canHScroll = client.Width() < view.Width();
	
	CPoint ptDelta;
	if ( canVScroll && canHScroll && GetHotkeyModifierFlags() == MOD_SHIFT) {
		ptDelta = CPoint(deltaPixels, 0); // default to horizontal scroll if shift is pressed
	} else if (canVScroll) {
		ptDelta = CPoint(0,deltaPixels);
	} else if (canHScroll) {
		ptDelta = CPoint(deltaPixels,0);
	}

	if ( ptDelta != CPoint(0,0) ) {
		MoveViewOriginDelta(ptDelta);
	}
	return 0;
}
LRESULT CListControlImpl::OnHWheel(UINT,WPARAM p_wp,LPARAM p_lp,BOOL&) {
	const CRect client = GetClientRectHook();
	int deltaPixels = HandleWheel(m_wheelAccumX,(short)HIWORD(p_wp), true);
	MoveViewOriginDelta(CPoint(-deltaPixels,0));
	return 0;
}

LRESULT CListControlImpl::OnVScroll(UINT,WPARAM p_wp,LPARAM,BOOL&) {
	int target = HandleScroll(LOWORD(p_wp),m_viewOrigin.y,GetVisibleRectAbs().Height(),GetItemHeight(),GetViewAreaRectAbs().bottom,GetScrollThumbPos(SB_VERT));
	MoveViewOrigin(CPoint(m_viewOrigin.x,target));
	return 0;
}
LRESULT CListControlImpl::OnHScroll(UINT,WPARAM p_wp,LPARAM,BOOL&) {
	int target = HandleScroll(LOWORD(p_wp),m_viewOrigin.x,GetVisibleRectAbs().Width(),GetItemHeight() /*fixme*/,GetViewAreaRectAbs().right,GetScrollThumbPos(SB_HORZ));
	MoveViewOrigin(CPoint(target,m_viewOrigin.y));
	return 0;
}

LRESULT CListControlImpl::OnGesture(UINT,WPARAM,LPARAM lParam,BOOL& bHandled) {
	if (!this->m_gestureAPI.IsAvailable()) {
		bHandled = FALSE;
		return 0;
	}
	HGESTUREINFO hGesture = (HGESTUREINFO) lParam;
	GESTUREINFO gestureInfo = {sizeof(gestureInfo)};
	if (m_gestureAPI.GetGestureInfo(hGesture, &gestureInfo)) {
		//console::formatter() << "WM_GESTURE " << pfc::format_hex( gestureInfo.dwFlags ) << " " << (int)gestureInfo.dwID << " X:" << gestureInfo.ptsLocation.x << " Y:" << gestureInfo.ptsLocation.y << " arg:" << (__int64) gestureInfo.ullArguments;
		CPoint pt( gestureInfo.ptsLocation.x, gestureInfo.ptsLocation.y );
		switch(gestureInfo.dwID) {
		case GID_BEGIN:
			m_gesturePoint = pt;
			break;
		case GID_END:
			break;
		case GID_PAN:
			MoveViewOriginDelta( this->m_gesturePoint - pt);
			m_gesturePoint = pt;
			break;
		}
	}

	m_gestureAPI.CloseGestureInfoHandle(hGesture);
	bHandled = TRUE;
	return 0;
}

LRESULT CListControlImpl::OnSize(UINT,WPARAM,LPARAM p_lp,BOOL&) {
	OnSizeAsync_Trigger();
	RefreshSliders();
	return 0;
}


void CListControlImpl::RenderBackground( CDCHandle dc, CRect const & rc ) {
	PaintUtils::FillRectSimple(dc,rc,GetSysColorHook(colorBackground));
}

void CListControlImpl::PaintContent(CRect rcPaint, HDC dc) {
	CDCHandle renderDC(dc);

	CMemoryDC bufferDC(renderDC,rcPaint);
	renderDC = bufferDC;
	this->RenderBackground(renderDC, rcPaint);
		
	{
		const CPoint pt = GetViewOffset();
		OffsetWindowOrgScope offsetScope(renderDC, pt);
		CRect renderRect = rcPaint; renderRect.OffsetRect(pt);
		RenderRect(renderRect,renderDC);
	}
}

void CListControlImpl::OnPrintClient(HDC dc, UINT uFlags) {
	CRect rcClient; this->GetClientRect( rcClient );
	PaintContent( rcClient, dc );
}

LRESULT CListControlImpl::OnPaint(UINT,WPARAM,LPARAM,BOOL&) {
	CPaintDC paintDC(*this);

	PaintContent( paintDC.m_ps.rcPaint, paintDC.m_hDC );

	return 0;
}

namespace {
	class comparator_rect {
	public:
		static int compare(const CRect & p_rect1,const CRect & p_rect2) {
			if (p_rect1.bottom <= p_rect2.top) return -1;
			else if (p_rect1.top >= p_rect2.bottom) return 1;
			else return 0;
		}
	};

	static int RectPointCompare(const CRect & p_item1,const int p_y) {
		if (p_item1.bottom <= p_y) return -1;
		else if (p_item1.top > p_y) return 1;
		else return 0;
	}

	class RectSearchHelper_Items {
	public:
		RectSearchHelper_Items(const CListControlImpl & p_control) : m_control(p_control) {}
		CRect operator[](t_size p_index) const {
			return m_control.GetItemRectAbs(p_index);
		}
	private:
		const CListControlImpl & m_control;
	};
	class RectSearchHelper_Groups {
	public:
		RectSearchHelper_Groups(const CListControlImpl & p_control) : m_control(p_control) {}
		CRect operator[](t_size p_index) const {
			CRect rect;
			if (!m_control.GetGroupHeaderRectAbs((int)(p_index + 1),rect)) rect.SetRectEmpty();
			return rect;
		}
	private:
		const CListControlImpl & m_control;
	};
}

bool CListControlImpl::GetItemRange(const CRect & p_rect,t_size & p_base,t_size & p_count) const {
	CRect temp(p_rect); temp.OffsetRect( GetViewOffset() );
	return GetItemRangeAbs(temp, p_base, p_count);
}



bool CListControlImpl::GetItemRangeAbsInclHeaders(const CRect & p_rect,t_size & p_base,t_size & p_count) const {
	CRect temp(p_rect);
	temp.bottom += this->GetGroupHeaderHeight();
	return GetItemRangeAbs(temp, p_base, p_count);
}

bool CListControlImpl::GetItemRangeAbs(const CRect & p_rect,t_size & p_base,t_size & p_count) const {
	if (p_rect.right < 0 || p_rect.left >= GetItemWidth()) return false;

	return pfc::binarySearch<comparator_rect>::runGroup(RectSearchHelper_Items(*this),0,GetItemCount(),p_rect,p_base,p_count);
}

void CListControlImpl::RenderRect(const CRect & p_rect,CDCHandle p_dc) {
	const CRect rectAbs = p_rect;

	t_size base, count;
	if (GetItemRangeAbs(rectAbs,base,count)) {
		for(t_size walk = 0; walk < count; ++walk) {
			CRect rcUpdate, rcItem = GetItemRectAbs(base+walk);
			if (rcUpdate.IntersectRect(rcItem,p_rect)) {
				DCStateScope dcState(p_dc);
				if (p_dc.IntersectClipRect(rcUpdate) != NULLREGION) {
					try {
						RenderItem(base+walk,rcItem,rcUpdate,p_dc);
					} catch(std::exception const & e) {
						(void) e;
						// console::complain("List Control: Item rendering failure", e);
					}
				}
			}
		}
	}

	if (pfc::binarySearch<comparator_rect>::runGroup(RectSearchHelper_Groups(*this),0,GetGroupCount(),rectAbs,base,count)) {
		for(t_size walk = 0; walk < count; ++walk) {
			CRect rcHeader, rcUpdate;
			const int id = (int)(base+walk+1);
			if (GetGroupHeaderRectAbs(id,rcHeader) && rcUpdate.IntersectRect(rcHeader,p_rect)) {
				DCStateScope dcState(p_dc);
				if (p_dc.IntersectClipRect(rcUpdate) != NULLREGION) {
					try {
						RenderGroupHeader(id,rcHeader,rcUpdate,p_dc);
					} catch(std::exception const & e) {
						(void) e;
						// console::complain("List Control: Group header rendering failure", e);
					}
				}
			}
		}
	}

	RenderOverlay(p_rect,p_dc);
}

CRect CListControlImpl::GetItemRect(t_size p_item) const {
	CRect rcItem = GetItemRectAbs(p_item);
	rcItem.OffsetRect( - GetViewOffset() );
	return rcItem;
}
bool CListControlImpl::GetGroupHeaderRect(int p_group,CRect & p_rect) const {
	if (!GetGroupHeaderRectAbs(p_group,p_rect)) return false;
	p_rect.OffsetRect( - GetViewOffset() );
	return true;
}

int CListControlImpl::GetViewAreaHeight() const {
	const t_size itemCount = GetItemCount();
	int subAreaBase = 0;
	if (itemCount > 0) {
		subAreaBase = GetItemRectAbs(itemCount - 1).bottom;
	}
	return subAreaBase;
}

CRect CListControlImpl::GetItemRectAbs(t_size p_item) const {
	CRect rcItem;
	const int itemHeight = GetItemHeight(), itemWidth = GetItemWidth(), groupHeight = GetGroupHeaderHeight(), itemGroup = GetItemGroup(p_item);
	rcItem.top = (int)p_item * itemHeight + groupHeight * itemGroup;
	rcItem.bottom = rcItem.top + itemHeight;
	rcItem.left = 0;
	rcItem.right = rcItem.left + itemWidth;
	return rcItem;
}
bool CListControlImpl::GetGroupHeaderRectAbs(int p_group,CRect & p_rect) const {
	if (p_group == 0) return false;
	t_size itemBase, itemCount;
	if (!ResolveGroupRange(p_group,itemBase,itemCount)) return false;
	const int itemHeight = GetItemHeight(), itemWidth = GetItemWidth(), groupHeight = GetGroupHeaderHeight();
	p_rect.bottom = (int) itemBase * itemHeight + groupHeight * p_group;
	p_rect.top = p_rect.bottom - groupHeight;
	p_rect.left = 0;
	p_rect.right = p_rect.left + itemWidth;
	return true;
}

CRect CListControlImpl::GetViewAreaRectAbs() const {
	return CRect(0,0,GetViewAreaWidth(),GetViewAreaHeight());
}

CRect CListControlImpl::GetViewAreaRect() const {
	CRect rc = GetViewAreaRectAbs();
	rc.OffsetRect( - GetViewOffset() );
	CRect ret; ret.IntersectRect(rc,GetClientRectHook());
	return ret;
}

t_size CListControlImpl::GetGroupCount() const {
	const t_size itemCount = GetItemCount();
	if (itemCount > 0) {
		return (t_size) GetItemGroup(itemCount-1);
	} else {
		return 0;
	}
}

void CListControlImpl::UpdateGroupHeader(int p_id) {
	CRect rect;
	if (GetGroupHeaderRect(p_id,rect)) {
		InvalidateRect(rect);
	}
}
static void AddUpdateRect(HRGN p_rgn,CRect const & p_rect) {
	CRgn temp; temp.CreateRectRgnIndirect(p_rect);
	CRgnHandle(p_rgn).CombineRgn(temp,RGN_OR);
}

void CListControlImpl::OnItemsReordered( const size_t * order, size_t count ) {
	PFC_ASSERT( count == GetItemCount() );
	ReloadItems( pfc::bit_array_order_changed(order) );
}
void CListControlImpl::UpdateItems(const pfc::bit_array & p_mask) {
	t_size base,count;
	if (GetItemRangeAbs(GetVisibleRectAbs(),base,count)) {
		const t_size max = base+count;
		CRgn updateRgn; updateRgn.CreateRectRgn(0,0,0,0);
		bool found = false;
		for(t_size walk = p_mask.find_first(true,base,max); walk < max; walk = p_mask.find_next(true,walk,max)) {
			found = true;
			AddUpdateRect(updateRgn,GetItemRect(walk));
		}
		if (found) {
			InvalidateRgn(updateRgn);
		}
	}
}

void CListControlImpl::UpdateItemsAndHeaders(const pfc::bit_array & p_mask) {
	t_size base,count;
	int groupWalk = 0;
	if (GetItemRangeAbsInclHeaders(GetVisibleRectAbs(),base,count)) {
		const t_size max = base+count;
		CRgn updateRgn; updateRgn.CreateRectRgn(0,0,0,0);
		bool found = false;
		for(t_size walk = p_mask.find_first(true,base,max); walk < max; walk = p_mask.find_next(true,walk,max)) {
			found = true;
			const int groupId = GetItemGroup(walk);
			if (groupId != groupWalk) {
				if (groupId > 0) {
					CRect rect;
					if (GetGroupHeaderRect(groupId,rect)) {
						AddUpdateRect(updateRgn,rect);
					}
				}
				groupWalk = groupId;
			}
			AddUpdateRect(updateRgn,GetItemRect(walk));
		}
		if (found) {
			InvalidateRgn(updateRgn);
		}
	}
}


CRect CListControlImpl::GetValidViewOriginArea() const {
	const CRect rcView = GetViewAreaRectAbs();
	const CRect rcClient = GetClientRectHook();
	CRect rcArea = rcView;
	rcArea.right -= pfc::min_t(rcView.Width(),rcClient.Width());
	rcArea.bottom -= pfc::min_t(rcView.Height(),rcClient.Height());
	return rcArea;
}

void CListControlImpl::OnViewAreaChanged(CPoint p_originOverride) {
	const CPoint oldViewOrigin = m_viewOrigin;
	m_viewOrigin = ClipPointToRect(p_originOverride,GetValidViewOriginArea());

	RefreshSliders();

	Invalidate();
	
	if (oldViewOrigin != m_viewOrigin) {
		OnViewOriginChange(m_viewOrigin - oldViewOrigin);
	}
}

bool CListControlImpl::ItemFromPointAbs(CPoint const & p_pt,t_size & p_item) const {
	if (p_pt.x < 0 || p_pt.x >= GetItemWidth()) return false;
	t_size dummy;
	return GetItemRangeAbs(CRect(p_pt,p_pt + CPoint(1,1)),p_item,dummy);
}

bool CListControlImpl::GroupHeaderFromPointAbs(CPoint const & p_pt,int & p_group) const {
	if (p_pt.x < 0 || p_pt.x >= GetItemWidth()) return false;
	t_size result;

	if (!pfc::binarySearch<comparator_rect>::run(RectSearchHelper_Groups(*this),0,GetGroupCount(),CRect(p_pt,p_pt + CSize(1,1)),result)) return false;


	//if (!pfc::bsearch_t(GetGroupCount(),RectSearchHelper_Groups(*this),RectCompare,CRect(p_pt,p_pt),result)) return false;
	p_group = (int) (result + 1);
	return true;
}

void CListControlImpl::OnThemeChanged() {
	m_themeCache.remove_all();
}

CTheme & CListControlImpl::themeFor(const char * what) {
	bool bNew;
	auto & ret = this->m_themeCache.find_or_add_ex( what, bNew );
	if (bNew) ret.OpenThemeData(*this, pfc::stringcvt::string_wide_from_utf8(what));
	return ret;
}

LRESULT CListControlImpl::OnCreatePassThru(UINT,WPARAM,LPARAM,BOOL& bHandled) {
	::SetWindowTheme(*this, _T("explorer"), NULL);
	OnViewAreaChanged();

	if (m_gestureAPI.IsAvailable()) {
		GESTURECONFIG config = {GID_PAN, GC_PAN_WITH_SINGLE_FINGER_VERTICALLY|GC_PAN_WITH_INERTIA, GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY | GC_PAN_WITH_GUTTER};
		m_gestureAPI.SetGestureConfig( *this, 0, 1, &config, sizeof(GESTURECONFIG));
	}

	bHandled = FALSE;
	return 0;
}
bool CListControlImpl::IsSameItemOrHeaderAbs(const CPoint & p_point1, const CPoint & p_point2) const {
	t_size item1, item2; int group1, group2;
	if (ItemFromPointAbs(p_point1, item1)) {
		if (ItemFromPointAbs(p_point2,item2)) {
			return item1 == item2;
		} else {
			return false;
		}
	}
	if (GroupHeaderFromPointAbs(p_point1, group1)) {
		if (GroupHeaderFromPointAbs(p_point2, group2)) {
			return group1 == group2;
		} else {
			return false;
		}
	}
	return false;
}

void CListControlImpl::OnSizeAsync_Trigger() {
	if (!m_sizeAsyncPending) {
		if (PostMessage(MSG_SIZE_ASYNC,0,0)) {
			m_sizeAsyncPending = true;
		} else {
			PFC_ASSERT(!"Shouldn't get here!");
			//should not happen
			ListHandleResize();
		}
	}
}

void CListControlImpl::ListHandleResize() {
	MoveViewOriginDelta(CPoint(0,0));
	m_sizeAsyncPending = false;
}

void CListControlImpl::AddGroupHeaderToUpdateRgn(HRGN p_rgn, int id) const {
	if (id > 0) {
		CRect rcHeader;
		if (GetGroupHeaderRect(id,rcHeader)) AddUpdateRect(p_rgn,rcHeader);
	}
}
void CListControlImpl::AddItemToUpdateRgn(HRGN p_rgn, t_size p_index) const {
	if (p_index < this->GetItemCount()) {
		AddUpdateRect(p_rgn,GetItemRect(p_index));
	}
}

COLORREF CListControlImpl::GetSysColorHook(int colorIndex) const {
	return GetSysColor(colorIndex);
}

LRESULT CListControlImpl::OnEraseBkgnd(UINT,WPARAM wp,LPARAM,BOOL&) {
#ifndef CListControl_ScrollWindowFix
	const CRect rcClient = GetClientRectHook();
	PaintUtils::FillRectSimple((HDC)wp,rcClient,GetColor(ui_color_background));
#endif
	return 1;
}

t_size CListControlImpl::InsertIndexFromPointEx(const CPoint & pt, bool & bInside) const {
	bInside = false;
	t_size insertMark = ~0;
	t_size itemIdx; int groupId;
	CPoint test(pt); test += GetViewOffset();
	test.x = GetItemWidth() / 2;
	if (test.y >= GetViewAreaHeight()) {
		return GetItemCount();
	} else if (ItemFromPointAbs(test,itemIdx)) {
		const CRect rc = GetItemRectAbs(itemIdx);
		if (test.y > rc.top + MulDiv(rc.Height(),2,3)) itemIdx++;
		else if (test.y >= rc.top + MulDiv(rc.Height(),1,3)) bInside = true;
		return itemIdx;
	} else if (GroupHeaderFromPointAbs(test,groupId)) {
		t_size base,count;
		if (ResolveGroupRange(groupId,base,count)) {
			return base;
		}
	}
	return ~0;
}
t_size CListControlImpl::InsertIndexFromPoint(const CPoint & pt) const {
	bool dummy; return InsertIndexFromPointEx(pt,dummy);
}

COLORREF CListControlImpl::BlendGridColor( COLORREF bk ) {
	return BlendGridColor( bk, PaintUtils::DetermineTextColor( bk ) );
}

COLORREF CListControlImpl::BlendGridColor( COLORREF bk, COLORREF tx ) {
	return PaintUtils::BlendColor(bk, tx, 10);
}

COLORREF CListControlImpl::GridColor() {
	return BlendGridColor( GetSysColorHook(colorBackground), GetSysColorHook(colorText) );
}

void CListControlImpl::RenderItemBackground(CDCHandle p_dc,const CRect & p_itemRect,size_t p_item, uint32_t bkColor) {
	switch( this->m_rowStyle ) {
	case rowStylePlaylistDelimited:
		PaintUtils::RenderItemBackground(p_dc,p_itemRect,p_item+GetItemGroup(p_item),bkColor);
		{
			auto blend = BlendGridColor(bkColor);
			CDCPen pen(p_dc, blend);
			SelectObjectScope scope(p_dc, pen);

			p_dc.MoveTo( p_itemRect.right-1, p_itemRect.top );
			p_dc.LineTo( p_itemRect.right-1, p_itemRect.bottom );
		}
		break;
	case rowStylePlaylist:
		PaintUtils::RenderItemBackground(p_dc,p_itemRect,p_item+GetItemGroup(p_item),bkColor);
		break;
	case rowStyleGrid:
		PaintUtils::FillRectSimple(p_dc, p_itemRect, bkColor );
		{
			auto blend = BlendGridColor(bkColor);
			CDCBrush brush(p_dc, blend);
			p_dc.FrameRect(&p_itemRect, brush);

		}
		break;
	case rowStyleFlat:
		PaintUtils::FillRectSimple(p_dc, p_itemRect, bkColor );
		break;
	}
}

void CListControlImpl::RenderGroupHeaderBackground(CDCHandle p_dc,const CRect & p_headerRect,int p_group) {
	const t_uint32 bkColor = GetSysColorHook(colorBackground);
	t_size index = 0;
	t_size base, count;
	if (p_group > 0 && ResolveGroupRange(p_group,base,count)) {
		index = base + (t_size) p_group - 1;
	}
	switch( this->m_rowStyle ) {
	default:
		PaintUtils::FillRectSimple( p_dc, p_headerRect, bkColor );
		break;
	case rowStylePlaylistDelimited:
	case rowStylePlaylist:
		PaintUtils::RenderItemBackground(p_dc,p_headerRect,index,bkColor);
		break;
	}
}

void CListControlImpl::RenderItem(t_size p_item,const CRect & p_itemRect,const CRect & p_updateRect,CDCHandle p_dc) {
	this->RenderItemBackground(p_dc, p_itemRect, p_item, GetSysColorHook(colorBackground) );

	DCStateScope backup(p_dc);
	p_dc.SetBkMode(TRANSPARENT);
	p_dc.SetBkColor(GetSysColorHook(colorBackground));
	p_dc.SetTextColor(GetSysColorHook(colorText));

	RenderItemText(p_item,p_itemRect,p_updateRect,p_dc, true);
}

void CListControlImpl::RenderGroupHeader(int p_group,const CRect & p_headerRect,const CRect & p_updateRect,CDCHandle p_dc) {
	this->RenderGroupHeaderBackground(p_dc, p_headerRect, p_group );

	DCStateScope backup(p_dc);
	p_dc.SetBkMode(TRANSPARENT);
	p_dc.SetBkColor(GetSysColorHook(colorBackground));
	p_dc.SetTextColor(GetSysColorHook(colorHighlight));

	RenderGroupHeaderText(p_group,p_headerRect,p_updateRect,p_dc);
}


CListControlFontOps::CListControlFontOps() : m_font((HFONT)::GetStockObject(DEFAULT_GUI_FONT)), m_itemHeight(), m_groupHeaderHeight() {
	UpdateGroupHeaderFont();
	CalculateHeights();
}

void CListControlFontOps::UpdateGroupHeaderFont() {
	try {
		m_groupHeaderFont = NULL;
		LOGFONT lf = {};
		WIN32_OP_D( m_font.GetLogFont(lf) );
		lf.lfHeight = pfc::rint32( (double) lf.lfHeight * GroupHeaderFontScale() );
		lf.lfWeight = GroupHeaderFontWeight(lf.lfWeight);
		WIN32_OP_D( m_groupHeaderFont.CreateFontIndirect(&lf) != NULL );
	} catch(std::exception const & e) {
		(void) e;
		// console::print(e.what());
		m_groupHeaderFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	}
}

void CListControlFontOps::CalculateHeights() {
	const t_uint32 spacing = MulDiv(4, m_dpi.cy, 96);
	m_itemHeight = GetFontHeight( m_font ) + spacing;
	m_groupHeaderHeight = GetFontHeight( m_groupHeaderFont ) + spacing;
}

void CListControlFontOps::SetFont(HFONT font,bool bUpdateView) {
	m_font = font;
	UpdateGroupHeaderFont(); CalculateHeights();
	OnSetFont(bUpdateView);
	if (bUpdateView && m_hWnd != NULL) OnViewAreaChanged();
	
}

LRESULT CListControlFontOps::OnSetFont(UINT,WPARAM wp,LPARAM,BOOL&) {
	SetFont((HFONT)wp);
	return 0;
}

LRESULT CListControlFontOps::OnGetFont(UINT,WPARAM,LPARAM,BOOL&) {
	return (LRESULT)(HFONT)m_font;
}

void CListControlImpl::SetCaptureEx(CaptureProc_t proc) {
	this->m_captureProc = proc; SetCapture();
}

LRESULT CListControlImpl::MousePassThru(UINT msg, WPARAM wp, LPARAM lp) {
	auto p = m_captureProc; // create local ref in case something in mid-captureproc clears it
	if ( p ) {
		CPoint pt(lp);
		if (!p(msg, (DWORD) wp, pt ) ) {
			ReleaseCapture();
			m_captureProc = nullptr;
		}
		return 0;
	}

	SetMsgHandled(FALSE);
	return 0;
}

LRESULT CListControlImpl::OnGetDlgCode(UINT, WPARAM wp, LPARAM) {
	switch(wp) {
	case VK_RETURN:
		return m_dlgWantEnter ? DLGC_WANTMESSAGE : 0;
	default:
		SetMsgHandled(FALSE);
		return 0;
	}
}

void CListControlImpl::CreateInDialog(CWindow wndDialog, UINT replaceControlID ) {
	CWindow lstReplace = wndDialog.GetDlgItem(replaceControlID);
	PFC_ASSERT( lstReplace != NULL );
	auto status = lstReplace.SendMessage(WM_GETDLGCODE, VK_RETURN );
	m_dlgWantEnter = (status & DLGC_WANTMESSAGE);
	CRect rc;
	CWindow wndPrev = wndDialog.GetNextDlgTabItem(lstReplace, TRUE);
	WIN32_OP_D( lstReplace.GetWindowRect(&rc) );
	WIN32_OP_D( wndDialog.ScreenToClient(rc) );
	WIN32_OP_D( lstReplace.DestroyWindow() );
	WIN32_OP_D( this->Create(wndDialog, &rc, 0, 0, WS_EX_STATICEDGE, replaceControlID) );
	if (wndPrev != NULL ) this->SetWindowPos(wndPrev, 0,0,0,0, SWP_NOSIZE | SWP_NOMOVE );
	this->SetFont(wndDialog.GetFont());
}


void CListControlImpl::defer(std::function<void() > f) {
	m_deferred.push_back( f );
	if (!m_defferredMsgPending) {
		if ( PostMessage(MSG_EXEC_DEFERRED) ) m_defferredMsgPending = true;
	}
}

LRESULT CListControlImpl::OnExecDeferred(UINT, WPARAM, LPARAM) {
	
	for ( ;; ) { 
		auto i = m_deferred.begin();
		if ( i == m_deferred.end() ) break;
		auto op = std::move(*i);
		m_deferred.erase(i); // erase first, execute later - avoid erratic behavior if op alters the list
		op();
	}

	m_defferredMsgPending = false;
	return 0;
}

// ========================================================================================
// Mouse wheel vs drag&drop hacks
// Install MouseHookProc for the duration of DoDragDrop and handle the input from there
// ========================================================================================
static HHOOK g_hook = NULL;
static CListControlImpl * g_dragDropInstance = nullptr;
LRESULT CALLBACK CListControlImpl::MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION && g_dragDropInstance != nullptr) {
		switch (wParam) {
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
			g_dragDropInstance->MouseWheelFromHook((UINT)wParam, lParam);
			break;
		}
	}
	return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

bool CListControlImpl::MouseWheelFromHook(UINT msg, LPARAM data) {
	MOUSEHOOKSTRUCTEX const * mhs = reinterpret_cast<MOUSEHOOKSTRUCTEX const *> ( data ); 
	if ( ::WindowFromPoint(mhs->pt) != m_hWnd ) return false;
	LRESULT dummyResult = 0;
	WPARAM wp = mhs->mouseData;
	LPARAM lp = MAKELPARAM( mhs->pt.x, mhs->pt.y );
	// If we get here, m_suppressMouseWheel should be true per our DoDragDrop()
	pfc::vartoggle_t<bool> scope(m_suppressMouseWheel, false);
	this->ProcessWindowMessage( m_hWnd, msg, wp, lp, dummyResult );
	return true;
}

HRESULT CListControlImpl::DoDragDrop(LPDATAOBJECT pDataObj, LPDROPSOURCE pDropSource, DWORD dwOKEffects, LPDWORD pdwEffect) {
	HRESULT ret = E_FAIL;
	// Should not get here with non null g_dragDropInstance - means we have a recursive call
	PFC_ASSERT(g_dragDropInstance == nullptr);
	if ( g_dragDropInstance == nullptr ) {
		// futureproofing: kill mouse wheel message processing if we get them delivered the regular way while this is in progress
		pfc::vartoggle_t<bool> scope(m_suppressMouseWheel, true);
		g_dragDropInstance = this;
		g_hook = SetWindowsHookEx(WH_MOUSE, MouseHookProc, NULL, GetCurrentThreadId());
		try {
			ret = ::DoDragDrop(pDataObj, pDropSource, dwOKEffects, pdwEffect);
		} catch (...) {
		}
		g_dragDropInstance = nullptr;
		UnhookWindowsHookEx(pfc::replace_null_t(g_hook));
	}
	return ret;
}

void CListControlImpl::OnKillFocus(CWindow) {
	if (m_captureProc) {
		ReleaseCapture();
		m_captureProc = nullptr;
	}
	SetMsgHandled(FALSE);
}

void CListControlImpl::OnWindowPosChanged(LPWINDOWPOS arg) {
	if ( arg->flags & SWP_HIDEWINDOW ) {
		if (m_captureProc) {
			ReleaseCapture();
			m_captureProc = nullptr;
		}
	}
	SetMsgHandled(FALSE);
}

void CListControlHeaderImpl::RenderItemBackground(CDCHandle p_dc,const CRect & p_itemRect,size_t item, uint32_t bkColor) {
	if ( ! this->DelimitColumns() ) {
		__super::RenderItemBackground(p_dc, p_itemRect, item, bkColor);
	} else {
		auto cnt = this->GetColumnCount();
		uint32_t x = 0;
		for( size_t walk = 0; walk < cnt; ) {
			auto span = this->GetSubItemSpan( item, walk );
			PFC_ASSERT( span > 0 );
			uint32_t width = 0;
			for( size_t walk2 = 0; walk2 < span; ++ walk2 ) {
				width += this->GetSubItemWidth( walk + walk2 );
			}
			CRect rc = p_itemRect;
			rc.left = x;
			x += width;
			rc.right = x;
			__super::RenderItemBackground(p_dc, rc, item, bkColor);
			walk += span;
		}
	}
}
