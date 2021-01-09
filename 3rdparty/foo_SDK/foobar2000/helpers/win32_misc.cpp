#include "stdafx.h"
#include "win32_misc.h"

#ifdef FOOBAR2000_MOBILE_WINDOWS
#include <pfc/pp-winapi.h>
#endif

#ifdef _WIN32

mutexScope::mutexScope(HANDLE hMutex_, abort_callback & abort) : hMutex(hMutex_) {
	HANDLE h[2] = { hMutex, abort.get_abort_event() };
	switch (WaitForMultipleObjectsEx(2, h, FALSE, INFINITE, FALSE)) {
	case WAIT_OBJECT_0:
		break; // and enter
	case WAIT_OBJECT_0 + 1:
		throw exception_aborted();
	default:
		uBugCheck();
	}
}
mutexScope::~mutexScope() {
	ReleaseMutex(hMutex);
}

CMutex::CMutex(const TCHAR * name) {
	WIN32_OP_CRITICAL("CreateMutex", m_hMutex = CreateMutex(NULL, FALSE, name));
}
CMutex::~CMutex() {
	CloseHandle(m_hMutex);
}

void CMutex::AcquireByHandle(HANDLE hMutex, abort_callback & aborter) {
	SetLastError(0);
	HANDLE hWait[2] = { hMutex, aborter.get_abort_event() };
	switch (WaitForMultipleObjects(2, hWait, FALSE, INFINITE)) {
	case WAIT_FAILED:
		WIN32_OP_FAIL_CRITICAL("WaitForSingleObject");
	case WAIT_OBJECT_0:
		return;
	case WAIT_OBJECT_0 + 1:
		PFC_ASSERT(aborter.is_aborting());
		throw exception_aborted();
	default:
		uBugCheck();
	}
}

void CMutex::Acquire(abort_callback& aborter) {
	AcquireByHandle(Handle(), aborter);
}

void CMutex::Release() {
	ReleaseMutex(Handle());
}


CMutexScope::CMutexScope(CMutex & mutex, DWORD timeOutMS, const char * timeOutBugMsg) : m_mutex(mutex) {
	SetLastError(0);
	const unsigned div = 4;
	for (unsigned walk = 0; walk < div; ++walk) {
		switch (WaitForSingleObject(m_mutex.Handle(), timeOutMS / div)) {
		case WAIT_FAILED:
			WIN32_OP_FAIL_CRITICAL("WaitForSingleObject");
		case WAIT_OBJECT_0:
			return;
		case WAIT_TIMEOUT:
			break;
		default:
			uBugCheck();
		}
	}
	TRACK_CODE(timeOutBugMsg, uBugCheck());
}

CMutexScope::CMutexScope(CMutex & mutex) : m_mutex(mutex) {
	SetLastError(0);
	switch (WaitForSingleObject(m_mutex.Handle(), INFINITE)) {
	case WAIT_FAILED:
		WIN32_OP_FAIL_CRITICAL("WaitForSingleObject");
	case WAIT_OBJECT_0:
		return;
	default:
		uBugCheck();
	}
}

CMutexScope::CMutexScope(CMutex & mutex, abort_callback & aborter) : m_mutex(mutex) {
	mutex.Acquire(aborter);
}

CMutexScope::~CMutexScope() {
	ReleaseMutex(m_mutex.Handle());
}

#endif

#ifdef FOOBAR2000_DESKTOP_WINDOWS

void registerclass_scope_delayed::toggle_on(UINT p_style,WNDPROC p_wndproc,int p_clsextra,int p_wndextra,HICON p_icon,HCURSOR p_cursor,HBRUSH p_background,const TCHAR * p_class_name,const TCHAR * p_menu_name) {
	toggle_off();
	WNDCLASS wc = {};
	wc.style = p_style;
	wc.lpfnWndProc = p_wndproc;
	wc.cbClsExtra = p_clsextra;
	wc.cbWndExtra = p_wndextra;
	wc.hInstance = core_api::get_my_instance();
	wc.hIcon = p_icon;
	wc.hCursor = p_cursor;
	wc.hbrBackground = p_background;
	wc.lpszMenuName = p_menu_name;
	wc.lpszClassName = p_class_name;
	WIN32_OP_CRITICAL("RegisterClass", (m_class = RegisterClass(&wc)) != 0);
}

void registerclass_scope_delayed::toggle_off() {
	if (m_class != 0) {
		UnregisterClass((LPCTSTR)m_class,core_api::get_my_instance());
		m_class = 0;
	}
}

void CModelessDialogEntry::Set(HWND p_new) {
	auto api = modeless_dialog_manager::get();
	if (m_wnd) api->remove(m_wnd);
	m_wnd = p_new;
	if (m_wnd) api->add(m_wnd);
}

OleInitializeScope::OleInitializeScope() {
	if (FAILED(OleInitialize(NULL))) throw pfc::exception("OleInitialize() failure");
}
OleInitializeScope::~OleInitializeScope() {
	OleUninitialize();
}

CoInitializeScope::CoInitializeScope() {
	if (FAILED(CoInitialize(NULL))) throw pfc::exception("CoInitialize() failed");
}
CoInitializeScope::CoInitializeScope(DWORD params) {
	if (FAILED(CoInitializeEx(NULL, params))) throw pfc::exception("CoInitialize() failed");
}
CoInitializeScope::~CoInitializeScope() {
	CoUninitialize();
}

void winLocalFileScope::open(const char * inPath, file::ptr inReader, abort_callback & aborter) {
	close();
	if (inPath != NULL) {
		if (_extract_native_path_ptr(inPath)) {
			pfc::string8 prefixed;
			pfc::winPrefixPath(prefixed, inPath);
			m_path = pfc::stringcvt::string_wide_from_utf8(prefixed);
			m_isTemp = false;
			return;
		}
	}

	pfc::string8 tempPath;
	if (!uGetTempPath(tempPath)) uBugCheck();
	tempPath.add_filename(PFC_string_formatter() << pfc::print_guid(pfc::createGUID()) << ".rar");

	m_path = pfc::stringcvt::string_wide_from_utf8(tempPath);

	if (inReader.is_empty()) {
		if (inPath == NULL) uBugCheck();
		inReader = fileOpenReadExisting(inPath, aborter, 1.0);
	}

	file::ptr writer = fileOpenWriteNew(PFC_string_formatter() << "file://" << tempPath, aborter, 1.0);
	file::g_transfer_file(inReader, writer, aborter);
	m_isTemp = true;
}
void winLocalFileScope::close() {
	if (m_isTemp && m_path.length() > 0) {
		pfc::lores_timer timer;
		timer.start();
		for (;;) {
			if (DeleteFile(m_path.c_str())) break;
			if (timer.query() > 1.0) break;
			Sleep(10);
		}
	}
	m_path.clear();
}

bool IsWindowsS() {
	bool ret = false;
#if FB2K_TARGET_MICROSOFT_STORE
	static bool cached = false;
	static bool inited = false;
	if ( inited ) {
		ret = cached;
	} else {
		HKEY key;
		if (RegOpenKey(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\CI\\Policy", &key) == 0) {
			DWORD dwVal = 0, dwType;
			DWORD valSize;
			valSize = sizeof(dwVal);
			if (RegQueryValueEx(key, L"SkuPolicyRequired", nullptr, &dwType, (LPBYTE)&dwVal, &valSize) == 0) {
				if (dwType == REG_DWORD && dwVal != 0) ret = true;
			}
			RegCloseKey(key);
		}
		cached = ret;
		inited = true;
	}
#endif
	return ret;
}

WORD GetOSVersion() {
	// wrap libPPUI function
	return ::GetOSVersionCode();
}
#endif // FOOBAR2000_DESKTOP_WINDOWS
