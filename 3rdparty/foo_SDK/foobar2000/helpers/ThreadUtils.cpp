#include "StdAfx.h"

#include "ThreadUtils.h"
#include "rethrow.h"

#include <exception>

namespace ThreadUtils {
	bool CRethrow::exec( std::function<void () > f ) throw() {
		m_rethrow = nullptr;
		bool rv = false;
		try {
			f();
			rv = true;
		} catch( ... ) {
			auto e = std::current_exception();
			m_rethrow = [e] { std::rethrow_exception(e); };
		}  

		return rv;
	}

	void CRethrow::rethrow() const {
		if ( m_rethrow ) m_rethrow();
	}
}
#ifdef _WIN32

#include "win32_misc.h"

#ifdef FOOBAR2000_MOBILE
#include <pfc/pp-winapi.h>
#endif


namespace ThreadUtils {
	bool WaitAbortable(HANDLE ev, abort_callback & abort, DWORD timeout) {
		const HANDLE handles[2] = {ev, abort.get_abort_event()};
		SetLastError(0);
		const DWORD status = WaitForMultipleObjects(2, handles, FALSE, timeout);
		switch(status) {
			case WAIT_TIMEOUT:
				PFC_ASSERT( timeout != INFINITE );
				return false;
			case WAIT_OBJECT_0:
				return true;
			case WAIT_OBJECT_0 + 1:
				throw exception_aborted();
			case WAIT_FAILED:
				WIN32_OP_FAIL();
			default:
				uBugCheck();
		}
	}
#ifdef FOOBAR2000_DESKTOP_WINDOWS
	void ProcessPendingMessagesWithDialog(HWND hDialog) {
		MSG msg = {};
		while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (!IsDialogMessage(hDialog, &msg)) {
				DispatchMessage(&msg);
			}
		}
	}
	void ProcessPendingMessages() {
		MSG msg = {};
		while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			DispatchMessage(&msg);
		}
	}
	void WaitAbortable_MsgLoop(HANDLE ev, abort_callback & abort) {
		abort.check();
		const HANDLE handles[2] = {ev, abort.get_abort_event()};
		MultiWait_MsgLoop(handles, 2);
		abort.check();
	
	}
	t_size MultiWaitAbortable_MsgLoop2(const HANDLE* ev, t_size evCount, abort_callback& abort) {
		abort.check();
		const size_t evCountEx = evCount + 1;
		HANDLE handles1[16];
		pfc::array_staticsize_t<HANDLE> handles2;
		HANDLE* pHandles = handles1;
		if (evCountEx > 16) {
			handles2.set_size_discard(evCountEx);
			pHandles = handles2.get_ptr();
		}
		pHandles[0] = abort.get_abort_event();
		pfc::memcpy_t(pHandles + 1, ev, evCount);
		DWORD status = MultiWait_MsgLoop(pHandles, (DWORD) evCountEx);
		abort.check();
		size_t ret = (size_t)(status - WAIT_OBJECT_0 - 1);
		PFC_ASSERT(ret < evCount);
		return ret;
	}

	t_size MultiWaitAbortable_MsgLoop(const HANDLE * ev, t_size evCount, abort_callback & abort) {
		// Retval is 1-based!
		// Originally a bug, now kept for compatibility if someone relies on this
		// Use MultiWaitAbortable_MsgLoop2() instead
		return MultiWaitAbortable_MsgLoop2(ev, evCount, abort) + 1;
	}

	void SleepAbortable_MsgLoop(abort_callback & abort, DWORD timeout) {
		HANDLE handles[] = { abort.get_abort_event() };
		MultiWait_MsgLoop(handles, 1, timeout);
		abort.check();
	}

	bool WaitAbortable_MsgLoop(HANDLE ev, abort_callback & abort, DWORD timeout) {
		abort.check();
		HANDLE handles[2] = { abort.get_abort_event(), ev };
		DWORD status = MultiWait_MsgLoop(handles, 2, timeout);
		abort.check();
		return status != WAIT_TIMEOUT;
	}
	
	DWORD MultiWait_MsgLoop(const HANDLE* ev, DWORD evCount) {
		for (;; ) {
			SetLastError(0);
			const DWORD status = MsgWaitForMultipleObjects((DWORD) evCount, ev, FALSE, INFINITE, QS_ALLINPUT);
			if (status == WAIT_FAILED) WIN32_OP_FAIL();
			if (status == WAIT_OBJECT_0 + evCount) {
				ProcessPendingMessages();
			} else if ( status >= WAIT_OBJECT_0 && status < WAIT_OBJECT_0 + evCount ) {
				return status;
			} else {
				uBugCheck();
			}
		}
	}

	DWORD MultiWait_MsgLoop(const HANDLE* ev, DWORD evCount, DWORD timeout) {
		if (timeout == INFINITE) return MultiWait_MsgLoop(ev, evCount);
		const DWORD entry = GetTickCount();
		DWORD now = entry;
		for (;;) {
			const DWORD done = now - entry;
			if (done >= timeout) return WAIT_TIMEOUT;
			SetLastError(0);
			const DWORD status = MsgWaitForMultipleObjects((DWORD)evCount, ev, FALSE, timeout - done, QS_ALLINPUT);
			if (status == WAIT_FAILED) WIN32_OP_FAIL();
			if (status == WAIT_OBJECT_0 + evCount) {
				ProcessPendingMessages();
			} else if (status == WAIT_TIMEOUT || (status >= WAIT_OBJECT_0 && status < WAIT_OBJECT_0 + evCount) ) {
				return status;
			} else {
				uBugCheck();
			}
			now = GetTickCount();
		}
	}

	bool pfcWaitMsgLoop(HANDLE ev, double timeout) {
		DWORD ms = pfc::event::g_calculate_wait_time(timeout);
		DWORD status = MultiWait_MsgLoop(&ev, 1, ms);
		return status == WAIT_OBJECT_0;
	}

#endif // FOOBAR2000_DESKTOP_WINDOWS

}
#endif