#pragma once

// CPUID stuff supported only on MSVC for now, irrelevant for non x86
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#define PFC_HAVE_CPUID 1
namespace pfc {
	enum {
		CPU_HAVE_SSE		= 1 << 2,
		CPU_HAVE_SSE2		= 1 << 3,
		CPU_HAVE_SSE3		= 1 << 4,
		CPU_HAVE_SSSE3		= 1 << 5,
		CPU_HAVE_SSE41		= 1 << 6,
		CPU_HAVE_SSE42		= 1 << 7,
		CPU_HAVE_AVX		= 1 << 8,
	};

	bool query_cpu_feature_set(unsigned p_value);
};
#endif

#ifndef PFC_HAVE_CPUID
#define PFC_HAVE_CPUID 0
#endif

namespace pfc {
	const char* cpuArch();
}

#ifdef _M_ARM64EC
#define PFC_CPU_ARCH "ARM64EC"
#elif defined(_M_X64) || defined(__x86_64__)
#define PFC_CPU_ARCH "x64"
#elif defined(_M_IX86) || defined(__i386__)
#define PFC_CPU_ARCH "x86"
#elif defined(_M_ARM64) || defined(__aarch64__)
#define PFC_CPU_ARCH "ARM64"
#elif defined(_M_ARM) || defined(__arm__)
#define PFC_CPU_ARCH "ARM"
#endif
