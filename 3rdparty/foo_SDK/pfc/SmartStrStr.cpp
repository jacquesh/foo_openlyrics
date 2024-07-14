#include "pfc-lite.h"

#include "string-conv-lite.h"
#include "string_conv.h"
#include "SmartStrStr.h"
#include <algorithm>
#include "SmartStrStr-table.h"
#include "SmartStrStr-twoCharMappings.h"

bool SmartStrStr::isWordChar(unsigned c) {
	// FIX ME map Unicode ranges somehow
	return c >= 128 || pfc::char_is_ascii_alphanumeric((char)c);
}

bool SmartStrStr::isWordChar(const char* ptr) {
	unsigned c;
	size_t d = pfc::utf8_decode_char(ptr, c);
	if (d == 0) return false; // bad UTF-8
	return isWordChar(c);
}

bool SmartStrStr::isValidWord(const char* ptr) {
	if (*ptr == 0) return false;
	do {
		unsigned c;
		size_t d = pfc::utf8_decode_char(ptr, c);
		if (d == 0) return false; // bad UTF-8
		if (!isWordChar(c)) return false;
		ptr += d;
	} while (*ptr != 0);
	return true;
}

void SmartStrStr::findWords(const char* str, std::function<void(pfc::string_part_ref)> cb) {
	size_t base = 0, walk = 0;
	for (;; ) {
		unsigned c = 0;
		size_t d = pfc::utf8_decode_char(str + walk, c);
		if (d == 0) break;
		
		if (!SmartStrStr::isWordChar(c)) {
			if (walk > base) {
				cb(pfc::string_part(str + base, walk - base));
			}
			base = walk + d;
		}
		walk += d;
	}
	if (walk > base) {
		cb(pfc::string_part(str + base, walk - base));
	}
}

SmartStrStr::SmartStrStr() {
	std::map<uint32_t, std::set<uint32_t> > substitutions, substitutionsReverse;
	std::map<uint32_t, uint32_t > downconvert;

#if 1
	for (auto& walk : SmartStrStrTable) {
		downconvert[walk.from] = walk.to;
		substitutions[walk.from].insert(walk.to);
	}
#else
	for (uint32_t walk = 128; walk < 0x10000; ++walk) {
		uint32_t c = Transform(walk);
		if (c != walk) {
			downconvert[walk] = c;
			substitutions[walk].insert(c);
		}
	}
#endif

	for (uint32_t walk = 32; walk < 0x10000; ++walk) {
		auto lo = ToLower(walk);
		if (lo != walk) {
			auto & s = substitutions[walk]; s.insert(lo);

			auto iter = substitutions.find(lo);
			if (iter != substitutions.end()) {
				s.insert(iter->second.begin(), iter->second.end());
			}
		}
	}

	for( auto & walk : substitutions ) {
		for( auto & walk2 : walk.second ) {
			substitutionsReverse[walk2].insert(walk.first);
		}
	}

	this->m_substitutions.initialize(std::move(substitutions));
	this->m_substitutionsReverse.initialize(std::move(substitutionsReverse));
	this->m_downconvert.initialize(std::move(downconvert));
	InitTwoCharMappings();
}

#ifdef _WIN32
static_assert(sizeof(wchar_t) == sizeof(char16_t));
const wchar_t * SmartStrStr::strStrEndW(const wchar_t * pString, const wchar_t * pSubString, size_t * outFoundAt) const {
	return reinterpret_cast<const wchar_t*>(strStrEnd16(reinterpret_cast<const char16_t*>(pString), reinterpret_cast<const char16_t*>(pSubString), outFoundAt));
}
const wchar_t * SmartStrStr::matchHereW(const wchar_t * pString, const wchar_t * pUserString) const {
	return reinterpret_cast<const wchar_t*>(matchHere16(reinterpret_cast<const char16_t*>(pString), reinterpret_cast<const char16_t*>(pUserString)));
}
#endif
const char16_t * SmartStrStr::matchHere16(const char16_t * pString, const char16_t * pUserString) const {
	auto walkData = pString;
	auto walkUser = pUserString;
	for (;; ) {
		if (*walkUser == 0) return walkData;

		uint32_t cData, cUser;
		size_t dData = pfc::utf16_decode_char(walkData, &cData);
		size_t dUser = pfc::utf16_decode_char(walkUser, &cUser);
		if (dData == 0 || dUser == 0) return nullptr;

		if (cData != cUser) {
			bool gotMulti = false;
			{
				const char * cDataSubst = m_twoCharMappings.query(cData);
				if (cDataSubst != nullptr) {
					PFC_ASSERT(strlen(cDataSubst) == 2);
					if (matchOneChar(cUser, (uint32_t)cDataSubst[0])) {
						auto walkUser2 = walkUser + dUser;
						uint32_t cUser2;
						auto dUser2 = pfc::utf16_decode_char(walkUser2, &cUser2);
						if (matchOneChar(cUser2, (uint32_t)cDataSubst[1])) {
							gotMulti = true;
							dUser += dUser2;
						}
					}
				}
			}
			if (!gotMulti) {
				if (!matchOneChar(cUser, cData)) return nullptr;
			}
		}

		walkData += dData;
		walkUser += dUser;
	}
}

bool SmartStrStr::equals(const char * pString, const char * pUserString) const {
	auto p = matchHere(pString, pUserString);
	if ( p == nullptr ) return false;
	return *p == 0;
}
bool SmartStrStr::equals16(const char16_t* pString, const char16_t* pUserString) const {
	auto p = matchHere16(pString, pUserString);
	if ( p == nullptr ) return false;
	return *p == 0;
}

const char * SmartStrStr::matchHere(const char * pString, const char * pUserString) const {
	const char * walkData = pString;
	const char * walkUser = pUserString;
	for (;; ) {
		if (*walkUser == 0) return walkData;

		uint32_t cData, cUser;
		size_t dData = pfc::utf8_decode_char(walkData, cData);
		size_t dUser = pfc::utf8_decode_char(walkUser, cUser);
		if (dData == 0 || dUser == 0) return nullptr;

		if (cData != cUser) {
			bool gotMulti = false;
			{
				const char* cDataSubst = m_twoCharMappings.query(cData);
				if (cDataSubst != nullptr) {
					PFC_ASSERT(strlen(cDataSubst) == 2);
					if (matchOneChar(cUser, (uint32_t)cDataSubst[0])) {
						auto walkUser2 = walkUser + dUser;
						uint32_t cUser2;
						auto dUser2 = pfc::utf8_decode_char(walkUser2, cUser2);
						if (matchOneChar(cUser2, (uint32_t)cDataSubst[1])) {
							gotMulti = true;
							dUser += dUser2;
						}
					}
				}
			}
			if (!gotMulti) {
				if (!matchOneChar(cUser, cData)) return nullptr;
			}
		}

		walkData += dData;
		walkUser += dUser;
	}
}

const char * SmartStrStr::strStrEnd(const char * pString, const char * pSubString, size_t * outFoundAt) const {
	size_t walk = 0;
	for (;; ) {
		if (pString[walk] == 0) return nullptr;
		auto end = matchHere(pString+walk, pSubString);
		if (end != nullptr) {
			if ( outFoundAt != nullptr ) * outFoundAt = walk;
			return end;
		}

		size_t delta = pfc::utf8_char_len( pString + walk );
		if ( delta == 0 ) return nullptr;
		walk += delta;
	}
}

const char16_t * SmartStrStr::strStrEnd16(const char16_t * pString, const char16_t * pSubString, size_t * outFoundAt) const {
	size_t walk = 0;
	for (;; ) {
		if (pString[walk] == 0) return nullptr;
		auto end = matchHere16(pString + walk, pSubString);
		if (end != nullptr) {
			if (outFoundAt != nullptr) * outFoundAt = walk;
			return end;
		}

		uint32_t dontcare;
		size_t delta = pfc::utf16_decode_char(pString + walk, & dontcare);
		if (delta == 0) return nullptr;
		walk += delta;
	}
}

static bool wordBeginsHere(const char* base, size_t offset) {
	if (offset == 0) return true;
	for (size_t len = 1; len <= offset && len <= 6; --len) {
		unsigned c;
		if (pfc::utf8_decode_char(base + offset - len, c) == len) {
			return !SmartStrStr::isWordChar(c);
		}
	}
	return false;
}

const char* SmartStrStr::strStrEndWord(const char* pString, const char* pSubString, size_t* outFoundAt) const {
	size_t walk = 0;
	for (;;) {
		size_t foundAt = 0;
		auto end = strStrEnd(pString + walk, pSubString, &foundAt);
		if (end == nullptr) return nullptr;
		foundAt += walk;
		if (!isWordChar(end) && wordBeginsHere(pString, foundAt)) {
			if (outFoundAt) *outFoundAt = foundAt;
			return end;
		}
		walk = end - pString;
	}
}

bool SmartStrStr::matchOneChar(uint32_t cInput, uint32_t cData) const {
	if (cInput == cData) return true;
	auto v = m_substitutions.query_ptr(cData);
	if (v == nullptr) return false;
	return v->count(cInput) > 0;
}

pfc::string8 SmartStrStr::transformStr(const char* str) const {
	pfc::string8 ret; transformStrHere(ret, str); return ret;
}

void SmartStrStr::transformStrHere(pfc::string8& out, const char* in) const {
	transformStrHere(out, in, strlen(in));
}

void SmartStrStr::transformStrHere(pfc::string8& out, const char* in, size_t inLen) const {
	out.prealloc(inLen);
	out.clear();
	for (size_t walk = 0; walk < inLen; ) {
		unsigned c;
		size_t d = pfc::utf8_decode_char(in + walk, c);
		if (d == 0 || walk+d>inLen) break;
		walk += d;
		const char* alt = m_twoCharMappings.query(c);
		if (alt != nullptr) {
			out << alt; continue;
		}
		unsigned alt2 = m_downconvert.query(c);
		if (alt2 != 0) {
			out.add_char(alt2); continue;
		}
		out.add_char(c);
	}
}

#if 0 // Windows specific code
uint32_t SmartStrStr::Transform(uint32_t c) {
	wchar_t wide[2] = {}; char out[4] = {};
	pfc::utf16_encode_char(c, wide);
	BOOL fail = FALSE;
	if (WideCharToMultiByte(pfc::stringcvt::codepage_ascii, 0, wide, 2, out, 4, "?", &fail) > 0) {
		if (!fail) {
			if (out[0] > 0 && out[1] == 0) {
				c = out[0];
			}
		}
	}
	return c;
}
#endif

uint32_t SmartStrStr::ToLower(uint32_t c) {
	return pfc::charLower(c);
}

void SmartStrStr::InitTwoCharMappings() {
	std::map<uint32_t, const char* > mappings;
	std::map<uint32_t, uint32_t> reverse;
	for (auto& walk : twoCharMappings) {
		mappings[walk.from] = walk.to;
		uint32_t c1, c2;
		const char * p = walk.to;
		size_t d;
		d = pfc::utf8_decode_char(p, c1);
		if ( d > 0 ) {
			p += d;
			d = pfc::utf8_decode_char(p, c2);
			if (d > 0) {
				if (c1 < 0x10000 && c2 < 0x10000) {
					reverse[c1 | (c2 << 16)] = walk.from;
				}
			}
		}
	}
	m_twoCharMappings.initialize(std::move(mappings));
	m_twoCharMappingsReverse.initialize(std::move(reverse));
}
bool SmartStrStr::testSubString_prefix(const char* str, const char* sub, const char * prefix, size_t prefixLen) const {

	switch(prefixLen) {
	case 0:
		return false;
	case 1:
		for(const char * walk = str;; ) {
			walk = strchr(walk, *prefix);
			if ( walk == nullptr ) return false;
			++walk;
			if (matchHere(walk, sub)) return true;
		}
	default:
		for(const char * walk = str;; ) {
			walk = strstr(walk, prefix);
			if ( walk == nullptr ) return false;
			walk += prefixLen;
			if (matchHere(walk, sub)) return true;
		}
	}
}
bool SmartStrStr::testSubString_prefix(const char* str, const char* sub, uint32_t c) const {
	size_t tempLen;
	char temp[8];
	tempLen = pfc::utf8_encode_char(c, temp); temp[tempLen] = 0;
	return testSubString_prefix(str, sub, temp, tempLen);
}
bool SmartStrStr::testSubString_prefix_subst(const char* str, const char* sub, uint32_t prefix) const {
	if ( testSubString_prefix(str, sub, prefix)) return true;

	auto alt = m_substitutionsReverse.query_ptr( prefix );
	if (alt != nullptr) {
		for (auto c : *alt) {
			if (testSubString_prefix(str, sub, c)) return true;
		}
	}

	return false;
}
bool SmartStrStr::testSubstring(const char* str, const char* sub) const {
#if 1
	unsigned prefix;
	const size_t skip = pfc::utf8_decode_char(sub, prefix);
	if ( skip == 0 ) return false;
	sub += skip;

	if (testSubString_prefix_subst(str, sub, prefix)) return true;

	unsigned prefix2;
	const size_t skip2 = pfc::utf8_decode_char(sub, prefix2);
	if (skip2 > 0 && prefix < 0x10000 && prefix2 < 0x10000) {
		sub += skip2;
		auto alt = m_twoCharMappingsReverse.query(prefix | (prefix2 << 16));
		if (alt != 0) {
			if (testSubString_prefix_subst(str, sub, alt)) return true;
		}
	}
	
	return false;
#else
	return this->strStrEnd(str, sub) != nullptr;
#endif
}
bool SmartStrStr::testSubstring16(const char16_t* str, const char16_t* sub) const {
	return this->strStrEnd16(str, sub) != nullptr;
}

SmartStrStr& SmartStrStr::global() {
	static SmartStrStr g;
	return g;
}


void SmartStrFilter::init(const char* ptr, size_t len) {
	pfc::string_formatter current, temp;
	bool inQuotation = false;

	auto addCurrent = [&] {
		if (!current.is_empty()) {
			++m_items[current.get_ptr()]; current.reset();
		}
	};

	for (t_size walk = 0; walk < len; ++walk) {
		const char c = ptr[walk];
		if (c == '\"') inQuotation = !inQuotation;
		else if (!inQuotation && is_spacing(c)) {
			addCurrent();
		} else {
			current.add_byte(c);
		}
	}
	if (inQuotation) {
		// Allow unbalanced quotes, take the whole string *with* quotation marks
		m_items.clear();
		current.set_string_nc(ptr, len);
	}

	addCurrent();
}


bool SmartStrFilter::test_disregardCounts(const char* src) const {
	if (m_items.size() == 0) return false;

	for (auto& walk : m_items) {
		if (!dc->strStrEnd(src, walk.first.c_str())) return false;
	}
	return true;
}

bool SmartStrFilter::testWords(const char* src) const {
	if (m_items.size() == 0) return false;

	for (auto& walk : m_items) {
		const t_size count = walk.second;
		const std::string& str = walk.first;
		const char* strWalk = src;
		for (t_size walk = 0; walk < count; ++walk) {
			const char* next = dc->strStrEndWord(strWalk, str.c_str());
			if (next == nullptr) return false;
			strWalk = next;
		}
	}
	return true;
}

bool SmartStrFilter::test(const char* src) const {

	if (m_items.size() == 0) return false;

	// Use the faster routine first, it can't be used to count occurances but nobody really knows about this feature
	for (auto& walk : m_items) {
		if (!dc->testSubstring(src, walk.first.c_str())) return false;
	}
	// Have any items where specific number of occurances is wanted?
	for (auto & walk : m_items) {
		const t_size count = walk.second;
		if (count == 1) continue;
		const std::string& str = walk.first;
		const char* strWalk = src;
		for (t_size walk = 0; walk < count; ++walk) {
			const char* next = dc->strStrEnd(strWalk, str.c_str());
			if (next == nullptr) return false;
			strWalk = next;
		}
	}
	return true;
}
