#pragma once

#include <functional>

namespace InPlaceEdit {

	enum {
		KEditAborted = 0,
		KEditTab,
		KEditShiftTab,
		KEditEnter,
		KEditLostFocus,

		KEditMaskReason = 0xFF,
		KEditFlagContentChanged = 0x100,

		KFlagReadOnly = 1 << 0,
		KFlagMultiLine = 1 << 1,
		KFlagAlignCenter = 1 << 2,
		KFlagAlignRight = 1 << 3,
		KFlagNumber = 1 << 4,
		KFlagNumberSigned = 1 << 5,
		
		KFlagCombo = 1 << 8, // FOR INTERNAL USE
	};
	
	typedef std::function< void (unsigned) > reply_t;
	
	typedef std::function< void(unsigned, unsigned) > comboReply_t; // status, index (UINT_MAX if n/a)

	HWND Start(HWND p_parentwnd, const RECT & p_rect, bool p_multiline, pfc::rcptr_t<pfc::string_base> p_content, reply_t p_notify);

	HWND StartEx(HWND p_parentwnd, const RECT & p_rect, unsigned p_flags, pfc::rcptr_t<pfc::string_base> p_content, reply_t p_notify, IUnknown * ACData = NULL, DWORD ACOpts = 0);


	HWND StartCombo(HWND p_parentwnd, const RECT & p_rect, unsigned p_flags, pfc::string_list_const & data, unsigned iDefault, comboReply_t p_notify);

	void Start_FromListView(HWND p_listview, unsigned p_item, unsigned p_subitem, unsigned p_linecount,  pfc::rcptr_t<pfc::string_base> p_content, reply_t p_notify);
	void Start_FromListViewEx(HWND p_listview, unsigned p_item, unsigned p_subitem, unsigned p_linecount, unsigned p_flags, pfc::rcptr_t<pfc::string_base> p_content, reply_t p_notify);

	bool TableEditAdvance(unsigned & p_item, unsigned & p_column, unsigned p_item_count, unsigned p_column_count, unsigned p_whathappened);
	bool TableEditAdvance_ListView(HWND p_listview, unsigned p_column_base, unsigned & p_item, unsigned & p_column, unsigned p_item_count, unsigned p_column_count, unsigned p_whathappened);

}
