#include "pfc-lite.h"
#include "string_conv.h"
#include "string-conv-lite.h"

namespace pfc {
	wstringLite wideFromUTF8(const char* pUTF8, size_t inSize) {
		wstringLite ret;
		size_t estimate = pfc::stringcvt::estimate_utf8_to_wide(pUTF8, inSize);
		ret.m_buffer.resize(estimate);
		pfc::stringcvt::convert_utf8_to_wide(ret.m_buffer.get_ptr(), estimate, pUTF8, inSize);
		return ret;
	}
	string8 utf8FromWide(const wchar_t* pWide, size_t inSize) {
		string8 ret;
		size_t estimate = pfc::stringcvt::estimate_wide_to_utf8(pWide,inSize);
		PFC_ASSERT(estimate > 0);
		char* ptr = ret.lock_buffer(estimate-1/*nullterm included in estimate*/);
		pfc::stringcvt::convert_wide_to_utf8(ptr, estimate, pWide, inSize);
		ret.unlock_buffer();
		return ret;
	}
	wstringLite wideFromUTF8(const char* pUTF8) {
		wstringLite ret;
		size_t estimate = pfc::stringcvt::estimate_utf8_to_wide(pUTF8);
		ret.m_buffer.resize(estimate);
		pfc::stringcvt::convert_utf8_to_wide_unchecked(ret.m_buffer.get_ptr(), pUTF8);
		return ret;
	}
	string8 utf8FromWide(const wchar_t* pWide) {
		return utf8FromWide(pWide, SIZE_MAX);
	}
}
