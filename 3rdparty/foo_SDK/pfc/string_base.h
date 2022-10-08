#pragma once

#include <utility> // std::forward

#include "primitives.h"
#include "string-part.h"

namespace pfc {
	
	t_size scan_filename(const char * ptr);

	bool is_path_separator(unsigned c);
	bool is_path_bad_char(unsigned c);
	bool is_valid_utf8(const char * param,t_size max = SIZE_MAX);
    bool is_canonical_utf8(const char * param, size_t max = SIZE_MAX);
	bool is_lower_ascii(const char * param);
	bool is_multiline(const char * p_string,t_size p_len = SIZE_MAX);
	bool has_path_bad_chars(const char * param);
	void recover_invalid_utf8(const char * src,char * out,unsigned replace);//out must be enough to hold strlen(char) + 1, or appropiately bigger if replace needs multiple chars
	void convert_to_lower_ascii(const char * src,t_size max,char * out,char replace = '?');//out should be at least strlen(src)+1 long

	template<typename char_t> inline char_t ascii_tolower(char_t c) {if (c >= 'A' && c <= 'Z') c += 'a' - 'A'; return c;}
	template<typename char_t> inline char_t ascii_toupper(char_t c) {if (c >= 'a' && c <= 'z') c += 'A' - 'a'; return c;}

	t_size string_find_first(const char * p_string,char p_tofind,t_size p_start = 0);	//returns infinite if not found
	t_size string_find_last(const char * p_string,char p_tofind,t_size p_start = SIZE_MAX);	//returns infinite if not found
	t_size string_find_first(const char * p_string,const char * p_tofind,t_size p_start = 0);	//returns infinite if not found
	t_size string_find_last(const char * p_string,const char * p_tofind,t_size p_start = SIZE_MAX);	//returns infinite if not found

	t_size string_find_first_ex(const char * p_string,t_size p_string_length,char p_tofind,t_size p_start = 0);	//returns infinite if not found
	t_size string_find_last_ex(const char * p_string,t_size p_string_length,char p_tofind,t_size p_start = SIZE_MAX);	//returns infinite if not found
	t_size string_find_first_ex(const char * p_string,t_size p_string_length,const char * p_tofind,t_size p_tofind_length,t_size p_start = 0);	//returns infinite if not found
	t_size string_find_last_ex(const char * p_string,t_size p_string_length,const char * p_tofind,t_size p_tofind_length,t_size p_start = SIZE_MAX);	//returns infinite if not found


	t_size string_find_first_nc(const char * p_string,t_size p_string_length,char c,t_size p_start = 0); // lengths MUST be valid, no checks are performed (faster than the other flavour)
	t_size string_find_first_nc(const char * p_string,t_size p_string_length,const char * p_tofind,t_size p_tofind_length,t_size p_start = 0); // lengths MUST be valid, no checks are performed (faster than the other falvour);


    bool string_has_prefix( const char * string, const char * prefix );
    bool string_has_prefix_i( const char * string, const char * prefix );
	const char * string_skip_prefix_i(const char* string, const char* prefix);
    bool string_has_suffix( const char * string, const char * suffix );
    bool string_has_suffix_i( const char * string, const char * suffix );
	
	bool string_is_numeric(const char * p_string,t_size p_length = SIZE_MAX) noexcept;
	template<typename char_t> inline bool char_is_numeric(char_t p_char) noexcept {return p_char >= '0' && p_char <= '9';}
	inline bool char_is_hexnumeric(char p_char) noexcept {return char_is_numeric(p_char) || (p_char >= 'a' && p_char <= 'f') || (p_char >= 'A' && p_char <= 'F');}
	inline bool char_is_ascii_alpha_upper(char p_char) noexcept {return p_char >= 'A' && p_char <= 'Z';}
	inline bool char_is_ascii_alpha_lower(char p_char) noexcept {return p_char >= 'a' && p_char <= 'z';}
	inline bool char_is_ascii_alpha(char p_char) noexcept {return char_is_ascii_alpha_lower(p_char) || char_is_ascii_alpha_upper(p_char);}
	inline bool char_is_ascii_alphanumeric(char p_char) noexcept {return char_is_ascii_alpha(p_char) || char_is_numeric(p_char);}
	
	unsigned atoui_ex(const char * ptr,t_size max) noexcept;
	t_int64 atoi64_ex(const char * ptr,t_size max) noexcept;
	t_uint64 atoui64_ex(const char * ptr,t_size max) noexcept;
	
	//Throws exception_invalid_params on failure.
	unsigned char_to_hex(char c);
	unsigned char_to_dec(char c);

	//Throws exception_invalid_params or exception_overflow on failure.
	template<typename t_uint> t_uint atohex(const char * in, t_size inLen) {
		t_uint ret = 0;
		const t_uint guard = (t_uint)0xF << (sizeof(t_uint) * 8 - 4);
		for(t_size walk = 0; walk < inLen; ++walk) {
			if (ret & guard) throw exception_overflow();
			ret = (ret << 4) | char_to_hex(in[walk]);
		}
		return ret;
	}
	template<typename t_uint> t_uint atodec(const char * in, t_size inLen) {
		t_uint ret = 0;
		for(t_size walk = 0; walk < inLen; ++walk) {
			const t_uint prev = ret;
			ret = (ret * 10) + char_to_dec(in[walk]);
			if ((ret / 10) != prev) throw exception_overflow();
		}
		return ret;
	}

	t_size strlen_utf8(const char * s,t_size num = SIZE_MAX) noexcept;//returns number of characters in utf8 string; num - no. of bytes (optional)
	t_size utf8_char_len(const char * s,t_size max = SIZE_MAX) noexcept;//returns size of utf8 character pointed by s, in bytes, 0 on error
	t_size utf8_char_len_from_header(char c) noexcept;
	t_size utf8_chars_to_bytes(const char* string, t_size count) noexcept;

	size_t strcpy_utf8_truncate(const char * src,char * out,size_t maxbytes);

	template<typename char_t> void strcpy_t( char_t * out, const char_t * in ) {
		for(;;) { char_t c = *in++; *out++ = c; if (c == 0) break; }
	}

	t_size utf8_decode_char(const char * src,unsigned & out,t_size src_bytes) noexcept;//returns length in bytes
	t_size utf8_decode_char(const char * src,unsigned & out) noexcept;//returns length in bytes

	t_size utf8_encode_char(unsigned c,char * out) noexcept;//returns used length in bytes, max 6


	t_size utf16_decode_char(const char16_t * p_source,unsigned * p_out,t_size p_source_length = SIZE_MAX) noexcept;
	t_size utf16_encode_char(unsigned c,char16_t * out) noexcept;
    
#ifdef _MSC_VER
	t_size utf16_decode_char(const wchar_t * p_source,unsigned * p_out,t_size p_source_length = SIZE_MAX) noexcept;
	t_size utf16_encode_char(unsigned c,wchar_t * out) noexcept;
#endif

	t_size wide_decode_char(const wchar_t * p_source,unsigned * p_out,t_size p_source_length = SIZE_MAX) noexcept;
	t_size wide_encode_char(unsigned c,wchar_t * out) noexcept;


	t_size strstr_ex(const char * p_string,t_size p_string_len,const char * p_substring,t_size p_substring_len) noexcept;


	t_size skip_utf8_chars(const char * ptr,t_size count) noexcept;
	char * strdup_n(const char * src,t_size len);

	unsigned utf8_get_char(const char * src);

	inline bool utf8_advance(const char * & var) noexcept {
		t_size delta = utf8_char_len(var);
		var += delta;
		return delta>0;
	}

	inline bool utf8_advance(char * & var) noexcept {
		t_size delta = utf8_char_len(var);
		var += delta;
		return delta>0;
	}

	inline const char * utf8_char_next(const char * src) noexcept {return src + utf8_char_len(src);}
	inline char * utf8_char_next(char * src) noexcept {return src + utf8_char_len(src);}

	template<t_size max_length>
	class string_fixed_t : public pfc::string_base {
	public:
		inline string_fixed_t() {init();}
		inline string_fixed_t(const string_fixed_t<max_length> & p_source) {init(); *this = p_source;}
		inline string_fixed_t(const char * p_source) {init(); set_string(p_source);}
		
		inline const string_fixed_t<max_length> & operator=(const string_fixed_t<max_length> & p_source) {set_string(p_source);return *this;}
		inline const string_fixed_t<max_length> & operator=(const char * p_source) {set_string(p_source);return *this;}

		char * lock_buffer(t_size p_requested_length) {
			if (p_requested_length >= max_length) return NULL;
			memset(m_data,0,sizeof(m_data));
			return m_data;
		}
		void unlock_buffer() {
			m_length = strlen(m_data);
		}

		inline operator const char * () const {return m_data;}
		
		const char * get_ptr() const {return m_data;}

		void add_string(const char * ptr,t_size len) {
			len = strlen_max(ptr,len);
			if (m_length + len < m_length || m_length + len > max_length) throw pfc::exception_overflow();
			for(t_size n=0;n<len;n++) {
				m_data[m_length++] = ptr[n];
			}
			m_data[m_length] = 0;
		}
		void truncate(t_size len) {
			if (len > max_length) len = max_length;
			if (m_length > len) {
				m_length = len;
				m_data[len] = 0;
			}
		}
		t_size get_length() const {return m_length;}
	private:
		inline void init() {
			PFC_STATIC_ASSERT(max_length>1);
			m_length = 0; m_data[0] = 0;
		}
		t_size m_length;
		char m_data[max_length+1];
	};

	typedef stringLite string8_fastalloc;
    typedef stringLite string8_fast;
    typedef stringLite string8_fast_aggressive;
	typedef stringLite string_formatter;
	typedef stringLite string;

}

namespace pfc {

	class string_buffer {
	private:
		string_base & m_owner;
		char * m_buffer;
	public:
		explicit string_buffer(string_base & p_string,t_size p_requested_length) : m_owner(p_string) {m_buffer = m_owner.lock_buffer(p_requested_length);}
		~string_buffer() {m_owner.unlock_buffer();}
		char * get_ptr() {return m_buffer;}
		operator char* () {return m_buffer;}
	};

	string8 string_printf(const char * fmt, ...);
	string8 string_printf_va(const char * fmt, va_list list);
	void string_printf_here(string_base & out, const char * fmt, ...);
	void string_printf_here_va(string_base & out, const char * fmt, va_list list);

	string8 format_time(uint64_t seconds);
	string8 format_time_ex(double seconds, unsigned extra = 3);


	double parse_timecode( const char * tc );

	string8 string_filename(const char * fn);
	string8 string_filename_ext(const char * fn);

    const char * filename_ext_v2 ( const char * fn, char slash = 0 );

	size_t find_extension_offset(const char * src);
	string8 string_extension(const char * src);
	string8 string_replace_extension(const char * p_path, const char * p_ext);
	string8 string_directory(const char * p_path);

	void float_to_string(char * out,t_size out_max,double val,unsigned precision,bool force_sign = false);//doesnt add E+X etc, has internal range limits, useful for storing float numbers as strings without having to bother with international coma/dot settings BS
	double string_to_float(const char * src,t_size len = SIZE_MAX) noexcept;

	string8 format_float(double p_val,unsigned p_width = 0,unsigned p_prec = 7);

	struct format_int_t {
		char m_buffer[64] = {};
		inline const char* c_str() const { return m_buffer; }
		inline const char* get_ptr() const { return m_buffer; }
		inline operator const char* () const { return c_str(); }
	};

	format_int_t format_int(t_int64 p_val, unsigned p_width = 0, unsigned p_base = 10);
	format_int_t format_uint(t_uint64 p_val, unsigned p_width = 0, unsigned p_base = 10);
	format_int_t format_hex(t_uint64 p_val, unsigned p_width = 0);
	format_int_t format_hex_lowercase(t_uint64 p_val, unsigned p_width = 0);
	
	char format_hex_char_lowercase(unsigned p_val);
	char format_hex_char(unsigned p_val);

    
	//typedef string8_fastalloc string_formatter;
#define PFC_string_formatter() ::pfc::string_formatter()._formatter()

    string8 format_ptr( const void * ptr );
	string8 format_hexdump(const void * p_buffer,t_size p_bytes,const char * p_spacing = " ");
	string8 format_hexdump_lowercase(const void * p_buffer,t_size p_bytes,const char * p_spacing = " ");
	string8 format_fixedpoint(t_int64 p_val,unsigned p_point);

	string8 format_char(char c);

	string8 format_pad_left(t_size p_chars, t_uint32 p_padding /* = ' ' */, const char * p_string, t_size p_string_length = ~0);

	string8 format_pad_right(t_size p_chars, t_uint32 p_padding /* = ' ' */, const char * p_string, t_size p_string_length = ~0);

	string8 format_file_size_short(uint64_t size, uint64_t * outScaleUsed = nullptr);

}

inline pfc::string_base & operator<<(pfc::string_base & p_fmt,const char * p_source) {p_fmt.add_string_(p_source); return p_fmt;}
	   pfc::string_base & operator<<(pfc::string_base & p_fmt,const wchar_t* p_source); // string_conv.cpp
inline pfc::string_base & operator<<(pfc::string_base & p_fmt,pfc::string_part_ref source) {p_fmt.add_string(source.m_ptr, source.m_len); return p_fmt;}
inline pfc::string_base & operator<<(pfc::string_base & p_fmt,short p_val) {p_fmt.add_string(pfc::format_int(p_val)); return p_fmt;}
inline pfc::string_base & operator<<(pfc::string_base & p_fmt,unsigned short p_val) {p_fmt.add_string(pfc::format_uint(p_val)); return p_fmt;}
inline pfc::string_base & operator<<(pfc::string_base & p_fmt,int p_val) {p_fmt.add_string(pfc::format_int(p_val)); return p_fmt;}
inline pfc::string_base & operator<<(pfc::string_base & p_fmt,unsigned p_val) {p_fmt.add_string(pfc::format_uint(p_val)); return p_fmt;}
inline pfc::string_base & operator<<(pfc::string_base & p_fmt,long p_val) {p_fmt.add_string(pfc::format_int(p_val)); return p_fmt;}
inline pfc::string_base & operator<<(pfc::string_base & p_fmt,unsigned long p_val) {p_fmt.add_string(pfc::format_uint(p_val)); return p_fmt;}
inline pfc::string_base & operator<<(pfc::string_base & p_fmt,long long p_val) {p_fmt.add_string(pfc::format_int(p_val)); return p_fmt;}
inline pfc::string_base & operator<<(pfc::string_base & p_fmt,unsigned long long p_val) {p_fmt.add_string(pfc::format_uint(p_val)); return p_fmt;}
inline pfc::string_base & operator<<(pfc::string_base & p_fmt,double p_val) {p_fmt.add_string(pfc::format_float(p_val)); return p_fmt;}
inline pfc::string_base & operator<<(pfc::string_base & p_fmt,std::exception const & p_exception) {p_fmt.add_string(p_exception.what()); return p_fmt;}
inline pfc::string_base & operator<<(pfc::string_base & p_fmt,pfc::format_int_t const& in) { p_fmt.add_string(in.c_str()); return p_fmt; }


namespace pfc {


	template<typename t_source> string8 format_array(t_source const & source, const char * separator = ", ") {
		string8 ret;
		const t_size count = array_size_t(source);
		if (count > 0) {
			ret << source[0];
			for(t_size walk = 1; walk < count; ++walk) ret << separator << source[walk];
		}
		return ret;
	};


	template<typename TWord> string8 format_hexdump_ex(const TWord * buffer, t_size bufLen, const char * spacing = " ") {
		string8 ret;
		for(t_size n = 0; n < bufLen; n++) {
			if (n > 0 && spacing != NULL) ret << spacing;
			ret << format_hex(buffer[n],sizeof(TWord) * 2);
		}
		return ret;
	}





	typedef stringLite string_simple;
}


namespace pfc {


	void stringToUpperAppend(string_base & p_out, const char * p_source, t_size p_sourceLen);
	void stringToLowerAppend(string_base & p_out, const char * p_source, t_size p_sourceLen);
	t_uint32 charLower(t_uint32 param);
	t_uint32 charUpper(t_uint32 param);
	char ascii_tolower_lookup(char c);


	class string_base_ref : public string_base {
	public:
		string_base_ref(const char * ptr) : m_ptr(ptr), m_len(strlen(ptr)) {}
		const char * get_ptr() const {return m_ptr;}
		t_size get_length() const {return m_len;}
	private:
		void add_string(const char *,t_size) {throw pfc::exception_not_implemented();}
		void set_string(const char *,t_size) {throw pfc::exception_not_implemented();}
		void truncate(t_size) {throw pfc::exception_not_implemented();}
		char * lock_buffer(t_size) {throw pfc::exception_not_implemented();}
		void unlock_buffer() {throw pfc::exception_not_implemented();}
	private:
		const char * const m_ptr;
		t_size const m_len;
	};

	//! Writes a string to a fixed-size buffer. Truncates the string if necessary. Always writes a null terminator.
	template<typename TChar, t_size len, typename TSource>
	void stringToBuffer(TChar (&buffer)[len], const TSource & source) {
		PFC_STATIC_ASSERT(len>0);
		t_size walk;
		for(walk = 0; walk < len - 1 && source[walk] != 0; ++walk) {
			buffer[walk] = source[walk];
		}
		buffer[walk] = 0;
	}

	//! Same as stringToBuffer() but throws exception_overflow() if the string could not be fully written, including null terminator.
	template<typename TChar, t_size len, typename TSource>
	void stringToBufferGuarded(TChar (&buffer)[len], const TSource & source) {
		t_size walk;
		for(walk = 0; source[walk] != 0; ++walk) {
			if (walk >= len) throw exception_overflow();
			buffer[walk] = source[walk];
		}
		if (walk >= len) throw exception_overflow();
		buffer[walk] = 0;
	}


	void urlEncodeAppendRaw(pfc::string_base & out, const char * in, t_size inSize);
	void urlEncodeAppend(pfc::string_base & out, const char * in);
	void urlEncode(pfc::string_base & out, const char * in);


	char * strDup(const char * src); // POSIX strdup() clone, prevent MSVC complaining

	string8 lineEndingsToWin( const char * str );

	string8 stringToUpper( const char * str, size_t len = SIZE_MAX );
	string8 stringToLower( const char * str, size_t len = SIZE_MAX );

	template<typename t_source> static void stringCombine(pfc::string_base& out, t_source const& in, const char* separator, const char* separatorLast) {
		out.reset();
		for (typename t_source::const_iterator walk = in.first(); walk.is_valid(); ++walk) {
			if (!out.is_empty()) {
				if (walk == in.last()) out << separatorLast;
				else out << separator;
			}
			out << stringToPtr(*walk);
		}
	}

	template<typename TList>
	string stringCombineList(const TList& list, stringp separator) {
		typename TList::const_iterator iter = list.first();
		string acc;
		if (iter.is_valid()) {
			acc = *iter;
			for (++iter; iter.is_valid(); ++iter) {
				acc = acc + separator + *iter;
			}
		}
		return acc;
	}


	inline void formatHere(pfc::string_base&) {}
	template<typename first_t, typename ... args_t> void formatHere(pfc::string_base& out, first_t && first, args_t && ... args) {
		out << std::forward<first_t>(first);
		formatHere(out, std::forward<args_t>(args) ...);
	}
	

	template<typename ... args_t>
	inline string format(args_t && ... args) {
		string ret; formatHere(ret, std::forward<args_t>(args) ...); return ret;
	}

	pfc::string8 prefixLines(const char* str, const char* prefix, const char * setEOL = "\n");
}
