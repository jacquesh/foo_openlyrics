#pragma once

class CListViewCtrlEx : public CListViewCtrl {
public:
	CListViewCtrlEx( HWND wnd = NULL ) : CListViewCtrl(wnd) {}
	CListViewCtrlEx const & operator=( HWND wnd ) { m_hWnd = wnd; return *this; }
	unsigned InsertColumnEx(unsigned index, const wchar_t * name, unsigned widthDLU);
	unsigned AddColumnEx( const wchar_t * name, unsigned widthDLU );
	void FixContextMenuPoint( CPoint & pt );
	unsigned GetColunnCount();

	unsigned InsertString( unsigned index, const wchar_t * str );
	unsigned InsertString8( unsigned index, const char * str );
	unsigned AddString( const wchar_t * str );
	unsigned AddString8(const char * str);
	void SetItemText(unsigned item, unsigned subItem, const wchar_t * str );
	void SetItemText8(unsigned item, unsigned subItem, const char * str );

	void AutoSizeColumn( int iCol ) { SetColumnWidth(iCol, LVSCW_AUTOSIZE) ;}
	int AddGroup(int iGroupID, const wchar_t * header);
};

// BOOL HandleLVKeyDownMod()
#define LVN_KEYDOWN_MOD_HANDLER(id, key, mod, func) \
	if (uMsg == WM_NOTIFY && LVN_KEYDOWN == ((LPNMHDR)lParam)->code && id == ((LPNMHDR)lParam)->idFrom && ((LPNMLVKEYDOWN)lParam)->wVKey == (key) && GetHotkeyModifierFlags() == (mod)) \
	{ \
		SetMsgHandled(TRUE); \
		lResult = func()?1:0; \
		if(IsMsgHandled()) \
			return TRUE; \
	}

// BOOL HandleLVCopy()
#define LVN_COPY_HANDLER(id, func) LVN_KEYDOWN_MOD_HANDLER(id, 'C', MOD_CONTROL, func)
