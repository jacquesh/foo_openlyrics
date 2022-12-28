#include "foobar2000-sdk-pch.h"
#include "foosortstring.h"

#ifdef _WIN32
#include <shlwapi.h>
#endif

namespace fb2k {
#ifdef _WIN32
	sortString_t makeSortString(const char* in) {
		wchar_t* out = new wchar_t[pfc::stringcvt::estimate_utf8_to_wide(in) + 1];
		out[0] = ' ';//StrCmpLogicalW bug workaround.
		pfc::stringcvt::convert_utf8_to_wide_unchecked(out + 1, in);
		return sortString_t(out);
	}
	int sortStringCompare(sortString_t const& s1, sortString_t const& s2) {
		return StrCmpLogicalW(s1.get(), s2.get());
	}
#else
	int sortStringCompare(const char* str1, const char* str2) {
		return pfc::naturalSortCompare(str1, str2);
	}
#endif
}