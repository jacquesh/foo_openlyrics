#pragma once

class uDebugLog_ : public pfc::string_formatter {
public:
    ~uDebugLog_() {*this << "\n"; uOutputDebugString(get_ptr());}
};

#define FB2K_DebugLog() uDebugLog_()._formatter()
#define uDebugLog() FB2K_DebugLog()

// since fb2k 1.5
namespace fb2k {
	class panicHandler {
	public:
		virtual void onPanic() = 0;
	};
}

// since fb2k 1.5
extern "C" 
{
	void SHARED_EXPORT uAddPanicHandler(fb2k::panicHandler*);
	void SHARED_EXPORT uRemovePanicHandler(fb2k::panicHandler*);
}

extern "C"
{
#ifdef _WIN32
	LPCSTR SHARED_EXPORT uGetCallStackPath();
	LONG SHARED_EXPORT uExceptFilterProc(LPEXCEPTION_POINTERS param);
#endif

	PFC_NORETURN void SHARED_EXPORT uBugCheck();

#if PFC_DEBUG
	void SHARED_EXPORT fb2kDebugSelfTest();
#endif

#ifdef _WIN32
    static inline void uAddDebugEvent(const char * msg) {uPrintCrashInfo_OnEvent(msg, strlen(msg));}
#else
    static inline void uAddDebugEvent( const char * msg ) { uOutputDebugString(pfc::format(msg, "\n")); }
#endif
}

#ifdef _WIN32
class uCallStackTracker {
	t_size param;
public:
	explicit SHARED_EXPORT uCallStackTracker(const char* name);
	SHARED_EXPORT ~uCallStackTracker();
};
#endif

#if FB2K_SUPPORT_CRASH_LOGS

#define TRACK_CALL(X) uCallStackTracker TRACKER__##X(#X)
#define TRACK_CALL_TEXT(X) uCallStackTracker TRACKER__BLAH(X)
#define TRACK_CODE(description,code) {uCallStackTracker __call_tracker(description); code;}

#else // FB2K_SUPPORT_CRASH_LOGS

#define TRACK_CALL(X)
#define TRACK_CALL_TEXT(X)
#define TRACK_CODE(description,code) {code;}

#endif // FB2K_SUPPORT_CRASH_LOGS

#if FB2K_SUPPORT_CRASH_LOGS
inline int uExceptFilterProc_inline(LPEXCEPTION_POINTERS param) {
	uDumpCrashInfo(param);
	TerminateProcess(GetCurrentProcess(), 0);
	return 0;// never reached
}
#endif


#define FB2K_DYNAMIC_ASSERT( X ) { if (!(X)) uBugCheck(); }

#define __except_instacrash __except(uExceptFilterProc(GetExceptionInformation()))
#if FB2K_SUPPORT_CRASH_LOGS
#define fb2k_instacrash_scope(X) __try { X; } __except_instacrash {}
#else
#define fb2k_instacrash_scope(X) {X;}
#endif

#if 0 // no longer used
PFC_NORETURN inline void fb2kCriticalError(DWORD code, DWORD argCount = 0, const ULONG_PTR * args = NULL) {
	fb2k_instacrash_scope( RaiseException(code,EXCEPTION_NONCONTINUABLE,argCount,args); );
}
PFC_NORETURN inline void fb2kDeadlock() {
	fb2kCriticalError(0x63d81b66);
}
#endif

#ifdef _WIN32
inline void fb2kWaitForCompletion(HANDLE hEvent) {
	switch(WaitForSingleObject(hEvent, INFINITE)) {
	case WAIT_OBJECT_0:
		return;
	default:
		uBugCheck();
	}
}

inline void fb2kWaitForThreadCompletion(HANDLE hWaitFor, DWORD threadID) {
	(void) threadID;
	switch(WaitForSingleObject(hWaitFor, INFINITE)) {
	case WAIT_OBJECT_0:
		return;
	default:
		uBugCheck();
	}
}

inline void fb2kWaitForThreadCompletion2(HANDLE hWaitFor, HANDLE hThread, DWORD threadID) {
	(void)threadID; (void)hThread;
	switch(WaitForSingleObject(hWaitFor, INFINITE)) {
	case WAIT_OBJECT_0:
		return;
	default:
		uBugCheck();
	}
}


inline void __cdecl _OverrideCrtAbort_handler(int signal) {
	const ULONG_PTR args[] = {(ULONG_PTR)signal};
	RaiseException(0x6F8E1DC8 /* random GUID */, EXCEPTION_NONCONTINUABLE, _countof(args), args);
}

static void __cdecl _PureCallHandler() {
	RaiseException(0xf6538887 /* random GUID */, EXCEPTION_NONCONTINUABLE, 0, 0);
}

static void _InvalidParameter(
   const wchar_t * expression,
   const wchar_t * function, 
   const wchar_t * file, 
   unsigned int line,
   uintptr_t pReserved
) {
	(void)pReserved; (void) line; (void) file; (void) function; (void) expression;
	RaiseException(0xd142b808 /* random GUID */, EXCEPTION_NONCONTINUABLE, 0, 0);
}

inline void OverrideCrtAbort() {
	const int signals[] = {SIGINT, SIGTERM, SIGBREAK, SIGABRT};
	for(size_t i=0; i<_countof(signals); i++) signal(signals[i], _OverrideCrtAbort_handler);
	_set_abort_behavior(0, UINT_MAX);

	_set_purecall_handler(_PureCallHandler);
	_set_invalid_parameter_handler(_InvalidParameter);
}
#endif

namespace fb2k {
#ifdef _WIN32
	PFC_NORETURN inline void crashWithMessage(const char * msg) {
		uAddDebugEvent(msg);
		uBugCheck();
	}
#else
    void crashWithMessage [[noreturn]] (const char*);
#endif
}


// implement me
#define FB2K_TRACE_ENABLED 0

#define FB2K_TRACE_FORCED(X)
#define FB2K_TRACE(X)
#define FB2K_TRACE_THIS FB2K_TRACE(__FUNCTION__)
#define FB2K_TRACE_THIS_FORCED FB2K_TRACE_FORCED(__FUNCTION__)

#define FB2K_BugCheck() fb2k::crashWithMessage( pfc::format("FB2K_BugCheck: ", __FUNCTION__ ) )
#define FB2K_BugCheckEx(msg) fb2k::crashWithMessage(msg)
#define FB2K_BugCheckUnlikely() uBugCheck()
