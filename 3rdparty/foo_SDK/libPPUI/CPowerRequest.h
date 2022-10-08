#pragma once

#ifdef _WIN32

#ifdef WINAPI_FAMILY_PARTITION
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define CPowerRequestAPI_Avail
#endif
#else // no WINAPI_FAMILY_PARTITION, desktop SDK
#define CPowerRequestAPI_Avail
#endif

#endif // _WIN32

#ifdef CPowerRequestAPI_Avail

class CPowerRequest {
public:
	CPowerRequest(const wchar_t * Reason);
	void SetSystem(bool bSystem);
	void SetExecution(bool bExecution);
	void SetDisplay(bool bDisplay);
	~CPowerRequest();

	CPowerRequest(const CPowerRequest&) = delete;
	void operator=(const CPowerRequest&) = delete;
private:
	HANDLE m_Request = INVALID_HANDLE_VALUE;
	bool m_bSystem = false, m_bDisplay = false;
};
#else

class CPowerRequest {
public:
	CPowerRequest(const wchar_t * Reason) {}
	void SetSystem(bool bSystem) {}
	void SetExecution(bool bExecution) {}
	void SetDisplay(bool bDisplay) {}
	CPowerRequest(const CPowerRequest&) = delete;
	void operator=(const CPowerRequest&) = delete;
};

#endif // CPowerRequestAPI_Avail
