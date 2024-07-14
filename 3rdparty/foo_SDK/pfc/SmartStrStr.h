#pragma once


#include <set>
#include <map>
#include <string>
#include <vector>
#include "fixed_map.h"

//! Implementation of string matching for search purposes, such as media library search or typefind in list views. \n
//! Inspired by Unicode asymetic search, but not strictly implementing the Unicode asymetric search specifications. \n
//! \n
//! Keeping a global instance of it is recommended, due to one time init overhead. \n
//! Thread safety: safe to call concurrently once constructed.

class SmartStrStr {
public:
	SmartStrStr();

	static bool isWordChar(unsigned c);
	static bool isWordChar(const char* ptr);
	static bool isValidWord(const char*);
	static void findWords(const char*, std::function<void(pfc::string_part_ref)>);

	//! Returns ptr to the end of the string if positive (for continuing search), nullptr if negative.
	const char * strStrEnd(const char * pString, const char * pSubString, size_t * outFoundAt = nullptr) const;
	const char16_t * strStrEnd16(const char16_t * pString, const char16_t * pSubString, size_t * outFoundAt = nullptr) const;

	const char* strStrEndWord(const char* pString, const char* pSubString, size_t* outFoundAt = nullptr) const;

	bool testSubstring( const char * str, const char * sub ) const;
	bool testSubstring16( const char16_t * str, const char16_t * sub ) const;
#ifdef _WIN32
	const wchar_t * strStrEndW(const wchar_t * pString, const wchar_t * pSubString, size_t * outFoundAt = nullptr) const;
#endif
	//! Returns ptr to the end of the string if positive (for continuing search), nullptr if negative.
	const char * matchHere(const char * pString, const char * pUserString) const;
	const char16_t * matchHere16(const char16_t * pString, const char16_t * pUserString) const;
#ifdef _WIN32
	const wchar_t * matchHereW( const wchar_t * pString, const wchar_t * pUserString) const;
#endif

	//! String-equals tool, compares strings rather than searching for occurance
	bool equals( const char * pString, const char * pUserString) const;
	bool equals16( const char16_t * pString, const char16_t * pUserString) const;

	//! One-char match. Doesn't use twoCharMappings, use only if you have to operate on char by char basis rather than call the other methods.
	bool matchOneChar(uint32_t cInput, uint32_t cData) const;

	static SmartStrStr& global();

	pfc::string8 transformStr(const char * str) const;
	void transformStrHere(pfc::string8& out, const char* in) const;
	void transformStrHere(pfc::string8& out, const char* in, size_t inLen) const;
private:
	bool testSubString_prefix(const char* str, const char* sub, const char * prefix, size_t prefixLen) const;
	bool testSubString_prefix(const char* str, const char* sub, uint32_t c) const;
	bool testSubString_prefix_subst(const char* str, const char* sub, uint32_t c) const;

	static uint32_t Transform(uint32_t c);
	static uint32_t ToLower(uint32_t c);

	void InitTwoCharMappings();

	fixed_map< uint32_t, uint32_t > m_downconvert;
	fixed_map< uint32_t, std::set<uint32_t> > m_substitutions;
	fixed_map< uint32_t, std::set<uint32_t> > m_substitutionsReverse;
	
	
	fixed_map<uint32_t, const char* > m_twoCharMappings;
	fixed_map<uint32_t, uint32_t> m_twoCharMappingsReverse;
};


class SmartStrFilter {
public:
	typedef std::map<std::string, t_size> t_stringlist;
	SmartStrFilter() { }
	SmartStrFilter(t_stringlist const& arg) : m_items(arg) {}
	SmartStrFilter(t_stringlist&& arg) : m_items(std::move(arg)) {}
	SmartStrFilter(const char* p) { init(p, strlen(p)); }
	SmartStrFilter(const char* p, size_t l) { init(p, l); }

	static bool is_spacing(char c) { return c == ' ' || c == 10 || c == 13 || c == '\t'; }

	void init(const char* ptr, size_t len);
	bool test(const char* src) const;
	bool testWords(const char* src) const;
	bool test_disregardCounts(const char* src) const;

	const t_stringlist& items() const { return m_items; }
	operator bool() const { return !m_items.empty(); }
	bool empty() const { return m_items.empty(); }
private:
	t_stringlist m_items;
	SmartStrStr * dc = &SmartStrStr::global();
};
