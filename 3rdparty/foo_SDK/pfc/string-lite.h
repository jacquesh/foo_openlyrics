#pragma once

#include "string-interface.h"
#include "mem_block.h"
#include "string-compare.h"

namespace pfc {

	typedef const char* stringp;
	inline size_t stringp_length(stringp s) noexcept { return strlen(s); }

	class stringLite : public string_base {
	public:
		class tagNoShrink {};
		stringLite(tagNoShrink) { this->setNoShrink(); }
		stringLite() {}
		stringLite( const stringLite & other ) { copy(other); }
		stringLite( stringLite && other ) noexcept { move(other); }
		stringLite( const char * p, size_t len = SIZE_MAX ) { set_string( p, len ); }
		stringLite( string_part_ref const & );
		stringLite( string_base const& );

		stringLite const & operator=( const stringLite & other ) { copy(other); return *this; }
		stringLite const & operator=( stringLite && other ) noexcept { move(other); return *this; }
		stringLite const & operator=( const char * p ) { set_string( p ); return *this; }
		stringLite const & operator=( const string_part_ref & );
		stringLite const & operator=( const string_base & );

		const char * get_ptr() const { return m_ptr; }
		operator const char*() const { return m_ptr; }
		const char* c_str() const { return m_ptr; }
		const char* ptr() const { return m_ptr; }
		void add_string(const char * p_string,t_size p_string_size = SIZE_MAX);
		void set_string(const char * p_string,t_size p_length = SIZE_MAX);
		void add_string_nc( const char * ptr, size_t length );
		void set_string_nc( const char * ptr, size_t length );
		void truncate(t_size len);
		t_size get_length() const { return m_length; }
		char * lock_buffer(t_size p_requested_length);
		void unlock_buffer();
		void set_char(size_t offset,char c);
		void prealloc(size_t);
		void setNoShrink(bool v = true) { m_noShrink = v; }

		size_t replace_nontext_chars(char p_replace = '_');
		size_t replace_char(unsigned c1,unsigned c2);
		size_t replace_byte(char c1,char c2);
		void remove_chars(t_size first,t_size count);
		void insert_chars(t_size first,const char * src, t_size count);
		void insert_chars_nc(t_size first,const char * src, t_size count);
		void insert_chars(t_size first,const char * src);

		bool operator==(const stringLite& other) const noexcept;
		bool operator!=(const stringLite& other) const noexcept;
		bool operator==(const char* other) const noexcept;
		bool operator!=(const char* other) const noexcept;
		bool operator>(const stringLite& other) const noexcept;
		bool operator<(const stringLite& other) const noexcept;
		bool operator>(const char* other) const noexcept;
		bool operator<(const char* other) const noexcept;
		bool operator>=(const stringLite& other) const noexcept;
		bool operator<=(const stringLite& other) const noexcept;
		bool operator>=(const char* other) const noexcept;
		bool operator<=(const char* other) const noexcept;


		stringLite const& operator+=(const stringLite& other) { add_string(other.c_str(), other.length()); return *this; }
		stringLite const& operator+=(const char* other) { add_string(other); return *this; }
		stringLite const& operator+=(string_part_ref other) { add_string_nc(other.m_ptr, other.m_len); return *this; }

		stringLite operator+(const stringLite& other) const {stringLite ret = *this; ret += other; return ret;}
		stringLite operator+(const char * other) const { stringLite ret = *this; ret += other; return ret; }
		stringLite operator+(string_part_ref other) const { stringLite ret = *this; ret += other; return ret; }

		bool equals(const stringLite& other) const noexcept;
		bool equals(const char* other) const noexcept;
		static bool equals( const stringLite & v1, const stringLite & v2 ) noexcept;
		static bool greater( const stringLite & v1, const stringLite & v2 ) noexcept;
		static bool greater( const char * v1, size_t s1, const char * v2, size_t s2) noexcept;

		void clear() noexcept;
		void copy( stringLite const & other );
		void move( stringLite & other ) noexcept;

		stringLite lowerCase() const;
		stringLite upperCase() const;
		stringLite toLower() const { return lowerCase(); }
		stringLite toUpper() const { return upperCase(); }

		typedef stringComparatorCaseSensitive comparatorCaseSensitive;
		typedef stringComparatorCaseInsensitive comparatorCaseInsensitive;
		typedef stringComparatorCaseInsensitiveASCII comparatorCaseInsensitiveASCII;

		static bool g_equals(const stringLite& p_item1, const stringLite& p_item2) { return p_item1 == p_item2; }
		static bool g_equalsCaseInsensitive(const stringLite& p_item1, const stringLite& p_item2) { return comparatorCaseInsensitive::compare(p_item1, p_item2) == 0; }

		stringLite subString(t_size base) const;
		stringLite subString(t_size base, t_size count) const;
		static bool isNonTextChar(char c) { return c >= 0 && c < 32; }

		//! @returns SIZE_MAX if not found.
		size_t indexOf(char c, t_size base = 0) const;
		//! @returns SIZE_MAX if not found.
		size_t lastIndexOf(char c, size_t base = SIZE_MAX) const;
		//! @returns SIZE_MAX if not found.
		size_t indexOf(stringp s, size_t base = 0) const;
		//! @returns SIZE_MAX if not found.
		size_t lastIndexOf(stringp s, size_t base = SIZE_MAX) const;
		//! @returns SIZE_MAX if not found.
		size_t indexOfAnyChar(stringp s, size_t base = 0) const;
		//! @returns SIZE_MAX if not found.
		size_t lastIndexOfAnyChar(stringp s, size_t base = SIZE_MAX) const;

		bool contains(char c) const;
		bool contains(stringp s) const;

		bool containsAnyChar(stringp s) const;

		bool startsWith(char c) const;
		bool startsWith(stringp s) const;
		bool endsWith(char c) const;
		bool endsWith(stringp s) const;

		char firstChar() const;
		char lastChar() const;

		bool isEmpty() const { return length() == 0; }
		
		stringLite replace(stringp strOld, stringp strNew) const;
		stringLite trim(char c) const;
	private:
		void _clear() noexcept;
		void makeRoom( size_t size );
		const char * m_ptr = "";
		size_t m_length = 0;
		mem_block m_mem;
		bool m_noShrink = false;
	};

	typedef stringLite string8;
	typedef stringLite string_formatter;
}
