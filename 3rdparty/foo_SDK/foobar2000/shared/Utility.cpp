#include "shared.h"

#include <mutex>

static std::once_flag g_infinitWaitInit;
static HANDLE g_infinitWaitEvent = NULL;

HANDLE SHARED_EXPORT GetInfiniteWaitEvent() {
	std::call_once(g_infinitWaitInit, [] {
		g_infinitWaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	} );
	return g_infinitWaitEvent;
}

struct createFileRequest {
	TCHAR * lpFileName;
	DWORD dwDesiredAccess;
	DWORD dwShareMode;
	LPSECURITY_ATTRIBUTES lpSecurityAttributes;
	DWORD dwCreationDisposition;
	DWORD dwFlagsAndAttributes;

	HANDLE hResultHandle;

	DWORD dwErrorCode;

	volatile LONG refCounter;
	
	void Delete() {
		free(lpFileName);
		if (hResultHandle != INVALID_HANDLE_VALUE) CloseHandle(hResultHandle);
		delete this;
	}

	void Release() {
		if (InterlockedDecrement(&refCounter) == 0) {
			Delete();
		}
	}
};

static unsigned CALLBACK createFileThread(void* p) {
	createFileRequest * req = reinterpret_cast<createFileRequest*>(p);

	SetLastError(0);
	req->hResultHandle = CreateFile(req->lpFileName, req->dwDesiredAccess, req->dwShareMode, req->lpSecurityAttributes, req->dwCreationDisposition, req->dwFlagsAndAttributes, NULL);
	req->dwErrorCode = GetLastError();

	req->Release();
	return 0;
}


typedef BOOL (WINAPI * pCancelSynchronousIo_t)(HANDLE hThread);
static BOOL myCancelSynchronousIo(HANDLE hThread) {
	pCancelSynchronousIo_t pCancelSynchronousIo = (pCancelSynchronousIo_t) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "CancelSynchronousIo");
	if (pCancelSynchronousIo == NULL) {SetLastError(ERROR_CALL_NOT_IMPLEMENTED); return FALSE;}
	return pCancelSynchronousIo(hThread);
}

HANDLE SHARED_EXPORT CreateFileAbortable(    __in     LPCWSTR lpFileName,
    __in     DWORD dwDesiredAccess,
    __in     DWORD dwShareMode,
    __in_opt LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    __in     DWORD dwCreationDisposition,
    __in     DWORD dwFlagsAndAttributes,
	__in_opt HANDLE hAborter
	) {
	if (hAborter == NULL || hAborter == GetInfiniteWaitEvent()) {
		return CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, NULL);
	}
	switch(WaitForSingleObject(hAborter, 0)) {
	case WAIT_TIMEOUT:
		break;
	case WAIT_OBJECT_0:
		SetLastError(ERROR_OPERATION_ABORTED);
		return INVALID_HANDLE_VALUE;
	default:
		return INVALID_HANDLE_VALUE;
	}

	createFileRequest * req = new createFileRequest();
	req->lpFileName = _tcsdup(lpFileName);
	req->dwDesiredAccess = dwDesiredAccess;
	req->dwShareMode = dwShareMode;
	req->lpSecurityAttributes = lpSecurityAttributes;
	req->dwCreationDisposition = dwCreationDisposition;
	req->dwFlagsAndAttributes = dwFlagsAndAttributes;
	req->hResultHandle = INVALID_HANDLE_VALUE;
	req->dwErrorCode = 0;
	req->refCounter = 2;

	HANDLE hThread = (HANDLE) _beginthreadex(NULL, 0, createFileThread, req, CREATE_SUSPENDED, NULL);
	if (hThread == NULL) {
		req->Delete();
		return INVALID_HANDLE_VALUE;
	}
	SetThreadPriority(hThread, GetThreadPriority(GetCurrentThread()));
	ResumeThread(hThread);
	
	DWORD waitStatus;
	{
		HANDLE waits[2] = {hThread, hAborter};
		waitStatus = WaitForMultipleObjects(2, waits, FALSE, INFINITE);
	}
	HANDLE hRetVal = INVALID_HANDLE_VALUE;
	DWORD dwErrorCode = 0;
	switch(waitStatus) {
	case WAIT_OBJECT_0: // thread completed
		hRetVal = req->hResultHandle;
		req->hResultHandle = INVALID_HANDLE_VALUE;
		dwErrorCode = req->dwErrorCode;
		break;
	case WAIT_OBJECT_0 + 1: // aborted
		dwErrorCode = ERROR_OPERATION_ABORTED;
		myCancelSynchronousIo(hThread);
		break;
	default: // unexpected, use last-error code from WFMO
		dwErrorCode = GetLastError();
		myCancelSynchronousIo(hThread);
		break;
	}
	req->Release();
	CloseHandle(hThread);
	SetLastError(dwErrorCode);
	return hRetVal;
}

namespace pfc {
	BOOL winFormatSystemErrorMessageImpl(pfc::string_base & p_out, DWORD p_code);
	BOOL winFormatSystemErrorMessageHook(pfc::string_base & p_out, DWORD p_code) {
		return winFormatSystemErrorMessageImpl(p_out, p_code);
	}

	void crashHook() {
		uBugCheck(); 
	}
}
