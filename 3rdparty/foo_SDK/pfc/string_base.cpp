#include "pfc-lite.h"
#include "string_base.h"
#include "pathUtils.h"
#include "primitives.h"
#include "other.h"
#include <set>
#include <math.h>
#include "splitString2.h"

namespace pfc {

bool string_base::is_valid_utf8() const { return pfc::is_valid_utf8(get_ptr()); }
t_size string_base::scan_filename() const { return pfc::scan_filename(get_ptr()); }
t_size string_base::find_first(char p_char, t_size p_start) const { return pfc::string_find_first(get_ptr(), p_char, p_start); }
t_size string_base::find_last(char p_char, t_size p_start) const { return pfc::string_find_last(get_ptr(), p_char, p_start); }
t_size string_base::find_first(const char* p_string, t_size p_start) const { return pfc::string_find_first(get_ptr(), p_string, p_start); }
t_size string_base::find_last(const char* p_string, t_size p_start) const { return pfc::string_find_last(get_ptr(), p_string, p_start); }
bool string_base::has_prefix(const char* prefix) const { return string_has_prefix(get_ptr(), prefix); }
bool string_base::has_prefix_i(const char* prefix) const { return string_has_prefix_i(get_ptr(), prefix); }
bool string_base::has_suffix(const char* suffix) const { return string_has_suffix(get_ptr(), suffix); }
bool string_base::has_suffix_i(const char* suffix) const { return string_has_suffix_i(get_ptr(), suffix); }
bool string_base::equals(const char* other) const { return strcmp(*this, other) == 0; }

void string_receiver::add_char(t_uint32 p_char)
{
	char temp[8];
	t_size len = utf8_encode_char(p_char,temp);
	if (len>0) add_string(temp,len);
}

void string_base::skip_trailing_chars( const char * lstCharsStr ) {
    std::set<unsigned> lstChars;
    for ( ;; ) {
        unsigned c;
        auto delta = utf8_decode_char( lstCharsStr, c );
        if ( delta == 0 ) break;
        lstCharsStr += delta;
        lstChars.insert( c );
    }
    
    const char * str = get_ptr();
    t_size ptr,trunc = 0;
    bool need_trunc = false;
    for(ptr=0;str[ptr];)
    {
        unsigned c;
        t_size delta = utf8_decode_char(str+ptr,c);
        if (delta==0) break;
        if ( lstChars.count( c ) > 0 )
        {
            if (!need_trunc) {
                need_trunc = true;
                trunc = ptr;
            }
        }
        else
        {
            need_trunc = false;
        }
        ptr += delta;
    }
    if (need_trunc) truncate(trunc);
}

void string_base::skip_trailing_char(unsigned skip)
{
	const char * str = get_ptr();
	t_size ptr,trunc = 0;
	bool need_trunc = false;
	for(ptr=0;str[ptr];)
	{
		unsigned c;
		t_size delta = utf8_decode_char(str+ptr,c);
		if (delta==0) break;
		if (c==skip)
		{
			if (!need_trunc) {
				need_trunc = true;
				trunc = ptr;
			}
		}
		else
		{
			need_trunc = false;
		}
		ptr += delta;
	}
	if (need_trunc) truncate(trunc);
}

string8 format_time(uint64_t p_seconds) {
	string8 ret;
	t_uint64 length = p_seconds;
	unsigned weeks,days,hours,minutes,seconds;
	
	weeks = (unsigned)( ( length / (60*60*24*7) ) );
	days = (unsigned)( ( length / (60*60*24) ) % 7 );
	hours = (unsigned) ( ( length / (60 * 60) ) % 24);
	minutes = (unsigned) ( ( length / (60 ) ) % 60 );
	seconds = (unsigned) ( ( length ) % 60 );

	if (weeks) {
		ret << weeks << "wk ";
	}
	if (days || weeks) {
		ret << days << "d ";
	}
	if (hours || days || weeks) {
		ret << hours << ":" << format_uint(minutes,2) << ":" << format_uint(seconds,2);
	} else {
		ret << minutes << ":" << format_uint(seconds,2);
	}
	return ret;
}

bool is_path_separator(unsigned c)
{
#ifdef _WIN32
	return c=='\\' || c=='/' || c=='|' || c==':';
#else
    return c == '/';
#endif
}

bool is_path_bad_char(unsigned c)
{
#ifdef _WINDOWS
	return c=='\\' || c=='/' || c=='|' || c==':' || c=='*' || c=='?' || c=='\"' || c=='>' || c=='<';
#else
	return c=='/' || c=='*' || c=='?';
#endif
}



char * strdup_n(const char * src,t_size len)
{
	len = strlen_max(src,len);
	char * ret = (char*)malloc(len+1);
	if (ret)
	{
		memcpy(ret,src,len);
		ret[len]=0;
	}
	return ret;
}

string8 string_filename(const char * fn)
{
	string8 ret;
	fn += pfc::scan_filename(fn);
	const char * ptr=fn,*dot=0;
	while(*ptr && *ptr!='?')
	{
		if (*ptr=='.') dot=ptr;
		ptr++;
	}

	if (dot && dot>fn) ret.set_string(fn,dot-fn);
	else ret.set_string(fn);
	return ret;
}

const char * filename_ext_v2( const char * fn, char slash ) {
    if ( slash == 0 ) {
		slash = pfc::io::path::getDefaultSeparator();
	}
    size_t split = pfc::string_find_last( fn, slash );
    if ( split == SIZE_MAX ) return fn;
	return fn + split + 1;
}

string8 string_filename_ext(const char * fn)
{
	string8 ret;
	fn += pfc::scan_filename(fn);
	const char * ptr = fn;
	while(*ptr && *ptr!='?') ptr++;
	ret.set_string(fn,ptr-fn);
	return ret;
}

size_t find_extension_offset(const char * src) {
	const char * start = src + pfc::scan_filename(src);
	const char * end = start + strlen(start);
	const char * ptr = end - 1;
	while (ptr > start && *ptr != '.')
	{
		if (*ptr == '?') end = ptr;
		ptr--;
	}

	if (ptr >= start && *ptr == '.')
	{
		return ptr - src;
	}

	return SIZE_MAX;
}

string8 string_extension(const char * src)
{
	string8 ret;
	const char * start = src + pfc::scan_filename(src);
	const char * end = start + strlen(start);
	const char * ptr = end-1;
	while(ptr>start && *ptr!='.')
	{
		if (*ptr=='?') end=ptr;
		ptr--;
	}

	if (ptr>=start && *ptr=='.')
	{
		ptr++;
		ret.set_string(ptr, end-ptr);
	}
	return ret;
}


bool has_path_bad_chars(const char * param)
{
	while(*param)
	{
		if (is_path_bad_char(*param)) return true;
		param++;
	}
	return false;
}

void float_to_string(char * out,t_size out_max,double val,unsigned precision,bool b_sign) {
	pfc::string_fixed_t<63> temp;
	t_size outptr;

	if (out_max == 0) return;
	out_max--;//for null terminator
	
	outptr = 0;	

	if (outptr == out_max) {out[outptr]=0;return;}

	if (val<0) {out[outptr++] = '-'; val = -val;}
	else if (val > 0 && b_sign) {out[outptr++] = '+';}

	if (outptr == out_max) {out[outptr]=0;return;}

	
	{
		double powval = pow((double)10.0,(double)precision);
		temp << (t_int64)floor(val * powval + 0.5);
		//_i64toa(blargh,temp,10);
	}
	
	const t_size temp_len = temp.length();
	if (temp_len <= precision)
	{
		out[outptr++] = '0';
		if (outptr == out_max) {out[outptr]=0;return;}
		out[outptr++] = '.';
		if (outptr == out_max) {out[outptr]=0;return;}
		t_size d;
		for(d=precision-temp_len;d;d--)
		{
			out[outptr++] = '0';
			if (outptr == out_max) {out[outptr]=0;return;}
		}
		for(d=0;d<temp_len;d++)
		{
			out[outptr++] = temp[d];
			if (outptr == out_max) {out[outptr]=0;return;}
		}
	}
	else
	{
		t_size d = temp_len;
		const char * src = temp;
		while(*src)
		{
			if (d-- == precision)
			{
				out[outptr++] = '.';
				if (outptr == out_max) {out[outptr]=0;return;}
			}
			out[outptr++] = *(src++);
			if (outptr == out_max) {out[outptr]=0;return;}
		}
	}
	out[outptr] = 0;
}


    
static double pfc_string_to_float_internal(const char * src) noexcept
{
	bool neg = false;
	t_int64 val = 0;
	int div = 0;
	bool got_dot = false;

	while(*src==' ') src++;

	if (*src=='-') {neg = true;src++;}
	else if (*src=='+') src++;
	
	while(*src)
	{
		if (*src>='0' && *src<='9')
		{
			int d = *src - '0';
			val = val * 10 + d;
			if (got_dot) div--;
			src++;
		}
		else if (*src=='.' || *src==',')
		{
			if (got_dot) break;
			got_dot = true;
			src++;
		}
		else if (*src=='E' || *src=='e')
		{
			src++;
			div += atoi(src);
			break;
		}
		else break;
	}
	if (neg) val = -val;

	if (val != 0) {
		// SPECIAL FIX: ensure 0.2 and 0.200000 return the EXACT same float
		while (val % 10 == 0) {
			val /= 10; ++div;
		}
	}
    return (double) val * exp_int(10, div);
}

double string_to_float(const char * src,t_size max) noexcept {
	//old function wants an oldstyle nullterminated string, and i don't currently care enough to rewrite it as it works appropriately otherwise
	char blargh[128];
	if (max > 127) max = 127;
	t_size walk;
	for(walk = 0; walk < max && src[walk]; walk++) blargh[walk] = src[walk];
	blargh[walk] = 0;
	return pfc_string_to_float_internal(blargh);
}



void string_base::convert_to_lower_ascii(const char * src,char replace)
{
	reset();
	PFC_ASSERT(replace>0);
	while(*src)
	{
		unsigned c;
		t_size delta = utf8_decode_char(src,c);
		if (delta==0) {c = replace; delta = 1;}
		else if (c>=0x80) c = replace;
		add_byte((char)c);
		src += delta;
	}
}

void convert_to_lower_ascii(const char * src,t_size max,char * out,char replace)
{
	t_size ptr = 0;
	PFC_ASSERT(replace>0);
	while(ptr<max && src[ptr])
	{
		unsigned c;
		t_size delta = utf8_decode_char(src+ptr,c,max-ptr);
		if (delta==0) {c = replace; delta = 1;}
		else if (c>=0x80) c = replace;
		*(out++) = (char)c;
		ptr += delta;
	}
	*out = 0;
}

t_size strstr_ex(const char * p_string,t_size p_string_len,const char * p_substring,t_size p_substring_len) noexcept
{
	p_string_len = strlen_max(p_string,p_string_len);
	p_substring_len = strlen_max(p_substring,p_substring_len);
	t_size index = 0;
	while(index + p_substring_len <= p_string_len)
	{
		if (memcmp(p_string+index,p_substring,p_substring_len) == 0) return index;
		t_size delta = utf8_char_len(p_string+index,p_string_len - index);
		if (delta == 0) break;
		index += delta;
	}
	return SIZE_MAX;
}

unsigned atoui_ex(const char * p_string,t_size p_string_len) noexcept
{
	unsigned ret = 0; t_size ptr = 0;
	while(ptr<p_string_len)
	{
		char c = p_string[ptr];
		if (! ( c >= '0' && c <= '9' ) ) break;
		ret = ret * 10 + (unsigned)( c - '0' );
		ptr++;
	}
	return ret;
}

int strcmp_nc(const char* p1, size_t n1, const char * p2, size_t n2) noexcept {
	t_size idx = 0;
	for(;;)
	{
		if (idx == n1 && idx == n2) return 0;
		else if (idx == n1) return -1;//end of param1
		else if (idx == n2) return 1;//end of param2

		char c1 = p1[idx], c2 = p2[idx];
		if (c1<c2) return -1;
		else if (c1>c2) return 1;
		
		idx++;
	}
}

int strcmp_ex(const char* p1,t_size n1,const char* p2,t_size n2) noexcept
{
	n1 = strlen_max(p1,n1); n2 = strlen_max(p2,n2);
	return strcmp_nc(p1, n1, p2, n2);
}

t_uint64 atoui64_ex(const char * src,t_size len) noexcept {
	len = strlen_max(src,len);
	t_uint64 ret = 0, mul = 1;
	t_size ptr = len;
	t_size start = 0;
//	start += skip_spacing(src+start,len-start);
	
	while(ptr>start)
	{
		char c = src[--ptr];
		if (c>='0' && c<='9')
		{
			ret += (c-'0') * mul;
			mul *= 10;
		}
		else
		{
			ret = 0;
			mul = 1;
		}
	}
	return ret;
}


t_int64 atoi64_ex(const char * src,t_size len) noexcept
{
	len = strlen_max(src,len);
	t_int64 ret = 0, mul = 1;
	t_size ptr = len;
	t_size start = 0;
	bool neg = false;
//	start += skip_spacing(src+start,len-start);
	if (start < len && src[start] == '-') {neg = true; start++;}
//	start += skip_spacing(src+start,len-start);
	
	while(ptr>start)
	{
		char c = src[--ptr];
		if (c>='0' && c<='9')
		{
			ret += (c-'0') * mul;
			mul *= 10;
		}
		else
		{
			ret = 0;
			mul = 1;
		}
	}
	return neg ? -ret : ret;
}


string8 format_float(double p_val,unsigned p_width,unsigned p_prec)
{
	string8 m_buffer;
	char temp[64];
	float_to_string(temp,64,p_val,p_prec,false);
	temp[63] = 0;
	t_size len = strlen(temp);
	if (len < p_width)
		m_buffer.add_chars(' ',p_width-len);
	m_buffer += temp;
	return m_buffer;
}

char format_hex_char(unsigned p_val)
{
	PFC_ASSERT(p_val < 16);
	return (p_val < 10) ? p_val + '0' : p_val - 10 + 'A';
}

format_int_t format_hex(t_uint64 p_val,unsigned p_width)
{
	format_int_t ret;

	if (p_width > 16) p_width = 16;
	else if (p_width == 0) p_width = 1;
	char temp[16];
	unsigned n;
	for(n=0;n<16;n++)
	{
		temp[15-n] = format_hex_char((unsigned)(p_val & 0xF));
		p_val >>= 4;
	}

	for(n=0;n<16 && temp[n] == '0';n++) {}
	
	if (n > 16 - p_width) n = 16 - p_width;
	
	char * out = ret.m_buffer;
	for(;n<16;n++)
		*(out++) = temp[n];
	*out = 0;
	return ret;
}

char format_hex_char_lowercase(unsigned p_val)
{
	PFC_ASSERT(p_val < 16);
	return (p_val < 10) ? p_val + '0' : p_val - 10 + 'a';
}

format_int_t format_hex_lowercase(t_uint64 p_val,unsigned p_width)
{
	format_int_t ret;
	if (p_width > 16) p_width = 16;
	else if (p_width == 0) p_width = 1;
	char temp[16];
	unsigned n;
	for(n=0;n<16;n++)
	{
		temp[15-n] = format_hex_char_lowercase((unsigned)(p_val & 0xF));
		p_val >>= 4;
	}

	for(n=0;n<16 && temp[n] == '0';n++) {}
	
	if (n > 16 - p_width) n = 16 - p_width;
	
	char * out = ret.m_buffer;
	for(;n<16;n++)
		*(out++) = temp[n];
	*out = 0;
	return ret;
}

format_int_t format_uint(t_uint64 val,unsigned p_width,unsigned p_base)
{
	format_int_t ret;
	
	enum {max_width = PFC_TABSIZE(ret.m_buffer) - 1};

	if (p_width > max_width) p_width = max_width;
	else if (p_width == 0) p_width = 1;

	char temp[max_width];
	
	unsigned n;
	for(n=0;n<max_width;n++)
	{
		temp[max_width-1-n] = format_hex_char((unsigned)(val % p_base));
		val /= p_base;
	}

	for(n=0;n<max_width && temp[n] == '0';n++) {}
	
	if (n > max_width - p_width) n = max_width - p_width;
	
	char * out = ret.m_buffer;

	for(;n<max_width;n++)
		*(out++) = temp[n];
	*out = 0;
	
	return ret;
}

string8 format_fixedpoint(t_int64 p_val,unsigned p_point)
{
	string8 m_buffer;
	unsigned div = 1;
	for(unsigned n=0;n<p_point;n++) div *= 10;

	if (p_val < 0) {m_buffer << "-";p_val = -p_val;}

	
	m_buffer << format_int(p_val / div) << "." << format_int(p_val % div, p_point);
	return m_buffer;
}


format_int_t format_int(t_int64 p_val,unsigned p_width,unsigned p_base)
{
	format_int_t ret;
	bool neg = false;
	t_uint64 val;
	if (p_val < 0) {neg = true; val = (t_uint64)(-p_val);}
	else val = (t_uint64)p_val;
	
	enum {max_width = PFC_TABSIZE(ret.m_buffer) - 1};

	if (p_width > max_width) p_width = max_width;
	else if (p_width == 0) p_width = 1;

	if (neg && p_width > 1) p_width --;
	
	char temp[max_width];
	
	unsigned n;
	for(n=0;n<max_width;n++)
	{
		temp[max_width-1-n] = format_hex_char((unsigned)(val % p_base));
		val /= p_base;
	}

	for(n=0;n<max_width && temp[n] == '0';n++) {}
	
	if (n > max_width - p_width) n = max_width - p_width;
	
	char * out = ret.m_buffer;

	if (neg) *(out++) = '-';

	for(;n<max_width;n++)
		*(out++) = temp[n];
	*out = 0;

	return ret;
}

string8 format_hexdump_lowercase(const void * p_buffer,t_size p_bytes,const char * p_spacing)
{
	string8 m_formatter;
	t_size n;
	const t_uint8 * buffer = (const t_uint8*)p_buffer;
	for(n=0;n<p_bytes;n++)
	{
		if (n > 0 && p_spacing != 0) m_formatter << p_spacing;
		m_formatter << format_hex_lowercase(buffer[n],2);
	}
	return m_formatter;
}

string8 format_hexdump(const void * p_buffer,t_size p_bytes,const char * p_spacing)
{
	string8 m_formatter;
	t_size n;
	const t_uint8 * buffer = (const t_uint8*)p_buffer;
	for(n=0;n<p_bytes;n++)
	{
		if (n > 0 && p_spacing != 0) m_formatter << p_spacing;
		m_formatter << format_hex(buffer[n],2);
	}
	return m_formatter;
}



string8 string_replace_extension(const char * p_path,const char * p_ext)
{
	string8 m_data;
	m_data = p_path;
	t_size dot = m_data.find_last('.');
	if (dot < m_data.scan_filename())
	{//argh
		m_data += ".";
		m_data += p_ext;
	}
	else
	{
		m_data.truncate(dot+1);
		m_data += p_ext;
	}
	return m_data;
}

string8 string_directory(const char * p_path)
{
	string8 ret;
	t_size ptr = scan_filename(p_path);
	if (ptr > 1) {
		if (is_path_separator(p_path[ptr-1]) && !is_path_separator(p_path[ptr-2])) --ptr;
	}
	ret.set_string(p_path,ptr);
	return ret;
}

t_size scan_filename(const char * ptr)
{
	t_size n;
	t_size _used = strlen(ptr);
	for(n=_used;n!=0;n--)
	{
		if (is_path_separator(ptr[n-1])) return n;
	}
	return 0;
}



t_size string_find_first(const char * p_string,char p_tofind,t_size p_start) {
	for(t_size walk = p_start; p_string[walk]; ++walk) {
		if (p_string[walk] == p_tofind) return walk;
	}
	return SIZE_MAX;
}
t_size string_find_last(const char * p_string,char p_tofind,t_size p_start) {
	return string_find_last_ex(p_string,SIZE_MAX,&p_tofind,1,p_start);
}
t_size string_find_first(const char * p_string,const char * p_tofind,t_size p_start) {
	return string_find_first_ex(p_string,SIZE_MAX,p_tofind,SIZE_MAX,p_start);
}
t_size string_find_last(const char * p_string,const char * p_tofind,t_size p_start) {
	return string_find_last_ex(p_string,SIZE_MAX,p_tofind,SIZE_MAX,p_start);
}

t_size string_find_first_ex(const char * p_string,t_size p_string_length,char p_tofind,t_size p_start) {
	for(t_size walk = p_start; walk < p_string_length && p_string[walk]; ++walk) {
		if (p_string[walk] == p_tofind) return walk;
	}
	return SIZE_MAX;
}
t_size string_find_last_ex(const char * p_string,t_size p_string_length,char p_tofind,t_size p_start) {
	return string_find_last_ex(p_string,p_string_length,&p_tofind,1,p_start);
}
t_size string_find_first_ex(const char * p_string,t_size p_string_length,const char * p_tofind,t_size p_tofind_length,t_size p_start) {
	p_string_length = strlen_max(p_string,p_string_length); p_tofind_length = strlen_max(p_tofind,p_tofind_length);
	if (p_string_length >= p_tofind_length) {
		t_size max = p_string_length - p_tofind_length;
		for(t_size walk = p_start; walk <= max; walk++) {
			if (_strcmp_partial_ex(p_string+walk,p_string_length-walk,p_tofind,p_tofind_length) == 0) return walk;
		}
	}
	return SIZE_MAX;
}
t_size string_find_last_ex(const char * p_string,t_size p_string_length,const char * p_tofind,t_size p_tofind_length,t_size p_start) {
	p_string_length = strlen_max(p_string,p_string_length); p_tofind_length = strlen_max(p_tofind,p_tofind_length);
	if (p_string_length >= p_tofind_length) {
		t_size max = min_t<t_size>(p_string_length - p_tofind_length,p_start);
		for(t_size walk = max; walk != (t_size)(-1); walk--) {
			if (_strcmp_partial_ex(p_string+walk,p_string_length-walk,p_tofind,p_tofind_length) == 0) return walk;
		}
	}
	return SIZE_MAX;
}

t_size string_find_first_nc(const char * p_string,t_size p_string_length,char c,t_size p_start) {
	for(t_size walk = p_start; walk < p_string_length; walk++) {
		if (p_string[walk] == c) return walk;
	}
	return SIZE_MAX;
}

t_size string_find_first_nc(const char * p_string,t_size p_string_length,const char * p_tofind,t_size p_tofind_length,t_size p_start) {
	if (p_string_length >= p_tofind_length) {
		t_size max = p_string_length - p_tofind_length;
		for(t_size walk = p_start; walk <= max; walk++) {
			if (memcmp(p_string+walk, p_tofind, p_tofind_length) == 0) return walk;
		}
	}
	return SIZE_MAX;
}


bool string_is_numeric(const char * p_string,t_size p_length) noexcept {
	bool retval = false;
	for(t_size walk = 0; walk < p_length && p_string[walk] != 0; walk++) {
		if (!char_is_numeric(p_string[walk])) {retval = false; break;}
		retval = true;
	}
	return retval;
}


void string_base::end_with(char p_char) {
	if (!ends_with(p_char)) add_byte(p_char);
}
bool string_base::ends_with(char c) const {
	t_size length = get_length();
	return length > 0 && get_ptr()[length-1] == c;
}

void string_base::end_with_slash() {
	end_with( io::path::getDefaultSeparator() );
}

char string_base::last_char() const {
    size_t l = this->length();
    if (l == 0) return 0;
    return this->get_ptr()[l-1];
}
void string_base::truncate_last_char() {
    size_t l = this->length();
    if (l > 0) this->truncate( l - 1 );
}
    
void string_base::truncate_number_suffix() {
    size_t l = this->length();
    const char * const p = this->get_ptr();
    while( l > 0 && char_is_numeric( p[l-1] ) ) --l;
    truncate( l );
}
    
bool is_multiline(const char * p_string,t_size p_len) {
	for(t_size n = 0; n < p_len && p_string[n]; n++) {
		switch(p_string[n]) {
		case '\r':
		case '\n':
			return true;
		}
	}
	return false;
}

static t_uint64 pow10_helper(unsigned p_extra) {
	t_uint64 ret = 1;
	for(unsigned n = 0; n < p_extra; n++ ) ret *= 10;
	return ret;
}

static uint64_t safeMulAdd(uint64_t prev, unsigned scale, uint64_t add) {
	if (add >= scale || scale == 0) throw pfc::exception_invalid_params();
	uint64_t v = prev * scale + add;
	if (v / scale != prev) throw pfc::exception_invalid_params();
	return v;
}

static size_t parseNumber(const char * in, uint64_t & outNumber) {
	size_t walk = 0;
	uint64_t total = 0;
	for (;;) {
		char c = in[walk];
		if (!pfc::char_is_numeric(c)) break;
		unsigned v = (unsigned)(c - '0');
		uint64_t newVal = total * 10 + v;
		if (newVal / 10 != total) throw pfc::exception_overflow();
		total = newVal;
		++walk;
	}
	outNumber = total;
	return walk;
}

double parse_timecode(const char * in) {
	char separator = 0;
	uint64_t seconds = 0;
	unsigned colons = 0;
	for (;;) {
		uint64_t number = 0;
		size_t digits = parseNumber(in, number);
		if (digits == 0) throw pfc::exception_invalid_params();
		in += digits;
		char nextSeparator = *in;
		switch (separator) { // *previous* separator
		case '.':
			if (nextSeparator != 0) throw pfc::exception_bug_check();
			return (double)seconds + (double)pfc::exp_int(10, -(int)digits) * number;
		case 0: // is first number in the string
			seconds = number;
			break;
		case ':':
			if (colons == 2) throw pfc::exception_invalid_params();
			++colons;
			seconds = safeMulAdd(seconds, 60, number);
			break;
		}

		if (nextSeparator == 0) {
			// end of string
			return (double)seconds;
		}

		++in;
		separator = nextSeparator;
	}
}

string8 format_time_ex(double p_seconds,unsigned p_extra) {
	string8 ret;
	if (p_seconds < 0) {ret << "-"; p_seconds = -p_seconds;}
	t_uint64 pow10 = pow10_helper(p_extra);
	t_uint64 ticks = pfc::rint64(pow10 * p_seconds);

	ret << pfc::format_time(ticks / pow10);
	if (p_extra>0) {
		ret << "." << pfc::format_uint(ticks % pow10, p_extra);
	}
	return ret;
}

void stringToUpperAppend(string_base & out, const char * src, t_size len) {
	while(len && *src) {
		unsigned c; t_size d;
		d = utf8_decode_char(src,c,len);
		if (d==0 || d>len) break;
		out.add_char(charUpper(c));
		src+=d;
		len-=d;
	}
}
void stringToLowerAppend(string_base & out, const char * src, t_size len) {
	while(len && *src) {
		unsigned c; t_size d;
		d = utf8_decode_char(src,c,len);
		if (d==0 || d>len) break;
		out.add_char(charLower(c));
		src+=d;
		len-=d;
	}
}

string8 format_file_size_short(uint64_t size, uint64_t * outUsedScale) {
	string8 ret;
	t_uint64 scale = 1;
	const char * unit = "B";
	const char * const unitTable[] = {"B","KB","MB","GB","TB"};
	for(t_size walk = 1; walk < PFC_TABSIZE(unitTable); ++walk) {
		t_uint64 next = scale * 1024;
		if (size < next) break;
		scale = next; unit = unitTable[walk];
	}
	ret << ( size  / scale );

	if (scale > 1 && ret.length() < 3) {
		t_size digits = 3 - ret.length();
		const t_uint64 mask = pow_int(10,digits);
		t_uint64 remaining = ( (size * mask / scale) % mask );
		while(digits > 0 && (remaining % 10) == 0) {
			remaining /= 10; --digits;
		}
		if (digits > 0) {
			ret << "." << format_uint(remaining, (t_uint32)digits);
		}
	}
	ret << " " << unit;
	if (outUsedScale != nullptr) *outUsedScale = scale;
	return ret;
}

bool string_base::truncate_eol(t_size start)
{
	const char * ptr = get_ptr() + start;
	for(t_size n=start;*ptr;n++)
	{
		if (*ptr==10 || *ptr==13)
		{
			truncate(n);
			return true;
		}
		ptr++;
	}
	return false;
}

bool string_base::fix_eol(const char * append,t_size start)
{
	const bool rv = truncate_eol(start);
	if (rv) add_string(append);
	return rv;
}

bool string_base::limit_length(t_size length_in_chars,const char * append)
{
	bool rv = false;
	const char * base = get_ptr(), * ptr = base;
	while(length_in_chars && utf8_advance(ptr)) length_in_chars--;
	if (length_in_chars==0)
	{
		truncate(ptr-base);
		add_string(append);
		rv = true;
	}
	return rv;
}

void string_base::truncate_to_parent_path() {
	size_t at = scan_filename();
#ifdef _WIN32
	while(at > 0 && (*this)[at-1] == '\\') --at;
	if (at > 0 && (*this)[at-1] == ':' && (*this)[at] == '\\') ++at;
#else
	// Strip trailing /
	while(at > 0 && (*this)[at-1] == '/') --at;

	// Hit empty? Bring root / back to life
	if (at == 0 && (*this)[0] == '/') ++at;

	// Deal with proto://
	if (at > 0 && (*this)[at-1] == ':') {
		while((*this)[at] == '/') ++at;
	}
#endif
	this->truncate( at );
}

size_t string_base::replace_string(const char * replace, const char * replaceWith, t_size start) {
	string_formatter temp;
	size_t ret = replace_string_ex(temp, replace, replaceWith, start);
	if ( ret > 0 ) * this = temp;
	return ret;
}
size_t string_base::replace_string_ex (string_base & temp, const char * replace, const char * replaceWith, t_size start) const {
    size_t srcDone = 0, walk = start;
    size_t occurances = 0;
    const char * const source = this->get_ptr();
    bool clear = false;
    const size_t replaceLen = strlen( replace );
    for(;;) {
        const char * ptr = strstr( source + walk, replace );
        if (ptr == NULL) {
            // end
            if (srcDone == 0) {
                return 0; // string not altered
            }
            temp.add_string( source + srcDone );
            break;
        }
        ++occurances;
        walk = ptr - source;
		if (! clear ) {
			temp.reset();
			clear = true;
		}
        temp.add_string( source + srcDone, walk - srcDone );
        temp.add_string( replaceWith );
        walk += replaceLen;
        srcDone = walk;
    }
    return occurances;
}

void urlEncodeAppendRaw(pfc::string_base & out, const char * in, t_size inSize) {
	for(t_size walk = 0; walk < inSize; ++walk) {
		const char c = in[walk];
		if (c == ' ') out.add_byte('+');
		else if (pfc::char_is_ascii_alphanumeric(c) || c == '_') out.add_byte(c);
		else out << "%" << pfc::format_hex((t_uint8)c, 2);
	}
}
void urlEncodeAppend(pfc::string_base & out, const char * in) {
	for(;;) {
		const char c = *(in++);
		if (c == 0) break;
		else if (c == ' ') out.add_byte('+');
		else if (pfc::char_is_ascii_alphanumeric(c) || c == '_') out.add_byte(c);
		else out << "%" << pfc::format_hex((t_uint8)c, 2);
	}
}
void urlEncode(pfc::string_base & out, const char * in) {
	out.reset(); urlEncodeAppend(out, in);
}

unsigned char_to_dec(char c) {
	PFC_ASSERT(c != 0);
	if (c >= '0' && c <= '9') return (unsigned)(c - '0');
	else throw exception_invalid_params();
}

unsigned char_to_hex(char c) {
	if (c >= '0' && c <= '9') return (unsigned)(c - '0');
	else if (c >= 'a' && c <= 'f') return (unsigned)(c - 'a' + 10);
	else if (c >= 'A' && c <= 'F') return (unsigned)(c - 'A' + 10);
	else throw exception_invalid_params();
}


static const t_uint8 ascii_tolower_table[128] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x5B,0x5C,0x5D,0x5E,0x5F,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F};

uint32_t charLower(uint32_t param)
{
	if (param<128) {
		return ascii_tolower_table[param];
	}
#ifdef PFC_WINDOWS_DESKTOP_APP
	else if (param<0x10000) {
		return (uint32_t)(size_t)CharLowerW((WCHAR*)(size_t)param);
	}
#endif
	else return param;
}

uint32_t charUpper(uint32_t param)
{
	if (param<128) {
		if (param>='a' && param<='z') param += 'A' - 'a';
		return param;
	}
#ifdef PFC_WINDOWS_DESKTOP_APP
	else if (param<0x10000) {
		return (uint32_t)(size_t)CharUpperW((WCHAR*)(size_t)param);
	}
#endif
	else return param;
}


bool stringEqualsI_ascii(const char * p1,const char * p2) noexcept {
	for(;;)
	{
		char c1 = *p1;
		char c2 = *p2;
		if (c1 > 0 && c2 > 0) {
			if (ascii_tolower_table[ (unsigned) c1 ] != ascii_tolower_table[ (unsigned) c2 ]) return false;
		} else {
			if (c1 == 0 && c2 == 0) return true;
			if (c1 == 0 || c2 == 0) return false;
			if (c1 != c2) return false;
		}
		++p1; ++p2;
	}
}

bool stringEqualsI_utf8(const char * p1,const char * p2) noexcept
{
	for(;;)
	{
		char c1 = *p1;
		char c2 = *p2;
		if (c1 > 0 && c2 > 0) {
			if (ascii_tolower_table[ (unsigned) c1 ] != ascii_tolower_table[ (unsigned) c2 ]) return false;
			++p1; ++p2;
		} else {
			if (c1 == 0 && c2 == 0) return true;
			if (c1 == 0 || c2 == 0) return false;
			unsigned w1,w2; t_size d1,d2;
			d1 = utf8_decode_char(p1,w1);
			d2 = utf8_decode_char(p2,w2);
			if (d1 == 0 || d2 == 0) return false; // bad UTF-8, bail
			if (w1 != w2) {
				if (charLower(w1) != charLower(w2)) return false;
			}
			p1 += d1;
			p2 += d2;
		}
	}
}

char ascii_tolower_lookup(char c) {
	PFC_ASSERT( c >= 0);
	return (char)ascii_tolower_table[ (unsigned) c ];
}

void string_base::fix_dir_separator(char c) {
#ifdef _WIN32
    end_with(c);
#else
    end_with_slash();
#endif
}
    

    bool string_has_prefix( const char * string, const char * prefix ) {
        for(size_t w = 0; ; ++w ) {
            char c = prefix[w];
            if (c == 0) return true;
            if (string[w] != c) return false;
        }
    }
	const char* string_skip_prefix_i(const char* string, const char* prefix) {
		const char* p1 = string; const char* p2 = prefix;
		for (;;) {
			unsigned w1, w2; size_t d1, d2;
			d1 = utf8_decode_char(p1, w1);
			d2 = utf8_decode_char(p2, w2);
			if (d2 == 0) return p1;
			if (d1 == 0) return nullptr;
			if (w1 != w2) {
				if (charLower(w1) != charLower(w2)) return nullptr;
			}
			p1 += d1; p2 += d2;
		}
	}
    bool string_has_prefix_i( const char * string, const char * prefix ) {
		return string_skip_prefix_i(string, prefix) != nullptr;
    }
    bool string_has_suffix( const char * string, const char * suffix ) {
        size_t len = strlen( string );
        size_t suffixLen = strlen( suffix );
        if (suffixLen > len) return false;
        size_t base = len - suffixLen;
        return memcmp( string + base, suffix, suffixLen * sizeof(char)) == 0;
    }
    bool string_has_suffix_i( const char * string, const char * suffix ) {
        for(;;) {
            if (*string == 0) return false;
            if (stringEqualsI_utf8( string, suffix )) return true;
            if (!utf8_advance(string)) return false;
        }
    }

	char * strDup(const char * src) {
#ifdef _MSC_VER
		return _strdup(src);
#else
		return strdup(src);
#endif
	}


	string_part_ref string_part_ref::make(const char * ptr, t_size len) {
		string_part_ref val = {ptr, len}; return val;
	}

	string_part_ref string_part_ref::substring(t_size base) const {
		PFC_ASSERT( base <= m_len );
		return make(m_ptr + base, m_len - base);
	}
	string_part_ref string_part_ref::substring(t_size base, t_size len) const {
		PFC_ASSERT( base <= m_len && base + len <= m_len );
		return make(m_ptr + base, len);
	}

	string_part_ref string_part_ref::make( const char * str ) {return make( str, strlen(str) ); }

	bool string_part_ref::equals( string_part_ref other ) const {
		if ( other.m_len != this->m_len ) return false;
		return memcmp( other.m_ptr, this->m_ptr, m_len ) == 0;
	}
	bool string_part_ref::equals( const char * str ) const {
		return equals(make(str) );
	}

	string8 lineEndingsToWin(const char * str) {
		string8 ret;
		const char * walk = str;
		for( ;; ) {
			const char * eol = strchr( walk, '\n' );
			if ( eol == nullptr ) {
				ret += walk; break;
			}
			const char * next = eol + 1;
			if ( eol > walk ) {
				if (eol[-1] == '\r') --eol;
				if ( eol > walk ) ret.add_string_nc(walk, eol-walk);
			}
			ret.add_string_nc("\r\n",2);
			walk = next;
		}
		return ret;
	}


	string8 format_char(char c) {
		string8 ret; ret.add_byte(c); return ret;
	}

    string8 format_ptr( const void * ptr ) {
        string8 temp;
        temp << "0x";
        temp << format_hex_lowercase( (size_t) ptr, sizeof(ptr) * 2 );
        return temp;
    }


	string8 format_pad_left(t_size p_chars, t_uint32 p_padding, const char * p_string, t_size p_string_length) {
		string8 m_buffer;
		t_size source_len = 0, source_walk = 0;

		while (source_walk < p_string_length && source_len < p_chars) {
			unsigned dummy;
			t_size delta = pfc::utf8_decode_char(p_string + source_walk, dummy, p_string_length - source_walk);
			if (delta == 0) break;
			source_len++;
			source_walk += delta;
		}

		m_buffer.add_string(p_string, source_walk);
		m_buffer.add_chars(p_padding, p_chars - source_len);
		return m_buffer;
	}

	string8 format_pad_right(t_size p_chars, t_uint32 p_padding, const char * p_string, t_size p_string_length) {
		string8 m_buffer;
		t_size source_len = 0, source_walk = 0;

		while (source_walk < p_string_length && source_len < p_chars) {
			unsigned dummy;
			t_size delta = pfc::utf8_decode_char(p_string + source_walk, dummy, p_string_length - source_walk);
			if (delta == 0) break;
			source_len++;
			source_walk += delta;
		}

		m_buffer.add_chars(p_padding, p_chars - source_len);
		m_buffer.add_string(p_string, source_walk);
		return m_buffer;
	}

	string8 stringToUpper(const char * str, size_t len) {
		string8 ret;
		stringToUpperAppend(ret, str, len);
		return ret;
	}
	string8 stringToLower(const char * str, size_t len) {
		string8 ret;
		stringToLowerAppend(ret, str, len);
		return ret;
	}

	pfc::string8 prefixLines(const char* str, const char* prefix, const char * setEOL) {
		const auto temp = pfc::splitStringByLines2(str);
		pfc::string8 ret; ret.prealloc(1024);
		for (auto& line : temp) {
			if ( line.length() > 0 ) ret << prefix << line << setEOL;
		}
		return ret;
	}

} //namespace pfc
