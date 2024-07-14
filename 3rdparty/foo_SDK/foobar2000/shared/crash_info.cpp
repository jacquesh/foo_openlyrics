#include "shared.h"
#include <imagehlp.h>
#include <forward_list>

#if SIZE_MAX == UINT32_MAX
#define STACKSPEC "%08X"
#define PTRSPEC "%08Xh"
#define OFFSETSPEC "%Xh"
#elif SIZE_MAX == UINT64_MAX
#define STACKSPEC "%016llX"
#define PTRSPEC "%016llXh"
#define OFFSETSPEC "%llXh"
#else
#error WTF?
#endif


static volatile bool g_didSuppress = false;
static critical_section g_panicHandlersSync;
static std::forward_list<fb2k::panicHandler*> g_panicHandlers;

static void callPanicHandlers() {
	insync(g_panicHandlersSync);
	for( auto i = g_panicHandlers.begin(); i != g_panicHandlers.end(); ++ i ) {
		try {
			(*i)->onPanic();
		} catch(...) {}
	}
}

void SHARED_EXPORT uAddPanicHandler(fb2k::panicHandler* p) {
	insync(g_panicHandlersSync);
	g_panicHandlers.push_front(p);
}

void SHARED_EXPORT uRemovePanicHandler(fb2k::panicHandler* p) {
	insync(g_panicHandlersSync);
	g_panicHandlers.remove(p);
}


enum { EXCEPTION_BUG_CHECK = 0xaa67913c };

#if FB2K_SUPPORT_CRASH_LOGS

static const unsigned char utf8_header[3] = {0xEF,0xBB,0xBF};

static __declspec(thread) char g_thread_call_stack[1024];
static __declspec(thread) t_size g_thread_call_stack_length;

static critical_section g_lastEventsSync;
static pfc::chain_list_v2_t<pfc::string8> g_lastEvents;
static constexpr t_size KLastEventCount = 200;

static pfc::string8 version_string;

static pfc::array_t<TCHAR> DumpPathBuffer;

static long crash_no = 0;

// Debug timer: GetTickCount() diff since app startup.
// Intentionally kept as dumb as possible (originally meant to format system time nicely).
// Do not want to do expensive preformat of every event that in >99% of scenarios isn't logged.
// Cannot do formatting & system calls in crash handler.
// So we just write amount of MS since app startup.
static const uint64_t debugTimerInit = GetTickCount64();
static pfc::format_int_t queryDebugTimer() { return pfc::format_int( GetTickCount64() - debugTimerInit ); }

static pfc::string8 g_components;

static void WriteFileString_internal(HANDLE hFile,const char * ptr,t_size len)
{
	DWORD bah;
	WriteFile(hFile,ptr,(DWORD)len,&bah,0);
}

static HANDLE create_failure_log()
{
	bool rv = false;
	if (DumpPathBuffer.get_size() == 0) return INVALID_HANDLE_VALUE;

	TCHAR * path = DumpPathBuffer.get_ptr();

	t_size lenWalk = _tcslen(path);

	if (lenWalk == 0 || path[lenWalk-1] != '\\') {path[lenWalk++] = '\\';}

	_tcscpy(path + lenWalk, _T("crash reports"));
	lenWalk += _tcslen(path + lenWalk);

	SetLastError(NO_ERROR);
	if (!CreateDirectory(path, NULL)) {
		if (GetLastError() != ERROR_ALREADY_EXISTS) return INVALID_HANDLE_VALUE;
	}
	path[lenWalk++] = '\\';

	TCHAR * fn_out = path + lenWalk;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	unsigned attempts = 0;
	for(;;) {
		wsprintf(fn_out,TEXT("failure_%08u.txt"),++attempts);
		hFile = CreateFile(path,GENERIC_WRITE,0,0,CREATE_NEW,0,0);
		if (hFile!=INVALID_HANDLE_VALUE) break;
		if (attempts > 1000) break;
	}
	if (hFile!=INVALID_HANDLE_VALUE) WriteFileString_internal(hFile, (const char*)utf8_header, sizeof(utf8_header));
	return hFile;
}


static void WriteFileString(HANDLE hFile,const char * str)
{
	const char * ptr = str;
	for(;;)
	{
		const char * start = ptr;
		ptr = strchr(ptr,'\n');
		if (ptr)
		{
			if (ptr>start) {
				t_size len = ptr-start;
				if (ptr[-1] == '\r') --len;
				WriteFileString_internal(hFile,start,len);
			}
			WriteFileString_internal(hFile,"\r\n",2);
			ptr++;
		}
		else
		{
			WriteFileString_internal(hFile,start,strlen(start));
			break;
		}
	}
}

static void WriteEvent(HANDLE hFile,const char * str) {
	bool haveText = false;
	const char * ptr = str;
	bool isLineBreak = false;
	while(*ptr) {
		const char * base = ptr;
		while(*ptr && *ptr != '\r' && *ptr != '\n') ++ptr;
		if (ptr > base) {
			if (isLineBreak) WriteFileString_internal(hFile,"\r\n",2);
			WriteFileString_internal(hFile,base,ptr-base);
			isLineBreak = false; haveText = true;
		}
		for(;;) {
			if (*ptr == '\n') isLineBreak = haveText;
			else if (*ptr != '\r') break;
			++ptr;
		}
	}
	if (haveText) WriteFileString_internal(hFile,"\r\n",2);
}

static bool read_int(t_size src, t_size* out)
{
	__try
	{
		*out = *(t_size*)src;
		return true;
	} __except (1) { return false; }
}

static bool hexdump8(char * out,size_t address,const char * msg,int from,int to)
{
	unsigned max = (to-from)*16;
	if (IsBadReadPtr((const void*)(address+(from*16)),max)) return false;
	out += sprintf(out,"\n%s (" PTRSPEC "):",msg,address);
	unsigned n;
	const unsigned char * src = (const unsigned char*)(address)+(from*16);
	
	for(n=0;n<max;n++)
	{
		if (n%16==0)
		{
			out += sprintf(out,"\n" PTRSPEC ": ",(size_t)src);
		}

		out += sprintf(out," %02X",*(src++));
	}
	*(out++) = '\n';
	*out=0;
	return true;
}

static bool hexdump_stack(char * out,size_t address,const char * msg,int from,int to)
{
	constexpr unsigned lineWords = 4;
	constexpr unsigned wordBytes = sizeof(size_t);
	constexpr unsigned lineBytes = lineWords * wordBytes;
	const unsigned max = (to-from)*lineBytes;
	if (IsBadReadPtr((const void*)(address+(from*lineBytes)),max)) return false;
	out += sprintf(out,"\n%s (" PTRSPEC "):",msg,address);
	unsigned n;
	const unsigned char * src = (const unsigned char*)(address)+(from*16);
	
	for(n=0;n<max;n+=4)
	{
		if (n % lineBytes == 0)
		{
			out += sprintf(out,"\n" PTRSPEC ": ",(size_t)src);
		}

		out += sprintf(out," " STACKSPEC,*(size_t*)src);
		src += wordBytes;
	}
	*(out++) = '\n';
	*out=0;
	return true;
}

static void call_stack_parse(size_t address,HANDLE hFile,char * temp,HANDLE hProcess)
{
	bool inited = false;
	t_size data;
	t_size count_done = 0;
	while(count_done<1024 && read_int(address, &data))
	{
		if (!IsBadCodePtr((FARPROC)data))
		{
			bool found = false;
			{
				IMAGEHLP_MODULE mod = {};
				mod.SizeOfStruct = sizeof(mod);
				if (SymGetModuleInfo(hProcess,data,&mod))
				{
					if (!inited)
					{
						WriteFileString(hFile,"\nStack dump analysis:\n");
						inited = true;
					}
					sprintf(temp, "Address: " PTRSPEC " (%s+" OFFSETSPEC ")", data, mod.ModuleName, data - mod.BaseOfImage);
					//sprintf(temp,"Address: %08Xh, location: \"%s\", loaded at %08Xh - %08Xh\n",data,mod.ModuleName,mod.BaseOfImage,mod.BaseOfImage+mod.ImageSize);
					WriteFileString(hFile,temp);
					found = true;
				}
			}


			if (found)
			{
				union
				{
					char buffer[128];
					IMAGEHLP_SYMBOL symbol;
				};
				memset(buffer,0,sizeof(buffer));
				symbol.SizeOfStruct = sizeof(symbol);
				symbol.MaxNameLength = (DWORD)(buffer + sizeof(buffer) - symbol.Name);
				DWORD_PTR offset = 0;
				if (SymGetSymFromAddr(hProcess,data,&offset,&symbol))
				{
					buffer[PFC_TABSIZE(buffer)-1]=0;
					if (symbol.Name[0])
					{
						sprintf(temp,", symbol: \"%s\" (+" OFFSETSPEC ")",symbol.Name,offset);
						WriteFileString(hFile,temp);
					}
				}
				WriteFileString(hFile, "\n");
			}
		}
		address += sizeof(size_t);
		count_done++;
	}
}

static BOOL CALLBACK EnumModulesCallback(PCSTR ModuleName,ULONG_PTR BaseOfDll,PVOID UserContext)
{
	IMAGEHLP_MODULE mod = {};
	mod.SizeOfStruct = sizeof(mod);
	if (SymGetModuleInfo(GetCurrentProcess(),BaseOfDll,&mod))
	{
		char temp[1024];
		char temp2[PFC_TABSIZE(mod.ModuleName)+1];
		strcpy(temp2,mod.ModuleName);

		{
			t_size ptr;
			for(ptr=strlen(temp2);ptr<PFC_TABSIZE(temp2)-1;ptr++)
				temp2[ptr]=' ';
			temp2[ptr]=0;
		}

		sprintf(temp,"%s loaded at " PTRSPEC " - " PTRSPEC "\n",temp2,mod.BaseOfImage,mod.BaseOfImage+mod.ImageSize);
		WriteFileString((HANDLE)UserContext,temp);
	}
	return TRUE;
}

static bool SalvageString(t_size rptr, char* temp) {
	__try {
		const char* ptr = reinterpret_cast<const char*>(rptr);
		t_size walk = 0;
		for (; walk < 255; ++walk) {
			char c = ptr[walk];
			if (c == 0) break;
			if (c < 0x20) c = ' ';
			temp[walk] = c;
		}
		temp[walk] = 0;
		return true;
	} __except (1) { return false; }
}

static void DumpCPPExceptionData(HANDLE hFile, const ULONG_PTR* params, char* temp) {
	t_size strPtr;
	if (read_int(params[1] + sizeof(void*), &strPtr)) {
		if (SalvageString(strPtr, temp)) {
			WriteFileString(hFile, "Message: ");
			WriteFileString(hFile, temp);
			WriteFileString(hFile, "\n");
		}
	}
}

static void writeFailureTxt(LPEXCEPTION_POINTERS param, HANDLE hFile, DWORD lastError) {
	char temp[2048];
	{
		t_size address = (t_size)param->ExceptionRecord->ExceptionAddress;
		sprintf(temp, "Illegal operation:\nCode: %08Xh, flags: %08Xh, address: " PTRSPEC "\n", param->ExceptionRecord->ExceptionCode, param->ExceptionRecord->ExceptionFlags, address);
		WriteFileString(hFile, temp);

		if (param->ExceptionRecord->ExceptionCode == EXCEPTION_BUG_CHECK) {
			WriteFileString(hFile, "Bug check\n");
		} else if (param->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && param->ExceptionRecord->NumberParameters >= 2) {
			sprintf(temp, "Access violation, operation: %s, address: " PTRSPEC "\n", param->ExceptionRecord->ExceptionInformation[0] ? "write" : "read", param->ExceptionRecord->ExceptionInformation[1]);
			WriteFileString(hFile, temp);
		} else if (param->ExceptionRecord->NumberParameters > 0) {
			WriteFileString(hFile, "Additional parameters:");
			for (DWORD walk = 0; walk < param->ExceptionRecord->NumberParameters; ++walk) {
				sprintf(temp, " " PTRSPEC, param->ExceptionRecord->ExceptionInformation[walk]);
				WriteFileString(hFile, temp);
			}
			WriteFileString(hFile, "\n");
		}

		if (param->ExceptionRecord->ExceptionCode == 0xE06D7363 && param->ExceptionRecord->NumberParameters >= 3) {	//C++ exception
			DumpCPPExceptionData(hFile, param->ExceptionRecord->ExceptionInformation, temp);
		}

		if (lastError) {
			sprintf(temp, "Last win32 error: %u\n", lastError);
			WriteFileString(hFile, temp);
		}

		if (g_thread_call_stack[0] != 0) {
			WriteFileString(hFile, "\nCall path:\n");
			WriteFileString(hFile, g_thread_call_stack);
			WriteFileString(hFile, "\n");
		} else {
			WriteFileString(hFile, "\nCall path not available.\n");
		}


		if (hexdump8(temp, address, "Code bytes", -4, +4)) WriteFileString(hFile, temp);
#ifdef _M_IX86
		if (hexdump_stack(temp, param->ContextRecord->Esp, "Stack", -2, +18)) WriteFileString(hFile, temp);
		sprintf(temp, "\nRegisters:\nEAX: %08X, EBX: %08X, ECX: %08X, EDX: %08X\nESI: %08X, EDI: %08X, EBP: %08X, ESP: %08X\n", param->ContextRecord->Eax, param->ContextRecord->Ebx, param->ContextRecord->Ecx, param->ContextRecord->Edx, param->ContextRecord->Esi, param->ContextRecord->Edi, param->ContextRecord->Ebp, param->ContextRecord->Esp);
		WriteFileString(hFile, temp);
#endif

#ifdef _M_X64
		if (hexdump_stack(temp, param->ContextRecord->Rsp, "Stack", -2, +18)) WriteFileString(hFile, temp);
		sprintf(temp, "\nRegisters:\nRAX: %016llX, RBX: %016llX, RCX: %016llX, RDX: %016llX\nRSI: %016llX, RDI: %016llX, RBP: %016llX, RSP: %016llX\n", param->ContextRecord->Rax, param->ContextRecord->Rbx, param->ContextRecord->Rcx, param->ContextRecord->Rdx, param->ContextRecord->Rsi, param->ContextRecord->Rdi, param->ContextRecord->Rbp, param->ContextRecord->Rsp);
		WriteFileString(hFile, temp);
#endif

#ifdef _M_ARM64
		if (hexdump_stack(temp, param->ContextRecord->Sp, "Stack", -2, +18)) WriteFileString(hFile, temp);
#endif

		WriteFileString(hFile, "\nTimestamp:\n");
		WriteFileString(hFile, queryDebugTimer() );
		WriteFileString(hFile, "ms\n");

		{
			const HANDLE hProcess = GetCurrentProcess();
			if (SymInitialize(hProcess, NULL, TRUE))
			{
				{
					IMAGEHLP_MODULE mod = {};
					mod.SizeOfStruct = sizeof(mod);
					if (!IsBadCodePtr((FARPROC)address) && SymGetModuleInfo(hProcess, address, &mod))
					{
						sprintf(temp, "\nCrash location:\nModule: %s\nOffset: " OFFSETSPEC "\n", mod.ModuleName, address - mod.BaseOfImage);
						WriteFileString(hFile, temp);
					} else
					{
						sprintf(temp, "\nUnable to identify crash location!\n");
						WriteFileString(hFile, temp);
					}
				}

				{
					union
					{
						char buffer[128];
						IMAGEHLP_SYMBOL symbol;
					};
					memset(buffer, 0, sizeof(buffer));
					symbol.SizeOfStruct = sizeof(symbol);
					symbol.MaxNameLength = (DWORD)(buffer + sizeof(buffer) - symbol.Name);
					DWORD_PTR offset = 0;
					if (SymGetSymFromAddr(hProcess, address, &offset, &symbol))
					{
						buffer[PFC_TABSIZE(buffer) - 1] = 0;
						if (symbol.Name[0])
						{
							sprintf(temp, "Symbol: \"%s\" (+" OFFSETSPEC ")\n", symbol.Name, offset);
							WriteFileString(hFile, temp);
						}
					}
				}

				WriteFileString(hFile, "\nLoaded modules:\n");
				SymEnumerateModules(hProcess, EnumModulesCallback, hFile);

#ifdef _M_IX86
				call_stack_parse(param->ContextRecord->Esp, hFile, temp, hProcess);
#endif

#ifdef _M_X64
				call_stack_parse(param->ContextRecord->Rsp, hFile, temp, hProcess);
#endif

				SymCleanup(hProcess);
			} else {
				WriteFileString(hFile, "\nFailed to get module/symbol info.\n");
			}
		}

		WriteFileString(hFile, "\nEnvironment:\n");
		WriteFileString(hFile, version_string);

		WriteFileString(hFile, "\n");

		if (!g_components.is_empty()) {
			WriteFileString(hFile, "\nComponents:\n");
			WriteFileString(hFile, g_components);
		}

		{
			insync(g_lastEventsSync);
			if (g_lastEvents.get_count() > 0) {
				WriteFileString(hFile, "\nRecent events:\n");
				for( auto & walk : g_lastEvents) WriteEvent(hFile, walk);
			}
		}
	}
}

static bool GrabOSVersion(char * out) {
	OSVERSIONINFO ver = {}; ver.dwOSVersionInfoSize = sizeof(ver);
	if (!GetVersionEx(&ver)) return false;
	*out = 0;
	char temp[16];
	strcat(out,"Windows ");
	 _itoa(ver.dwMajorVersion,temp,10); strcat(out,temp);
	 strcat(out,".");
	 _itoa(ver.dwMinorVersion,temp,10); strcat(out,temp);
	return true;
}

static void OnLogFileWritten() {
	TCHAR exePath[MAX_PATH + 1] = {};
	TCHAR params[1024];
	GetModuleFileName(NULL, exePath, MAX_PATH);
	exePath[MAX_PATH] = 0;
	//unsafe...
	wsprintf(params, _T("/crashed \"%s\""), DumpPathBuffer.get_ptr());
	ShellExecute(NULL, NULL, exePath, params, NULL, SW_SHOW);
}

BOOL WriteMiniDumpHelper(HANDLE, LPEXCEPTION_POINTERS); //minidump.cpp


void SHARED_EXPORT uDumpCrashInfo(LPEXCEPTION_POINTERS param)
{
	if (g_didSuppress) return;
	const DWORD lastError = GetLastError();
	if (InterlockedIncrement(&crash_no) > 1) {Sleep(10000);return;}
	HANDLE hFile = create_failure_log();
	if (hFile == INVALID_HANDLE_VALUE) return;

	{
		_tcscpy(_tcsrchr(DumpPathBuffer.get_ptr(), '.'), _T(".dmp"));
		HANDLE hDump = CreateFile(DumpPathBuffer.get_ptr(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		if (hDump != INVALID_HANDLE_VALUE) {
			const BOOL written = WriteMiniDumpHelper(hDump, param);
			CloseHandle(hDump);
			if (!written) { //don't bother proceeding if we don't have a valid minidump
				DeleteFile(DumpPathBuffer.get_ptr()); 
				CloseHandle(hFile);
				_tcscpy(_tcsrchr(DumpPathBuffer.get_ptr(), '.'), _T(".txt"));
				DeleteFile(DumpPathBuffer.get_ptr()); 
				return;
			} 
		}
		_tcscpy(_tcsrchr(DumpPathBuffer.get_ptr(), '.'), _T(".txt"));
	}

	writeFailureTxt(param, hFile, lastError);

	CloseHandle(hFile);

	OnLogFileWritten();
}

//No longer used.
size_t SHARED_EXPORT uPrintCrashInfo(LPEXCEPTION_POINTERS param,const char * extra_info,char * outbase) {
	*outbase = 0;
	return 0;
}


LONG SHARED_EXPORT uExceptFilterProc(LPEXCEPTION_POINTERS param) {
	if (g_didSuppress) return UnhandledExceptionFilter(param);
	callPanicHandlers();
	if (IsDebuggerPresent()) {
		return UnhandledExceptionFilter(param);
	} else {
		if ( param->ExceptionRecord->ExceptionCode == STATUS_STACK_OVERFLOW ) {
			pfc::thread2 trd;
			trd.startHere( [param] {
				uDumpCrashInfo(param);
				} );
			trd.waitTillDone();
		} else {
			uDumpCrashInfo(param);
		}
		TerminateProcess(GetCurrentProcess(), 0);
		return 0;// never reached
	}
}

void SHARED_EXPORT uPrintCrashInfo_Init(const char * name)//called only by exe on startup
{
	version_string = pfc::format( "App: ", name, "\nArch: ", pfc::cpuArch());
	
	SetUnhandledExceptionFilter(uExceptFilterProc);
}
void SHARED_EXPORT uPrintCrashInfo_Suppress() {
	g_didSuppress = true;
}

void SHARED_EXPORT uPrintCrashInfo_AddEnvironmentInfo(const char * p_info) {
	version_string << "\n" << p_info;
}

void SHARED_EXPORT uPrintCrashInfo_SetComponentList(const char * p_info) {
	g_components = p_info;
}

void SHARED_EXPORT uPrintCrashInfo_SetDumpPath(const char * p_path) {
	pfc::stringcvt::string_os_from_utf8 temp(p_path);
	DumpPathBuffer.set_size(temp.length() + 256);
	_tcscpy(DumpPathBuffer.get_ptr(), temp.get_ptr());
}

static HANDLE hLogFile = INVALID_HANDLE_VALUE;

static void logEvent(const char* message) {
	if ( hLogFile != INVALID_HANDLE_VALUE ) {
		DWORD wrote = 0;
		WriteFile(hLogFile, message, (DWORD) strlen(message), &wrote, NULL );
		WriteFile(hLogFile, "\r\n", 2, &wrote, NULL);
	}
}

void SHARED_EXPORT uPrintCrashInfo_StartLogging(const char * path) {
	insync(g_lastEventsSync);
	PFC_ASSERT(hLogFile == INVALID_HANDLE_VALUE);
	hLogFile = CreateFile( pfc::stringcvt::string_wide_from_utf8(path), GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
	PFC_ASSERT(hLogFile != INVALID_HANDLE_VALUE);

	for( auto & walk : g_lastEvents ) {
		logEvent( walk.c_str() );
	}
}

void SHARED_EXPORT uPrintCrashInfo_OnEvent(const char * message, t_size length) {

	pfc::string8 msg = pfc::format("[", queryDebugTimer(), "ms] ");
	msg.add_string( message, length );

	insync(g_lastEventsSync);
	logEvent(msg);
	while(g_lastEvents.get_count() >= KLastEventCount) g_lastEvents.remove(g_lastEvents.first());
	g_lastEvents.insert_last( std::move(msg) );
}

static void callstack_add(const char * param)
{
	enum { MAX = PFC_TABSIZE(g_thread_call_stack) - 1} ;
	t_size len = strlen(param);
	if (g_thread_call_stack_length + len > MAX) len = MAX - g_thread_call_stack_length;
	if (len>0)
	{
		memcpy(g_thread_call_stack+g_thread_call_stack_length,param,len);
		g_thread_call_stack_length += len;
		g_thread_call_stack[g_thread_call_stack_length]=0;
	}
}

uCallStackTracker::uCallStackTracker(const char * name)
{
	param = g_thread_call_stack_length;
	if (g_thread_call_stack_length>0) callstack_add("=>");
	callstack_add(name);
}

uCallStackTracker::~uCallStackTracker()
{
	g_thread_call_stack_length = param;
	g_thread_call_stack[param]=0;

}

extern "C" {LPCSTR SHARED_EXPORT uGetCallStackPath() {return g_thread_call_stack;} }

#ifdef _DEBUG
extern "C" {
	void SHARED_EXPORT fb2kDebugSelfTest() {
		auto ptr = SetUnhandledExceptionFilter(NULL);
		PFC_ASSERT( ptr == uExceptFilterProc );
		SetUnhandledExceptionFilter(ptr);
	}
}
#endif

#else

void SHARED_EXPORT uDumpCrashInfo(LPEXCEPTION_POINTERS param) {}
void SHARED_EXPORT uPrintCrashInfo_OnEvent(const char * message, t_size length) {}
LONG SHARED_EXPORT uExceptFilterProc(LPEXCEPTION_POINTERS param) {
	return UnhandledExceptionFilter(param);
}
uCallStackTracker::uCallStackTracker(const char * name) {}
uCallStackTracker::~uCallStackTracker() {}
extern "C" {
	LPCSTR SHARED_EXPORT uGetCallStackPath() { return ""; } 
	void SHARED_EXPORT fb2kDebugSelfTest() {}
}
#endif

PFC_NORETURN void SHARED_EXPORT uBugCheck() {
	fb2k_instacrash_scope(RaiseException(EXCEPTION_BUG_CHECK, EXCEPTION_NONCONTINUABLE, 0, NULL); );
}
