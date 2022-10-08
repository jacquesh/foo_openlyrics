#include "pfc-lite.h"
#include "string-lite.h"
#include "string_base.h"

namespace pfc {
	const unsigned alloc_minimum = 128;

	void stringLite::add_string(const char* p_string, t_size p_string_size) {
		add_string_nc(p_string, strlen_max(p_string, p_string_size));
	}

	void stringLite::add_string_nc(const char* ptr, size_t length) {
		if (length > 0) {
			const size_t base = m_length;
			const size_t newLen = base + length;
			makeRoom(newLen);
			auto p = (char*)m_mem.ptr();
			memcpy(p + base, ptr, length);
			p[newLen] = 0;
			m_length = newLen;
			m_ptr = p;
		}

	}

	void stringLite::set_string(const char* p_string, t_size p_length) {
		set_string_nc(p_string, strlen_max(p_string, p_length));
	}

	void stringLite::set_string_nc(const char* ptr, size_t length) {
		if (length > 0) {
			makeRoom(length);
			auto p = (char*)m_mem.ptr();
			memcpy(p, ptr, length);
			p[length] = 0;
			m_ptr = p;
			m_length = length;
		} else {
			clear();
		}
	}

	void stringLite::truncate(t_size len) {
		if (len < m_length) {
			makeRoom(len);
			if (len > 0) {
				auto p = (char*)m_mem.ptr();
				p[len] = 0;
				m_ptr = p;
			} else {
				m_ptr = "";
			}
			m_length = len;
		}
	}

	char* stringLite::lock_buffer(t_size p_requested_length) {
		makeRoom(p_requested_length);
		memset(m_mem.ptr(), 0, m_mem.size());
		return (char*)m_mem.ptr();
	}
	void stringLite::unlock_buffer() {
		auto p = (char*)m_mem.ptr();
		m_ptr = p;
		m_length = strlen(p);
	}

	void stringLite::clear() noexcept {
		m_ptr = "";
		m_length = 0;
		if (!m_noShrink && m_mem.size() > alloc_minimum) m_mem.clear();
	}
	void stringLite::copy(stringLite const& other) {
		set_string_nc(other.m_ptr, other.m_length);
		// m_noShrink deliberately NOT transferred
	}
	void stringLite::move(stringLite& other) noexcept {
		m_ptr = other.m_ptr;
		m_length = other.m_length;
		m_mem = std::move(other.m_mem);
		m_noShrink = other.m_noShrink;
		other._clear();
	}
	void stringLite::_clear() noexcept {
		m_ptr = "";
		m_length = 0;
	}
	void stringLite::makeRoom(size_t newLength) {
		size_t size = newLength + 1; // null term
		if (m_mem.size() < size) {
			size_t target = (size / 2) * 3;
			if (target < alloc_minimum) target = alloc_minimum;
			m_mem.resize(target);
		} else if (!m_noShrink && m_mem.size() / 2 >= size) {
			size_t target = size;
			if (target < alloc_minimum) target = alloc_minimum;
			m_mem.resize(target);
		}
	}

	stringLite const& stringLite::operator=(const string_base& src) {
		set_string_nc(src.c_str(), src.length());
		return *this;
	}

	stringLite::stringLite(const string_base& src) {
		set_string_nc(src.c_str(), src.length());
	}

	stringLite::stringLite(string_part_ref const& ref) {
		set_string_nc(ref.m_ptr, ref.m_len);
	}
	stringLite const& stringLite::operator=(const string_part_ref& ref) {
		set_string_nc(ref.m_ptr, ref.m_len); return *this;
	}

	size_t stringLite::replace_nontext_chars(char p_replace) {
		auto p = (char*)m_mem.ptr();
		size_t ret = 0;
		for (size_t w = 0; w < m_length; ++w) {
			if ((uint8_t)p[w] < 32) { p[w] = p_replace; ++ret; }
		}
		return ret;
	}
	size_t stringLite::replace_char(unsigned c1, unsigned c2) {
		if (c1 < 128 && c2 < 128) return replace_byte((char)c1, (char)c2);
		else {
			unsigned start = 0;
			stringLite temp(get_ptr() + start);
			truncate(start);
			const char* ptr = temp;
			t_size rv = 0;
			while (*ptr)
			{
				unsigned test;
				t_size delta = utf8_decode_char(ptr, test);
				if (delta == 0 || test == 0) break;
				if (test == c1) { test = c2; rv++; }
				add_char(test);
				ptr += delta;
			}
			return rv;
		}
	}

	size_t stringLite::replace_byte(char c1, char c2) {
		auto p = (char*)m_mem.ptr();
		size_t ret = 0;
		for (size_t w = 0; w < m_length; ++w) {
			if (p[w] == c1) { p[w] = c2; ++ret; }
		}
		return ret;
	}

	void stringLite::set_char(size_t offset, char c) {

		if (offset < m_length) {
			if (c == 0) {
				truncate(offset);
			} else {
				auto p = (char*)m_mem.ptr();
				p[offset] = c;
			}
		}
	}
	void stringLite::prealloc(size_t s) {
		m_noShrink = true;
		makeRoom(s);
	}

	void stringLite::remove_chars(t_size first, t_size count) {
		if (count == 0) return;
		if (count == m_length) {
			clear();
		} else {
			PFC_ASSERT(first + count > first);
			PFC_ASSERT(first + count <= m_length);

			auto p = (char*)m_mem.ptr();
			size_t last = first + count;
			size_t trailing = m_length - last;
			memmove(p + first, p + last, trailing + 1);

			size_t newLen = m_length - count;
			makeRoom(newLen);
			m_length = newLen;
			m_ptr = (char*)m_mem.ptr();
		}
	}
	void stringLite::insert_chars_nc(t_size first, const char* src, t_size count) {
		if (count == 0) return;
		if (first > m_length) first = m_length;
		size_t trailing = m_length - first;
		size_t newLen = m_length + count;
		makeRoom(newLen);
		auto p = (char*)m_mem.ptr();
		memmove(p + first + count, p + first, trailing + 1);
		memcpy(p + first, src, count);
		m_length = newLen;
		m_ptr = p;
	}
	void stringLite::insert_chars(t_size first, const char* src, t_size count) {
		insert_chars_nc(first, src, strlen_max(src, count));
	}
	void stringLite::insert_chars(t_size first, const char* src) {
		insert_chars_nc(first, src, strlen(src));
	}
	bool stringLite::equals(const stringLite& other) const noexcept {
		return equals(*this, other);
	}
	bool stringLite::equals(const char* other) const noexcept {
		return strcmp(c_str(), other) == 0;
	}
	bool stringLite::equals(const stringLite& v1, const stringLite& v2) noexcept {
		if (v1.m_length != v2.m_length) return false;
		return memcmp(v1.m_ptr, v2.m_ptr, v1.m_length) == 0;
	}
	bool stringLite::greater(const char* v1, size_t s1, const char* v2, size_t s2) noexcept {
		if (s1 > s2) return true;
		if (s1 < s2) return false;
		return memcmp(v1, v2, s1) > 0;
	}
	bool stringLite::greater(const stringLite& v1, const stringLite& v2) noexcept {
		return greater(v1.m_ptr, v1.m_length, v2.m_ptr, v2.m_length);
	}
	stringLite stringLite::lowerCase() const {
		stringLite ret;
		pfc::stringToLowerAppend(ret, this->c_str(), this->length());
		return ret;
	}
	stringLite stringLite::upperCase() const {
		stringLite ret;
		pfc::stringToUpperAppend(ret, this->c_str(), this->length());
		return ret;
	}

	stringLite stringLite::subString(t_size base) const {
		if (base > length()) throw exception_overflow();
		return string(c_str() + base);
	}
	stringLite stringLite::subString(t_size base, t_size count) const {
		return string(c_str() + base, count);
	}


	t_size stringLite::indexOf(char c, t_size base) const {
		return pfc::string_find_first(ptr(), c, base);
	}
	t_size stringLite::lastIndexOf(char c, t_size base) const {
		return pfc::string_find_last(ptr(), c, base);
	}
	t_size stringLite::indexOf(stringp s, t_size base) const {
		return pfc::string_find_first(ptr(), s, base);
	}
	t_size stringLite::lastIndexOf(stringp s, t_size base) const {
		return pfc::string_find_last(ptr(), s, base);
	}
	t_size stringLite::indexOfAnyChar(stringp _s, t_size base) const {
		string s(_s);
		const t_size len = length();
		const char* content = ptr();
		for (t_size walk = 0; walk < len; ++walk) {
			if (s.contains(content[walk])) return walk;
		}
		return SIZE_MAX;
	}
	t_size stringLite::lastIndexOfAnyChar(stringp _s, t_size base) const {
		string s(_s);
		const char* content = ptr();
		for (t_size _walk = length(); _walk > 0; --_walk) {
			const t_size walk = _walk - 1;
			if (s.contains(content[walk])) return walk;
		}
		return SIZE_MAX;
	}
	bool stringLite::startsWith(char c) const {
		return (*this)[0] == c;
	}
	bool stringLite::startsWith(stringp s) const {
		const char* walk = ptr();
		const char* subWalk = s;
		for (;;) {
			if (*subWalk == 0) return true;
			if (*walk != *subWalk) return false;
			walk++; subWalk++;
		}
	}
	bool stringLite::endsWith(char c) const {
		const t_size len = length();
		if (len == 0) return false;
		return ptr()[len - 1] == c;
	}
	bool stringLite::endsWith(stringp s) const {
		const t_size len = length(), subLen = stringp_length(s);
		if (subLen > len) return false;
		return subString(len - subLen) == s;
	}

	char stringLite::firstChar() const {
		return (*this)[0];
	}
	char stringLite::lastChar() const {
		const t_size len = length();
		return len > 0 ? (*this)[len - 1] : (char)0;
	}
	bool stringLite::contains(char c) const { return indexOf(c) != SIZE_MAX; }
	bool stringLite::contains(stringp s) const { return indexOf(s) != SIZE_MAX; }
	bool stringLite::containsAnyChar(stringp s) const { return indexOfAnyChar(s) != SIZE_MAX; }

	stringLite stringLite::replace(stringp strOld, stringp strNew) const {
		stringLite ret;
		size_t status = this->replace_string_ex(ret, strOld, strNew);
		if (status == 0) ret = *this;
		return ret;
	}

	stringLite stringLite::trim(char c) const {
		size_t base = 0, end = 0;
		const char* p = c_str();
		size_t walk = 0;
		while (p[walk] == c) ++walk;
		base = end = walk;
		while (p[walk] != 0) {
			if (p[walk] != c) end = walk + 1;
			++walk;
		}
		stringLite ret;
		ret.set_string_nc(p + base, end - base);
		return ret;
	}

	bool stringLite::operator==(const stringLite& other) const noexcept { return equals(*this, other); }
	bool stringLite::operator!=(const stringLite& other) const noexcept { return !equals(*this, other); }
	bool stringLite::operator==(const char* other) const noexcept { return strcmp(c_str(), other) == 0; }
	bool stringLite::operator!=(const char* other) const noexcept { return strcmp(c_str(), other) != 0; }
	bool stringLite::operator>(const stringLite& other) const noexcept { return greater(*this, other); }
	bool stringLite::operator<(const stringLite& other) const noexcept { return greater(other, *this); }
	bool stringLite::operator>(const char* other) const noexcept { return greater(m_ptr, m_length, other, strlen(other)); }
	bool stringLite::operator<(const char* other) const noexcept { return greater(other, strlen(other), m_ptr, m_length); }
	bool stringLite::operator>=(const stringLite& other) const noexcept { return !greater(other, *this); }
	bool stringLite::operator<=(const stringLite& other) const noexcept { return !greater(*this, other); }
	bool stringLite::operator>=(const char* other) const noexcept { return !greater(other, strlen(other), m_ptr, m_length); }
	bool stringLite::operator<=(const char* other) const noexcept { return !greater(m_ptr, m_length, other, strlen(other)); }

}


