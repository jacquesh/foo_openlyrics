#pragma once

#ifdef _WIN32


#include <libPPUI/win32_op.h>
#include <libPPUI/win32_utility.h>


class mutexScope {
public:
	mutexScope(HANDLE hMutex_, abort_callback & abort);
	~mutexScope();
private:
	PFC_CLASS_NOT_COPYABLE_EX(mutexScope);
	HANDLE hMutex;
};

#ifdef FOOBAR2000_DESKTOP_WINDOWS

class registerclass_scope_delayed {
public:
	registerclass_scope_delayed() {}
	bool is_registered() const {return m_class != 0;}
	void toggle_on(UINT p_style,WNDPROC p_wndproc,int p_clsextra,int p_wndextra,HICON p_icon,HCURSOR p_cursor,HBRUSH p_background,const TCHAR * p_classname,const TCHAR * p_menuname);
	void toggle_off();
	ATOM get_class() const {return m_class;}

	~registerclass_scope_delayed() {toggle_off();}
private:
	registerclass_scope_delayed(const registerclass_scope_delayed &) = delete;
	const registerclass_scope_delayed & operator=(const registerclass_scope_delayed &) = delete;

	ATOM m_class = 0;
};


typedef CGlobalLockScope CGlobalLock; // compatibility

class OleInitializeScope {
public:
	OleInitializeScope();
	~OleInitializeScope();

private:
	PFC_CLASS_NOT_COPYABLE_EX(OleInitializeScope);
};

class CoInitializeScope {
public:
	CoInitializeScope();
	CoInitializeScope(DWORD params);
	~CoInitializeScope();
private:
	PFC_CLASS_NOT_COPYABLE_EX(CoInitializeScope)
};

WORD GetOSVersion();

#if _WIN32_WINNT >= 0x501
#define WS_EX_COMPOSITED_Safe() WS_EX_COMPOSITED
#else
static DWORD WS_EX_COMPOSITED_Safe() {
	return (GetOSVersion() < 0x501) ? 0 : 0x02000000L;
}
#endif


class CModelessDialogEntry {
public:
	CModelessDialogEntry() : m_wnd() {}
	CModelessDialogEntry(HWND p_wnd) : m_wnd() {Set(p_wnd);}
	~CModelessDialogEntry() {Set(NULL);}

	void Set(HWND p_new);
private:
	PFC_CLASS_NOT_COPYABLE_EX(CModelessDialogEntry);
	HWND m_wnd;
};

class CDLL {
public:
#ifdef _DEBUG
	static LPTOP_LEVEL_EXCEPTION_FILTER _GetEH() {
		LPTOP_LEVEL_EXCEPTION_FILTER rv = SetUnhandledExceptionFilter(NULL);
		SetUnhandledExceptionFilter(rv);
		return rv;
	}
#endif
	CDLL(const wchar_t * Name) : hMod() {
		Load(Name);
	}
	CDLL() : hMod() {}
	void Load(const wchar_t * Name) {
		PFC_ASSERT( hMod == NULL );
#ifdef _DEBUG
		auto handlerBefore = _GetEH();
#endif
		WIN32_OP( hMod = LoadLibrary(Name) );
#ifdef _DEBUG
		PFC_ASSERT( handlerBefore == _GetEH() );
#endif
	}


	~CDLL() {
		if (hMod) FreeLibrary(hMod);
	}
	template<typename funcptr_t> void Bind(funcptr_t & outFunc, const char * name) {
		WIN32_OP( outFunc = (funcptr_t)GetProcAddress(hMod, name) );
	}

	HMODULE hMod;

	PFC_CLASS_NOT_COPYABLE_EX(CDLL);
};

class winLocalFileScope {
public:
	void open(const char * inPath, file::ptr inReader, abort_callback & aborter);
	void close();

	winLocalFileScope() {}
	winLocalFileScope(const char * inPath, file::ptr inReader, abort_callback & aborter) : m_isTemp() {
		open(inPath, inReader, aborter);
	}

	~winLocalFileScope() {
		close();
	}

	const wchar_t * Path() const { return m_path.c_str(); }
	bool isTemp() const { return m_isTemp; }
private:
	bool m_isTemp = false;
	std::wstring m_path;
};

#endif // FOOBAR2000_DESKTOP_WINDOWS


class CMutex {
public:
	CMutex(const TCHAR * name = NULL);
	~CMutex();
	HANDLE Handle() {return m_hMutex;}
	static void AcquireByHandle( HANDLE hMutex, abort_callback & aborter );
	void Acquire( abort_callback& aborter );
	void Release();
private:
	CMutex(const CMutex&); void operator=(const CMutex&);
	HANDLE m_hMutex;
};

class CMutexScope {
public:
	CMutexScope(CMutex & mutex, DWORD timeOutMS, const char * timeOutBugMsg);
	CMutexScope(CMutex & mutex);
	CMutexScope(CMutex & mutex, abort_callback & aborter);
	~CMutexScope();
private:
	CMutexScope(const CMutexScope &); void operator=(const CMutexScope&);
	CMutex & m_mutex;
};

bool IsWindowsS();

#endif // _WIN32
