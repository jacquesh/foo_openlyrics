#include "pfc-lite.h"
#include "cpuid.h"


#ifdef _MSC_VER
#include <intrin.h>
#endif

#if PFC_HAVE_CPUID
namespace pfc {

	bool query_cpu_feature_set(unsigned p_value) {

#ifdef __AVX__
		// AVX implies all supported values are set
		return true;
#else

#if _M_IX86_FP >= 2 || defined(_M_X64)
		// don't bother checking for SSE/SSE2 if compiled to use them
		p_value &= ~(CPU_HAVE_SSE | CPU_HAVE_SSE2);
		if (p_value == 0) return true;
#endif

#ifdef _MSC_VER
		__try {
#endif
			if (p_value & (CPU_HAVE_SSE | CPU_HAVE_SSE2 | CPU_HAVE_SSE3 | CPU_HAVE_SSSE3 | CPU_HAVE_SSE41 | CPU_HAVE_SSE42 | CPU_HAVE_AVX)) {
				int buffer[4];
				__cpuid(buffer,1);
				if (p_value & CPU_HAVE_SSE) {
					if ((buffer[3]&(1<<25)) == 0) return false;
				}
				if (p_value & CPU_HAVE_SSE2) {
					if ((buffer[3]&(1<<26)) == 0) return false;
				}
				if (p_value & CPU_HAVE_SSE3) {
					if ((buffer[2]&(1<<0)) == 0) return false;
				}
				if (p_value & CPU_HAVE_SSSE3) {
					if ((buffer[2]&(1<<9)) == 0) return false;
				}
				if (p_value & CPU_HAVE_SSE41) {
					if ((buffer[2]&(1<<19)) == 0) return false;
				}
				if (p_value & CPU_HAVE_SSE42) {
					if ((buffer[2]&(1<<20)) == 0) return false;
				}
				if (p_value & CPU_HAVE_AVX) {
					if ((buffer[2] & (1 << 28)) == 0) return false;
				}
			}
			return true;
#ifdef _MSC_VER
		} __except(1) {
			return false;
		}
#endif
#endif
	}
}

#endif

namespace pfc {
	const char* cpuArch() {
#ifdef PFC_CPU_ARCH
		return PFC_CPU_ARCH;
#else
		return "Unknown";
#endif
	}
}
