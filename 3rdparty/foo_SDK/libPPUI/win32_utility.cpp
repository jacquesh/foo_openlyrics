#include "stdafx.h"
#include "win32_utility.h"
#include "win32_op.h"

unsigned QueryScreenDPI(HWND wnd) {
	HDC dc = GetDC(wnd);
	unsigned ret = GetDeviceCaps(dc, LOGPIXELSY);
	ReleaseDC(wnd, dc);
	return ret;
}
unsigned QueryScreenDPI_X(HWND wnd) {
	HDC dc = GetDC(wnd);
	unsigned ret = GetDeviceCaps(dc, LOGPIXELSX);
	ReleaseDC(wnd, dc);
	return ret;
}
unsigned QueryScreenDPI_Y(HWND wnd) {
	HDC dc = GetDC(wnd);
	unsigned ret = GetDeviceCaps(dc, LOGPIXELSY);
	ReleaseDC(wnd, dc);
	return ret;
}

SIZE QueryScreenDPIEx(HWND wnd) {
	HDC dc = GetDC(wnd);
	SIZE ret = { GetDeviceCaps(dc,LOGPIXELSX), GetDeviceCaps(dc,LOGPIXELSY) };
	ReleaseDC(wnd, dc);
	return ret;
}

void HeaderControl_SetSortIndicator(HWND header_, int column, bool isUp) {
	CHeaderCtrl header(header_);
	const int total = header.GetItemCount();
	for (int walk = 0; walk < total; ++walk) {
		HDITEM item = {}; item.mask = HDI_FORMAT;
		if (header.GetItem(walk, &item)) {
			DWORD newFormat = item.fmt;
			newFormat &= ~(HDF_SORTUP | HDF_SORTDOWN);
			if (walk == column) {
				newFormat |= isUp ? HDF_SORTUP : HDF_SORTDOWN;
			}
			if (newFormat != item.fmt) {
				item.fmt = newFormat;
				header.SetItem(walk, &item);
			}
		}
	}
}

HINSTANCE GetThisModuleHandle() {
	return (HINSTANCE)_AtlBaseModule.m_hInst;
}

WinResourceRef_t WinLoadResource(HMODULE hMod, const TCHAR * name, const TCHAR * type, WORD wLang) {
	SetLastError(0);
	HRSRC res = wLang ? FindResourceEx(hMod, type, name, wLang) : FindResource(hMod, name, type);
	if ( res == NULL ) WIN32_OP_FAIL();
	SetLastError(0);
	HGLOBAL hglob = LoadResource(hMod, res);
	if ( hglob == NULL ) WIN32_OP_FAIL();
	SetLastError(0);
	void * ptr = LockResource(hglob);
	if ( ptr == nullptr ) WIN32_OP_FAIL();
	WinResourceRef_t ref;
	ref.ptr = ptr;
	ref.bytes = SizeofResource(hMod, res);
	return ref;
}

CComPtr<IStream> WinLoadResourceAsStream(HMODULE hMod, const TCHAR * name, const TCHAR * type, WORD wLang) {
	auto res = WinLoadResource(hMod, name, type, wLang );
	auto str = SHCreateMemStream( (const BYTE*) res.ptr, (UINT) res.bytes );
	if ( str == nullptr ) throw std::bad_alloc();
	CComPtr<IStream> ret;
	ret.Attach( str );
	return ret;
}

UINT GetFontHeight(HFONT font)
{
	UINT ret;
	HDC dc = CreateCompatibleDC(0);
	SelectObject(dc, font);
	ret = GetTextHeight(dc);
	DeleteDC(dc);
	return ret;
}

UINT GetTextHeight(HDC dc)
{
	TEXTMETRIC tm;
	POINT pt[2];
	GetTextMetrics(dc, &tm);
	pt[0].x = 0;
	pt[0].y = tm.tmHeight;
	pt[1].x = 0;
	pt[1].y = 0;
	LPtoDP(dc, pt, 2);

	int ret = pt[0].y - pt[1].y;
	return ret > 1 ? (unsigned)ret : 1;
}


LRESULT RelayEraseBkgnd(HWND p_from, HWND p_to, HDC p_dc) {
	LRESULT status;
	POINT pt = { 0, 0 }, pt_old = { 0,0 };
	MapWindowPoints(p_from, p_to, &pt, 1);
	OffsetWindowOrgEx(p_dc, pt.x, pt.y, &pt_old);
	status = SendMessage(p_to, WM_ERASEBKGND, (WPARAM)p_dc, 0);
	SetWindowOrgEx(p_dc, pt_old.x, pt_old.y, 0);
	return status;
}

pfc::string8 EscapeTooltipText(const char * src)
{
	pfc::string8 out;
	while (*src)
	{
		if (*src == '&')
		{
			out.add_string("&&&");
			src++;
			while (*src == '&')
			{
				out.add_string("&&");
				src++;
			}
		} else out.add_byte(*(src++));
	}
	return out;
}

bool IsMenuNonEmpty(HMENU menu) {
	unsigned n, m = GetMenuItemCount(menu);
	for (n = 0; n < m; n++) {
		if (GetSubMenu(menu, n)) return true;
		if (!(GetMenuState(menu, n, MF_BYPOSITION)&MF_SEPARATOR)) return true;
	}
	return false;
}

void SetDefaultMenuItem(HMENU p_menu, unsigned p_id) {
	MENUITEMINFO info = { sizeof(info) };
	info.fMask = MIIM_STATE;
	GetMenuItemInfo(p_menu, p_id, FALSE, &info);
	info.fState |= MFS_DEFAULT;
	SetMenuItemInfo(p_menu, p_id, FALSE, &info);
}

static bool running_under_wine(void) {
	HMODULE module = GetModuleHandle(_T("ntdll.dll"));
	if (!module) return false;
	return GetProcAddress(module, "wine_server_call") != NULL;
}
static bool FetchWineInfoAppend(pfc::string_base & out) {
	typedef const char *(__cdecl *t_wine_get_build_id)(void);
	typedef void(__cdecl *t_wine_get_host_version)(const char **sysname, const char **release);
	const HMODULE ntdll = GetModuleHandle(_T("ntdll.dll"));
	if (ntdll == NULL) return false;
	t_wine_get_build_id wine_get_build_id;
	t_wine_get_host_version wine_get_host_version;
	wine_get_build_id = (t_wine_get_build_id)GetProcAddress(ntdll, "wine_get_build_id");
	wine_get_host_version = (t_wine_get_host_version)GetProcAddress(ntdll, "wine_get_host_version");
	if (wine_get_build_id == NULL || wine_get_host_version == NULL) {
		if (GetProcAddress(ntdll, "wine_server_call") != NULL) {
			out << "wine (unknown version)";
			return true;
		}
		return false;
	}
	const char * sysname = NULL; const char * release = NULL;
	wine_get_host_version(&sysname, &release);
	out << wine_get_build_id() << ", on: " << sysname << " / " << release;
	return true;
}

static void GetOSVersionStringAppend(pfc::string_base & out) {

	if (FetchWineInfoAppend(out)) return;

	OSVERSIONINFO ver = {}; ver.dwOSVersionInfoSize = sizeof(ver);
	WIN32_OP(GetVersionEx(&ver));
	SYSTEM_INFO info = {};
	GetNativeSystemInfo(&info);

	out << "Windows " << (int)ver.dwMajorVersion << "." << (int)ver.dwMinorVersion << "." << (int)ver.dwBuildNumber;
	if (ver.szCSDVersion[0] != 0) out << " " << pfc::stringcvt::string_utf8_from_os(ver.szCSDVersion, PFC_TABSIZE(ver.szCSDVersion));

	switch (info.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_AMD64:
		out << " x64"; break;
	case PROCESSOR_ARCHITECTURE_IA64:
		out << " IA64"; break;
	case PROCESSOR_ARCHITECTURE_INTEL:
		out << " x86"; break;
	}
}

void GetOSVersionString(pfc::string_base & out) {
	out.reset(); GetOSVersionStringAppend(out);
}
WORD GetOSVersionCode() {
	OSVERSIONINFO ver = {sizeof(ver)};
	WIN32_OP_D(GetVersionEx(&ver));
	
	DWORD ret = ver.dwMinorVersion;
	ret += ver.dwMajorVersion << 8;

	return (WORD)ret;
}


POINT GetCursorPos() {
	POINT pt;
	WIN32_OP_D( GetCursorPos(&pt) );
	return pt;
}
