#include "pfc-lite.h"

#include "timers.h"
#include "debug.h"

#if defined(_WIN32) && defined(PFC_HAVE_PROFILER)
#include <ShlObj.h>
#endif

#ifndef _WIN32
#include "nix-objects.h"
#include <time.h>
#endif

namespace pfc {

#ifdef PFC_HAVE_PROFILER

profiler_static::profiler_static(const char * p_name)
{
	name = p_name;
	total_time = 0;
	num_called = 0;
}

static void profilerMsg(const char* msg) {
#ifdef _WIN32
    if (!IsDebuggerPresent()) {
        static HANDLE hWriteTo = INVALID_HANDLE_VALUE;
        static bool initialized = false;
        if (!initialized) {
            initialized = true;
            wchar_t path[1024] = {};
            if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, path))) {
                size_t l = wcslen(path);
                if (l > 0) {
                    if (path[l - 1] != '\\') {
                        wcscat_s(path, L"\\");
                    }
                    wchar_t fn[256];
                    wsprintf(fn, L"profiler-%u.txt", GetProcessId(GetCurrentProcess()));
                    wcscat_s(path, fn);
                    hWriteTo = CreateFile(path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
                }
            }
        }
        if (hWriteTo != INVALID_HANDLE_VALUE) {
            SetFilePointer(hWriteTo, 0, 0, SEEK_END);
            pfc::string8 temp = msg;
            temp += "\r\n";
            DWORD written = 0;
            WriteFile(hWriteTo, temp.c_str(), (DWORD) temp.length(), &written, NULL);
        }
    }
#endif
    outputDebugLine(msg);
}

profiler_static::~profiler_static()
{
	try {
		pfc::string8 message;
		message << "profiler: " << pfc::format_pad_left(48,' ',name) << " - " << 
			pfc::format_pad_right(16,' ',pfc::format_uint(total_time) ) << " cycles";

		if (num_called > 0) {
			message << " (executed " << num_called << " times, " << (total_time / num_called) << " average)";
		}
        profilerMsg(message);
	} catch(...) {
		//should never happen
		OutputDebugString(_T("unexpected profiler failure\n"));
	}
}
#endif

#ifndef _WIN32

    void hires_timer::start() {
        m_start = nixGetTime();
    }
    double hires_timer::query() const {
        return nixGetTime() - m_start;
    }
    double hires_timer::query_reset() {
        double t = nixGetTime();
        double r = t - m_start;
        m_start = t;
        return r;
    }
    pfc::string8 hires_timer::queryString(unsigned precision) const {
        return format_time_ex( query(), precision ).get_ptr();
    }
#endif


    uint64_t fileTimeWtoU(uint64_t ft) {
        return (ft - 116444736000000000 + /*rounding*/10000000/2) / 10000000;
    }
    uint64_t fileTimeUtoW(uint64_t ft) {
        return (ft * 10000000) + 116444736000000000;
    }
#ifndef _WIN32
    uint64_t fileTimeUtoW(const timespec & ts) {
        uint64_t ft = (uint64_t)ts.tv_sec * 10000000 + (uint64_t)ts.tv_nsec / 100;
        return ft + 116444736000000000;
    }
#endif
    
	uint64_t fileTimeNow() {
#ifdef _WIN32
		uint64_t ret;
		GetSystemTimeAsFileTime((FILETIME*)&ret);
		return ret;
#else
        
#if defined( __APPLE__ ) && defined(TIME_UTC)
        if (__builtin_available(iOS 13.0, macOS 10.15, *)) {
            struct timespec ts;
            timespec_get(&ts, TIME_UTC);
            return fileTimeUtoW(ts);
        }
#endif

        // Generic inaccurate method
		return fileTimeUtoW(time(NULL));
#endif
	}
    
}
