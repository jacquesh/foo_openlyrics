#include "stdafx.h"

#include "CPowerRequest.h"

#ifdef CPowerRequestAPI_Avail

class CPowerRequestAPI {
public:
	static void ToggleSystem(HANDLE hRequest, bool bSystem) {
		Toggle(hRequest, bSystem, PowerRequestSystemRequired);
	}

	static void ToggleExecution(HANDLE hRequest, bool bSystem) {
		const POWER_REQUEST_TYPE RequestType = IsWindows8OrGreater() ? PowerRequestExecutionRequired : PowerRequestSystemRequired;
		Toggle(hRequest, bSystem, RequestType);
	}

	static void ToggleDisplay(HANDLE hRequest, bool bDisplay) {
		Toggle(hRequest, bDisplay, PowerRequestDisplayRequired);
	}

	static void Toggle(HANDLE hRequest, bool bToggle, POWER_REQUEST_TYPE what) {
		if (bToggle) {
			PowerSetRequest(hRequest, what);
		} else {
			PowerClearRequest(hRequest, what);
		}
	}
};

CPowerRequest::CPowerRequest(const wchar_t * Reason) : m_Request(INVALID_HANDLE_VALUE), m_bSystem(), m_bDisplay() {
	REASON_CONTEXT ctx = {POWER_REQUEST_CONTEXT_VERSION, POWER_REQUEST_CONTEXT_SIMPLE_STRING};
	ctx.Reason.SimpleReasonString = const_cast<wchar_t*>(Reason);
	m_Request = PowerCreateRequest(&ctx);
}

void CPowerRequest::SetSystem(bool bSystem) {
	if (bSystem == m_bSystem) return;
	m_bSystem = bSystem;
	if (m_Request != INVALID_HANDLE_VALUE) {
		CPowerRequestAPI::ToggleSystem( m_Request, bSystem );
	}
}

void CPowerRequest::SetExecution(bool bExecution) {
	if (bExecution == m_bSystem) return;
	m_bSystem = bExecution;
	if (m_Request != INVALID_HANDLE_VALUE) {
		CPowerRequestAPI::ToggleExecution( m_Request, bExecution );
	}
}
	
void CPowerRequest::SetDisplay(bool bDisplay) {
	if (bDisplay == m_bDisplay) return;
	m_bDisplay = bDisplay;
	if (m_Request != INVALID_HANDLE_VALUE) {
		CPowerRequestAPI::ToggleDisplay(m_Request, bDisplay);
	}
}

CPowerRequest::~CPowerRequest() {
	if (m_Request != INVALID_HANDLE_VALUE) {
		CloseHandle(m_Request);
	}
}

#endif // _WIN32

