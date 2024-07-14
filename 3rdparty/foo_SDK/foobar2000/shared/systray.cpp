#include "shared.h"
#include <shellapi.h>

void SHARED_EXPORT uFixAmpersandChars(const char * src,pfc::string_base & out)
{
	out.reset();
	while(*src)
	{
		if (*src=='&')
		{
			out.add_string("&&&");
			src++;
			while(*src=='&')
			{
				out.add_string("&&");
				src++;
			}
		}
		else out.add_byte(*(src++));
	}
}

void SHARED_EXPORT uFixAmpersandChars_v2(const char * src,pfc::string_base & out)
{
	out.reset();
	while(*src)
	{
		if (*src=='&')
		{
			out.add_string("&&");
			src++;
		}
		else out.add_byte(*(src++));
	}
}

static BOOL run_action(DWORD action,NOTIFYICONDATA * data)
{
	if (Shell_NotifyIcon(action,data)) return TRUE;
	if (action==NIM_MODIFY)
	{
		if (Shell_NotifyIcon(NIM_ADD,data)) return TRUE;
	}
	return FALSE;
}

extern "C"
{

	BOOL SHARED_EXPORT uShellNotifyIcon(DWORD action,HWND wnd,UINT id,UINT callbackmsg,HICON icon,const char * tip)
	{
		NOTIFYICONDATA nid = {};
		nid.cbSize = sizeof(nid);
		nid.hWnd = wnd;
		nid.uID = id;
		nid.uFlags = 0;
		if (callbackmsg)
		{
			nid.uFlags |= NIF_MESSAGE;
			nid.uCallbackMessage = callbackmsg;
		}
		if (icon)
		{
			nid.uFlags |= NIF_ICON;
			nid.hIcon = icon;
		}			
		if (tip)
		{
			nid.uFlags |= NIF_TIP;
			pfc::stringToBuffer(nid.szTip,pfc::stringcvt::string_os_from_utf8(tip));
		}

		return run_action(action,&nid);
	}

	BOOL SHARED_EXPORT uShellNotifyIconEx(DWORD action,HWND wnd,UINT id,UINT callbackmsg,HICON icon,const char * tip,const char * balloon_title,const char * balloon_msg)
	{
		NOTIFYICONDATA nid = {};
		nid.cbSize = sizeof(nid);
		nid.hWnd = wnd;
		nid.uID = id;
		if (callbackmsg)
		{
			nid.uFlags |= NIF_MESSAGE;
			nid.uCallbackMessage = callbackmsg;
		}
		if (icon)
		{
			nid.uFlags |= NIF_ICON;
			nid.hIcon = icon;
		}			
		if (tip)
		{
			nid.uFlags |= NIF_TIP;
			pfc::stringToBuffer(nid.szTip,pfc::stringcvt::string_os_from_utf8(tip));
		}

		nid.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;
		//if (balloon_title || balloon_msg)
		{
			nid.uFlags |= NIF_INFO;
			if (balloon_title) pfc::stringToBuffer(nid.szInfoTitle,pfc::stringcvt::string_os_from_utf8(balloon_title));
			if (balloon_msg) pfc::stringToBuffer(nid.szInfo,pfc::stringcvt::string_os_from_utf8(balloon_msg));
		}
		return run_action(action,&nid);
	}

}//extern "C"
