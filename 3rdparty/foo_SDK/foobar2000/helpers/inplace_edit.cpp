#include "stdafx.h"
#include "inplace_edit.h"

// Functionality moved to libPPUI

namespace InPlaceEdit {
	static reply_t wrapCN( completion_notify::ptr cn ) {
		return [cn](unsigned code) { cn->on_completion(code); };
	}
	HWND Start(HWND p_parentwnd, const RECT & p_rect, bool p_multiline, pfc::rcptr_t<pfc::string_base> p_content, completion_notify_ptr p_notify) {
		return Start(p_parentwnd, p_rect, p_multiline, p_content, wrapCN(p_notify) );
	}

	HWND StartEx(HWND p_parentwnd, const RECT & p_rect, unsigned p_flags, pfc::rcptr_t<pfc::string_base> p_content, completion_notify_ptr p_notify, IUnknown * ACData, DWORD ACOpts ) {
		return StartEx(p_parentwnd, p_rect, p_flags, p_content, wrapCN(p_notify), ACData, ACOpts );
	}

	void Start_FromListView(HWND p_listview, unsigned p_item, unsigned p_subitem, unsigned p_linecount, pfc::rcptr_t<pfc::string_base> p_content, completion_notify_ptr p_notify) {
		Start_FromListView(p_listview,p_item, p_subitem, p_linecount, p_content, wrapCN(p_notify) );
	}
	void Start_FromListViewEx(HWND p_listview, unsigned p_item, unsigned p_subitem, unsigned p_linecount, unsigned p_flags, pfc::rcptr_t<pfc::string_base> p_content, completion_notify_ptr p_notify) {
		Start_FromListViewEx(p_listview, p_item, p_subitem, p_linecount, p_flags, p_content, wrapCN(p_notify) );
	}

}