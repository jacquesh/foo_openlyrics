#pragma once

#ifdef _WIN32
#include <memory> // std::unique_ptr<>
#endif

namespace fb2k {
#ifdef _WIN32
	typedef std::unique_ptr<wchar_t[]> sortString_t;
	sortString_t makeSortString(const char* str);
	int sortStringCompare(sortString_t const & s1, sortString_t const & s2);
#else
	typedef pfc::string8 sortString_t;
	inline sortString_t makeSortString(const char* str) { return str; }
	inline sortString_t makeSortString(pfc::string8 && str) { return std::move(str); }
	int sortStringCompare(const char* str1, const char* str2);
#endif
}