#include "StdAfx.h"

#ifdef FOOBAR2000_DESKTOP_WINDOWS

#include "window_placement_helper.h"
#include <SDK/config_object.h>

static bool g_is_enabled()
{
	return standard_config_objects::query_remember_window_positions();
}

static BOOL CALLBACK __MonitorEnumProc(
  HMONITOR hMonitor,  // handle to display monitor
  HDC hdcMonitor,     // handle to monitor DC
  LPRECT lprcMonitor, // monitor intersection rectangle
  LPARAM dwData       // data
  ) {
	RECT * clip = (RECT*)dwData;
	RECT newclip;
	if (UnionRect(&newclip,clip,lprcMonitor)) {
		*clip = newclip;
	}
	return TRUE;
}

static bool test_rect(const RECT * rc) {
	RECT clip = {};
	if (EnumDisplayMonitors(NULL,NULL,__MonitorEnumProc,(LPARAM)&clip)) {
		const LONG sanitycheck = 4;
		const LONG cwidth = clip.right - clip.left;
		const LONG cheight = clip.bottom - clip.top;
		
		const LONG width = rc->right - rc->left;
		const LONG height = rc->bottom - rc->top;

		if (width > cwidth * sanitycheck || height > cheight * sanitycheck) return false;
	}
	
	return MonitorFromRect(rc,MONITOR_DEFAULTTONULL) != NULL;
}


bool cfg_window_placement::read_from_window(HWND window)
{
	WINDOWPLACEMENT wp = {};
	if (g_is_enabled()) {
		wp.length = sizeof(wp);
		if (!GetWindowPlacement(window,&wp)) {
			PFC_ASSERT(!"GetWindowPlacement fail!");
			memset(&wp,0,sizeof(wp));
		} else {
			// bad, breaks with taskbar on top
			/*if (wp.showCmd == SW_SHOWNORMAL) {
				GetWindowRect(window, &wp.rcNormalPosition);
			}*/

			if ( !IsWindowVisible( window ) ) wp.showCmd = SW_HIDE;
		}
		/*else
		{
			if (!IsWindowVisible(window)) wp.showCmd = SW_HIDE;
		}*/
	}

	try { set(wp); } catch(...) {} // this tends to be called often / we really couldn't care less about this failing

	return wp.length == sizeof(wp);
}

bool applyWindowPlacement(HWND window, WINDOWPLACEMENT const& data, bool allowHidden) {
	bool ret = false;
	if (data.length == sizeof(data) && test_rect(&data.rcNormalPosition))
	{
		if (allowHidden || data.showCmd != SW_HIDE) {
			if (data.showCmd == SW_HIDE && (data.flags & WPF_RESTORETOMAXIMIZED)) {
				// Special case of hidden-from-maximized
				auto fix = data;
				fix.showCmd = SW_SHOWMINIMIZED;
				if (SetWindowPlacement(window, &fix)) {
					ShowWindow(window, SW_HIDE);
					ret = true;
				}
			} else {
				if (SetWindowPlacement(window, &data)) {
					ret = true;
				}
			}
		}
	}
	return ret;
}

bool cfg_window_placement::apply_to_window(HWND window, bool allowHidden) {
	bool ret = false;
	if (g_is_enabled())
	{
		auto data = get();
		ret = applyWindowPlacement(window, data, allowHidden);
	}

	return ret;
}

void cfg_window_placement::on_window_creation_silent(HWND window) {
}
bool cfg_window_placement::on_window_creation(HWND window, bool allowHidden) {
	
	return apply_to_window(window, allowHidden);
}


void cfg_window_placement::on_window_destruction(HWND window)
{
	read_from_window(window);
}

static BOOL SetWindowSize(HWND p_wnd,unsigned p_x,unsigned p_y)
{
	if (p_x != ~0 && p_y != ~0)
		return SetWindowPos(p_wnd,0,0,0,p_x,p_y,SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
	else
		return FALSE;
}

bool cfg_window_size::apply_to_window(HWND p_wnd) {
	bool ret = false;

	if (g_is_enabled())
	{
		auto v = get();
		if (v.cx > 0 && v.cy > 0) {
			if (SetWindowSize(p_wnd, v.cx, v.cy)) ret = true;
		}

	}
	return ret;
}
bool cfg_window_size::on_window_creation(HWND p_wnd)
{
	return apply_to_window(p_wnd);
}

void cfg_window_size::on_window_destruction(HWND p_wnd)
{
	read_from_window(p_wnd);
}

bool cfg_window_size::read_from_window(HWND p_wnd)
{
	SIZE val = {};
	if (g_is_enabled())
	{
		RECT r;
		if (GetWindowRect(p_wnd,&r))
		{
			val.cx = r.right - r.left;
			val.cy = r.bottom - r.top;
		}
	}

	set(val);
	return val.cx > 0 && val.cy > 0;
}

#endif // FOOBAR2000_DESKTOP_WINDOWS
