#pragma once
#include "string-part.h"
#include "primitives.h"

namespace pfc {
	int wstricmp_ascii(const wchar_t* s1, const wchar_t* s2) throw();
	int stricmp_ascii(const char* s1, const char* s2) throw();
	int stricmp_ascii_ex(const char* s1, t_size len1, const char* s2, t_size len2) throw();
	int naturalSortCompare(const char* s1, const char* s2) throw();
	int naturalSortCompareI(const char* s1, const char* s2) throw();

	int strcmp_ex(const char* p1, t_size n1, const char* p2, t_size n2) throw();
	int strcmp_nc(const char* p1, size_t n1, const char* p2, size_t n2) throw();

	bool stringEqualsI_utf8(const char* p1, const char* p2) throw();
	bool stringEqualsI_ascii(const char* p1, const char* p2) throw();

	int stringCompareCaseInsensitive(const char* s1, const char* s2);
	int stringCompareCaseInsensitiveEx(string_part_ref s1, string_part_ref s2);

	template<typename T> inline const char* stringToPtr(T const& val) { return val.c_str(); }
	inline const char* stringToPtr(const char* val) { return val; }

	class _stringComparatorCommon {
	protected:
		template<typename T> static const char* myStringToPtr(const T& val) { return stringToPtr(val); }
		static const char* myStringToPtr(string_part_ref);
	};

	class stringComparatorCaseSensitive : private _stringComparatorCommon {
	public:
		template<typename T1, typename T2>
		static int compare(T1 const& v1, T2 const& v2) {
			if (is_same_type<T1, string_part_ref>::value || is_same_type<T2, string_part_ref>::value) {
				return compare_ex(stringToRef(v1), stringToRef(v2));
			} else {
				return strcmp(myStringToPtr(v1), myStringToPtr(v2));
			}
		}
		static int compare_ex(string_part_ref v1, string_part_ref v2) {
			return strcmp_ex(v1.m_ptr, v1.m_len, v2.m_ptr, v2.m_len);
		}
		static int compare_ex(const char* v1, t_size l1, const char* v2, t_size l2) {
			return strcmp_ex(v1, l1, v2, l2);
		}
	};
	class stringComparatorCaseInsensitive : private _stringComparatorCommon {
	public:
		template<typename T1, typename T2>
		static int compare(T1 const& v1, T2 const& v2) {
			if (is_same_type<T1, string_part_ref>::value || is_same_type<T2, string_part_ref>::value) {
				return stringCompareCaseInsensitiveEx(stringToRef(v1), stringToRef(v2));
			} else {
				return stringCompareCaseInsensitive(myStringToPtr(v1), myStringToPtr(v2));
			}
		}
	};
	class stringComparatorCaseInsensitiveASCII : private _stringComparatorCommon {
	public:
		template<typename T1, typename T2>
		static int compare(T1 const& v1, T2 const& v2) {
			if (is_same_type<T1, string_part_ref>::value || is_same_type<T2, string_part_ref>::value) {
				return compare_ex(stringToRef(v1), stringToRef(v2));
			} else {
				return stricmp_ascii(myStringToPtr(v1), myStringToPtr(v2));
			}
		}
		static int compare_ex(string_part_ref v1, string_part_ref v2) {
			return stricmp_ascii_ex(v1.m_ptr, v1.m_len, v2.m_ptr, v2.m_len);
		}
		static int compare_ex(const char* v1, t_size l1, const char* v2, t_size l2) {
			return stricmp_ascii_ex(v1, l1, v2, l2);
		}
	};

	class comparator_strcmp {
	public:
		inline static int compare(const char* p_item1, const char* p_item2) { return strcmp(p_item1, p_item2); }
		inline static int compare(const wchar_t* item1, const wchar_t* item2) { return wcscmp(item1, item2); }

		static int compare(const char* p_item1, string_part_ref p_item2) {
			return strcmp_ex(p_item1, SIZE_MAX, p_item2.m_ptr, p_item2.m_len);
		}
		static int compare(string_part_ref p_item1, string_part_ref p_item2) {
			return strcmp_ex(p_item1.m_ptr, p_item1.m_len, p_item2.m_ptr, p_item2.m_len);
		}
		static int compare(string_part_ref p_item1, const char* p_item2) {
			return strcmp_ex(p_item1.m_ptr, p_item1.m_len, p_item2, SIZE_MAX);
		}
	};

	class comparator_stricmp_ascii {
	public:
		inline static int compare(const char* p_item1, const char* p_item2) { return stricmp_ascii(p_item1, p_item2); }
	};


	class comparator_naturalSort {
	public:
		inline static int compare(const char* i1, const char* i2) throw() { return naturalSortCompare(i1, i2); }
	};


	template<typename t_char> int _strcmp_partial_ex(const t_char* p_string, t_size p_string_length, const t_char* p_substring, t_size p_substring_length) throw() {
		for (t_size walk = 0; walk < p_substring_length; walk++) {
			t_char stringchar = (walk >= p_string_length ? 0 : p_string[walk]);
			t_char substringchar = p_substring[walk];
			int result = compare_t(stringchar, substringchar);
			if (result != 0) return result;
		}
		return 0;
	}

	template<typename t_char> int strcmp_partial_ex_t(const t_char* p_string, t_size p_string_length, const t_char* p_substring, t_size p_substring_length) throw() {
		p_string_length = strlen_max_t(p_string, p_string_length); p_substring_length = strlen_max_t(p_substring, p_substring_length);
		return _strcmp_partial_ex(p_string, p_string_length, p_substring, p_substring_length);
	}

	template<typename t_char>
	int strcmp_partial_t(const t_char* p_string, const t_char* p_substring) throw() { return strcmp_partial_ex_t(p_string, SIZE_MAX, p_substring, SIZE_MAX); }

	inline int strcmp_partial_ex(const char* str, t_size strLen, const char* substr, t_size substrLen) throw() { return strcmp_partial_ex_t(str, strLen, substr, substrLen); }
	inline int strcmp_partial(const char* str, const char* substr) throw() { return strcmp_partial_t(str, substr); }

	int stricmp_ascii_partial(const char* str, const char* substr) throw();

}