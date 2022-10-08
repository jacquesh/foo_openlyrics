#pragma once

#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>

typedef HDITEMA uHDITEM;
typedef TOOLINFOA uTOOLINFO;
typedef REBARBANDINFOA uREBARBANDINFO;

#define RECT_CX(rc) ((rc).right - (rc).left)
#define RECT_CY(rc) ((rc).bottom - (rc).top)

// set insert to false to set the item instead
bool uRebar_InsertItem(HWND wnd, int n, uREBARBANDINFO* rbbi, bool insert = true);

// fixes '&' characters also, set insert to false to set the item instead
int uTabCtrl_InsertItemText(HWND wnd, int idx, const char* text, bool insert = true);

// get rect of wnd in wnd_parent coordinates
inline void GetRelativeRect(HWND wnd, HWND wnd_parent, RECT* rc)
{
    GetWindowRect(wnd, rc);
    MapWindowPoints(HWND_DESKTOP, wnd_parent, reinterpret_cast<LPPOINT>(rc), 2);
}

bool uComboBox_GetText(HWND combo, UINT index, pfc::string_base& out);
bool uStatus_SetText(HWND wnd, int part, const char* text);

HFONT uCreateIconFont();
HFONT uCreateMenuFont(bool vertical = false);

void uGetMenuFont(LOGFONT* p_lf);
void uGetIconFont(LOGFONT* p_lf);

inline void GetMessagePos(LPPOINT pt)
{
    DWORD mp = GetMessagePos();
    pt->x = GET_X_LPARAM(mp);
    pt->y = GET_Y_LPARAM(mp);
}

BOOL uFormatMessage(DWORD dw_error, pfc::string_base& out);

inline BOOL uGetLastErrorMessage(pfc::string_base& out)
{
    return uFormatMessage(GetLastError(), out);
}

HWND uFindParentPopup(HWND wnd_child);

#if (WINVER >= 0x0500)
typedef MONITORINFOEXA uMONITORINFOEX, *uLPMONITORINFOEX;

BOOL uGetMonitorInfo(HMONITOR monitor, LPMONITORINFO lpmi);
// pt_parent is in parent window coordinates!
HWND uRecursiveChildWindowFromPoint(HWND parent, POINT pt_parent);
#endif

[[deprecated("No longer maintained.")]] int uHeader_InsertItem(HWND wnd, int n, uHDITEM* hdi, bool insert = true);
[[deprecated("No longer maintained.")]] int uHeader_SetItemText(HWND wnd, int n, const char* text);
[[deprecated("No longer maintained.")]] int uHeader_SetItemWidth(HWND wnd, int n, UINT cx);
[[deprecated("No longer maintained.")]] bool uToolTip_AddTool(HWND wnd, uTOOLINFO* ti, bool update = false);
[[deprecated("No longer maintained.")]] bool uComboBox_SelectString(HWND combo, const char* src);

namespace win32_helpers {

void send_message_to_direct_children(HWND wnd_parent, UINT msg, WPARAM wp, LPARAM lp);
int message_box(HWND wnd, const TCHAR* text, const TCHAR* caption, UINT type);

[[deprecated("No longer maintained.")]] void send_message_to_all_children(
    HWND wnd_parent, UINT msg, WPARAM wp, LPARAM lp);
[[deprecated("No longer maintained.")]] bool tooltip_add_tool(HWND wnd, TOOLINFO* ti, bool update = false);

}; // namespace win32_helpers

namespace uie::win32 {

LRESULT paint_background_using_parent(HWND wnd, HDC dc, bool use_wm_printclient);

}
