#pragma once

class CHeaderCtrlEx : public CHeaderCtrl {
public:
	CHeaderCtrlEx(HWND wnd = NULL) : CHeaderCtrl(wnd) {}
	CHeaderCtrlEx const & operator=(HWND wnd) { m_hWnd = wnd; return *this; }

	// Column sort marker operations
	// If they appear to have no effect, you're probably missing Common Controls 6 manifest, see link-CommonControls6.h
	DWORD GetItemFormat(int iItem);
	void SetItemFormat(int iItem, DWORD flags);
	void SetItemSort(int iItem, int direction);
	void SetSingleItemSort(int iItem, int direction);
	void ClearSort();
};
