#include "StdAfx.h"
#include "win-systemtime.h"
#include <ctime>

#ifdef _WIN32
static void theSelfTest(); // has to be outside namespace
#endif

namespace winTimeWrapper {
	uint64_t FileTimeToInt(FILETIME ft) {
		return ((uint64_t)ft.dwLowDateTime) | ((uint64_t)ft.dwHighDateTime << 32);
	}
	FILETIME FileTimeFromInt(uint64_t i) {
		return { (uint32_t)(i & 0xFFFFFFFF), (uint32_t)(i >> 32) };
	}
	bool SystemTimeToFileTime(const SYSTEMTIME* st, FILETIME* ft) {
		struct tm tmTemp = {};
		if (st->wYear < 1900) return false;
		tmTemp.tm_year = st->wYear - 1900;
		if (st->wMonth < 1 || st->wMonth > 12) return false;
		tmTemp.tm_mon = st->wMonth - 1;
		tmTemp.tm_mday = st->wDay;
		tmTemp.tm_hour = st->wHour;
		tmTemp.tm_min = st->wMinute;
		tmTemp.tm_sec = st->wSecond;
		tmTemp.tm_isdst = -1;

		time_t timeTemp = mktime(&tmTemp);
		uint64_t wintime = pfc::fileTimeUtoW(timeTemp);
		wintime += (filetimestamp_1second_increment / 1000) * st->wMilliseconds;
		auto ftTemp = FileTimeFromInt( wintime );
		return LocalFileTimeToFileTime(&ftTemp, ft);
	}
	bool LocalFileTimeToFileTime(const FILETIME* lpLocalFileTime, FILETIME* lpFileTime) {
		uint64_t t64 = FileTimeToInt(*lpLocalFileTime);

		time_t rawtime = pfc::fileTimeWtoU(t64);
		
		auto gm = * gmtime(&rawtime);
		auto loc = * localtime(&rawtime);

		int64_t diff = (int64_t)mktime(&loc) - (int64_t)mktime(&gm);

		*lpFileTime = FileTimeFromInt(t64 + diff * filetimestamp_1second_increment);
		return true;
	}

#ifdef _WIN32
	void selfTest() { theSelfTest(); }
#endif
}


#ifdef _WIN32
static void theSelfTest() {
	uint64_t ft1, ft2;
	auto ft_1s = filetimestamp_1second_increment;
	{
		SYSTEMTIME st = {};
		st.wDay = 21; st.wMonth = 2; st.wYear = 2022;
		st.wHour = 15; st.wMinute = 15;
		FILETIME ft = {};
		SystemTimeToFileTime(&st, &ft);
		ft1 = *(uint64_t*)(&ft);
	}
	{
		winTimeWrapper::SYSTEMTIME st = {};
		st.wDay = 21; st.wMonth = 2; st.wYear = 2022;
		st.wHour = 15; st.wMinute = 15;
		winTimeWrapper::FILETIME ft = {};
		winTimeWrapper::SystemTimeToFileTime(&st, &ft);
		ft2 = winTimeWrapper::FileTimeToInt(ft);
	}

	if (ft1 < ft2) {
		pfc::outputDebugLine(pfc::format("ahead by ", (ft2 - ft1) / ft_1s));
	} else if (ft1 > ft2) {
		pfc::outputDebugLine(pfc::format("behind by ", (ft1 - ft2) / ft_1s));
	} else {
		pfc::outputDebugLine("ok");
	}

	pfc::nop();
}
#endif
