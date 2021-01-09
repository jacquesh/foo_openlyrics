#include "stdafx.h"
#include "TypeFind.h"
#include "SmartStrStr.h"

static size_t _ItemCount(HWND wnd) {
	return ListView_GetItemCount(wnd);
}

static const wchar_t * _ItemText(wchar_t * buffer, int bufSize, HWND wnd, size_t index, int subItem) {
	NMLVDISPINFO info = {};
	info.hdr.code = LVN_GETDISPINFO;
	info.hdr.idFrom = GetDlgCtrlID(wnd);
	info.hdr.hwndFrom = wnd;
	info.item.iItem = (int) index;
	info.item.iSubItem = subItem;
	info.item.mask = LVIF_TEXT;
	info.item.pszText = buffer;
	info.item.cchTextMax = bufSize;
	::SendMessage(::GetParent(wnd), WM_NOTIFY, info.hdr.idFrom, reinterpret_cast<LPARAM>(&info));
	if (info.item.pszText == NULL) return L"";
	if ( bufSize > 0 && info.item.pszText == buffer ) buffer[ bufSize - 1 ] = 0;
	return info.item.pszText;
}

LRESULT TypeFind::Handler(NMHDR* hdr, int subItemFrom, int subItemCnt) {
	NMLVFINDITEM * info = reinterpret_cast<NMLVFINDITEM*>(hdr);
	const HWND wnd = hdr->hwndFrom;
	if (info->lvfi.flags & LVFI_NEARESTXY) return -1;
	const size_t count = _ItemCount(wnd);
	if (count == 0) return -1;
	const size_t base = (size_t)info->iStart % count;

	static SmartStrStr tool;
	
	enum {
		bufSize = 1024,
	};
	wchar_t textBuf[bufSize];

	for (size_t walk = 0; walk < count; ++walk) {
		const size_t index = (walk + base) % count;
		for (int subItem = subItemFrom; subItem < subItemFrom + subItemCnt; ++subItem) {
			if (tool.matchHereW(_ItemText(textBuf, bufSize, wnd, index, subItem), info->lvfi.psz)) return (LRESULT)index;
		}
	}
	for (size_t walk = 0; walk < count; ++walk) {
		const size_t index = (walk + base) % count;
		for (int subItem = subItemFrom; subItem < subItemFrom + subItemCnt; ++subItem) {
			if (tool.strStrEndW(_ItemText(textBuf, bufSize, wnd, index, subItem), info->lvfi.psz)) return (LRESULT)index;
		}
	}
	return -1;
}
