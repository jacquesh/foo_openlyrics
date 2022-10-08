#pragma once
#include "array.h"
#include "string_base.h"
namespace pfc {

	class wstringLite {
	public:
		const wchar_t* c_str() const noexcept {
			return m_buffer.size() > 0 ? m_buffer.get_ptr() : L"";
		}
		operator const wchar_t* () const noexcept { return c_str(); }
		size_t length() const noexcept { return wcslen(c_str()); }

		pfc::array_t<wchar_t> m_buffer;
	};

	wstringLite wideFromUTF8(const char* pUTF8, size_t inSize);
	string8 utf8FromWide(const wchar_t* pWide, size_t inSize);

	wstringLite wideFromUTF8(const char* pUTF8);
	string8 utf8FromWide(const wchar_t* pWide);
}