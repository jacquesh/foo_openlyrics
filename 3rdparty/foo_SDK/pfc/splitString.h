#pragma once
#include "array.h"
#include "string_base.h"

namespace pfc {
	template<typename t_output, typename t_splitCheck>
	void splitStringEx(t_output& p_output, const t_splitCheck& p_check, const char* p_string, t_size p_stringLen = SIZE_MAX) {
		t_size walk = 0, splitBase = 0;
		const t_size max = strlen_max(p_string, p_stringLen);
		for (; walk < max;) {
			t_size delta = p_check(p_string + walk, max - walk);
			if (delta > 0) {
				if (walk > splitBase) p_output(p_string + splitBase, walk - splitBase);
				splitBase = walk + delta;
			} else {
				delta = utf8_char_len(p_string + walk, max - walk);
				if (delta == 0) break;
			}
			walk += delta;
		}
		if (walk > splitBase) p_output(p_string + splitBase, walk - splitBase);
	}

	class __splitStringSimple_calculateSubstringCount {
	public:
		__splitStringSimple_calculateSubstringCount() : m_count() {}
		void operator() (const char*, t_size) { ++m_count; }
		t_size get() const { return m_count; }
	private:
		t_size m_count;
	};

	template<typename t_param> class _splitStringSimple_check;

	template<> class _splitStringSimple_check<const char*> {
	public:
		_splitStringSimple_check(const char* p_chars) {
			m_chars.set_size(strlen_utf8(p_chars));
			for (t_size walk = 0, ptr = 0; walk < m_chars.get_size(); ++walk) {
				ptr += utf8_decode_char(p_chars + ptr, m_chars[walk]);
			}
		}
		t_size operator()(const char* p_string, t_size p_stringLen) const {
			t_uint32 c;
			t_size delta = utf8_decode_char(p_string, c, p_stringLen);
			if (delta > 0) {
				for (t_size walk = 0; walk < m_chars.get_size(); ++walk) {
					if (m_chars[walk] == c) return delta;
				}
			}
			return 0;
		}
	private:
		array_t<t_uint32> m_chars;
	};
	template<> class _splitStringSimple_check<char> {
	public:
		_splitStringSimple_check(char c) : m_char(c) {}
		t_size operator()(const char* str, t_size len) const {
			PFC_ASSERT(len > 0);
			if (*str == m_char) return 1;
			else return 0;
		}
	private:
		const char m_char;
	};
	template<typename t_array>
	class __splitStringSimple_arrayWrapper {
	public:
		__splitStringSimple_arrayWrapper(t_array& p_array) : m_walk(), m_array(p_array) {}
		void operator()(const char* p_string, t_size p_stringLen) {
			m_array[m_walk++] = string_part(p_string, p_stringLen);
		}
	private:
		t_size m_walk;
		t_array& m_array;
	};
	template<typename t_list>
	class __splitStringSimple_listWrapper {
	public:
		__splitStringSimple_listWrapper(t_list& p_list) : m_list(p_list) {}
		void operator()(const char* p_string, t_size p_stringLen) {
			m_list += string_part(p_string, p_stringLen);
		}
	private:
		t_list& m_list;
	};

	template<typename t_array, typename t_split>
	void splitStringSimple_toArray(t_array& p_output, t_split p_split, const char* p_string, t_size p_stringLen = SIZE_MAX) {
		_splitStringSimple_check<t_split> strCheck(p_split);

		{
			__splitStringSimple_calculateSubstringCount wrapper;
			splitStringEx(wrapper, strCheck, p_string, p_stringLen);
			p_output.set_size(wrapper.get());
		}

		{
			__splitStringSimple_arrayWrapper<t_array> wrapper(p_output);
			splitStringEx(wrapper, strCheck, p_string, p_stringLen);
		}
	}
	template<typename t_list, typename t_split>
	void splitStringSimple_toList(t_list& p_output, t_split p_split, const char* p_string, t_size p_stringLen = SIZE_MAX) {
		_splitStringSimple_check<t_split> strCheck(p_split);

		__splitStringSimple_listWrapper<t_list> wrapper(p_output);
		splitStringEx(wrapper, strCheck, p_string, p_stringLen);
	}

	template<typename t_out> void splitStringByLines(t_out& out, const char* str) {
		for (;;) {
			const char* next = strchr(str, '\n');
			if (next == NULL) {
				out += string_part(str, strlen(str)); break;
			}
			const char* walk = next;
			while (walk > str && walk[-1] == '\r') --walk;
			out += string_part(str, walk - str);
			str = next + 1;
		}
	}
	template<typename t_out> void splitStringByChar(t_out& out, const char* str, char c) {
		for (;;) {
			const char* next = strchr(str, c);
			if (next == NULL) {
				out += string_part(str, strlen(str)); break;
			}
			out += string_part(str, next - str);
			str = next + 1;
		}
	}
	template<typename t_out> void splitStringBySubstring(t_out& out, const char* str, const char* split) {
		for (;;) {
			const char* next = strstr(str, split);
			if (next == NULL) {
				out += string_part(str, strlen(str)); break;
			}
			out += string_part(str, next - str);
			str = next + strlen(split);
		}
	}
}
