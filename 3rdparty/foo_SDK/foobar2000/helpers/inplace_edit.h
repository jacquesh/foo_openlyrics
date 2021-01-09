#pragma once

#include <libPPUI/InPlaceEdit.h>

namespace InPlaceEdit {

	HWND Start(HWND p_parentwnd,const RECT & p_rect,bool p_multiline,pfc::rcptr_t<pfc::string_base> p_content,completion_notify_ptr p_notify);

	HWND StartEx(HWND p_parentwnd,const RECT & p_rect,unsigned p_flags,pfc::rcptr_t<pfc::string_base> p_content,completion_notify_ptr p_notify, IUnknown * ACData = NULL, DWORD ACOpts = 0);

	void Start_FromListView(HWND p_listview,unsigned p_item,unsigned p_subitem,unsigned p_linecount,pfc::rcptr_t<pfc::string_base> p_content,completion_notify_ptr p_notify);
	void Start_FromListViewEx(HWND p_listview,unsigned p_item,unsigned p_subitem,unsigned p_linecount,unsigned p_flags,pfc::rcptr_t<pfc::string_base> p_content,completion_notify_ptr p_notify);
}
