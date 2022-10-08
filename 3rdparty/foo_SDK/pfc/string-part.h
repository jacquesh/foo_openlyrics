#pragma once

namespace pfc {
	struct string_part_ref {
		const char* m_ptr;
		t_size m_len;


		static string_part_ref make(const char* ptr, t_size len);
		string_part_ref substring(t_size base) const;
		string_part_ref substring(t_size base, t_size len) const;
		static string_part_ref make(const char* str);
		bool equals(string_part_ref other) const;
		bool equals(const char* str) const;
	};

	inline string_part_ref string_part(const char* ptr, t_size len) {
		string_part_ref val = { ptr, len }; return val;
	}

	template<typename T> static string_part_ref stringToRef(T const& val) { return string_part(stringToPtr(val), val.length()); }
	inline string_part_ref stringToRef(string_part_ref val) { return val; }
	inline string_part_ref stringToRef(const char* val) { return string_part(val, strlen(val)); }
}