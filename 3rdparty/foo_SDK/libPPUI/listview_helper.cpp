#include "stdafx.h"

#include "win32_utility.h"
#include "win32_op.h"
#include "listview_helper.h"
#include "CListViewCtrlEx.h"
#include "CHeaderCtrlEx.h"

namespace listview_helper {

	unsigned insert_item(HWND p_listview,unsigned p_index,const char * p_name,LPARAM p_param)
	{
		if (p_index == ~0) p_index = ListView_GetItemCount(p_listview);
		LVITEM item = {};

		pfc::stringcvt::string_os_from_utf8 os_string_temp(p_name);

		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.iItem = p_index;
		item.lParam = p_param;
		item.pszText = const_cast<TCHAR*>(os_string_temp.get_ptr());
		
		LRESULT ret = SendMessage(p_listview,LVM_INSERTITEM,0,(LPARAM)&item);
		if (ret < 0) return ~0;
		else return (unsigned) ret;
	}

	unsigned insert_item2(HWND p_listview, unsigned p_index, const char * col0, const char * col1, LPARAM p_param) {
		unsigned i = insert_item( p_listview, p_index, col0, p_param );
		if (i != ~0) {
			set_item_text( p_listview, i, 1, col1 );
		}
		return i;
	}

	unsigned insert_item3(HWND p_listview, unsigned p_index, const char * col0, const char * col1, const char * col2, LPARAM p_param) {
		unsigned i = insert_item( p_listview, p_index, col0, p_param );
		if (i != ~0) {
			set_item_text( p_listview, i, 1, col1 );
			set_item_text( p_listview, i, 2, col2 );
		}
		return i;
	}

	unsigned insert_column(HWND p_listview,unsigned p_index,const char * p_name,unsigned p_width_dlu)
	{
		pfc::stringcvt::string_os_from_utf8 os_string_temp(p_name);

		RECT rect = {0,0,(LONG)p_width_dlu,0};
		MapDialogRect(GetParent(p_listview),&rect);

		LVCOLUMN data = {};
		data.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
		data.fmt = LVCFMT_LEFT;
		data.cx = rect.right;
		data.pszText = const_cast<TCHAR*>(os_string_temp.get_ptr());
		
		LRESULT ret = SendMessage(p_listview,LVM_INSERTCOLUMN,p_index,(LPARAM)&data);
		if (ret < 0) return UINT_MAX;
		else return (unsigned) ret;
	}

	void get_item_text(HWND p_listview,unsigned p_index,unsigned p_column,pfc::string_base & p_out) {
		enum {buffer_length = 1024*64};
		pfc::array_t<TCHAR> buffer; buffer.set_size(buffer_length);
		ListView_GetItemText(p_listview,p_index,p_column,buffer.get_ptr(),buffer_length);
		p_out = pfc::stringcvt::string_utf8_from_os(buffer.get_ptr(),buffer_length);
	}

	bool set_item_text(HWND p_listview,unsigned p_index,unsigned p_column,const char * p_name)
	{
		LVITEM item = {};

		pfc::stringcvt::string_os_from_utf8 os_string_temp(p_name);

		item.mask = LVIF_TEXT;
		item.iItem = p_index;
		item.iSubItem = p_column;
		item.pszText = const_cast<TCHAR*>(os_string_temp.get_ptr());
		return SendMessage(p_listview,LVM_SETITEM,0,(LPARAM)&item) ? true : false;
	}

	bool is_item_selected(HWND p_listview,unsigned p_index)
	{
		LVITEM item = {};
		item.mask = LVIF_STATE;
		item.iItem = p_index;
		item.stateMask = LVIS_SELECTED;
		if (!SendMessage(p_listview,LVM_GETITEM,0,(LPARAM)&item)) return false;
		return (item.state & LVIS_SELECTED) ? true : false;
	}

	void set_item_selection(HWND p_listview,unsigned p_index,bool p_state)
	{
		PFC_ASSERT( ::IsWindow(p_listview) );
		LVITEM item = {};
		item.stateMask = LVIS_SELECTED;
		item.state = p_state ? LVIS_SELECTED : 0;
		WIN32_OP_D( SendMessage(p_listview,LVM_SETITEMSTATE,(WPARAM)p_index,(LPARAM)&item) );
	}

	bool select_single_item(HWND p_listview,unsigned p_index)
	{
		LRESULT temp = SendMessage(p_listview,LVM_GETITEMCOUNT,0,0);
		if (temp < 0) return false;
		ListView_SetSelectionMark(p_listview,p_index);
		unsigned n; const unsigned m = pfc::downcast_guarded<unsigned>(temp);
		for(n=0;n<m;n++) {
			enum {mask = LVIS_FOCUSED | LVIS_SELECTED};
			ListView_SetItemState(p_listview,n,n == p_index ? mask : 0, mask);
		}
		return ensure_visible(p_listview,p_index);
	}

	bool ensure_visible(HWND p_listview,unsigned p_index)
	{
		return SendMessage(p_listview,LVM_ENSUREVISIBLE,p_index,FALSE) ? true : false;
	}
}


void ListView_GetContextMenuPoint(HWND p_list,LPARAM p_coords,POINT & p_point,int & p_selection) {
	POINT pt = {(short)LOWORD(p_coords),(short)HIWORD(p_coords)};
	ListView_GetContextMenuPoint(p_list, pt, p_point, p_selection);
}

void ListView_GetContextMenuPoint(HWND p_list,POINT p_coords,POINT & p_point,int & p_selection) {
	if (p_coords.x == -1 && p_coords.y == -1) {
		int firstsel = ListView_GetFirstSelection(p_list);
		if (firstsel >= 0) {
			ListView_EnsureVisible(p_list, firstsel, FALSE);
			RECT rect;
			WIN32_OP_D( ListView_GetItemRect(p_list,firstsel,&rect,LVIR_BOUNDS) );
			p_point.x = (rect.left + rect.right) / 2;
			p_point.y = (rect.top + rect.bottom) / 2;
			WIN32_OP_D( ClientToScreen(p_list,&p_point) );
		} else {
			RECT rect;
			WIN32_OP_D(GetClientRect(p_list,&rect));
			p_point.x = (rect.left + rect.right) / 2;
			p_point.y = (rect.top + rect.bottom) / 2;
			WIN32_OP_D(ClientToScreen(p_list,&p_point));
		}
		p_selection = firstsel;
	} else {
		POINT pt = p_coords; // {(short)LOWORD(p_coords),(short)HIWORD(p_coords)};
		p_point = pt;
		POINT client = pt;
		WIN32_OP_D( ScreenToClient(p_list,&client) );
		LVHITTESTINFO info = {};
		info.pt = client;
		p_selection = ListView_HitTest(p_list,&info);
	}
}

int ListView_GetColumnCount(HWND listView) {
	HWND header = ListView_GetHeader(listView);
	PFC_ASSERT(header != NULL);
	return Header_GetItemCount(header);
}

void ListView_FixContextMenuPoint(CListViewCtrl list, CPoint & coords) {
	if (coords == CPoint(-1, -1)) {
		int selWalk = -1;
		CRect rcClient; WIN32_OP_D(list.GetClientRect(rcClient));
		for (;;) {
			selWalk = list.GetNextItem(selWalk, LVNI_SELECTED);
			if (selWalk < 0) {
				CRect rc;
				WIN32_OP_D(list.GetWindowRect(&rc));
				coords = rc.CenterPoint();
				return;
			}
			CRect rcItem, rcVisible;
			WIN32_OP_D(list.GetItemRect(selWalk, &rcItem, LVIR_BOUNDS));
			if (rcVisible.IntersectRect(rcItem, rcClient)) {
				coords = rcVisible.CenterPoint();
				WIN32_OP_D(list.ClientToScreen(&coords));
				return;
			}
		}
	}
}

unsigned CListViewCtrlEx::GetColunnCount() {
	return (unsigned) ListView_GetColumnCount( *this );
}
unsigned CListViewCtrlEx::AddColumnEx(const wchar_t * name, unsigned widthDLU) {
	return InsertColumnEx( GetColunnCount(), name, widthDLU );
}
unsigned CListViewCtrlEx::InsertColumnEx(unsigned index, const wchar_t * name, unsigned widthDLU) {

	RECT rect = { 0,0,(LONG)widthDLU,0 };
	MapDialogRect(GetParent(), &rect);

	LVCOLUMN data = {};
	data.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
	data.fmt = LVCFMT_LEFT;
	data.cx = rect.right;
	data.pszText = const_cast<wchar_t*>(name);

	auto ret = this->InsertColumn(index, & data );
	if (ret < 0) return UINT_MAX;
	else return (unsigned)ret;
}

void CListViewCtrlEx::FixContextMenuPoint(CPoint & pt) {
	ListView_FixContextMenuPoint(*this, pt);
}



unsigned CListViewCtrlEx::InsertString(unsigned index, const wchar_t * str) {
	LVITEM item = {};

	item.mask = LVIF_TEXT;
	item.iItem = index;
	item.pszText = const_cast<wchar_t*>(str);

	auto ret = InsertItem(&item);
	if ( ret < 0 ) return UINT_MAX;
	else return (unsigned) ret;
}
unsigned CListViewCtrlEx::InsertString8(unsigned index, const char * str) {
	return InsertString(index, pfc::stringcvt::string_os_from_utf8( str ) );
}
unsigned CListViewCtrlEx::AddString(const wchar_t * str) {
	return InsertString(GetItemCount(), str);
}
unsigned CListViewCtrlEx::AddString8(const char * str) {
	return AddString(pfc::stringcvt::string_os_from_utf8( str ) );
}
void CListViewCtrlEx::SetItemText(unsigned iItem, unsigned iSubItem, const wchar_t * str) {
	LVITEM item = {};
	item.mask = LVIF_TEXT;
	item.iItem = iItem;
	item.iSubItem = iSubItem;
	item.pszText = const_cast<wchar_t*>(str);
	SetItem(&item);
}
void CListViewCtrlEx::SetItemText8(unsigned item, unsigned subItem, const char * str) {
	SetItemText( item, subItem, pfc::stringcvt::string_os_from_utf8( str ) );
}


DWORD CHeaderCtrlEx::GetItemFormat(int iItem) {
	HDITEM item = {};
	item.mask = HDI_FORMAT;
	if (!this->GetItem(iItem, &item)) return 0;
	return item.fmt;
}
void CHeaderCtrlEx::SetItemFormat(int iItem, DWORD flags) {
	HDITEM item = {};
	item.mask = HDI_FORMAT;
	item.fmt = flags;
	SetItem(iItem, &item);
}

void CHeaderCtrlEx::SetItemSort(int iItem, int direction) {
	DWORD fmtWas = GetItemFormat(iItem);
	DWORD fmt = fmtWas & ~(HDF_SORTDOWN | HDF_SORTUP);
	if (direction > 0) fmt |= HDF_SORTDOWN;
	else if (direction < 0) fmt |= HDF_SORTUP;
	if (fmt != fmtWas) SetItemFormat(iItem, fmt);
}

void CHeaderCtrlEx::SetSingleItemSort(int iItem, int direction) {
	const int total = GetItemCount();
	for (int walk = 0; walk < total; ++walk) {
		SetItemSort(walk, walk == iItem ? direction : 0);
	}
}

void CHeaderCtrlEx::ClearSort() {
	SetSingleItemSort(-1,0);
}

int CListViewCtrlEx::AddGroup(int iGroupID, const wchar_t * header) {
	LVGROUP g = { sizeof(g) };
	g.mask = LVGF_HEADER | LVGF_GROUPID;
	g.pszHeader = const_cast<wchar_t*>( header );
	g.iGroupId = iGroupID;
	return __super::AddGroup(&g);
}
