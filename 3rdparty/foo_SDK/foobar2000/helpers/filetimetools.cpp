#include "StdAfx.h"

#include "filetimetools.h"

#include <pfc/timers.h>

typedef exception_io_data exception_time_error;

#ifndef _WIN32
namespace {
    typedef uint16_t WORD;

    typedef struct _SYSTEMTIME {
        WORD wYear;
        WORD wMonth;
        WORD wDayOfWeek;
        WORD wDay;
        WORD wHour;
        WORD wMinute;
        WORD wSecond;
        WORD wMilliseconds;
    } SYSTEMTIME, * PSYSTEMTIME, * LPSYSTEMTIME;

}
static void SystemTimeToNix(const SYSTEMTIME& st, struct tm& Time) {
    memset(&Time, 0, sizeof(Time));
    Time.tm_sec = st.wSecond;
    Time.tm_min = st.wMinute;
    Time.tm_hour = st.wHour;
    Time.tm_mday = st.wDay;
    Time.tm_mon = st.wMonth - 1;
    Time.tm_year = st.wYear - 1900;
}

static t_filetimestamp ExportSystemTime(const SYSTEMTIME& st) {
    struct tm Time;
    SystemTimeToNix(st, Time);
    return pfc::fileTimeUtoW(mktime(&Time));
}

static t_filetimestamp ExportSystemTimeLocal(const SYSTEMTIME& st) {
    struct tm Time, Local;
    SystemTimeToNix(st, Time);
    time_t t = mktime(&Time);
    localtime_r(&t, &Local);
    return pfc::fileTimeUtoW(mktime(&Local));
}
static void SystemTimeFromNix(SYSTEMTIME& st, struct tm const& Time) {
    memset(&st, 0, sizeof(st));
    st.wSecond = Time.tm_sec;
    st.wMinute = Time.tm_min;
    st.wHour = Time.tm_hour;
    st.wDay = Time.tm_mday;
    st.wDayOfWeek = Time.tm_wday;
    st.wMonth = Time.tm_mon + 1;
    st.wYear = Time.tm_year + 1900;
}

static bool MakeSystemTime(SYSTEMTIME& st, t_filetimestamp ts) {
    time_t t = (time_t)pfc::fileTimeWtoU(ts);
    struct tm Time;
    if (gmtime_r(&t, &Time) == NULL) return false;
    SystemTimeFromNix(st, Time);
    return true;
}

static bool MakeSystemTimeLocal(SYSTEMTIME& st, t_filetimestamp ts) {
    time_t t = (time_t)pfc::fileTimeWtoU(ts);
    struct tm Time;
    if (localtime_r(&t, &Time) == NULL) return false;
    SystemTimeFromNix(st, Time);
    return true;
}

#else
static t_filetimestamp ExportSystemTime(const SYSTEMTIME& st) {
    t_filetimestamp base;
    if (!SystemTimeToFileTime(&st, (FILETIME*)&base)) throw exception_time_error();
    return base;
}
static t_filetimestamp ExportSystemTimeLocal(const SYSTEMTIME& st) {
#ifdef FOOBAR2000_DESKTOP_WINDOWS
    t_filetimestamp base, out;
    if (!SystemTimeToFileTime(&st, (FILETIME*)&base)) throw exception_time_error();
    if (!LocalFileTimeToFileTime((const FILETIME*)&base, (FILETIME*)&out)) throw exception_time_error();
    return out;
#else
    SYSTEMTIME UTC;
    if (!TzSpecificLocalTimeToSystemTime(NULL, &st, &UTC)) throw exception_time_error();
    return ExportSystemTime(UTC);
#endif
}
static bool MakeSystemTime(SYSTEMTIME& st, t_filetimestamp ts) {
    if (ts == filetimestamp_invalid) return false;
    return !!FileTimeToSystemTime((const FILETIME*)&ts, &st);

}
static bool MakeSystemTimeLocal(SYSTEMTIME& st, t_filetimestamp ts) {
    if (ts == filetimestamp_invalid) return false;
#ifdef FOOBAR2000_DESKTOP_WINDOWS
    FILETIME ft;
    if (FileTimeToLocalFileTime((FILETIME*)&ts, &ft)) {
        if (FileTimeToSystemTime(&ft, &st)) {
            return true;
        }
    }
    return false;
#else
    SYSTEMTIME UTC;
    if (FileTimeToSystemTime((FILETIME*)&ts, &UTC)) {
        if (SystemTimeToTzSpecificLocalTime(NULL, &UTC, &st)) return true;
    }
    return false;
#endif
}
#endif // _WIN32

static bool is_spacing(char c) { return c == ' ' || c == 10 || c == 13 || c == '\t'; }

static unsigned ParseDateElem(const char* ptr, t_size len) {
    unsigned ret = 0;
    for (t_size walk = 0; walk < len; ++walk) {
        const char c = ptr[walk];
        if (c < '0' || c > '9') throw exception_time_error();
        ret = ret * 10 + (unsigned)(c - '0');
    }
    return ret;
}

static bool st_sanity(SYSTEMTIME const& st) {
    return st.wYear >= 1601 && st.wMonth >= 1 && st.wMonth <= 12 && st.wDay >= 1 && st.wDay <= 31 && st.wHour < 24 && st.wMinute < 60 && st.wSecond < 60 && st.wMilliseconds < 1000;
}
static t_filetimestamp filetimestamp_from_string_internal(const char* date, bool local) {
    // Accepted format
    // YYYY-MM-DD HH:MM:SS
    try {
        SYSTEMTIME st = {};
        st.wDay = 1; st.wMonth = 1;

        unsigned walk = 0;
        auto worker = [&](unsigned n) {
            auto ret = ParseDateElem(date + walk, n);
            walk += n;
            if (ret > UINT16_MAX) throw exception_time_error();
            return (WORD)ret;;
        };
        
        auto skip = [&](char c) {
            if (date[walk] == c)  ++walk;
        };

        auto skipSpacing = [&] {
            while (is_spacing(date[walk])) ++walk;
        };
        skipSpacing();
        st.wYear = worker(4);
        skip('-');
        st.wMonth = worker(2);
        skip('-');
        st.wDay = worker(2);
        skipSpacing();
        st.wHour = worker(2);
        skip(':');
        st.wMinute = worker(2);
        skip(':');
        st.wSecond = worker(2);
        if (date[walk] == '.') {
            double v = pfc::string_to_float(date + walk);
            st.wMilliseconds = (WORD)floor(v * 1000.f); // don't ever round up, don't want to handle ms of 1000
        }

        if (!st_sanity(st)) throw exception_time_error();

        if (local) {
            return ExportSystemTimeLocal(st);
        } else {
            return ExportSystemTime(st);
        }
    } catch (exception_time_error) {
        return filetimestamp_invalid;
    }
}

namespace foobar2000_io {
    t_filetimestamp filetimestamp_from_string(const char* date) {
        return filetimestamp_from_string_internal(date, true);
    }

    t_filetimestamp filetimestamp_from_string_utc(const char* date) {
        return filetimestamp_from_string_internal(date, false);
    }

    static constexpr char g_invalidMsg[] = "<invalid timestamp>";

    pfc::string_formatter format_filetimestamp(t_filetimestamp p_timestamp) {
        try {
            SYSTEMTIME st;
            if (MakeSystemTimeLocal(st, p_timestamp)) {
                pfc::string_formatter buffer;
                buffer
                    << pfc::format_uint(st.wYear, 4) << "-" << pfc::format_uint(st.wMonth, 2) << "-" << pfc::format_uint(st.wDay, 2) << " "
                    << pfc::format_uint(st.wHour, 2) << ":" << pfc::format_uint(st.wMinute, 2) << ":" << pfc::format_uint(st.wSecond, 2);
                return buffer;
            }
        } catch (...) {}
        return g_invalidMsg;
    }

    pfc::string_formatter format_filetimestamp_ms(t_filetimestamp p_timestamp) {
        try {
            SYSTEMTIME st;
            if (MakeSystemTimeLocal(st, p_timestamp)) {
                pfc::string_formatter buffer;
                buffer
                    << pfc::format_uint(st.wYear, 4) << "-" << pfc::format_uint(st.wMonth, 2) << "-" << pfc::format_uint(st.wDay, 2) << " "
                    << pfc::format_uint(st.wHour, 2) << ":" << pfc::format_uint(st.wMinute, 2) << ":" << pfc::format_uint(st.wSecond, 2) << "." << pfc::format_uint(st.wMilliseconds, 3);
                return buffer;
            }
        } catch (...) {}
        return g_invalidMsg;
    }

    pfc::string_formatter format_filetimestamp_utc(t_filetimestamp p_timestamp) {
        try {
            SYSTEMTIME st;
            if (MakeSystemTime(st, p_timestamp)) {
                pfc::string_formatter buffer;
                buffer
                    << pfc::format_uint(st.wYear, 4) << "-" << pfc::format_uint(st.wMonth, 2) << "-" << pfc::format_uint(st.wDay, 2) << " "
                    << pfc::format_uint(st.wHour, 2) << ":" << pfc::format_uint(st.wMinute, 2) << ":" << pfc::format_uint(st.wSecond, 2);
                return buffer;
            }
        } catch (...) {}
        return g_invalidMsg;
    }

} // namespace foobar2000_io

namespace {
    struct dateISO_t {
        unsigned Y, M, D;
        unsigned h, m, s;
        double sfrac;
        int tzdelta;
    };

    dateISO_t read_ISO_8601(const char* dateISO) {
        dateISO_t ret = {};
        // 2022-01-26T13:44:51.200000Z
        // 2010-02-19T14:54:23.031+08:00
        // 2022-01-27T11:01:49+00:00
        // 2022-01-27T11:01:49Z
        // 20220127T110149Z

        unsigned walk = 0;
        auto worker = [&](unsigned n) {
            auto ret = ParseDateElem(dateISO + walk, n);
            walk += n;
            return ret;
        };
        auto skip = [&](char c) {
            if (dateISO[walk] == c)  ++walk;
        };
        auto expect = [&](char c) {
            if (dateISO[walk] != c) throw exception_time_error();
            ++walk;
        };
        ret.Y = worker(4);
        skip('-');
        ret.M = worker(2);
        skip('-');
        ret.D = worker(2);
        expect('T');
        ret.h = worker(2);
        skip(':');
        ret.m = worker(2);
        skip(':');
        ret.s = worker(2);
        if (dateISO[walk] == '.') {
            unsigned base = walk;
            ++walk;
            while (pfc::char_is_numeric(dateISO[walk])) ++walk;
            ret.sfrac = pfc::string_to_float(dateISO + base, walk - base);
        }
        if (dateISO[walk] == '+' || dateISO[walk] == '-') {
            bool neg = dateISO[walk] == '-';
            ++walk;
            unsigned tz_h = worker(2);
            if (tz_h >= 24) throw exception_time_error();
            skip(':');
            unsigned tz_m = worker(2);
            if (tz_m >= 60) throw exception_time_error();
            tz_m += tz_h * 60;
            ret.tzdelta = neg ? (int)tz_m : -(int)tz_m; // reversed! it's a timezone offset, have to *add* it if timezone has a minus
        }
        return ret;
    }
}

t_filetimestamp foobar2000_io::filetimestamp_from_string_ISO_8601(const char* dateISO) {
    try {
        auto elems = read_ISO_8601(dateISO);

        SYSTEMTIME st = {};
        st.wDay = 1; st.wMonth = 1;
        st.wYear = elems.Y;
        st.wMonth = elems.M;
        st.wDay = elems.D;
        st.wHour = elems.h;
        st.wMinute = elems.m;
        st.wSecond = elems.s;
        st.wMilliseconds = (WORD)floor(elems.sfrac * 1000.f);

        if (!st_sanity(st)) throw exception_time_error();

        auto ret = ExportSystemTime(st);

        ret += filetimestamp_1second_increment * elems.tzdelta * 60;
        
        return ret;
    } catch (...) {
        return filetimestamp_invalid;
    }
}
