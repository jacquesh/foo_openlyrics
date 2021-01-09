#include "stdafx.h"
#include "win32_op.h"
#include <assert.h>

PFC_NORETURN PFC_NOINLINE void WIN32_OP_FAIL() {
	const DWORD code = GetLastError();
	PFC_ASSERT(code != NO_ERROR);
	throw exception_win32(code);
}

PFC_NORETURN PFC_NOINLINE void WIN32_OP_FAIL_CRITICAL(const char * what) {
#if PFC_DEBUG
	const DWORD code = GetLastError();
	PFC_ASSERT(code != NO_ERROR);
#endif
	pfc::crash();
#if 0
	pfc::string_formatter msg; msg << what << " failure #" << (uint32_t)code;
	TRACK_CODE(msg.get_ptr(), uBugCheck());
#endif
}

#if PFC_DEBUG
void WIN32_OP_D_FAIL(const wchar_t * _Message, const wchar_t *_File, unsigned _Line) {
	const DWORD code = GetLastError();
	pfc::array_t<wchar_t> msgFormatted; msgFormatted.set_size(pfc::strlen_t(_Message) + 64);
	wsprintfW(msgFormatted.get_ptr(), L"%s (code: %u)", _Message, code);
	if (IsDebuggerPresent()) {
		OutputDebugString(TEXT("WIN32_OP_D() failure:\n"));
		OutputDebugString(msgFormatted.get_ptr());
		OutputDebugString(TEXT("\n"));
		pfc::crash();
	}
	_wassert(msgFormatted.get_ptr(), _File, _Line);
}
#endif
