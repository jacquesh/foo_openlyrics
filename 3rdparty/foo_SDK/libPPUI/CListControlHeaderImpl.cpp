#include "stdafx.h"
#include "CListControl.h"
#include "CListControlHeaderImpl.h" // redundant but makes intelisense quit showing false errors
#include "CListControl-Cells.h"
#include "PaintUtils.h"
#include "GDIUtils.h"
#include "win32_utility.h"
#include "DarkMode.h"
#include <vssym32.h>

enum {
	lineBelowHeaderCY = 1
};

static bool testDrawLineBelowHeader() {
	// Win10
	return IsWindows10OrGreater();
}

void CListControlHeaderImpl::OnThemeChangedPT() {
	if (m_header) {
		auto dark = GetDarkMode();
		if (dark != m_headerDark) {
			m_headerDark = dark;
			DarkMode::ApplyDarkThemeCtrl(m_header, dark, L"ItemsView");
		}
	}
	this->SetMsgHandled(FALSE);
}

void CListControlHeaderImpl::InitializeHeaderCtrl(DWORD flags) {
	PFC_ASSERT(IsWindow());
	PFC_ASSERT(!IsHeaderEnabled());
	m_headerDark = false;
	WIN32_OP_D( m_header.Create(*this,NULL,NULL,WS_CHILD | flags) != NULL );
	m_header.SetFont( GetFont() );

	InjectParentEraseHandler(m_header);

	this->OnThemeChangedPT();

	if (testDrawLineBelowHeader()) {
		WIN32_OP_D( m_headerLine.Create( *this, NULL, NULL, WS_CHILD ) != NULL );
		InjectParentEraseHandler(m_headerLine);
	}

	UpdateHeaderLayout();
}

void CListControlHeaderImpl::UpdateHeaderLayout() {
	CRect client; WIN32_OP_D( GetClientRect(client) );
	int cw_old = m_clientWidth;
	m_clientWidth = client.Width();
	if (IsHeaderEnabled()) {
		auto rc = client; 
		rc.left -= GetViewOffset().x;
		WINDOWPOS wPos = {};
		HDLAYOUT layout = {&rc, &wPos};
		if (m_header.Layout(&layout)) {
			m_header.SetWindowPos(wPos.hwndInsertAfter,wPos.x,wPos.y,wPos.cx,wPos.cy,wPos.flags | SWP_SHOWWINDOW);
			if (m_headerLine != NULL) m_headerLine.SetWindowPos(m_header, wPos.x, wPos.y + wPos.cy, wPos.cx, lineBelowHeaderCY, wPos.flags | SWP_SHOWWINDOW);
		} else {
			m_header.ShowWindow(SW_HIDE);
			if (m_headerLine != NULL) m_headerLine.ShowWindow(SW_HIDE);
		}
	} else {
		if ( cw_old != m_clientWidth) Invalidate();
	}
}

int CListControlHeaderImpl::GetItemWidth() const {
	if (IsHeaderEnabled()) return m_itemWidth;
	else return m_clientWidth;
}

LRESULT CListControlHeaderImpl::OnSizePassThru(UINT,WPARAM,LPARAM) {
	UpdateHeaderLayout();

	ProcessAutoWidth();
	
	SetMsgHandled(FALSE);
	return 0;
}

void CListControlHeaderImpl::OnViewOriginChange(CPoint p_delta) {
	TParent::OnViewOriginChange(p_delta);
	if (p_delta.x != 0) UpdateHeaderLayout();
}

void CListControlHeaderImpl::SetHeaderFont(HFONT font) {
	if (IsHeaderEnabled()) {
		m_header.SetFont(font); UpdateHeaderLayout();
	}
}

LRESULT CListControlHeaderImpl::OnHeaderCustomDraw(LPNMHDR hdr) {
	LPNMCUSTOMDRAW nmcd = reinterpret_cast<LPNMCUSTOMDRAW>(hdr);
	if ( m_header != NULL && this->GetDarkMode() && nmcd->hdr.hwndFrom == m_header) {
		switch (nmcd->dwDrawStage)
		{
		case CDDS_PREPAINT:
			return CDRF_NOTIFYITEMDRAW;
		case CDDS_ITEMPREPAINT:
			{
				CDCHandle dc(nmcd->hdc);
				dc.SetTextColor(0xdedede); 
				dc.SetBkColor(0x191919); // disregarded anyway
			}
			return CDRF_DODEFAULT;
		}
	}
	SetMsgHandled(FALSE); return 0;
}

LRESULT CListControlHeaderImpl::OnDividerDoubleClick(int,LPNMHDR hdr,BOOL&) {
	const NMHEADER * info = (const NMHEADER *) hdr;
	if (info->iButton == 0) {
		AutoColumnWidth((t_size)info->iItem);
	}
	return 0;
}

LRESULT CListControlHeaderImpl::OnHeaderItemClick(int,LPNMHDR p_hdr,BOOL&) {
	const NMHEADER * info = (const NMHEADER *) p_hdr;
	if (info->iButton == 0) {
		OnColumnHeaderClick((t_uint32)info->iItem);
	}
	return 0;
}

LRESULT CListControlHeaderImpl::OnHeaderItemChanged(int,LPNMHDR p_hdr,BOOL&) {
	const NMHEADER * info = (const NMHEADER*) p_hdr;
	if (info->pitem->mask & (HDI_WIDTH | HDI_ORDER)) {
		if(!m_ownColumnsChange) ProcessColumnsChange();
	}
	return 0;
}

LRESULT CListControlHeaderImpl::OnHeaderEndDrag(int,LPNMHDR hdr,BOOL&) {
	NMHEADER * info = (NMHEADER*) hdr;
	return OnColumnHeaderDrag(info->iItem,info->pitem->iOrder) ? TRUE : FALSE;
}

bool CListControlHeaderImpl::OnColumnHeaderDrag(t_size index, t_size newPos) {
	index = GetSubItemOrder(index);
	const t_size count = this->GetColumnCount();
	if ( count == 0 ) return false;
	std::vector<size_t> perm; perm.resize(count); pfc::create_move_items_permutation(&perm[0],count, pfc::bit_array_one(index), (int) newPos - (int) index );
	std::vector<int> order, newOrder; order.resize(count); newOrder.resize(count);
	WIN32_OP_D(m_header.GetOrderArray((int)count, &order[0]));
	for(t_size walk = 0; walk < count; ++walk) newOrder[walk] = order[perm[walk]];
	WIN32_OP_D(m_header.SetOrderArray((int)count, &newOrder[0]));
	OnColumnsChanged();
	return true;
}
t_size CListControlHeaderImpl::SubItemFromPointAbs(CPoint pt) const {
	
	auto order = GetColumnOrderArray();
	const t_size colCount = order.size();

	size_t item;
	if (! ItemFromPointAbs(pt, item ) ) item = SIZE_MAX;

	long xWalk = 0;
	for(t_size _walk = 0; _walk < colCount; ) {
		const t_size walk = (t_size) order[_walk];

		size_t span = 1;
		if (item != SIZE_MAX) span = this->GetSubItemSpan(item, walk);

		PFC_ASSERT( span == 1 || _walk == walk );

		if ( walk + span > colCount ) span = colCount - walk;

		long width = 0;
		for( size_t sub = 0; sub < span; ++ sub ) {
			width += (long)this->GetSubItemWidth(walk + sub);
		}

		if (xWalk + width > pt.x) return walk;
		xWalk += width;
		_walk += span;
	}
	return SIZE_MAX;
}

bool CListControlHeaderImpl::OnClickedSpecial(DWORD status, CPoint pt) {
	
	const DWORD maskButtons = MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 | MK_XBUTTON2;

	if ( (status & maskButtons) != MK_LBUTTON ) return false;

	if (!GetCellTypeSupported()) return false;

	size_t item; size_t subItem;
	if (! this->GetItemAtPointAbsEx( this->PointClientToAbs(pt), item, subItem ) ) {
		return false;
	}

	auto cellType = GetCellType(item, subItem);
	if ( !CellTypeReactsToMouseOver( cellType ) ) return false;
	auto rcHot = CellHotRect( item, subItem, cellType );
	if (!rcHot.PtInRect( pt )) return false;

	SetPressedItem(item, subItem);
	mySetCapture([=](UINT msg, DWORD newStatus, CPoint pt) {

		if (msg == WM_MOUSELEAVE) {
			ClearPressedItem(); return false;
		}

		{
			CRect rc = this->GetClientRectHook();
			if (!rc.PtInRect(pt)) {
				ClearPressedItem(); return false;
			}
		}

		size_t newItem, newSubItem;
		if (!this->GetItemAtPointAbsEx(this->PointClientToAbs(pt), newItem, newSubItem) || newItem != item || newSubItem != subItem) {
			ClearPressedItem(); return false;
		}

		DWORD buttons = newStatus & maskButtons;
		if (buttons == 0) {
			// button released?
			this->defer( [=] {
				OnSubItemClicked(item, subItem, pt);
			} );
			ClearPressedItem(); return false;
		}
		if (buttons != MK_LBUTTON) {
			// another button pressed?
			ClearPressedItem(); return false;
		}

		return true;
	});
	return true;
}

bool CListControlHeaderImpl::CellTypeUsesSpecialHitTests( cellType_t ct ) {
	if ( ct == nullptr ) return false;
	return ct->SuppressRowSelect();
}

bool CListControlHeaderImpl::OnClickedSpecialHitTest(CPoint pt) {
	if ( ! GetCellTypeSupported() ) return false;
	auto ct = GetCellTypeAtPointAbs( PointClientToAbs( pt ) );
	return CellTypeUsesSpecialHitTests(ct);
}

void CListControlHeaderImpl::OnItemClicked(t_size item, CPoint pt) {
	t_size subItem = SubItemFromPointAbs( PointClientToAbs( pt ) );
	if (subItem != SIZE_MAX) {
		if ( this->GetCellTypeSupported() ) {
			auto ct = this->GetCellType(item, subItem );
			// we don't handle hyperlink & button clicks thru here
			if (CellTypeUsesSpecialHitTests(ct)) return;
		}
		OnSubItemClicked(item, subItem, pt);
	}
}

std::vector<int> CListControlHeaderImpl::GetColumnOrderArray() const {
	const size_t cCount = this->GetColumnCount();
	std::vector<int> order; 
	if ( cCount > 0 ) {
		order.resize(cCount);
		if (IsHeaderEnabled()) {
			PFC_ASSERT(m_header.IsWindow());
			PFC_ASSERT(m_header.GetItemCount() == (int)cCount);
			WIN32_OP_D(m_header.GetOrderArray((int)cCount, &order[0]));
		} else {
			for (size_t c = 0; c < cCount; ++c) order[c] = (int)c;
		}
	}
	return order;
}

void CListControlHeaderImpl::RenderItemText(t_size item,const CRect & itemRect,const CRect & updateRect,CDCHandle dc, bool allowColors) {
	
	int xWalk = itemRect.left;
	CRect subItemRect(itemRect);
	auto order = GetColumnOrderArray();
	const size_t cCount = order.size();
	SelectObjectScope fontScope(dc,GetFont());
	for(t_size _walk = 0; _walk < cCount; ) {
		const t_size walk = order[_walk];
		
		size_t span = GetSubItemSpan(item, walk);

		PFC_ASSERT( walk == _walk || span == 1 );

		int width = GetSubItemWidth(walk);

		if ( span > 1 ) {
			if ( walk + span > cCount ) span = cCount - walk;
			for( size_t extraWalk = 1; extraWalk < span; ++ extraWalk ) {
				width += GetSubItemWidth(walk + extraWalk);
			}
		}

		subItemRect.left = xWalk; subItemRect.right = xWalk + width;
		CRect test;
		if (test.IntersectRect(itemRect, subItemRect)) {
			CRect subUpdate;
			if (subUpdate.IntersectRect(subItemRect, updateRect)) {
				DCStateScope scope(dc);
				if (dc.IntersectClipRect(subItemRect) != NULLREGION) {
					RenderSubItemText(item, walk, subItemRect, subUpdate, dc, allowColors);
				}
			}
		}
		xWalk += width;

		_walk += span;
	}
}

t_size CListControlHeaderImpl::GetSubItemOrder(t_size subItem) const {
	if ( ! IsHeaderEnabled( ) ) return subItem;
	HDITEM hditem = {};
	hditem.mask = HDI_ORDER;
	WIN32_OP_D( m_header.GetItem( (int) subItem, &hditem ) );
	return (t_size) hditem.iOrder;
}

size_t CListControlHeaderImpl::GetSubItemSpan(size_t row, size_t column) const {
	(void)row; (void)column;
	return 1;
}

uint32_t CListControlHeaderImpl::GetSubItemWidth(t_size subItem) const {
	if ( ! IsHeaderEnabled( ) ) {
		// Should be overridden for custom columns layout
		PFC_ASSERT( GetColumnCount() == 1 );
		PFC_ASSERT( subItem == 0 );
		return GetItemWidth();
	}

	if ( subItem < m_colRuntime.size() ) return m_colRuntime[subItem].m_widthPixels;
	PFC_ASSERT( !"bad column idx");
	return 0;
}

int CListControlHeaderImpl::GetHeaderItemWidth( int which ) {
	HDITEM hditem = {};
	hditem.mask = HDI_WIDTH;
	WIN32_OP_D( m_header.GetItem( which, &hditem) );
	return hditem.cxy;
}

void CListControlHeaderImpl::OnColumnsChanged() {
	if ( IsHeaderEnabled() ) {
		for( size_t walk = 0; walk < m_colRuntime.size(); ++ walk ) {
			m_colRuntime[walk].m_widthPixels = GetHeaderItemWidth( (int) walk );
		}
		RecalcItemWidth();
	}
	this->OnViewAreaChanged();
}

void CListControlHeaderImpl::ResetColumns(bool update) {
	m_colRuntime.clear();
	m_itemWidth = 0;
	PFC_ASSERT(IsHeaderEnabled());
	for(;;) {
		int count = m_header.GetItemCount();
		if (count <= 0) break;
		m_header.DeleteItem(count - 1);
	}
	if (update) OnColumnsChanged();
}

void CListControlHeaderImpl::SetColumn( size_t which, const char * label, DWORD fmtFlags, bool updateView) {
	PFC_ASSERT( IsHeaderEnabled() );
	pfc::stringcvt::string_os_from_utf8 labelOS;

	HDITEM item = {};

	if (label != nullptr) {
		if (which < m_colRuntime.size()) m_colRuntime[which].m_text = label;

		labelOS.convert(label);
		item.mask |= HDI_TEXT;
		item.pszText = const_cast<TCHAR*>(labelOS.get_ptr());
	}
	if (fmtFlags != UINT32_MAX) {
		item.mask |= HDI_FORMAT;
		item.fmt = fmtFlags | HDF_STRING;
	}
	
	m_header.SetItem( (int) which, &item );

	if (updateView) OnColumnsChanged();
}

void CListControlHeaderImpl::GetColumnText(size_t which, pfc::string_base & out) const {
	if (which < m_colRuntime.size()) {
		out = m_colRuntime[which].m_text.c_str();
	} else {
		out = "";
	}
}

void CListControlHeaderImpl::ResizeColumn(t_size index, t_uint32 widthPixels, bool updateView) {
	PFC_ASSERT( IsHeaderEnabled() );
	PFC_ASSERT( index < m_colRuntime.size() );
	auto& rt = m_colRuntime[index];
	rt.m_userWidth = widthPixels;
	if (rt.autoWidth()) {
		this->ProcessAutoWidth();
	} else {
		rt.m_widthPixels = widthPixels;
		HDITEM item = {};
		item.mask = HDI_WIDTH;
		item.cxy = rt.autoWidth() ? 0 : widthPixels;
		{ pfc::vartoggle_t<bool> scope(m_ownColumnsChange, true); m_header.SetItem((int)index, &item); }
		RecalcItemWidth();
		if (updateView) OnColumnsChanged();
	}
}

void CListControlHeaderImpl::DeleteColumns( pfc::bit_array const & mask, bool updateView ) {
	int nDeleted = 0;
	const size_t oldCount = GetColumnCount();
	mask.for_each(true, 0, oldCount, [&] (size_t idx) {
		int iDelete = (int) idx - nDeleted;
		bool bDeleted = m_header.DeleteItem( iDelete );
		PFC_ASSERT( bDeleted );
		if ( bDeleted ) ++ nDeleted;
		} );

	pfc::remove_mask_t( m_colRuntime, mask );

	ColumnWidthFix();

	if (updateView) {
		OnColumnsChanged();
	}

}

bool CListControlHeaderImpl::DeleteColumn(size_t index, bool update) {
	PFC_ASSERT( IsHeaderEnabled() );

	if (!m_header.DeleteItem( (int) index )) return false;

	pfc::remove_mask_t( m_colRuntime, pfc::bit_array_one( index ) );

	ColumnWidthFix();

	if (update) {
		OnColumnsChanged();
	}

	return true;
}

void CListControlHeaderImpl::SetSortIndicator( size_t whichColumn, bool isUp ) {
	HeaderControl_SetSortIndicator( GetHeaderCtrl(), (int) whichColumn, isUp );
}

void CListControlHeaderImpl::ClearSortIndicator() {
	HeaderControl_SetSortIndicator(GetHeaderCtrl(), -1, false);
}

bool CListControlHeaderImpl::HaveAutoWidthContentColumns() const {
	for( auto i = m_colRuntime.begin(); i != m_colRuntime.end(); ++ i ) {
		if ( i->autoWidthContent() ) return true;
	}
	return false;
}
bool CListControlHeaderImpl::HaveAutoWidthColumns() const {
	for( auto i = m_colRuntime.begin(); i != m_colRuntime.end(); ++ i ) {
		if ( i->autoWidth() ) return true;
	}
	return false;
}
void CListControlHeaderImpl::AddColumnEx( const char * label, uint32_t widthPixelsAt96DPI, DWORD fmtFlags, bool update ) {
	uint32_t w = widthPixelsAt96DPI;
	if ( w <= columnWidthMax ) {
		w = MulDiv( w, m_dpi.cx, 96 );
	}
	AddColumn( label, w, fmtFlags, update );
}

void CListControlHeaderImpl::AddColumnDLU( const char * label, uint32_t widthDLU, DWORD fmtFlags, bool update ) {
	uint32_t w = widthDLU;
	if ( w <= columnWidthMax ) {
		w = ::MapDialogWidth( GetParent(), w );
	}
	AddColumn( label, w, fmtFlags, update );
}
void CListControlHeaderImpl::AddColumnF( const char * label, float widthF, DWORD fmtFlags, bool update) {
	uint32_t w = columnWidthMax;
	if ( widthF >= 0 ) {
		w = pfc::rint32( widthF * m_dpi.cx / 96.0f );
	}
	AddColumn( label, w, fmtFlags, update );
}
void CListControlHeaderImpl::AddColumn(const char * label, uint32_t width, DWORD fmtFlags,bool update) {
	PFC_ASSERT(IsWindow());
	if (! IsHeaderEnabled( ) ) InitializeHeaderCtrl();

	pfc::stringcvt::string_os_from_utf8 labelOS(label);
	HDITEM item = {};
	item.mask = HDI_TEXT | HDI_FORMAT;
	if ( width != UINT32_MAX ) {
		item.cxy = width;
		item.mask |= HDI_WIDTH;
	}
	
	item.pszText = const_cast<TCHAR*>(labelOS.get_ptr());
	item.fmt = HDF_STRING | fmtFlags;
	int iColumn;
	WIN32_OP_D( (iColumn = m_header.InsertItem(m_header.GetItemCount(),&item) ) >= 0 );
	colRuntime_t rt;
	rt.m_text = label;
	rt.m_userWidth = width;
	if ( width <= columnWidthMax ) {
		m_itemWidth += width;
		rt.m_widthPixels = width;
	}
	m_colRuntime.push_back( std::move(rt) );
	
	if (update) OnColumnsChanged();

	ProcessAutoWidth();
}

float CListControlHeaderImpl::GetColumnWidthF(size_t subItem) const {
	auto w = GetSubItemWidth(subItem);
	return (float) w * 96.0f / (float)m_dpi.cx;
}

void CListControlHeaderImpl::RenderBackground(CDCHandle dc, CRect const& rc) {
	__super::RenderBackground(dc,rc);
#if 0
	if ( m_drawLineBelowHeader && IsHeaderEnabled()) {
		CRect rcHeader;
		if (m_header.GetWindowRect(rcHeader)) {
			// Draw a grid line below header
			int y = rcHeader.Height();
			if ( y >= rc.top && y < rc.bottom ) {
				CDCPen pen(dc, GridColor());
				SelectObjectScope scope(dc, pen);
				dc.MoveTo(rc.left, y);
				dc.LineTo(rc.right, y);
			}
		}
	}
#endif
}

CRect CListControlHeaderImpl::GetClientRectHook() const {
	CRect rcClient = __super::GetClientRectHook();
	if (m_header != NULL) {
		PFC_ASSERT( m_header.IsWindow() );
		CRect rcHeader;
		if (m_header.GetWindowRect(rcHeader)) {
			int h = rcHeader.Height();
			if ( m_headerLine != NULL ) h += lineBelowHeaderCY;
			rcClient.top = pfc::min_t(rcClient.bottom,rcClient.top + h);
		}
	}
	return rcClient;
}

CRect CListControlHeaderImpl::GetItemTextRectHook(size_t, size_t, CRect const & rc) const {
	return GetItemTextRect( rc );
}

CRect CListControlHeaderImpl::GetItemTextRect(const CRect & itemRect) const {
	CRect rc ( itemRect );
	rc.DeflateRect(GetColumnSpacing(),0);
	return rc;
}

void CListControlHeaderImpl::SetColumnSort(t_size which, bool isUp) {
	HeaderControl_SetSortIndicator(m_header,(int)which,isUp);
}

void CListControlHeaderImpl::SetColumnFormat(t_size which, DWORD format) {
	HDITEM item = {};
	item.mask = HDI_FORMAT;
	item.fmt = HDF_STRING | format;
	WIN32_OP_D( m_header.SetItem((int)which,&item) );
}

DWORD CListControlHeaderImpl::GetColumnFormat(t_size which) const {
	if (!IsHeaderEnabled()) return HDF_LEFT;
	HDITEM hditem = {};
	hditem.mask = HDI_FORMAT;
	WIN32_OP_D( m_header.GetItem( (int) which, &hditem) );
	return hditem.fmt;
}

BOOL CListControlHeaderImpl::OnSetCursor(CWindow, UINT nHitTest, UINT message) {
	(void)nHitTest;
	if ( message != 0 && GetCellTypeSupported() ) {
		CPoint pt( (LPARAM) GetMessagePos() );
		WIN32_OP_D( ScreenToClient( &pt ) );
		size_t item, subItem;
		if (GetItemAtPointAbsEx(this->PointClientToAbs(pt), item, subItem)) {
			auto ct = GetCellType(item, subItem);
			if (CellTypeReactsToMouseOver(ct)) {
				auto rc = CellHotRect(item, subItem, ct);
				if (PtInRect(rc, pt)) {
					auto cursor = ct->HotCursor();
					if (cursor) {
						SetCursor(cursor); return TRUE;
					}
				}
			}
		}
	}
	SetMsgHandled(FALSE); return FALSE;
}

bool CListControlHeaderImpl::GetItemAtPointAbsEx(CPoint pt, size_t & outItem, size_t & outSubItem) const {
	size_t item, subItem;
	if (ItemFromPointAbs(pt, item)) {
		subItem = SubItemFromPointAbs(pt);
		if (subItem != SIZE_MAX) {
			outItem = item; outSubItem = subItem; return true;
		}
	}
	return false;
}

CListControlHeaderImpl::cellType_t CListControlHeaderImpl::GetCellTypeAtPointAbs(CPoint pt) const {
	size_t item, subItem;
	if ( GetItemAtPointAbsEx( pt, item, subItem) ) {
		return GetCellType( item, subItem );
	}
	return nullptr;
}

CListCell * CListControlHeaderImpl::GetCellType( size_t item, size_t subItem ) const {
	(void)item; (void)subItem;
	return &PFC_SINGLETON(CListCell_Text);
}

void CListControlHeaderImpl::RenderSubItemText(t_size item, t_size subItem,const CRect & subItemRect,const CRect & updateRect,CDCHandle dc, bool allowColors) {
	(void)updateRect;
	const auto cellType = GetCellType( item, subItem );
	if ( cellType == nullptr ) {
		return;
	}
	pfc::string_formatter label;
	const bool bHaveText = GetSubItemText(item,subItem,label);
	if (! bHaveText ) {
		label = ""; //sanity
	}

	pfc::stringcvt::string_os_from_utf8_fast labelOS ( label );
	CListCell::DrawContentArg_t arg;
	arg.darkMode = this->GetDarkMode();
	arg.hdrFormat = GetColumnFormat( subItem );
	arg.subItemRect = subItemRect;
	arg.dc = dc;
	arg.text = labelOS.get_ptr();
	arg.allowColors = allowColors;
	bool bPressed;
	if ( cellType->IsToggle() ) bPressed = this->GetCellCheckState(item, subItem);
	else bPressed = (item == m_pressedItem) && (subItem == m_pressedSubItem);
	bool bHot = (item == m_hotItem) && ( subItem == m_hotSubItem );
	if ( bPressed ) arg.cellState |= CListCell::cellState_pressed;
	if ( bHot ) arg.cellState|= CListCell::cellState_hot;
	arg.rcText = GetItemTextRectHook(item, subItem, subItemRect);
	arg.rcHot = CellHotRect(item, subItem, cellType, subItemRect);

	auto strTheme = cellType->Theme();
	if ( strTheme != nullptr ) {
		arg.theme = themeFor( strTheme ).m_theme;
	}
	arg.colorHighlight = GetSysColorHook(colorHighlight);

	arg.thisWnd = m_hWnd;

	if (this->IsSubItemGrayed( item, subItem ) ) {
		arg.cellState |= CListCell::cellState_disabled;
	}

	if (this->RenderCellImageTest(item, subItem)) {
		arg.imageRenderer = [this, item, subItem](CDCHandle dc, const CRect& rc) { this->RenderCellImage(item, subItem, dc, rc); };
	}

	CFontHandle fontRestore;
	CFont fontOverride;

	LOGFONT data;
	if (dc.GetCurrentFont().GetLogFont(&data)) {
		if ( cellType->ApplyTextStyle( data, CellTextScale(item, subItem ), arg.cellState ) ) {
			if (fontOverride.CreateFontIndirect( & data )) {
				fontRestore = dc.SelectFont( fontOverride );
			}
		}
	}
	cellType->DrawContent( arg );

	if ( fontRestore != NULL ) dc.SelectFont( fontRestore );
}

void CListControlHeaderImpl::RenderGroupHeaderText2(size_t baseItem,const CRect & headerRect,const CRect & updateRect,CDCHandle dc) {
	(void)updateRect;
	pfc::string_formatter label;
	if (GetGroupHeaderText2(baseItem,label)) {
		SelectObjectScope fontScope(dc,GetGroupHeaderFont());
		pfc::stringcvt::string_os_from_utf8 cvt(label);
		CRect contentRect(GetItemTextRect(headerRect));
		dc.DrawText(cvt,(int)cvt.length(),contentRect,DT_NOPREFIX | DT_END_ELLIPSIS | DT_SINGLELINE | DT_VCENTER | DT_LEFT );
		SIZE txSize;
		const int lineSpacing = contentRect.Height() / 2;
		if (dc.GetTextExtent(cvt,(int)cvt.length(),&txSize)) {
			if (txSize.cx + lineSpacing < contentRect.Width()) {
				const CPoint center = contentRect.CenterPoint();
				const CPoint pt1(contentRect.left + txSize.cx + lineSpacing, center.y), pt2(contentRect.right, center.y);
				const COLORREF lineColor = PaintUtils::BlendColor(dc.GetTextColor(),dc.GetBkColor(),25);

#ifndef CListControl_ScrollWindowFix
#error FIXME CMemoryDC needed
#endif
				PaintUtils::DrawSmoothedLine(dc, pt1, pt2, lineColor, 1.0 * (double)m_dpi.cy / 96.0);
			}
		}
	}
}

uint32_t CListControlHeaderImpl::GetOptimalColumnWidthFixed(const char * fixedText, bool pad) const {
	CWindowDC dc(*this);
	SelectObjectScope fontScope(dc, GetFont());
	GetOptimalWidth_Cache cache;
	cache.m_dc = dc;
	cache.m_stringTemp = fixedText;
	uint32_t ret = cache.GetStringTempWidth();
	if ( pad ) ret += this->GetColumnSpacing() * 2;
	return ret;
}

t_uint32 CListControlHeaderImpl::GetOptimalSubItemWidth(t_size item, t_size subItem, GetOptimalWidth_Cache & cache) const {
	const t_uint32 base = this->GetColumnSpacing() * 2;
	if (GetSubItemText(item,subItem,cache.m_stringTemp)) {
		return base + cache.GetStringTempWidth();
	} else {
		return base;
	}
}

t_uint32 CListControlHeaderImpl::GetOptimalWidth_Cache::GetStringTempWidth() {
	if (m_stringTemp.replace_string_ex(m_stringTempUnfuckAmpersands, "&", "&&") > 0) {
		m_convertTemp.convert(m_stringTempUnfuckAmpersands);
	} else {
		m_convertTemp.convert(m_stringTemp);
	}
	return PaintUtils::TextOutColors_CalcWidth(m_dc, m_convertTemp);
}

t_uint32 CListControlHeaderImpl::GetOptimalColumnWidth(t_size which, GetOptimalWidth_Cache & cache) const {
	const t_size totalItems = GetItemCount();
	t_uint32 val = 0;
	for(t_size item = 0; item < totalItems; ++item) {
		pfc::max_acc( val, GetOptimalSubItemWidth( item, which, cache ) );
	}
	return val;
}

t_uint32 CListControlHeaderImpl::GetOptimalSubItemWidthSimple(t_size item, t_size subItem) const {
	CWindowDC dc(*this);
	SelectObjectScope fontScope(dc, GetFont() );
	GetOptimalWidth_Cache cache;
	cache.m_dc = dc;
	return GetOptimalSubItemWidth(item, subItem, cache);
}

LRESULT CListControlHeaderImpl::OnKeyDown(UINT,WPARAM wp,LPARAM,BOOL& bHandled) {
	switch(wp) {
	case VK_ADD:
		if (IsKeyPressed(VK_CONTROL)) {
			AutoColumnWidths();
			return 0;
		}
		break;
	}
	bHandled = FALSE;
	return 0;
}

uint32_t CListControlHeaderImpl::GetOptimalColumnWidth(size_t colIndex) const {
	CWindowDC dc(*this);
	SelectObjectScope fontScope(dc, GetFont());
	GetOptimalWidth_Cache cache;
	cache.m_dc = dc;
	uint32_t ret = 0;
	const auto itemCount = GetItemCount();
	for (t_size walk = 0; walk < itemCount; ++walk) {
		pfc::max_acc(ret, GetOptimalSubItemWidth(walk, colIndex, cache));
	}
	return ret;
}

void CListControlHeaderImpl::AutoColumnWidths(const pfc::bit_array & mask, bool expandLast) {
	PFC_ASSERT( IsHeaderEnabled() );
	if (!IsHeaderEnabled()) return;
	const t_size itemCount = GetItemCount();
	if (itemCount == 0) return;
	const t_size columnCount = (t_size) m_header.GetItemCount();
	if (columnCount == 0) return;
	pfc::array_t<t_uint32> widths; widths.set_size(columnCount); widths.fill_null();
	{
		CWindowDC dc(*this);
		SelectObjectScope fontScope(dc,GetFont());
		GetOptimalWidth_Cache cache;
		cache.m_dc = dc;
		for(t_size walk = 0; walk < itemCount; ++walk) {
			for(t_size colWalk = mask.find_first(true,0,columnCount); colWalk < columnCount; colWalk = mask.find_next(true,colWalk,columnCount)) {
				pfc::max_acc(widths[colWalk], GetOptimalSubItemWidth(walk,colWalk,cache));
			}
		}
	}

	if (expandLast) {
		uint32_t usedWidth = 0; size_t lastCol = SIZE_MAX;
		pfc::array_t<int> order; order.set_size(columnCount);
		WIN32_OP_D( m_header.GetOrderArray((int)columnCount,order.get_ptr()) );
		for(size_t _walk = 0; _walk < columnCount; ++_walk) {
			const size_t colWalk = (size_t) order[_walk];
			PFC_ASSERT( colWalk < columnCount );
			if (mask[colWalk]) {
				lastCol = colWalk;
				usedWidth += widths[colWalk];
			} else {
				usedWidth += GetSubItemWidth(colWalk);
			}
		}
		if (lastCol != SIZE_MAX) {
			t_uint32 clientWidth = this->GetClientRectHook().Width(); 
			if (clientWidth > 0) --clientWidth; // $!@# scrollbar hack
			if (usedWidth < clientWidth) {
				widths[lastCol] += clientWidth - usedWidth;
			}
		}
	}
	for(t_size colWalk = mask.find_first(true,0,columnCount); colWalk < columnCount; colWalk = mask.find_next(true,colWalk,columnCount)) {
		ResizeColumn(colWalk,widths[colWalk],false);
	}
	ProcessColumnsChange();
}

t_uint32 CListControlHeaderImpl::GetOptimalGroupHeaderWidth2(size_t baseItem) const {
	CWindowDC dc(*this);
	SelectObjectScope fontScope(dc,GetGroupHeaderFont());
	GetOptimalWidth_Cache cache; cache.m_dc = dc;
	const t_uint32 base = this->GetColumnSpacing() * 2;
	if (GetGroupHeaderText2(baseItem,cache.m_stringTemp)) {
		return base + cache.GetStringTempWidth();
	} else {
		return base;
	}
}

size_t CListControlHeaderImpl::GetColumnCount() const {
	if ( ! IsHeaderEnabled() ) return 1;
#if PFC_DEBUG
	int iHeaderCount = m_header.GetItemCount();
	PFC_ASSERT( m_colRuntime.size() == (size_t) iHeaderCount );
#endif
	return m_colRuntime.size();
}

void CListControlHeaderImpl::ColumnWidthFix() {
	if ( this->HaveAutoWidthColumns() ) {
		ProcessAutoWidth();
	} else {
		RecalcItemWidth();
	}
}

void CListControlHeaderImpl::ProcessAutoWidth() {
	if ( HaveAutoWidthColumns() ) {
		const int clientWidth = this->GetClientRectHook().Width();

		if ( ! this->HaveAutoWidthContentColumns( ) && clientWidth == m_itemWidth) return;

		const size_t count = GetColumnCount();
		uint32_t totalNonAuto = 0;
		size_t numAutoWidth = 0;
		for(size_t walk = 0; walk < count; ++ walk ) {
			if ( m_colRuntime[walk].autoWidth() ) {
				++ numAutoWidth;
			} else {
				totalNonAuto += GetSubItemWidth(walk);
			}
		}
		int toDivide = clientWidth - totalNonAuto;
		if ( toDivide < 0 ) toDivide = 0;

		size_t numLeft = numAutoWidth;
		auto worker = [&] ( size_t iCol ) {
			auto & rt = m_colRuntime[iCol];
			int lo = this->GetOptimalColumnWidthFixed( rt.m_text.c_str() );
			if ( rt.autoWidthContent() ) {
				int lo2 = this->GetOptimalColumnWidth( iCol );
				if ( lo < lo2 ) lo = lo2;
			}
			int width = (int)(toDivide / numLeft);
			if ( width < lo ) width = lo;
			
			HDITEM item = {};
			item.mask = HDI_WIDTH;
			item.cxy = width;
			WIN32_OP_D( m_header.SetItem( (int) iCol, &item ) );
			rt.m_widthPixels = width;

			if ( toDivide > width ) {
				toDivide -= width;
			} else {
				toDivide = 0;
			}
			-- numLeft;

		};
		for( size_t iCol = 0; iCol < count; ++ iCol ) {
			if (m_colRuntime[iCol].autoWidthContent() ) worker(iCol);
		}
		for( size_t iCol = 0; iCol < count; ++ iCol ) {
			if ( m_colRuntime[iCol].autoWidthPlain() ) worker(iCol);
		}

		RecalcItemWidth();
		OnColumnsChanged();
		m_header.Invalidate();
	}
}

void CListControlHeaderImpl::RecalcItemWidth() {
	int total = 0;
	const t_size count = GetColumnCount();
	for(t_size walk = 0; walk < count; ++walk) total += GetSubItemWidth(walk);
	m_itemWidth = total;
}

CRect CListControlHeaderImpl::GetSubItemRectAbs(t_size item,t_size subItem) const {
	CRect rc = GetItemRectAbs(item);
	auto order = GetColumnOrderArray();
	const t_size colCount = order.size();
	for(t_size _walk = 0; _walk < colCount; ++_walk) {
		const t_size walk = (t_size) order[_walk];

		t_size width = this->GetSubItemWidth(walk);
		if (subItem == walk) {
			
			size_t span = GetSubItemSpan(item, walk);
			if ( walk + span > colCount ) span = colCount - walk;
			for( size_t extra = 1; extra < span; ++ extra ) {
				width += GetSubItemWidth( walk + extra);
			}
			
			rc.right = rc.left + (long)width;

			return rc;
		} else {
			rc.left += (long)width;
		}
	}
	throw pfc::exception_invalid_params();
}

CRect CListControlHeaderImpl::GetSubItemRect(t_size item,t_size subItem) const {
	return RectAbsToClient(GetSubItemRectAbs(item, subItem));
}

void CListControlHeaderImpl::SetHotItem(size_t row, size_t column) {
	if ( m_hotItem != row  || m_hotSubItem != column ) {
		const size_t total = GetItemCount();
		if (m_hotItem < total) InvalidateRect(GetSubItemRect(m_hotItem, m_hotSubItem));
		m_hotItem = row; m_hotSubItem = column;
		if (m_hotItem < total) InvalidateRect(GetSubItemRect(m_hotItem, m_hotSubItem));
		HotItemChanged(row, column);
	}
}

void CListControlHeaderImpl::SetPressedItem(size_t row, size_t column) {
	if (m_pressedItem != row || m_pressedSubItem != column) {
		if (m_pressedItem != SIZE_MAX) InvalidateRect(GetSubItemRect(m_pressedItem, m_pressedSubItem));
		m_pressedItem = row; m_pressedSubItem = column;
		if (m_pressedItem != SIZE_MAX) InvalidateRect(GetSubItemRect(m_pressedItem, m_pressedSubItem));
		PressedItemChanged(row, column);
	}
}

void CListControlHeaderImpl::SetCellCheckState(size_t item, size_t subItem, bool value) {
	ReloadItem(item); (void)subItem; (void)value;
	// Subclass deals with keeping track of state
}

bool CListControlHeaderImpl::ToggleSelectedItemsHook(const pfc::bit_array & mask) {
	if (this->GetCellTypeSupported() ) {
		bool handled = false;
		bool setTo = true;

		mask.walk(GetItemCount(), [&](size_t idx) {
			auto ct = this->GetCellType(idx, 0);
			if ( ct != nullptr && ct->IsToggle() ) {
				if ( ct->IsRadioToggle() ) {
					if (!handled) {
						handled = true;
						setTo = !this->GetCellCheckState(idx, 0);
						this->SetCellCheckState(idx, 0, setTo);
					}
				} else {
					if (!handled) {
						handled = true;
						setTo = ! this->GetCellCheckState(idx,0);
					}
					this->SetCellCheckState(idx,0,setTo);
				}
			}
		});

		if (handled) return true;
	}
	return __super::ToggleSelectedItemsHook(mask);
}

void CListControlHeaderImpl::OnSubItemClicked(t_size item, t_size subItem, CPoint pt) {
	auto ct = GetCellType(item, subItem);
	if (ct != nullptr) {
		if (ct->IsToggle()) {
			if (ct->HotRect(GetSubItemRect(item, subItem)).PtInRect(pt)) {
				this->SetCellCheckState(item, subItem, !GetCellCheckState(item, subItem));
			}
		} else if (ct->ClickToEdit()) {
			this->RequestEditItem(item, subItem);
		}
	}
}


bool CListControlHeaderImpl::AllowTypeFindInCell(size_t item, size_t subItem) const {
	auto cell = GetCellType( item, subItem );
	if ( cell == nullptr ) return false;
	return cell->AllowTypeFind();
}

bool CListControlHeaderImpl::CellTypeReactsToMouseOver(cellType_t ct) {
	return ct != nullptr && ct->IsInteractive();
}

CRect CListControlHeaderImpl::CellHotRect( size_t, size_t, cellType_t ct, CRect rcCell) {
	if ( ct != nullptr ) {
		return ct->HotRect(rcCell);
	}
	return rcCell;
}
CRect CListControlHeaderImpl::CellHotRect(size_t item, size_t subItem, cellType_t ct) {
	return CellHotRect( item, subItem, ct, GetSubItemRect( item, subItem ) );
}
void CListControlHeaderImpl::OnMouseMove(UINT nFlags, CPoint pt) {
	const DWORD maskButtons = MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 | MK_XBUTTON2;
	if (GetCellTypeSupported() && (nFlags & maskButtons) == 0 ) {
		size_t item; size_t subItem;
		if (this->GetItemAtPointAbsEx( PointClientToAbs(pt), item, subItem)) {
			auto ct = this->GetCellType( item, subItem );
			if (CellTypeReactsToMouseOver(ct) ) {
				auto rc = CellHotRect( item, subItem, ct );
				if ( PtInRect( rc, pt ) ) {
					const bool bTrack = ct != nullptr && ct->TracksMouseMove();
					SetHotItem(item, subItem);

					if (bTrack) this->CellTrackMouseMove(item, subItem, WM_MOUSEMOVE, nFlags, pt);

					mySetCapture([=](UINT msg, DWORD newStatus, CPoint pt) {
						if (msg == WM_MOUSELEAVE) {
							this->ClearHotItem();
							return false;
						}

						if ((newStatus & maskButtons) != 0 || msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL ) {
							// A button has been pressed or wheel has been moved
							this->ClearHotItem();
							mySetCaptureMsgHandled(FALSE);
							return false;
						}

						if ( ! PtInRect( rc, pt ) || item >= GetItemCount() ) {
							// Left the rect
							this->ClearHotItem();
							this->mySetCaptureMsgHandled(FALSE);
							return false;
						}
						if (bTrack) {
							this->CellTrackMouseMove(item, subItem, msg, newStatus, pt);
						}

						return true;
					});
				}
			}
		}
	}
	SetMsgHandled(FALSE);
}

bool CListControlHeaderImpl::AllowScrollbar(bool vertical) const {
	if ( vertical ) {
		// vertical
		return true;
	} else {
		// horizontal
		if (! IsHeaderEnabled( ) ) return false; // no header?
		return true;
	}
}

void CListControlHeaderImpl::OnDestroy() {
	m_colRuntime.clear();
	m_header = NULL;
	m_headerLine = NULL;
	m_headerDark = false;
	SetMsgHandled(FALSE);
}


uint32_t CListControlHeaderImpl::GetColumnsBlankWidth( size_t colExclude ) const {
	auto client = this->GetClientRectHook().Width();
	int item = GetItemWidth();
	if (colExclude) item -= GetSubItemWidth(colExclude);
	if ( item  < 0 ) item = 0;
	if ( item < client ) return (uint32_t)( client - item );
	else return 0;
}

void CListControlHeaderImpl::SizeColumnToContent( size_t which, uint32_t minWidth ) {
	auto width = this->GetOptimalColumnWidth( which );
	if ( width < minWidth ) width = minWidth;
	this->ResizeColumn( which, width );
}

void CListControlHeaderImpl::SizeColumnToContentFillBlank( size_t which ) {
	this->SizeColumnToContent( which, this->GetColumnsBlankWidth(which) );
}

HBRUSH CListControlHeaderImpl::OnCtlColorStatic(CDCHandle dc, CStatic wndStatic) {
	if ( wndStatic == m_headerLine ) {
		COLORREF col = GridColor();
		dc.SetDCBrushColor( col );
		return (HBRUSH) GetStockObject(DC_BRUSH);
	}
	SetMsgHandled(FALSE);
	return NULL;
}

void CListControlHeaderImpl::ReloadData() {
	__super::ReloadData();
	if ( this->HaveAutoWidthContentColumns( ) ) {
		this->ColumnWidthFix();
	}
}

void CListControlHeaderImpl::RenderGroupOverlay(size_t baseItem, const CRect& p_groupWhole, const CRect& p_updateRect, CDCHandle dc) {
	CRect subItemRect = p_groupWhole;
	t_uint32 xWalk = p_groupWhole.left;
	auto order = GetColumnOrderArray();
	const size_t cCount = order.size();
	for (t_size _walk = 0; _walk < cCount; ) {
		const t_size walk = order[_walk];

		t_uint32 width = GetSubItemWidth(walk);

		subItemRect.left = xWalk; subItemRect.right = xWalk + width;
		CRect subUpdate;
		if (subUpdate.IntersectRect(subItemRect, p_updateRect)) {
			DCStateScope scope(dc);
			if (dc.IntersectClipRect(subItemRect) != NULLREGION) {
				this->RenderGroupOverlayColumn(baseItem, walk, subItemRect, subUpdate, dc);
			}
		}
		xWalk += width;

		_walk += 1;
	}
}

void CListControlHeaderImpl::UpdateGroupOverlayColumnByID(groupID_t groupID, size_t subItem) {
	if (this->GetItemCount() == 0) return;
	CRect rc = this->GetSubItemRectAbs(0, subItem);
	this->UpdateGroupOverlayByID(groupID, rc.left, rc.right);
}

void CListControlHeaderImpl::mySetCapture(CaptureProc_t proc) {
	this->m_captureProc = proc;
	this->TrackMouseLeave();
}

void CListControlHeaderImpl::myReleaseCapture() {
	m_captureProc = nullptr;
}

void CListControlHeaderImpl::TrackMouseLeave() {
	TRACKMOUSEEVENT tme = { sizeof(tme) };
	tme.dwFlags = TME_LEAVE;
	tme.hwndTrack = m_hWnd;
	TrackMouseEvent(&tme);
}

LRESULT CListControlHeaderImpl::MousePassThru(UINT msg, WPARAM wp, LPARAM lp) {
	auto p = m_captureProc; // create local ref in case something in mid-captureproc clears it
	if (p) {
		CPoint pt(lp);
		if (!p(msg, (DWORD)wp, pt)) {
			myReleaseCapture();
		}
		return 0;
	}

	SetMsgHandled(FALSE);
	return 0;
}

void CListControlHeaderImpl::OnMouseLeave() {
	if (m_captureProc) {
		m_captureProc(WM_MOUSELEAVE, 0, -1);
		myReleaseCapture();
	}
}

void CListControlHeaderImpl::OnKillFocus(CWindow) {
	myReleaseCapture();
	SetMsgHandled(FALSE);
}

void CListControlHeaderImpl::OnWindowPosChanged(LPWINDOWPOS arg) {
	if (arg->flags & SWP_HIDEWINDOW) {
		myReleaseCapture();
	}
	SetMsgHandled(FALSE);
}

void CListControlHeaderImpl::RequestEditItem(size_t, size_t) {
	PFC_ASSERT(!"Please enable CListControl_EditImpl");
}

void CListControlHeaderImpl::OnLButtonDblClk(UINT nFlags, CPoint point) {
	(void)nFlags;
	if (this->GetCellTypeSupported()) {
		auto ptAbs = PointClientToAbs(point);
		size_t item = this->ItemFromPointAbs(ptAbs);
		size_t subItem = SubItemFromPointAbs(ptAbs);
		if (item != SIZE_MAX && subItem != SIZE_MAX) {
			auto ct = GetCellType(item, subItem);
			if (ct != nullptr) {
				if (ct->IsToggle()) {
					if (ct->HotRect(GetSubItemRect(item, subItem)).PtInRect(point)) {
						return; // disregard doubleclick on checkbox
					}
				}
			}
		}
	}
	SetMsgHandled(FALSE); // handle doubleclick
}


void CListControlHeaderImpl::RenderItemBackground(CDCHandle p_dc, const CRect& p_itemRect, size_t item, uint32_t bkColor) {
	if (!this->DelimitColumns()) {
		__super::RenderItemBackground(p_dc, p_itemRect, item, bkColor);
	} else {
		auto cnt = this->GetColumnCount();
		const auto order = this->GetColumnOrderArray();
		uint32_t x = p_itemRect.left;
		for (size_t walk = 0; walk < cnt; ) {
			const size_t sub = order[walk];
			auto span = this->GetSubItemSpan(item, sub);
			PFC_ASSERT(span > 0);
			uint32_t width = 0;
			for (size_t walk2 = 0; walk2 < span; ++walk2) {
				width += this->GetSubItemWidth(sub + walk2);
			}
			CRect rc = p_itemRect;
			rc.left = x;
			x += width;
			rc.right = x;

			CRect test;
			if (test.IntersectRect(rc, p_itemRect)) {
				DCStateScope s(p_dc);
				if (p_dc.IntersectClipRect(rc) != NULLREGION) {
					__super::RenderItemBackground(p_dc, rc, item, bkColor);
				}
			}
			walk += span;
		}
	}
}
