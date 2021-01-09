#include "stdafx.h"
#include "CListControl.h"
#include "PaintUtils.h"

LRESULT CListControlTruncationTooltipImpl::OnTTShow(int,LPNMHDR,BOOL&) {
	SetTimer(KTooltipTimer,KTooltipTimerDelay);
	return 0;
}
LRESULT CListControlTruncationTooltipImpl::OnTTPop(int,LPNMHDR,BOOL&) {
	KillTimer(KTooltipTimer);
	return 0;
}
LRESULT CListControlTruncationTooltipImpl::OnTTGetDispInfo(int,LPNMHDR p_hdr,BOOL&) {
	LPNMTTDISPINFO info = (LPNMTTDISPINFO)p_hdr;

	info->lpszText = const_cast<TCHAR*>(this->m_tooltipText.get_ptr());
	info->hinst = 0;
	info->uFlags = 0;
		
	return 0;
}

LRESULT CListControlTruncationTooltipImpl::OnDestroyPassThru(UINT,WPARAM,LPARAM,BOOL& bHandled) {
	if (m_tooltip.m_hWnd != NULL) m_tooltip.DestroyWindow();
	KillTimer(KTooltipTimer);
	bHandled = FALSE; return 0;
}

CListControlTruncationTooltipImpl::CListControlTruncationTooltipImpl() 
	: m_toolinfo()
	, m_tooltipRect(0,0,0,0)
{
}



void CListControlTruncationTooltipImpl::TooltipRemove() {
	m_tooltipRect = CRect(0,0,0,0);
	if (m_tooltip.m_hWnd != NULL) {
		m_tooltip.TrackActivate(&m_toolinfo,FALSE);
	}
}

void CListControlTruncationTooltipImpl::TooltipRemoveCheck() {
	CPoint pt = GetCursorPos();
	if (ScreenToClient(&pt)) {
		TooltipRemoveCheck( MAKELPARAM( pt.x, pt.y ) );
	}
}
void CListControlTruncationTooltipImpl::TooltipRemoveCheck(LPARAM pos) {
	if (!m_tooltipRect.IsRectEmpty()) {
		CPoint pt(pos);
		if (!GetClientRectHook().PtInRect(pt)) {
			TooltipRemove();
		} else {
			ClientToScreen(&pt);
			if (!m_tooltipRect.PtInRect(pt)) {
				TooltipRemove();
			}
		}
	}
}

LRESULT CListControlTruncationTooltipImpl::OnTimer(UINT,WPARAM wp,LPARAM,BOOL& bHandled) {
	switch(wp) {
		case KTooltipTimer:
			TooltipRemoveCheck();
			return 0;
		default:
			bHandled = FALSE;
			return 0;
	}
}

LRESULT CListControlTruncationTooltipImpl::OnMouseMovePassThru(UINT,WPARAM,LPARAM lp,BOOL& bHandled) {
	TooltipRemoveCheck(lp);
	{
		TRACKMOUSEEVENT ev = {sizeof(ev)};
		ev.dwFlags = TME_HOVER;
		ev.hwndTrack = *this;
		ev.dwHoverTime = HOVER_DEFAULT;
		TrackMouseEvent(&ev);
	}
	bHandled = FALSE;
	return 0;
}


bool CListControlTruncationTooltipImpl::IsRectPartiallyObscuredAbs(CRect const & r) const {
	CRect cl = this->GetClientRectHook(); cl.OffsetRect( this->GetViewOffset() );
	return r.right > cl.right || r.top < cl.top || r.bottom > cl.bottom;
}

bool CListControlTruncationTooltipImpl::IsRectFullyVisibleAbs(CRect const & r) {
	CRect cl = this->GetClientRectHook(); cl.OffsetRect( this->GetViewOffset() );
	return r.left >= cl.left && r.right <= cl.right && r.top >= cl.top && r.bottom <= cl.bottom;
}

bool CListControlTruncationTooltipImpl::GetTooltipData(CPoint pt, pfc::string_base & outText, CRect & outRC, CFontHandle & outFont) const {
	t_size item; int group;
	if (ItemFromPointAbs(pt, item)) {
		const CRect itemRectAbs = this->GetItemRectAbs(item);
		/*if (this->IsHeaderEnabled()) */{
			t_uint32 cbase = 0;
			auto orderArray = this->GetColumnOrderArray();
			for (t_size _cwalk = 0; _cwalk < orderArray.size(); ++_cwalk) {
				const t_size cwalk = orderArray[_cwalk];
				//const TColumnRuntime & col = m_columns[cwalk];

				const t_uint32 width = GetSubItemWidth(cwalk);
				if ((t_uint32)pt.x < cbase + width) {
					t_uint32 estWidth = GetOptimalSubItemWidthSimple(item, cwalk);
					CRect rc = itemRectAbs; rc.left = cbase; rc.right = cbase + estWidth;
					if (estWidth > width || (IsRectPartiallyObscuredAbs(rc) && rc.PtInRect(pt))) {
						pfc::string_formatter label, ccTemp;
						if (GetSubItemText(item, cwalk, label)) {
							PaintUtils::TextOutColors_StripCodes(ccTemp, label);
							outFont = GetFont(); outRC = rc; outText = ccTemp;
							return true;
						}
					}
					break;
				}
				cbase += width;
			}
		}
	} else if (GroupHeaderFromPointAbs(pt, group)) {
		CRect rc;
		if (GetGroupHeaderRectAbs(group, rc) && rc.PtInRect(pt)) {
			const t_uint32 estWidth = GetOptimalGroupHeaderWidth(group);
			CRect rcText = rc; rcText.right = rcText.left + estWidth;
			if (estWidth > (t_uint32)rc.Width() || (IsRectPartiallyObscuredAbs(rcText) && rcText.PtInRect(pt))) {
				pfc::string_formatter label;
				if (GetGroupHeaderText(group, label)) {
					outFont = GetGroupHeaderFont(); outRC = rc; outText = label; 
					return true;
				}
			}
		}
	}
	return false;
}
LRESULT CListControlTruncationTooltipImpl::OnHover(UINT,WPARAM wp,LPARAM lp,BOOL&) {
	if (!m_tooltipRect.IsRectEmpty()) {
		return 0;
	}
	if (wp & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)) return 0;
	const CPoint viewOffset = GetViewOffset();
	CPoint pt ( lp ); pt += viewOffset;

	CFontHandle font;
	CRect rc;
	pfc::string8 text;
	if ( this->GetTooltipData(pt, text, rc, font) ) {
		this->m_tooltipFont = font;
		// Gets stuck if the text is very long!
		if (text.length() < 4096) {
			TooltipActivateAbs(text, rc);
		}
	}
	return 0;
}

void CListControlTruncationTooltipImpl::TooltipActivateAbs(const char * label, const CRect & rect)  {
	CRect temp(rect);
	temp.OffsetRect( - GetViewOffset() );
	ClientToScreen(temp);
	TooltipActivate(label,temp);
}
void CListControlTruncationTooltipImpl::TooltipActivate(const char * label, const CRect & rect) {
	if (rect.IsRectEmpty()) return;
	if (m_tooltip.m_hWnd == NULL) {
		try {
			InitTooltip();
		} catch(std::exception const & e) {
			(void) e;
			// console::complain("Tooltip initialization failure", e);
			return;
		}
	}
	
	m_tooltipText.convert( EscapeTooltipText( label ) );

	m_tooltipRect = rect;

	TooltipUpdateFont();
	m_tooltip.TrackPosition(rect.left,rect.top);
	m_tooltip.TrackActivate(&m_toolinfo,TRUE);
}

void CListControlTruncationTooltipImpl::TooltipUpdateFont() {
	if (m_tooltip.m_hWnd != NULL) {
		if (m_tooltipFont) {
			m_tooltip.SetFont(m_tooltipFont);
		}
	}
}

void CListControlTruncationTooltipImpl::InitTooltip() {
	m_tooltipRect = CRect(0,0,0,0);
	WIN32_OP( m_tooltip.Create(NULL,NULL,NULL,WS_POPUP,WS_EX_TRANSPARENT) );
	m_toolinfo.cbSize = sizeof(m_toolinfo);
	m_toolinfo.uFlags = TTF_TRACK | TTF_IDISHWND | TTF_ABSOLUTE | TTF_TRANSPARENT;
	m_toolinfo.hwnd = *this;
	m_toolinfo.uId = 0;
	m_toolinfo.lpszText = LPSTR_TEXTCALLBACK;
	m_toolinfo.hinst = GetThisModuleHandle();
	WIN32_OP_D( m_tooltip.AddTool(&m_toolinfo) );
}
