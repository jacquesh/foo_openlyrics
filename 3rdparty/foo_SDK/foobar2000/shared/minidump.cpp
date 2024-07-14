#include "shared.h"
#include <imagehlp.h>

#ifdef _M_ARM64EC
typedef ARM64EC_NT_CONTEXT myCONTEXT;
#else
typedef CONTEXT myCONTEXT;
#endif
struct DumpState {
  int state;
  myCONTEXT*context;
};

__declspec(noinline) static bool safeRead(volatile const void* addr, size_t& dest)
{
	__try {
		dest = *(const volatile size_t*)addr;
		return true;
	}
	__except (1) {
		return false;
	}
}

__declspec(noinline) static bool safeTestReadAccess(volatile const void* addr)
{
	size_t dummy;
	return safeRead(addr, dummy);
}

#if defined(_M_ARM64) || defined(_M_ARM64EC)

BOOL WINAPI MiniDumpCallback(PVOID CallbackParam,
	const PMINIDUMP_CALLBACK_INPUT CallbackInput,
	PMINIDUMP_CALLBACK_OUTPUT CallbackOutput)
{
	static const unsigned STACK_SEARCH_SIZE = 0x400;
	static const unsigned MEM_BLOCK_SIZE = 0x400;

	if (CallbackInput->CallbackType == MemoryCallback) {
		// Called to get user defined blocks of memory to write until
		// callback returns FALSE or CallbackOutput->MemorySize == 0.

		DumpState* ds = (DumpState*)CallbackParam;
		switch (ds->state) {
			// Save memory referenced by registers.
		case 0: CallbackOutput->MemoryBase = ds->context->X0; ds->state++; break;
		case 1: CallbackOutput->MemoryBase = ds->context->X1; ds->state++; break;
		case 2: CallbackOutput->MemoryBase = ds->context->X2; ds->state++; break;
		case 3: CallbackOutput->MemoryBase = ds->context->X3; ds->state++; break;
		case 4: CallbackOutput->MemoryBase = ds->context->X4; ds->state++; break;
		case 5: CallbackOutput->MemoryBase = ds->context->X5; ds->state++; break;
		case 6: CallbackOutput->MemoryBase = ds->context->X6; ds->state++; break;
		case 7: CallbackOutput->MemoryBase = ds->context->X7; ds->state++; break;
		case 8: CallbackOutput->MemoryBase = ds->context->X8; ds->state++; break;
		case 9: CallbackOutput->MemoryBase = ds->context->X9; ds->state++; break;
		case 10: CallbackOutput->MemoryBase = ds->context->X10; ds->state++; break;
		case 11: CallbackOutput->MemoryBase = ds->context->X11; ds->state++; break;
		case 12: CallbackOutput->MemoryBase = ds->context->X12; ds->state++; break;
#ifndef _M_ARM64EC
		case 13: CallbackOutput->MemoryBase = ds->context->X13; ds->state++; break;
		case 14: CallbackOutput->MemoryBase = ds->context->X14; ds->state++; break;
		case 15: CallbackOutput->MemoryBase = ds->context->X15; ds->state++; break;
		case 16: CallbackOutput->MemoryBase = ds->context->X16; ds->state++; break;
		case 17: CallbackOutput->MemoryBase = ds->context->X17; ds->state++; break;
		case 18: CallbackOutput->MemoryBase = ds->context->X18; ds->state++; break;
		case 19: CallbackOutput->MemoryBase = ds->context->X19; ds->state++; break;
		case 20: CallbackOutput->MemoryBase = ds->context->X20; ds->state++; break;
		case 21: CallbackOutput->MemoryBase = ds->context->X21; ds->state++; break;
		case 22: CallbackOutput->MemoryBase = ds->context->X22; ds->state++; break;
		case 23: CallbackOutput->MemoryBase = ds->context->X23; ds->state++; break;
		case 24: CallbackOutput->MemoryBase = ds->context->X24; ds->state++; break;
		case 25: CallbackOutput->MemoryBase = ds->context->X25; ds->state++; break;
		case 26: CallbackOutput->MemoryBase = ds->context->X26; ds->state++; break;
		case 27: CallbackOutput->MemoryBase = ds->context->X27; ds->state++; break;
		case 28: CallbackOutput->MemoryBase = ds->context->X28; ds->state++; break;
#endif

			// Save memory referenced by values in stack.
		default:
			if (ds->state < 0x1000)
				ds->state = 0x1000;

			size_t addr;
			do {
				if (ds->state > 0x1000 + STACK_SEARCH_SIZE)
					return FALSE;

				if (!safeRead((void*)((ds->context->Sp & ~7) + ds->state - 0x1000), addr))
					return FALSE;

				ds->state += 4;
			} while (addr < 0x1000 || !safeTestReadAccess((void*)addr));

			CallbackOutput->MemoryBase = addr;
			break;
		}

		if (CallbackOutput->MemoryBase >= MEM_BLOCK_SIZE / 2)
			CallbackOutput->MemoryBase -= MEM_BLOCK_SIZE / 2;
		CallbackOutput->MemorySize = MEM_BLOCK_SIZE;

		// No need to perform additional checks here, the minidump engine
		// safely clips the addresses to valid memory pages only.
		// Also seems to apply for overlapped areas etc.
	}

	return TRUE;
}

#elif defined(_M_IX86)

BOOL WINAPI MiniDumpCallback(PVOID CallbackParam,
	const PMINIDUMP_CALLBACK_INPUT CallbackInput,
	PMINIDUMP_CALLBACK_OUTPUT CallbackOutput)
{
	static const unsigned STACK_SEARCH_SIZE = 0x400;
	static const unsigned MEM_BLOCK_SIZE = 0x400;

	if (CallbackInput->CallbackType == MemoryCallback) {
		// Called to get user defined blocks of memory to write until
		// callback returns FALSE or CallbackOutput->MemorySize == 0.

		DumpState* ds = (DumpState*)CallbackParam;
		switch (ds->state) {
			// Save memory referenced by registers.
		case 0: CallbackOutput->MemoryBase = ds->context->Eax; ds->state++; break;
		case 1: CallbackOutput->MemoryBase = ds->context->Ebx; ds->state++; break;
		case 2: CallbackOutput->MemoryBase = ds->context->Ecx; ds->state++; break;
		case 3: CallbackOutput->MemoryBase = ds->context->Edx; ds->state++; break;
		case 4: CallbackOutput->MemoryBase = ds->context->Esi; ds->state++; break;
		case 5: CallbackOutput->MemoryBase = ds->context->Edi; ds->state++; break;
		case 6: CallbackOutput->MemoryBase = ds->context->Ebp; ds->state++; break;
		case 7: CallbackOutput->MemoryBase = ds->context->Esp; ds->state++; break;
		case 8: CallbackOutput->MemoryBase = ds->context->Eip; ds->state++; break;

			// Save memory referenced by values in stack.
		default:
			if (ds->state < 0x1000)
				ds->state = 0x1000;

			size_t addr;
			do {
				if (ds->state > 0x1000 + STACK_SEARCH_SIZE)
					return FALSE;

				if (!safeRead((void*)((ds->context->Esp & ~3) + ds->state - 0x1000), addr))
					return FALSE;

				ds->state += 4;
			} while (addr < 0x1000 || !safeTestReadAccess((void*)addr));

			CallbackOutput->MemoryBase = addr;
			break;
		}

		if (CallbackOutput->MemoryBase >= MEM_BLOCK_SIZE / 2)
			CallbackOutput->MemoryBase -= MEM_BLOCK_SIZE / 2;
		CallbackOutput->MemorySize = MEM_BLOCK_SIZE;

		// No need to perform additional checks here, the minidump engine
		// safely clips the addresses to valid memory pages only.
		// Also seems to apply for overlapped areas etc.
	}

	return TRUE;
}

#elif defined(_M_X64)

BOOL WINAPI MiniDumpCallback(PVOID CallbackParam,
	const PMINIDUMP_CALLBACK_INPUT CallbackInput,
	PMINIDUMP_CALLBACK_OUTPUT CallbackOutput)
{
	static const unsigned STACK_SEARCH_SIZE = 0x400;
	static const unsigned MEM_BLOCK_SIZE = 0x400;

	if (CallbackInput->CallbackType == MemoryCallback) {
		// Called to get user defined blocks of memory to write until
		// callback returns FALSE or CallbackOutput->MemorySize == 0.

		DumpState* ds = (DumpState*)CallbackParam;
		switch (ds->state) {
			// Save memory referenced by registers.
		case 0: CallbackOutput->MemoryBase = ds->context->Rax; ds->state++; break;
		case 1: CallbackOutput->MemoryBase = ds->context->Rbx; ds->state++; break;
		case 2: CallbackOutput->MemoryBase = ds->context->Rcx; ds->state++; break;
		case 3: CallbackOutput->MemoryBase = ds->context->Rdx; ds->state++; break;
		case 4: CallbackOutput->MemoryBase = ds->context->Rsi; ds->state++; break;
		case 5: CallbackOutput->MemoryBase = ds->context->Rdi; ds->state++; break;
		case 6: CallbackOutput->MemoryBase = ds->context->Rsp; ds->state++; break;
		case 7: CallbackOutput->MemoryBase = ds->context->Rbp; ds->state++; break;
		case 8: CallbackOutput->MemoryBase = ds->context->R8; ds->state++; break;
		case 9: CallbackOutput->MemoryBase = ds->context->R9; ds->state++; break;
		case 10: CallbackOutput->MemoryBase = ds->context->R10; ds->state++; break;
		case 11: CallbackOutput->MemoryBase = ds->context->R11; ds->state++; break;
		case 12: CallbackOutput->MemoryBase = ds->context->R12; ds->state++; break;
		case 13: CallbackOutput->MemoryBase = ds->context->R13; ds->state++; break;
		case 14: CallbackOutput->MemoryBase = ds->context->R14; ds->state++; break;
		case 15: CallbackOutput->MemoryBase = ds->context->R15; ds->state++; break;

			// Save memory referenced by values in stack.
		default:
			if (ds->state < 0x1000)
				ds->state = 0x1000;

			size_t addr;
			do {
				if (ds->state > 0x1000 + STACK_SEARCH_SIZE)
					return FALSE;

				if (!safeRead((void*)((ds->context->Rsp & ~7) + ds->state - 0x1000), addr))
					return FALSE;

				ds->state += 4;
			} while (addr < 0x1000 || !safeTestReadAccess((void*)addr));

			CallbackOutput->MemoryBase = addr;
			break;
		}

		if (CallbackOutput->MemoryBase >= MEM_BLOCK_SIZE / 2)
			CallbackOutput->MemoryBase -= MEM_BLOCK_SIZE / 2;
		CallbackOutput->MemorySize = MEM_BLOCK_SIZE;

		// No need to perform additional checks here, the minidump engine
		// safely clips the addresses to valid memory pages only.
		// Also seems to apply for overlapped areas etc.
	}

	return TRUE;
}

#endif // _M_X64




BOOL WriteMiniDumpHelper(HANDLE hDump, LPEXCEPTION_POINTERS param) {
	MINIDUMP_EXCEPTION_INFORMATION exception = {};
	exception.ThreadId = GetCurrentThreadId();
	exception.ExceptionPointers = param;
	exception.ClientPointers = FALSE;
	DumpState ds;
    ds.state = 0;
    ds.context = reinterpret_cast<myCONTEXT*>(param->ContextRecord);
    MINIDUMP_CALLBACK_INFORMATION mci;
    mci.CallbackRoutine = &MiniDumpCallback;
	mci.CallbackParam = (void*)&ds;
	return MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDump,  (MINIDUMP_TYPE)(MiniDumpWithUnloadedModules), &exception, NULL, &mci);
}
