#pragma once

namespace pfc {
    inline t_size _strParamLen(const char* str) { return strlen(str); }

    class NOVTABLE string_receiver {
    public:
        virtual void add_string(const char* p_string, t_size p_string_size = SIZE_MAX) = 0;
        inline void add_string_(const char* str) { add_string(str, _strParamLen(str)); }

        void add_char(t_uint32 c);//adds unicode char to the string
        void add_byte(char c) { add_string(&c, 1); }
        void add_chars(t_uint32 p_char, t_size p_count) { for (; p_count; p_count--) add_char(p_char); }
    protected:
        string_receiver() {}
        ~string_receiver() {}
    };

	class NOVTABLE string_base : public pfc::string_receiver {
	public:
		virtual const char* get_ptr() const = 0;
		const char* c_str() const { return get_ptr(); }
		virtual void add_string(const char* p_string, t_size p_length = SIZE_MAX) = 0;//same as string_receiver method
		virtual void set_string(const char* p_string, t_size p_length = SIZE_MAX) { reset(); add_string(p_string, p_length); }
		virtual void truncate(t_size len) = 0;
		virtual t_size get_length() const { return strlen(get_ptr()); }
		virtual char* lock_buffer(t_size p_requested_length) = 0;
		virtual void unlock_buffer() = 0;

		void set_string_(const char* str) { set_string(str, _strParamLen(str)); }

		inline const char* toString() const { return get_ptr(); }

		//! For compatibility with old conventions.
		inline t_size length() const { return get_length(); }

		inline void reset() { truncate(0); }
		inline void clear() { truncate(0); }

		inline bool is_empty() const { return *get_ptr() == 0; }

		void skip_trailing_chars(const char* lstChars);
		void skip_trailing_char(unsigned c = ' ');

		bool is_valid_utf8() const;

		void convert_to_lower_ascii(const char* src, char replace = '?');

		inline const string_base& operator= (const char* src) { set_string_(src); return *this; }
		inline const string_base& operator+= (const char* src) { add_string_(src); return *this; }
		inline const string_base& operator= (const string_base& src) { set_string(src); return *this; }
		inline const string_base& operator+= (const string_base& src) { add_string(src); return *this; }

		bool operator==(const string_base& p_other) const { return strcmp(*this, p_other) == 0; }
		bool operator!=(const string_base& p_other) const { return strcmp(*this, p_other) != 0; }
		bool operator>(const string_base& p_other) const { return strcmp(*this, p_other) > 0; }
		bool operator<(const string_base& p_other) const { return strcmp(*this, p_other) < 0; }
		bool operator>=(const string_base& p_other) const { return strcmp(*this, p_other) >= 0; }
		bool operator<=(const string_base& p_other) const { return strcmp(*this, p_other) <= 0; }

		inline operator const char* () const { return get_ptr(); }

		t_size scan_filename() const;

		t_size find_first(char p_char, t_size p_start = 0) const;
		t_size find_last(char p_char, t_size p_start = SIZE_MAX) const;
		t_size find_first(const char* p_string, t_size p_start = 0) const;
		t_size find_last(const char* p_string, t_size p_start = SIZE_MAX) const;

		void fix_dir_separator(char c = '\\'); // Backwards compat function, "do what I mean" applied on non Windows
		void end_with(char c);
		void end_with_slash();
		bool ends_with(char c) const;
		void delimit(const char* c) { if (length() > 0) add_string(c); }
		char last_char() const;
		void truncate_last_char();
		void truncate_number_suffix();

		bool truncate_eol(t_size start = 0);
		bool fix_eol(const char* append = " (...)", t_size start = 0);
		bool limit_length(t_size length_in_chars, const char* append = " (...)");

		void truncate_filename() { truncate(scan_filename()); }
		void truncate_to_parent_path();
		void add_filename(const char* fn) { end_with_slash(); *this += fn; }

		//! Replaces one string with another. Returns the number of occurances - zero if the string was not altered.
		size_t replace_string(const char* replace, const char* replaceWith, t_size start = 0);
		//! Replaces one string with another, writing the output to another string object. \n
		//! Returns the number of occurances replaced. \n
		//! Special: returns zero if no occurances were found - and the target string is NOT modified if so. Use with care!
		size_t replace_string_ex(pfc::string_base& target, const char* replace, const char* replaceWith, t_size start = 0) const;

		string_base& _formatter() const { return const_cast<string_base&>(*this); }

		bool has_prefix(const char* prefix) const;
		bool has_prefix_i(const char* prefix) const;
		bool has_suffix(const char* suffix) const;
		bool has_suffix_i(const char* suffix) const;

		bool equals(const char* other) const;
	protected:
		string_base() {}
		~string_base() {}
	};
}
