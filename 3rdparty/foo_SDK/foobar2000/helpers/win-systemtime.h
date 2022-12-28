#pragma once

namespace winTimeWrapper {
    typedef struct _SYSTEMTIME {
        uint16_t wYear;
        uint16_t wMonth;
        uint16_t wDayOfWeek;
        uint16_t wDay;
        uint16_t wHour;
        uint16_t wMinute;
        uint16_t wSecond;
        uint16_t wMilliseconds;
    } SYSTEMTIME, * PSYSTEMTIME, * LPSYSTEMTIME;

    typedef struct _FILETIME {
        uint32_t dwLowDateTime;
        uint32_t dwHighDateTime;
    } FILETIME, * PFILETIME, * LPFILETIME;


    bool SystemTimeToFileTime(const SYSTEMTIME* lpSystemTime, FILETIME* lpFileTime);
    bool LocalFileTimeToFileTime(const FILETIME* lpLocalFileTime, FILETIME* lpFileTime);

    uint64_t FileTimeToInt(FILETIME);
    FILETIME FileTimeFromInt(uint64_t);

#ifdef _WIN32
    void selfTest();
#endif
}

#ifndef _WIN32
using namespace winTimeWrapper;
#endif
