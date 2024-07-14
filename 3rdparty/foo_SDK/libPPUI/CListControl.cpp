#include "stdafx.h"
#include "CListControl.h"
#include "PaintUtils.h"
#include "CListControlUserOptions.h"
#include "GDIUtils.h"
#include "DarkMode.h"

#define PrepLayoutCache_Debug 0
#define Scroll_Debug 0

CListControlUserOptions * CListControlUserOptions::instance = nullptr;

CRect CListControlImpl::GetClientRectHook() const {
	CRect temp; 
	if ( m_hWnd == NULL || !GetClientRect(temp)) temp.SetRectEmpty(); 
	return temp;
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
	PFC_ASSERT(p_item < GetItemCount());
	if (this->PrepLayoutCache(m_viewOrigin, p_item, p_item+1 )) {
		RefreshSliders(); Invalidate();
	}
	EnsureVisibleRectAbs(GetItemRectAbs(p_item));
	m_ensureVisibleUser = false;
}
void CListControlImpl::EnsureHeaderVisible2(size_t atItem) {
	CRect rect;
	if (GetGroupHeaderRectAbs2(atItem,rect)) EnsureVisibleRectAbs(rect);
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

#if Scroll_Debug
	pfc::debugLog() << "RefreshSlider vertical=" << p_vertical << ", nPage=" << si.nPage << ", nMin=" << si.nMin << ", nMax=" << si.nMax << ", nPos=" << si.nPos;
#endif

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
	WIN32_OP_D( GetScrollInfo(which,&si) );
	return si.nTrackPos;
}

bool CListControlImpl::ResolveGroupRangeCached(size_t itemInGroup, size_t& outBegin, size_t& outEnd) const {
	auto end = this->m_groupHeaders.upper_bound(itemInGroup);
	if (end == this->m_groupHeaders.begin()) return false;
	auto begin = end; --begin;
	outBegin = *begin;
	if (end == this->m_groupHeaders.end()) outEnd = this->GetItemCount();
	else outEnd = *end;
	return true;
}

size_t CListControlImpl::ResolveGroupRange2(t_size p_base) const {
	const auto id = this->GetItemGroup(p_base);
	const size_t count = this->GetItemCount();
	size_t walk = p_base + 1;
	while (walk < count && GetItemGroup(walk) == id) ++walk;
	return walk - p_base;
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
	PrepLayoutCache(p_target);
	const CPoint old = m_viewOrigin;
	m_viewOrigin = p_target;

	if (m_viewOrigin != old) {
#if PrepLayoutCache_Debug
		PFC_DEBUGLOG << "MoveViewOriginNoClip: m_viewOrigin=" << m_viewOrigin.x << "," << m_viewOrigin.y;
#endif
		
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
	PrepLayoutCache(p_target);
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

LRESULT CListControlImpl::OnVWheel(UINT,WPARAM p_wp,LPARAM,BOOL&) {
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
LRESULT CListControlImpl::OnHWheel(UINT,WPARAM p_wp,LPARAM,BOOL&) {
	// const CRect client = GetClientRectHook();
	int deltaPixels = HandleWheel(m_wheelAccumX,(short)HIWORD(p_wp), true);
	MoveViewOriginDelta(CPoint(-deltaPixels,0));
	return 0;
}

// WM_VSCROLL special fix
// We must expect SCROLLINFO to go out of sync with layout, due to group partitioning happening as the user scrolls
// SetScrollInfo() is apparently disregarded while the user is scrolling, causing nonsensical behavior if we live update it as we discover new groups
// When handling input, we must take the position as % of the set scrollbar range and map it to our coordinates - even though it is mappable directly if no groups etc are in use
LRESULT CListControlImpl::OnVScroll(UINT,WPARAM p_wp,LPARAM,BOOL&) {
	SCROLLINFO si = {};
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	WIN32_OP_D(GetScrollInfo(SB_VERT, &si));
	int thumb = si.nTrackPos; // HIWORD(p_wp);
	auto bottom = GetViewAreaRectAbs().bottom;
	auto visible = GetVisibleHeight();

	if (si.nMax < si.nMin) return 0;
	double p = (double)(thumb - si.nMin) / (double)(si.nMax + 1 - si.nMin);
	thumb = pfc::rint32(p * bottom);
	int target = HandleScroll(LOWORD(p_wp), m_viewOrigin.y, visible, GetItemHeight(), bottom, thumb);

#if Scroll_Debug
	pfc::debugLog() << "OnVScroll thumb=" << thumb << ", target=" << target << ", bottom=" << bottom << ", visible=" << visible << ", p=" << p;
#endif
	MoveViewOrigin(CPoint(m_viewOrigin.x, target));

	return 0;
}

// ====== Logitech scroll bug explanation ======
// With Logitech wheel hscroll, we must use WPARAM position, not GetScrollInfo() value.
// However this is wrong, we'll get nonsense if scroll range doesn't fit in 16-bit!
// As a workaround, we use GetScrollInfo() value for vscroll (good)
// and workaround Logitech bug by using WPARAM position with hscroll (practically impossible to overflow)
LRESULT CListControlImpl::OnHScroll(UINT,WPARAM p_wp,LPARAM,BOOL&) {
	int thumb = HIWORD(p_wp); // GetScrollThumbPos(SB_HORZ);
	int target = HandleScroll(LOWORD(p_wp),m_viewOrigin.x,GetVisibleRectAbs().Width(),GetItemHeight() /*fixme*/,GetViewAreaRectAbs().right,thumb);
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

LRESULT CListControlImpl::OnSize(UINT,WPARAM,LPARAM,BOOL&) {
	this->PrepLayoutCache(m_viewOrigin);
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
		
	RenderRect(rcPaint, renderDC);
}

void CListControlImpl::OnPrintClient(HDC dc, UINT) {
	CRect rcClient; this->GetClientRect( rcClient );
	PaintContent( rcClient, dc );
}

void CListControlImpl::OnPaint(CDCHandle target) {
	auto toggle = pfc::autoToggle(m_paintInProgress, true);
	if (target) {
		CRect rcClient; this->GetClientRect(rcClient);
		PaintContent(rcClient, target);
	} else {
		CPaintDC paintDC(*this);
		PaintContent(paintDC.m_ps.rcPaint, paintDC.m_hDC);
	}
}

bool CListControlImpl::GetItemRange(const CRect & p_rect,t_size & p_base,t_size & p_count) const {
	return GetItemRangeAbs(this->RectClientToAbs(p_rect), p_base, p_count);
}



bool CListControlImpl::GetItemRangeAbsInclHeaders(const CRect & p_rect,t_size & p_base,t_size & p_count) const {
	CRect temp(p_rect);
	temp.bottom += this->GetGroupHeaderHeight();
	return GetItemRangeAbs(temp, p_base, p_count);
}

bool CListControlImpl::GetItemRangeAbs(const CRect & p_rect,t_size & p_base,t_size & p_count) const {
	const size_t count = GetItemCount();
	if (p_rect.right < 0 || p_rect.left >= GetItemWidth() || count == 0) return false;

	size_t top = IndexFromPointAbs(CPoint(0, p_rect.top));
	size_t bottom = IndexFromPointAbs(CPoint(0, p_rect.bottom));
	if (top == SIZE_MAX) return false;
	if (bottom > count-1) bottom = count - 1;
	p_base = top;
	p_count = bottom - top + 1;
	PFC_ASSERT(p_base + p_count <= count);
	return true;
}

void CListControlImpl::RenderRect(const CRect & p_rect,CDCHandle p_dc) {
	t_size base, count;
	if (GetItemRange(p_rect,base,count)) {
		for(t_size walk = 0; walk < count; ++walk) {
			size_t atItem = base + walk;
			if (m_groupHeaders.count(atItem) > 0) {
				CRect rcHeader, rcUpdate;
				if (GetGroupHeaderRectAbs2(atItem, rcHeader) ) {
					rcHeader = RectAbsToClient(rcHeader);
					if (rcUpdate.IntersectRect(rcHeader, p_rect)) {
						DCStateScope dcState(p_dc);
						if (p_dc.IntersectClipRect(rcUpdate) != NULLREGION) {
							try {
								RenderGroupHeader2(atItem, rcHeader, rcUpdate, p_dc);
							} catch (...) {
								PFC_ASSERT(!"Should not get here");
							}
						}
					}
				}
			}

			CRect rcUpdate, rcItem = GetItemRect(atItem);
			if (rcUpdate.IntersectRect(rcItem,p_rect)) {
				DCStateScope dcState(p_dc);
				if (p_dc.IntersectClipRect(rcUpdate) != NULLREGION) {
					try {
						RenderItem(atItem,rcItem,rcUpdate,p_dc);
					} catch(...) {
						PFC_ASSERT(!"Should not get here");
					}
				}
			}
		}

		if ( this->m_groupHeaders.size() > 0 ) {
			auto iter = m_groupHeaders.upper_bound(base);
			if (iter != m_groupHeaders.begin()) {
				--iter;
				while ( iter != m_groupHeaders.end() && *iter < base + count) {
					auto iter2 = iter; ++iter2;
					
					size_t begin = *iter;
					size_t end;
					if (iter2 == m_groupHeaders.end()) end = this->GetItemCount();
					else end = *iter2;

					CRect rc;
					rc.top = this->GetItemOffsetAbs(begin);
					rc.bottom = this->GetItemBottomOffsetAbs(end-1);
					rc.left = 0;
					rc.right = this->GetItemWidth();
					rc = this->RectAbsToClient(rc);
					CRect rcUpdate;
					if (rcUpdate.IntersectRect(rc, p_rect)) {
						DCStateScope dcState(p_dc);
						if (p_dc.IntersectClipRect(rcUpdate) != NULLREGION) {
							try {
								this->RenderGroupOverlay(begin, rc, rcUpdate, p_dc);
							} catch (...) {
								PFC_ASSERT(!"Should not get here");
							}
						}
					}


					iter = iter2;
				}
			}
		}
	}

	RenderOverlay2(p_rect,p_dc);
}

bool CListControlImpl::GetGroupOverlayRectAbs(size_t atItem, CRect& outRect) {
	auto iter = m_groupHeaders.upper_bound(atItem);
	if (iter == m_groupHeaders.begin()) return false;
	auto iter2 = iter; --iter;

	size_t begin = *iter;
	size_t end;
	if (iter2 == m_groupHeaders.end()) end = this->GetItemCount();
	else end = *iter2;

	CRect rc;

	rc.top = this->GetItemOffsetAbs(begin);
	rc.bottom = this->GetItemBottomOffsetAbs(end - 1);
	rc.left = 0;
	rc.right = this->GetItemWidth();

	outRect = rc;
	return true;
}

void CListControlImpl::MinGroupHeight2ChangedForGroup(groupID_t groupID, bool reloadWhole) {
	for (auto iter = m_groupHeaders.begin(); iter != m_groupHeaders.end(); ++iter) {
		if (groupID == GetItemGroup(*iter)) {
			this->MinGroupHeight2Changed(*iter, reloadWhole);
		}
	}
}

void CListControlImpl::UpdateGroupOverlayByID(groupID_t groupID, int xFrom, int xTo) {
	t_size base, count;
	if (GetItemRangeAbs(GetVisibleRectAbs(), base, count)) {
		bool on = false; // Have to walk whole range - there may be multiple groups with the same ID
		for (size_t walk = 0; walk < count; ++walk) {
			bool test = (groupID == GetItemGroup(base + walk));
			if (test && !on) {
				CRect rc;
				if (GetGroupOverlayRectAbs(base + walk, rc)) {
					if (xFrom < xTo) {
						rc.left = xFrom; rc.right = xTo;						
					}
					this->InvalidateRect(this->RectAbsToClient(rc));
				}
			}

			on = test;
		}
	}
}

CRect CListControlImpl::GetItemRect(t_size p_item) const {
	return this->RectAbsToClient(GetItemRectAbs(p_item));
}

bool CListControlImpl::GetGroupHeaderRect2(size_t atItem,CRect & p_rect) const {
	CRect temp;
	if (!GetGroupHeaderRectAbs2(atItem,temp)) return false;
	p_rect = RectAbsToClient(temp);
	return true;
}

size_t CListControlImpl::FindGroupBaseCached(size_t itemFor) const {
	auto iter = m_groupHeaders.upper_bound(itemFor);
	if (iter == m_groupHeaders.begin()) return 0;
	--iter;
	return *iter;
}

size_t CListControlImpl::FindGroupBase(size_t itemFor) const {
	return this->FindGroupBase(itemFor, this->GetItemGroup(itemFor));
}

size_t CListControlImpl::FindGroupBase(size_t itemFor, groupID_t id) const {
	size_t walk = itemFor;
	while (walk > 0) {
		size_t prev = walk - 1;
		if (this->GetItemGroup(prev) != id) break;
		walk = prev;
	}
	return walk;
}

bool CListControlImpl::PrepLayoutCache(CPoint& ptOrigin, size_t indexLo, size_t indexHi) {
	const size_t count = GetItemCount();
	if (count == 0) return false;
#if PrepLayoutCache_Debug
	PFC_DEBUGLOG << "PrepLayoutCache entry";
	PFC_DEBUGLOG << "PrepLayoutCache: count=" << count << " knownGroups=" << this->m_groupHeaders.size();
	PFC_DEBUGLOG << "PrepLayoutCache: indexLo=" << pfc::format_index(indexLo) << " indexHi=" << pfc::format_index(indexHi);
#endif
	const int clientHeight = pfc::max_t<int>(this->GetClientRectHook().Height(), 100);

	// Always walk 2*clientHeight, with area above and below
	int yMax = -1, yBase = 0;
	size_t baseItem = 0, endItem = SIZE_MAX;

	if (!m_greedyGroupLayout) {
		if (indexLo == SIZE_MAX) {
			yBase = pfc::max_t<int>(ptOrigin.y - clientHeight / 2, 0);
			yMax = yBase + clientHeight * 2;
			baseItem = pfc::min_t<size_t>(this->IndexFromPointAbs(yBase), count - 1);
		} else {
			auto itemHeight = GetItemHeight();
			size_t extraItems = (size_t)(clientHeight / itemHeight);
#if PrepLayoutCache_Debug
			PFC_DEBUGLOG << "PrepLayoutCache: clientHeight=" << clientHeight << " itemHeight=" << itemHeight << " extraItems=" << extraItems;
#endif
			if (indexLo < extraItems) baseItem = 0;
			else baseItem = indexLo - extraItems;

			if (indexHi == SIZE_MAX) {
				endItem = baseItem + extraItems;
			} else {
				endItem = indexHi + extraItems;
			}
			if (endItem > count) endItem = count;

#if PrepLayoutCache_Debug
			PFC_DEBUGLOG << "PrepLayoutCache: baseItem=" << baseItem << " endItem=" << endItem;
#endif
		}
	}




	size_t item = baseItem;
	{
		const auto group = this->GetItemGroup(baseItem);
		if (group != 0) {
			size_t hdr = this->FindGroupBase(baseItem, group);
			if (hdr < baseItem) {
				item = hdr;
			}
		}
	}

#if PrepLayoutCache_Debug
	if (yMax != -1) {
		PFC_DEBUGLOG << "PrepLayoutCache: yBase=" << yBase << " yMax=" << yMax;
	}
	if (indexLo != SIZE_MAX) {
		pfc::string_formatter msg;
		msg << "PrepLayoutCache: indexLo=" << indexLo;
		if (indexHi != SIZE_MAX) {
			msg << " indexHi=" << indexHi;
		}
		pfc::outputDebugLine(msg);
	}
	PFC_DEBUGLOG << "PrepLayoutCache: baseItem=" << baseItem;
#endif
	
	size_t anchorIdx = m_greedyGroupLayout ? SIZE_MAX : this->IndexFromPointAbs(ptOrigin.y);
	int anchorDelta = 0;
	bool anchorIsFirstInGroup = IsItemFirstInGroupCached(anchorIdx);
	if (anchorIdx != SIZE_MAX) {
		anchorDelta = ptOrigin.y - GetItemOffsetAbs(anchorIdx);
	}

#if PrepLayoutCache_Debug
	PFC_DEBUGLOG << "PrepLayoutCache: anchorIdx=" << pfc::format_index(anchorIdx) << " anchorDelta=" << anchorDelta << " anchorIsFirstInGroup=" << anchorIsFirstInGroup;
#endif

	bool bChanged = false;
	int gh = -1;
	int ih = -1;
	int yWalk = yBase;
	groupID_t prevGroup = 0;
	if (item > 1) prevGroup = this->GetItemGroup(item - 1);
	for (; item < count; ++item) {
		int yDelta = 0;
		auto group = this->GetItemGroup(item);
		if (group != prevGroup) {
			if (m_groupHeaders.insert(item).second) bChanged = true;
			if (gh < 0) gh = GetGroupHeaderHeight();
			yDelta += gh;
		} else {
			if (m_groupHeaders.erase(item) > 0) bChanged = true;
		}
		prevGroup = group;

		auto iter = m_varItemHeights.find(item);
		int varHeight = this->GetItemHeight2(item);
		if (varHeight < 0) {
			if (iter != m_varItemHeights.end()) {
				m_varItemHeights.erase(iter);
				bChanged = true;
			}
			if (ih < 0) ih = this->GetItemHeight();
			yDelta += ih;
		} else {
			if (iter == m_varItemHeights.end()) {
				m_varItemHeights[item] = varHeight;
				bChanged = true;
			} else if ( iter->second != varHeight ) {
				iter->second = varHeight;
				bChanged = true;
			}
		
			yDelta += varHeight;
		}

		if (item >= endItem) {
			break;
		}
		if (item >= baseItem && yMax != -1) {
			yWalk += yDelta;
			if (yWalk > yMax) break;
		}
	}

#if PrepLayoutCache_Debug
	PFC_DEBUGLOG << "PrepLayoutCache: bChanged=" << bChanged << " knownGroups=" << m_groupHeaders.size() << " knownVarHeights=" << m_varItemHeights.size();
#endif

	if (bChanged) {
		if (anchorIdx != SIZE_MAX) {
			int fix = GetItemOffsetAbs(anchorIdx) + anchorDelta;

			// View would begin exactly with an item that became a first item in a group?
			if (anchorDelta == 0 && !anchorIsFirstInGroup && IsItemFirstInGroupCached(anchorIdx)) {
				if (gh < 0) gh = GetGroupHeaderHeight();
				fix -= gh;
			}

#if PrepLayoutCache_Debug
			PFC_DEBUGLOG << "PrepLayoutCache: fixing origin: " << ptOrigin.y << " to " << fix;
#endif

			ptOrigin.y = fix;
			if (&ptOrigin != &m_viewOrigin && m_hWnd != NULL) {
#if PrepLayoutCache_Debug
				PFC_DEBUGLOG << "PrepLayoutCache: invalidating view";
#endif
				Invalidate();
			}
		}
	}

	if ( bChanged ) {
		// DO NOT update sliders from here, causes mayhem, SetScrollInfo() in mid-scroll is not really handled
		// this->RefreshSliders();
	}
	return bChanged;
}

int CListControlImpl::GetViewAreaHeight() const {
	auto ret = GetItemOffsetAbs(GetItemCount());
#if Scroll_Debug
	PFC_DEBUGLOG << "GetViewAreaHeight: " << ret;
#endif
	return ret;
}

int CListControlImpl::GetItemBottomOffsetAbs(size_t item) const {
	return GetItemOffsetAbs(item) + GetItemHeightCached(item);
}

int CListControlImpl::GetItemOffsetAbs2(size_t base, size_t item) const {
	// Also valid with item == GetItemCount()
	size_t varcount = 0;
	int acc = 0;
	const bool baseValid = (base != SIZE_MAX);
	const size_t itemDelta = baseValid ? item - base : item;
	for (auto iter = (baseValid ? m_varItemHeights.lower_bound(base) : m_varItemHeights.begin()); iter != m_varItemHeights.end(); ++iter){
		if (iter->first >= item) break;
		if (iter->second > 0) acc += iter->second;
		++varcount;
	}
	if (varcount < itemDelta) {
		acc += GetItemHeight() * (int)(itemDelta - varcount);
	}

	int gh = -1;
	for (auto iter = (baseValid ? m_groupHeaders.upper_bound(base) : m_groupHeaders.begin()); iter != m_groupHeaders.end(); ++iter){
		if (*iter > item) break;
		if (gh < 0) gh = GetGroupHeaderHeight();
		acc += gh;
	}

	return acc;
}

int CListControlImpl::GetItemOffsetAbs(size_t item) const {
	// Also valid with item == GetItemCount()
	return GetItemOffsetAbs2(SIZE_MAX, item);
}

int CListControlImpl::GetItemContentHeightCached(size_t item) const {
	auto iter = m_varItemHeights.find(item);
	if (iter == m_varItemHeights.end()) return GetItemHeight();
	else return this->GetItemHeight2Content( item, iter->second );
}

int CListControlImpl::GetItemHeightCached(size_t item) const {
	auto iter = m_varItemHeights.find(item);
	if (iter == m_varItemHeights.end()) return GetItemHeight();
	else return iter->second;
}

CRect CListControlImpl::GetItemRectAbs(t_size p_item) const {
	PFC_ASSERT(p_item < GetItemCount());
	// const int normalHeight = GetItemHeight();
	CRect rcItem;
	rcItem.top = GetItemOffsetAbs(p_item);
	rcItem.bottom = rcItem.top + GetItemContentHeightCached(p_item);
	rcItem.left = 0;
	rcItem.right = rcItem.left + GetItemWidth();
	return rcItem;
}

bool CListControlImpl::GetGroupHeaderRectAbs2(size_t atItem,CRect & p_rect) const {

	if (m_groupHeaders.count(atItem) == 0) return false;

	p_rect.bottom = GetItemOffsetAbs(atItem);
	p_rect.top = p_rect.bottom - GetGroupHeaderHeight();
	p_rect.left = 0;
	p_rect.right = GetItemWidth();
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

void CListControlImpl::UpdateGroupHeader2(size_t atItem) {
	CRect rect;
	if (GetGroupHeaderRect2(atItem,rect)) {
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

std::pair<size_t, size_t> CListControlImpl::GetVisibleRange() const {
	const size_t total = GetItemCount();
	CRect rcVisible = this->GetVisibleRectAbs();
	size_t lo = this->IndexFromPointAbs(rcVisible.top);
	PFC_ASSERT(lo != SIZE_MAX);
	if (lo == SIZE_MAX) lo = 0; // should not happen
	size_t hi = this->IndexFromPointAbs(rcVisible.bottom);
	if (hi < total) ++hi;
	else hi = total;
	return { lo, hi };
}

bool CListControlImpl::IsItemVisible(size_t which) const {
	CRect rcVisible = this->GetVisibleRectAbs();
	CRect rcItem = this->GetItemRectAbs(which);
	return rcItem.top >= rcVisible.top && rcItem.bottom <= rcVisible.bottom;
}

void CListControlImpl::UpdateItemsAndHeaders(const pfc::bit_array & p_mask) {
	t_size base,count;
	groupID_t groupWalk = 0;
	if (GetItemRangeAbsInclHeaders(GetVisibleRectAbs(),base,count)) {
		const t_size max = base+count;
		CRgn updateRgn; updateRgn.CreateRectRgn(0,0,0,0);
		bool found = false;
		for(t_size walk = p_mask.find_first(true,base,max); walk < max; walk = p_mask.find_next(true,walk,max)) {
			found = true;
			const groupID_t groupId = GetItemGroup(walk);
			if (groupId != groupWalk) {
				CRect rect;
				if (GetGroupHeaderRect2(walk,rect)) {
					AddUpdateRect(updateRgn,rect);
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

	PrepLayoutCache(p_originOverride);

	m_viewOrigin = ClipPointToRect(p_originOverride,GetValidViewOriginArea());

	if (m_viewOrigin != p_originOverride) {
		// Did clip from the requested?
		PrepLayoutCache(m_viewOrigin);
	}
#if PrepLayoutCache_Debug
	PFC_DEBUGLOG << "OnViewAreaChanged: m_viewOrigin=" << m_viewOrigin.x << "," << m_viewOrigin.y;
#endif

	RefreshSliders();

	Invalidate();
	
	if (oldViewOrigin != m_viewOrigin) {
		OnViewOriginChange(m_viewOrigin - oldViewOrigin);
	}
}

size_t CListControlImpl::IndexFromPointAbs(CPoint pt) const {
	if (pt.x < 0 || pt.x >= GetItemWidth()) return SIZE_MAX;
	return IndexFromPointAbs(pt.y);
}

size_t CListControlImpl::IndexFromPointAbs(int ptY) const {
	const size_t count = GetItemCount();
	if (count == 0) return SIZE_MAX;
	
	class wrapper {
	public:
		wrapper(const CListControlImpl & o) : owner(o) {}
		int operator[] (size_t idx) const {
			// Return LAST line of this item
			return owner.GetItemBottomOffsetAbs(idx)-1;
		}
		const CListControlImpl & owner;
	};

	wrapper w(*this);
	size_t result = SIZE_MAX;
	pfc::binarySearch<>::run(w, 0, count, ptY, result);
	PFC_ASSERT(result != SIZE_MAX);
	return result;
}

bool CListControlImpl::ItemFromPointAbs(CPoint const & p_pt,t_size & p_item) const {
	size_t idx = IndexFromPointAbs(p_pt);
	if (idx >= GetItemCount()) return false;
	CRect rc = this->GetItemRectAbs(idx);
	if (!rc.PtInRect(p_pt)) return false;
	p_item = idx;
	return true;
}

size_t CListControlImpl::ItemFromPointAbs(CPoint const& p_pt) const {
	size_t ret = SIZE_MAX;
	ItemFromPointAbs(p_pt, ret);
	return ret;
}

bool CListControlImpl::GroupHeaderFromPointAbs2(CPoint const & p_pt,size_t & atItem) const {
	size_t idx = IndexFromPointAbs(p_pt);
	if (idx == SIZE_MAX) return false;
	CRect rc;
	if (!this->GetGroupHeaderRectAbs2(idx, rc)) return false;
	if (!rc.PtInRect(p_pt)) return false;
	atItem = idx;
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

void CListControlImpl::SetDarkMode(bool v) {
	if (m_darkMode != v) {
		m_darkMode = v;
		RefreshDarkMode();
	}
}

void CListControlImpl::RefreshDarkMode() {
	if (m_hWnd != NULL) {
		Invalidate();

		// GOD DAMNIT: Should use ItemsView, but only Explorer fixes scrollbars
		DarkMode::ApplyDarkThemeCtrl(m_hWnd, m_darkMode, L"Explorer");
	}
}

LRESULT CListControlImpl::OnCreatePassThru(UINT,WPARAM,LPARAM,BOOL& bHandled) {

	RefreshDarkMode();
	
	
	OnViewAreaChanged();

	if (m_gestureAPI.IsAvailable()) {
		GESTURECONFIG config = {GID_PAN, GC_PAN_WITH_SINGLE_FINGER_VERTICALLY|GC_PAN_WITH_INERTIA, GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY | GC_PAN_WITH_GUTTER};
		m_gestureAPI.SetGestureConfig( *this, 0, 1, &config, sizeof(GESTURECONFIG));
	}

	bHandled = FALSE;
	return 0;
}
bool CListControlImpl::IsSameItemOrHeaderAbs(const CPoint & p_point1, const CPoint & p_point2) const {
	t_size item1, item2;
	if (ItemFromPointAbs(p_point1, item1)) {
		if (ItemFromPointAbs(p_point2,item2)) {
			return item1 == item2;
		} else {
			return false;
		}
	}
	if (GroupHeaderFromPointAbs2(p_point1, item1)) {
		if (GroupHeaderFromPointAbs2(p_point2, item2)) {
			return item1 == item2;
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

void CListControlImpl::AddGroupHeaderToUpdateRgn2(HRGN p_rgn, size_t atItem) const {
	CRect rcHeader;
	if (GetGroupHeaderRect2(atItem,rcHeader)) AddUpdateRect(p_rgn,rcHeader);
}
void CListControlImpl::AddItemToUpdateRgn(HRGN p_rgn, t_size p_index) const {
	if (p_index < this->GetItemCount()) {
		AddUpdateRect(p_rgn,GetItemRect(p_index));
	}
}

COLORREF CListControlImpl::GetSysColorHook(int colorIndex) const {
	if (m_darkMode) {
		return DarkMode::GetSysColor(colorIndex);
	} else {
		return GetSysColor(colorIndex);
	}
}

BOOL CListControlImpl::OnEraseBkgnd(CDCHandle dc) {
	
	if (paintInProgress()) return FALSE;

	CRect rcClient; WIN32_OP_D(GetClientRect(rcClient)); // SPECIAL CASE: No GetClientRectHook() here, fill physical client area, not logical
	PaintUtils::FillRectSimple(dc,rcClient,this->GetSysColorHook(COLOR_WINDOW));

	return TRUE;
}

t_size CListControlImpl::InsertIndexFromPointEx(const CPoint & pt, bool & bInside) const {
	bInside = false;
	int y_abs = pt.y + GetViewOffset().y;
	
	if (y_abs >= GetViewAreaHeight()) {
		return GetItemCount();
	}
	size_t itemIdx = IndexFromPointAbs(y_abs);
	if (itemIdx == SIZE_MAX) return SIZE_MAX;

	{
		CRect rc;
		if (!this->GetGroupHeaderRectAbs2(itemIdx, rc)) {
			if (y_abs >= rc.top && y_abs <= rc.bottom) {
				bInside = false;
				return itemIdx;
			}
		}
	}
	if (itemIdx != SIZE_MAX) {
		const CRect rc = GetItemRectAbs(itemIdx);
		if (y_abs > rc.top + MulDiv(rc.Height(), 2, 3)) itemIdx++;
		else if (y_abs >= rc.top + MulDiv(rc.Height(), 1, 3)) bInside = true;
		return itemIdx;
	}
	return SIZE_MAX;
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
		PaintUtils::RenderItemBackground(p_dc,p_itemRect,p_item,bkColor);
		{
			auto blend = BlendGridColor(bkColor);
			CDCPen pen(p_dc, blend);
			SelectObjectScope scope(p_dc, pen);

			p_dc.MoveTo( p_itemRect.right-1, p_itemRect.top );
			p_dc.LineTo( p_itemRect.right-1, p_itemRect.bottom );
		}
		break;
	case rowStylePlaylist:
		PaintUtils::RenderItemBackground(p_dc,p_itemRect,p_item,bkColor);
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
	(void)p_group;
	const t_uint32 bkColor = GetSysColorHook(colorBackground);
	size_t pretendIndex = 0;
	switch( this->m_rowStyle ) {
	default:
		PaintUtils::FillRectSimple( p_dc, p_headerRect, bkColor );
		break;
	case rowStylePlaylistDelimited:
	case rowStylePlaylist:
		PaintUtils::RenderItemBackground(p_dc,p_headerRect,pretendIndex,bkColor);
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

void CListControlImpl::RenderGroupHeader2(size_t baseItem,const CRect & p_headerRect,const CRect & p_updateRect,CDCHandle p_dc) {
	this->RenderGroupHeaderBackground(p_dc, p_headerRect, 0 );

	DCStateScope backup(p_dc);
	p_dc.SetBkMode(TRANSPARENT);
	p_dc.SetBkColor(GetSysColorHook(colorBackground));
	p_dc.SetTextColor(GetSysColorHook(colorHighlight));

	RenderGroupHeaderText2(baseItem,p_headerRect,p_updateRect,p_dc);
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

LRESULT CListControlImpl::OnGetDlgCode(UINT, WPARAM wp, LPARAM) {
	switch(wp) {
	case VK_RETURN:
		return m_dlgWantEnter ? DLGC_WANTMESSAGE : 0;
	default:
		SetMsgHandled(FALSE);
		return 0;
	}
}

HWND CListControlImpl::CreateInDialog(CWindow wndDialog, UINT replaceControlID, CWindow lstReplace) {
	PFC_ASSERT(lstReplace != NULL);
	auto status = lstReplace.SendMessage(WM_GETDLGCODE, VK_RETURN);
	m_dlgWantEnter = (status & DLGC_WANTMESSAGE);
	CRect rc;
	CWindow wndPrev = wndDialog.GetNextDlgTabItem(lstReplace, TRUE);
	WIN32_OP_D(lstReplace.GetWindowRect(&rc));
	WIN32_OP_D(wndDialog.ScreenToClient(rc));
	WIN32_OP_D(lstReplace.DestroyWindow());
	WIN32_OP_D(this->Create(wndDialog, &rc, 0, 0, WS_EX_STATICEDGE, replaceControlID));
	if (wndPrev != NULL) this->SetWindowPos(wndPrev, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	// this->BringWindowToTop();
	this->SetFont(wndDialog.GetFont());
	return m_hWnd;
}

HWND CListControlImpl::CreateInDialog(CWindow wndDialog, UINT replaceControlID ) {
	return this->CreateInDialog(wndDialog, replaceControlID, wndDialog.GetDlgItem(replaceControlID));
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


CPoint CListControlImpl::PointAbsToClient(CPoint pt) const {
	return pt - GetViewOffset();
}

CPoint CListControlImpl::PointClientToAbs(CPoint pt) const {
	return pt + GetViewOffset();
}

CRect CListControlImpl::RectAbsToClient(CRect rc) const {
	CRect ret;
#if 1
	ret = rc;
	ret.OffsetRect(-GetViewOffset());
#else
	ret.TopLeft() = PointAbsToClient(rc.TopLeft());
	ret.BottomRight() = PointAbsToClient(rc.BottomRight());
#endif
	return ret;
}

CRect CListControlImpl::RectClientToAbs(CRect rc) const {
	CRect ret;
#if 1
	ret = rc;
	ret.OffsetRect(GetViewOffset());
#else
	ret.TopLeft() = PointClientToAbs(rc.TopLeft());
	ret.BottomRight() = PointAbsToClient(rc.BottomRight());
#endif
	return ret;
}

size_t CListControlImpl::ItemFromPoint(CPoint const& pt) const {
	size_t ret = SIZE_MAX;
	if (!ItemFromPoint(pt, ret)) ret = SIZE_MAX;
	return ret;
}

bool CListControlImpl::ItemFromPoint(CPoint const & p_pt, t_size & p_item) const {
	return ItemFromPointAbs( PointClientToAbs( p_pt ), p_item);
}

void CListControlImpl::ReloadData() {
	this->m_varItemHeights.clear();
	this->m_groupHeaders.clear();
	OnViewAreaChanged(); 
}

void CListControlImpl::ReloadItems(pfc::bit_array const & mask) { 
	bool bReLayout = false;
	mask.for_each(true, 0, GetItemCount(), [this, &bReLayout] (size_t idx) {
		int hNew = this->GetItemHeight2(idx);
		int hOld = -1;
		{
			auto iter = m_varItemHeights.find(idx);
			if (iter != m_varItemHeights.end()) hOld = iter->second;
		}
		if (hNew != hOld) {
			m_varItemHeights[idx] = hNew;
			bReLayout = true;
		}
	});
	if (bReLayout) {
		OnViewAreaChanged();
	} else {
		UpdateItems(mask);
	}
	
}

void CListControlImpl::MinGroupHeight2Changed(size_t itemInGroup, bool reloadWhole) {
	size_t lo, hi;
	if (ResolveGroupRangeCached(itemInGroup, lo, hi)) {
		if (reloadWhole) {
			CRect rc;
			if (this->GetGroupOverlayRectAbs(itemInGroup, rc)) {
				this->InvalidateRect(this->RectAbsToClient(rc));
			}
			{
				pfc::bit_array_range range(lo, hi-lo);
				this->ReloadItems(range);
			}
		} else {
			this->ReloadItem(hi - 1);
		}
	}
}

bool CListControlImpl::IsItemFirstInGroupCached(size_t item) const {
	return m_groupHeaders.count(item) > 0;
}

bool CListControlImpl::IsItemFirstInGroup(size_t item) const {
	if (item == 0) return true;
	return GetItemGroup(item) != GetItemGroup(item - 1);
}
bool CListControlImpl::IsItemLastInGroup(size_t item) const {
	size_t next = item + 1;
	if (next >= GetItemCount()) return true;
	return GetItemGroup(item) != GetItemGroup(next);
}

int CListControlImpl::GetItemHeight2(size_t which) const {
	if (!IsItemLastInGroup(which)) return -1;

	const int minGroupHeight = this->GetMinGroupHeight2(which);
	if (minGroupHeight <= 0) return -1;

	const int heightNormal = this->GetItemHeight();

	const size_t base = FindGroupBase(which);
	
	const int groupHeightWithout = (which > base ? this->GetItemOffsetAbs2(base,which) : 0);
	
	const int minItemHeight = minGroupHeight - groupHeightWithout; // possibly negative

	if (minItemHeight > heightNormal) return minItemHeight;
	else return -1; // use normal	
}

void CListControlImpl::wndSetDarkMode(CWindow wndListControl, bool bDark) { 
	wndListControl.SendMessage(DarkMode::msgSetDarkMode(), bDark ? 1 : 0);
}

LRESULT CListControlImpl::OnSetDark(UINT, WPARAM wp, LPARAM) {
	switch (wp) {
	case 0:
		this->SetDarkMode(false);
		break;
	case 1:
		this->SetDarkMode(true);
		break;
	}
	return 1;
}

void CListControlImpl::OnItemRemoved(size_t which) {
	this->OnItemsRemoved(pfc::bit_array_one(which), GetItemCount() + 1);
}


UINT CListControlImpl::msgSetDarkMode() {
	return DarkMode::msgSetDarkMode();
}

void CListControlImpl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
	(void)nRepCnt; (void)nFlags;
	switch (nChar) {
	case VK_LEFT:
		MoveViewOriginDelta(CPoint(-MulDiv(16, m_dpi.cx, 96), 0));
		break;
	case VK_RIGHT:
		MoveViewOriginDelta(CPoint(MulDiv(16, m_dpi.cx, 96), 0));
		break;
	default:
		SetMsgHandled(FALSE); break;
	}
}

void CListControlImpl::OnItemsInserted(size_t at, size_t count, bool bSelect) {
	size_t newCount = this->GetItemCount();
	this->OnItemsInsertedEx(pfc::bit_array_range(at, count, true), newCount - count, newCount, bSelect);
}