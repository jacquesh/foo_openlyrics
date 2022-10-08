#pragma once
#include "array.h"
namespace pfc {
	// FOR LEGACY CODE ONLY
	// DO NOT USE IN NEW CODE
	template<typename t_char>
	class string_simple_t {
	private:
		typedef string_simple_t<t_char> t_self;
	public:
		t_size length() const {
			t_size s = m_buffer.get_size();
			if (s == 0) return 0; else return s - 1;
		}
		bool is_empty() const { return m_buffer.get_size() == 0; }

		void set_string_nc(const t_char* p_source, t_size p_length) {
			if (p_length == 0) {
				m_buffer.set_size(0);
			} else {
				m_buffer.set_size(p_length + 1);
				pfc::memcpy_t(m_buffer.get_ptr(), p_source, p_length);
				m_buffer[p_length] = 0;
			}
		}
		void set_string(const t_char* p_source) {
			set_string_nc(p_source, pfc::strlen_t(p_source));
		}
		void set_string(const t_char* p_source, t_size p_length) {
			set_string_nc(p_source, strlen_max_t(p_source, p_length));
		}
		void add_string(const t_char* p_source, t_size p_length) {
			add_string_nc(p_source, strlen_max_t(p_source, p_length));
		}
		void add_string(const t_char* p_source) {
			add_string_nc(p_source, strlen_t(p_source));
		}
		void add_string_nc(const t_char* p_source, t_size p_length) {
			if (p_length > 0) {
				t_size base = length();
				m_buffer.set_size(base + p_length + 1);
				memcpy_t(m_buffer.get_ptr() + base, p_source, p_length);
				m_buffer[base + p_length] = 0;
			}
		}
		string_simple_t() {}
		string_simple_t(const t_char* p_source) { set_string(p_source); }
		string_simple_t(const t_char* p_source, t_size p_length) { set_string(p_source, p_length); }
		const t_self& operator=(const t_char* p_source) { set_string(p_source); return *this; }
		operator const t_char* () const { return get_ptr(); }
		const t_char* get_ptr() const { return m_buffer.get_size() > 0 ? m_buffer.get_ptr() : pfc::empty_string_t<t_char>(); }
		const t_char* c_str() const { return get_ptr(); }
	private:
		pfc::array_t<t_char> m_buffer;
	};

	template<typename t_char> class traits_t<string_simple_t<t_char> > : public traits_t<array_t<t_char> > {};

}