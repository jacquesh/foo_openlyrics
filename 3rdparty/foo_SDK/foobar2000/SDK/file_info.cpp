#include "foobar2000-sdk-pch.h"
#include "file_info.h"
#include "console.h"
#include "filesystem.h"

#include <pfc/unicode-normalize.h>
#ifndef _MSC_VER
#define strcat_s strcat
#define _atoi64 atoll
#endif

static constexpr char info_WAVEFORMATEXTENSIBLE_CHANNEL_MASK[] = "WAVEFORMATEXTENSIBLE_CHANNEL_MASK";

t_size file_info::meta_find_ex(const char * p_name,t_size p_name_length) const
{
	t_size n, m = meta_get_count();
	for(n=0;n<m;n++)
	{
		if (pfc::stricmp_ascii_ex(meta_enum_name(n),SIZE_MAX,p_name,p_name_length) == 0) return n;
	}
	return SIZE_MAX;
}

bool file_info::meta_exists_ex(const char * p_name,t_size p_name_length) const
{
	return meta_find_ex(p_name,p_name_length) != SIZE_MAX;
}

void file_info::meta_remove_field_ex(const char * p_name,t_size p_name_length)
{
	auto index = meta_find_ex(p_name,p_name_length);
	if (index!= SIZE_MAX) meta_remove_index(index);
}


void file_info::meta_remove_index(t_size p_index)
{
	meta_remove_mask(pfc::bit_array_one(p_index));
}

void file_info::meta_remove_all()
{
	meta_remove_mask(pfc::bit_array_true());
}

void file_info::meta_remove_value(t_size p_index,t_size p_value)
{
	meta_remove_values(p_index, pfc::bit_array_one(p_value));
}

t_size file_info::meta_get_count_by_name_ex(const char * p_name,t_size p_name_length) const
{
	auto index = meta_find_ex(p_name,p_name_length);
	if (index == SIZE_MAX) return 0;
	return meta_enum_value_count(index);
}

t_size file_info::info_find_ex(const char * p_name,t_size p_name_length) const
{
	t_size n, m = info_get_count();
	for(n=0;n<m;n++) {
		if (pfc::stricmp_ascii_ex(info_enum_name(n), SIZE_MAX,p_name,p_name_length) == 0) return n;
	}
	return SIZE_MAX;
}

bool file_info::info_exists_ex(const char * p_name,t_size p_name_length) const
{
	return info_find_ex(p_name,p_name_length) != SIZE_MAX;
}

void file_info::info_remove_index(t_size p_index)
{
	info_remove_mask(pfc::bit_array_one(p_index));
}

void file_info::info_remove_all()
{
	info_remove_mask(pfc::bit_array_true());
}

bool file_info::info_remove_ex(const char * p_name,t_size p_name_length)
{
	auto index = info_find_ex(p_name,p_name_length);
	if (index != SIZE_MAX)
	{
		info_remove_index(index);
		return true;
	}
	else return false;
}

void file_info::overwrite_meta(const file_info & p_source) {
	const t_size total = p_source.meta_get_count();
	for(t_size walk = 0; walk < total; ++walk) {
		copy_meta_single(p_source, walk);
	}
}

bool file_info::overwrite_meta_if_changed( const file_info & source ) {
	const t_size total = source.meta_get_count();
	bool changed = false;
	for(t_size walk = 0; walk < total; ++walk) {
		auto name = source.meta_enum_name(walk);
		auto idx = this->meta_find(name);
		if ( idx != SIZE_MAX ) {
			if (field_value_equals(*this, idx, source, walk)) continue;
		}

		copy_meta_single(source, walk);
		changed = true;
	}
	return changed;
}

void file_info::copy_meta_single(const file_info & p_source,t_size p_index)
{
	copy_meta_single_rename(p_source,p_index,p_source.meta_enum_name(p_index));
}

void file_info::copy_meta_single_nocheck(const file_info & p_source,t_size p_index)
{
	const char * name = p_source.meta_enum_name(p_index);
	t_size n, m = p_source.meta_enum_value_count(p_index);
	t_size new_index = SIZE_MAX;
	for(n=0;n<m;n++)
	{
		const char * value = p_source.meta_enum_value(p_index,n);
		if (n == 0) new_index = meta_set_nocheck(name,value);
		else meta_add_value(new_index,value);
	}
}

void file_info::copy_meta_single_by_name_ex(const file_info & p_source,const char * p_name,t_size p_name_length)
{
	auto index = p_source.meta_find_ex(p_name,p_name_length);
	if (index != SIZE_MAX) copy_meta_single(p_source,index);
}

void file_info::copy_info_single_by_name_ex(const file_info & p_source,const char * p_name,t_size p_name_length)
{
	auto index = p_source.info_find_ex(p_name,p_name_length);
	if (index != SIZE_MAX) copy_info_single(p_source,index);
}

void file_info::copy_meta_single_by_name_nocheck_ex(const file_info & p_source,const char * p_name,t_size p_name_length)
{
	auto index = p_source.meta_find_ex(p_name,p_name_length);
	if (index != SIZE_MAX) copy_meta_single_nocheck(p_source,index);
}

void file_info::copy_info_single_by_name_nocheck_ex(const file_info & p_source,const char * p_name,t_size p_name_length)
{
	auto index = p_source.info_find_ex(p_name,p_name_length);
	if (index != SIZE_MAX) copy_info_single_nocheck(p_source,index);
}

void file_info::copy_info_single(const file_info & p_source,t_size p_index)
{
	info_set(p_source.info_enum_name(p_index),p_source.info_enum_value(p_index));
}

void file_info::copy_info_single_nocheck(const file_info & p_source,t_size p_index)
{
	info_set_nocheck(p_source.info_enum_name(p_index),p_source.info_enum_value(p_index));
}

void file_info::copy_meta(const file_info & p_source)
{
	if (&p_source != this) {
		meta_remove_all();
		t_size n, m = p_source.meta_get_count();
		for(n=0;n<m;n++)
			copy_meta_single_nocheck(p_source,n);
	}
}

void file_info::copy_info(const file_info & p_source)
{
	if (&p_source != this) {
		info_remove_all();
		t_size n, m = p_source.info_get_count();
		for(n=0;n<m;n++)
			copy_info_single_nocheck(p_source,n);
	}
}

void file_info::copy(const file_info & p_source)
{
	if (&p_source != this) {
		copy_meta(p_source);
		copy_info(p_source);
		set_length(p_source.get_length());
		set_replaygain(p_source.get_replaygain());
	}
}


const char * file_info::meta_get_ex(const char * p_name,t_size p_name_length,t_size p_index) const
{
	auto index = meta_find_ex(p_name,p_name_length);
	if (index == SIZE_MAX) return 0;
	auto max = meta_enum_value_count(index);
	if (p_index >= max) return 0;
	return meta_enum_value(index,p_index);
}

const char * file_info::info_get_ex(const char * p_name,t_size p_name_length) const
{
	auto index = info_find_ex(p_name,p_name_length);
	if (index == SIZE_MAX) return 0;
	return info_enum_value(index);
}

t_int64 file_info::info_get_int(const char * name) const
{
	PFC_ASSERT(pfc::is_valid_utf8(name));
	const char * val = info_get(name);
	if (val==0) return 0;
	return _atoi64(val);
}

t_int64 file_info::info_get_length_samples() const
{
	t_int64 ret = 0;
	double len = get_length();
	t_int64 srate = info_get_int("samplerate");

	if (srate>0 && len>0)
	{
		ret = audio_math::time_to_samples(len,(unsigned)srate);
	}
	return ret;
}

double file_info::info_get_float(const char * name) const
{
	const char * ptr = info_get(name);
	if (ptr) return pfc::string_to_float(ptr);
	else return 0;
}

void file_info::info_set_int(const char * name,t_int64 value)
{
	PFC_ASSERT(pfc::is_valid_utf8(name));
	info_set(name,pfc::format_int(value));
}

void file_info::info_set_float(const char * name,double value,unsigned precision,bool force_sign,const char * unit)
{
	PFC_ASSERT(pfc::is_valid_utf8(name));
	PFC_ASSERT(unit==0 || strlen(unit) <= 64);
	char temp[128];
	pfc::float_to_string(temp,64,value,precision,force_sign);
	temp[63] = 0;
	if (unit)
	{
		strcat_s(temp," ");
		strcat_s(temp,unit);
	}
	info_set(name,temp);
}


void file_info::info_set_replaygain_album_gain(float value)
{
	replaygain_info temp = get_replaygain();
	temp.m_album_gain = value;
	set_replaygain(temp);
}

void file_info::info_set_replaygain_album_peak(float value)
{
	replaygain_info temp = get_replaygain();
	temp.m_album_peak = value;
	set_replaygain(temp);
}

void file_info::info_set_replaygain_track_gain(float value)
{
	replaygain_info temp = get_replaygain();
	temp.m_track_gain = value;
	set_replaygain(temp);
}

void file_info::info_set_replaygain_track_peak(float value)
{
	replaygain_info temp = get_replaygain();
	temp.m_track_peak = value;
	set_replaygain(temp);
}


static bool is_valid_bps(t_int64 val)
{
	return val>0 && val<=256;
}

unsigned file_info::info_get_decoded_bps() const
{
	t_int64 val = info_get_int("decoded_bitspersample");
	if (is_valid_bps(val)) return (unsigned)val;
	val = info_get_int("bitspersample");
	if (is_valid_bps(val)) return (unsigned)val;
	return 0;
}

bool file_info::info_get_codec_long(pfc::string_base& out, const char * delim) const {
	const char * codec;
	codec = this->info_get("codec_long");
	if (codec != nullptr) {
		out = codec; return true;
	}
	codec = this->info_get("codec");
	if (codec != nullptr) {
		out = codec;
		const char * profile = this->info_get("codec_profile");
		if (profile != nullptr) {
			out << delim << profile;
		}
		return true;
	}
	return false;
}

void file_info::reset()
{
	info_remove_all();
	meta_remove_all();
	set_length(0);
	reset_replaygain();
}

void file_info::reset_replaygain()
{
	replaygain_info temp;
	temp.reset();
	set_replaygain(temp);
}

void file_info::copy_meta_single_rename_ex(const file_info & p_source,t_size p_index,const char * p_new_name,t_size p_new_name_length)
{
	t_size n, m = p_source.meta_enum_value_count(p_index);
	t_size new_index = SIZE_MAX;
	for(n=0;n<m;n++)
	{
		const char * value = p_source.meta_enum_value(p_index,n);
		if (n == 0) new_index = meta_set_ex(p_new_name,p_new_name_length,value,SIZE_MAX);
		else meta_add_value(new_index,value);
	}
}

t_size file_info::meta_add_ex(const char * p_name,t_size p_name_length,const char * p_value,t_size p_value_length)
{
	auto index = meta_find_ex(p_name,p_name_length);
	if (index == SIZE_MAX) return meta_set_nocheck_ex(p_name,p_name_length,p_value,p_value_length);
	else
	{
		meta_add_value_ex(index,p_value,p_value_length);
		return index;
	}
}

void file_info::meta_add_value_ex(t_size p_index,const char * p_value,t_size p_value_length)
{
	meta_insert_value_ex(p_index,meta_enum_value_count(p_index),p_value,p_value_length);
}


t_size file_info::meta_calc_total_value_count() const
{
	t_size n, m = meta_get_count(), ret = 0;
	for(n=0;n<m;n++) ret += meta_enum_value_count(n);
	return ret;
}

bool file_info::info_set_replaygain_ex(const char * p_name,t_size p_name_len,const char * p_value,t_size p_value_len)
{
	replaygain_info temp = get_replaygain();
	if (temp.set_from_meta_ex(p_name,p_name_len,p_value,p_value_len))
	{
		set_replaygain(temp);
		return true;
	}
	else return false;
}

void file_info::info_set_replaygain_auto_ex(const char * p_name,t_size p_name_len,const char * p_value,t_size p_value_len)
{
	if (!info_set_replaygain_ex(p_name,p_name_len,p_value,p_value_len))
		info_set_ex(p_name,p_name_len,p_value,p_value_len);
}

static bool _matchGain(float g1, float g2) {
	if (g1 == replaygain_info::gain_invalid && g2 == replaygain_info::gain_invalid) return true;
	else if (g1 == replaygain_info::gain_invalid || g2 == replaygain_info::gain_invalid) return false;
	else return fabs(g1-g2) < 0.1;
}
static bool _matchPeak(float p1, float p2) {
	if (p1 == replaygain_info::peak_invalid && p2 == replaygain_info::peak_invalid) return true;
	else if (p1 == replaygain_info::peak_invalid || p2 == replaygain_info::peak_invalid) return false;
	else return fabs(p1-p2) < 0.01;
}
bool replaygain_info::g_equalLoose( const replaygain_info & i1, const replaygain_info & i2) {
	return _matchGain(i1.m_track_gain, i2.m_track_gain) && _matchGain(i1.m_album_gain, i2.m_album_gain) && _matchPeak(i1.m_track_peak, i2.m_track_peak) && _matchPeak(i1.m_album_peak, i2.m_album_peak);
}
bool replaygain_info::g_equal(const replaygain_info & item1,const replaygain_info & item2)
{
	return	item1.m_album_gain == item2.m_album_gain &&
			item1.m_track_gain == item2.m_track_gain &&
			item1.m_album_peak == item2.m_album_peak &&
			item1.m_track_peak == item2.m_track_peak;
}

bool file_info::are_meta_fields_identical(t_size p_index1,t_size p_index2) const
{
	const t_size count = meta_enum_value_count(p_index1);
	if (count != meta_enum_value_count(p_index2)) return false;
	t_size n;
	for(n=0;n<count;n++)
	{
		if (strcmp(meta_enum_value(p_index1,n),meta_enum_value(p_index2,n))) return false;
	}
	return true;
}


void file_info::meta_format_entry(t_size index, pfc::string_base & out, const char * separator) const {
	out.reset();
	t_size val, count = meta_enum_value_count(index);
	PFC_ASSERT( count > 0);
	for(val=0;val<count;val++)
	{
		if (val > 0) out += separator;
		out += meta_enum_value(index,val);
	}
}

bool file_info::meta_format(const char * p_name,pfc::string_base & p_out, const char * separator) const {
	p_out.reset();
	auto index = meta_find(p_name);
	if (index == SIZE_MAX) return false;
	meta_format_entry(index, p_out, separator);
	return true;
}

void file_info::info_calculate_bitrate(uint64_t p_filesize,double p_length)
{
	unsigned b = audio_math::bitrate_kbps( p_filesize, p_length );
	if ( b > 0 ) info_set_bitrate(b);
}

void file_info::info_set_bitspersample(uint32_t val, bool isFloat) {
	// Bits per sample semantics
	// "bitspersample" is set to integer value of bits per sample
	// "bitspersample_extra" is used for bps of 32 or 64, either "floating-point" or "fixed-point"
	// bps other than 32 or 64 are implicitly fixed-point as floating-point for such makes no sense

	info_set_int("bitspersample", val);
	if ( isFloat || val == 32 || val == 64 ) {
		info_set("bitspersample_extra", isFloat ? "floating-point" : "fixed-point");
	} else {
		info_remove("bitspersample_extra");
	}
}

bool file_info::is_encoding_float() const {
	auto bs = info_get_int("bitspersample");
	auto extra = info_get("bitspersample_extra");
	if (bs == 32 || bs == 64) {
		if (extra == nullptr || strcmp(extra, "floating-point") == 0) return true;
	}
	return false;
}

bool file_info::is_encoding_overkill() const {
#if audio_sample_size == 32
	auto bs = info_get_int("bitspersample");
	auto extra = info_get("bitspersample_extra");
	if ( bs <= 24 ) return false; // fixedpoint up to 24bit, OK
	if ( bs > 32 ) return true; // fixed or float beyond 32bit, overkill

	if ( extra != nullptr ) {
		if (strcmp(extra, "fixed-point") == 0) return true; // int32, overkill
	}
#endif
	return false;
}

bool file_info::is_encoding_lossy() const {
	const char * encoding = info_get("encoding");
	if (encoding != NULL) {
		if (pfc::stricmp_ascii(encoding,"lossy") == 0 /*|| pfc::stricmp_ascii(encoding,"hybrid") == 0*/) return true;
	} else {
		//the old way
		//disabled: don't whine if we're not sure what we're dealing with - might be a file with info not-yet-loaded in oddball cases or a mod file
		//if (info_get("bitspersample") == NULL) return true;
	}
	return false;
}

bool file_info::g_is_meta_equal(const file_info & p_item1,const file_info & p_item2) {
	const t_size count = p_item1.meta_get_count();
	if (count != p_item2.meta_get_count()) {
		//uDebugLog() << "meta count mismatch";
		return false;
	}
	pfc::map_t<const char*,t_size,field_name_comparator> item2_meta_map;
	for(t_size n=0; n<count; n++) {
		item2_meta_map.set(p_item2.meta_enum_name(n),n);
	}
	for(t_size n1=0; n1<count; n1++) {
		t_size n2;
		if (!item2_meta_map.query(p_item1.meta_enum_name(n1),n2)) {
			//uDebugLog() << "item2 doesn't have " << p_item1.meta_enum_name(n1);
			return false;
		}
		t_size value_count = p_item1.meta_enum_value_count(n1);
		if (value_count != p_item2.meta_enum_value_count(n2)) {
			//uDebugLog() << "meta value count mismatch: " << p_item1.meta_enum_name(n1) << " : " << value_count << " vs " << p_item2.meta_enum_value_count(n2);
			return false;
		}
		for(t_size v = 0; v < value_count; v++) {
			if (strcmp(p_item1.meta_enum_value(n1,v),p_item2.meta_enum_value(n2,v)) != 0) {
				//uDebugLog() << "meta mismatch: " << p_item1.meta_enum_name(n1) << " : " << p_item1.meta_enum_value(n1,v) << " vs " << p_item2.meta_enum_value(n2,v);
				return false;
			}
		}
	}
	return true;
}

bool file_info::g_is_meta_equal_debug(const file_info & p_item1,const file_info & p_item2) {
	const t_size count = p_item1.meta_get_count();
	if (count != p_item2.meta_get_count()) {
		FB2K_DebugLog() << "meta count mismatch";
		return false;
	}
	pfc::map_t<const char*,t_size,field_name_comparator> item2_meta_map;
	for(t_size n=0; n<count; n++) {
		item2_meta_map.set(p_item2.meta_enum_name(n),n);
	}
	for(t_size n1=0; n1<count; n1++) {
		t_size n2;
		if (!item2_meta_map.query(p_item1.meta_enum_name(n1),n2)) {
			FB2K_DebugLog() << "item2 doesn't have " << p_item1.meta_enum_name(n1);
			return false;
		}
		t_size value_count = p_item1.meta_enum_value_count(n1);
		if (value_count != p_item2.meta_enum_value_count(n2)) {
			FB2K_DebugLog() << "meta value count mismatch: " << p_item1.meta_enum_name(n1) << " : " << (uint32_t)value_count << " vs " << (uint32_t)p_item2.meta_enum_value_count(n2);
			return false;
		}
		for(t_size v = 0; v < value_count; v++) {
			if (strcmp(p_item1.meta_enum_value(n1,v),p_item2.meta_enum_value(n2,v)) != 0) {
				FB2K_DebugLog() << "meta mismatch: " << p_item1.meta_enum_name(n1) << " : " << p_item1.meta_enum_value(n1,v) << " vs " << p_item2.meta_enum_value(n2,v);
				return false;
			}
		}
	}
	return true;
}

bool file_info::g_is_info_equal(const file_info & p_item1,const file_info & p_item2) {
	t_size count = p_item1.info_get_count();
	if (count != p_item2.info_get_count()) {
		//uDebugLog() << "info count mismatch";
		return false;
	}
	for(t_size n1=0; n1<count; n1++) {
		t_size n2 = p_item2.info_find(p_item1.info_enum_name(n1));
		if (n2 == SIZE_MAX) {
			//uDebugLog() << "item2 does not have " << p_item1.info_enum_name(n1);
			return false;
		}
		if (strcmp(p_item1.info_enum_value(n1),p_item2.info_enum_value(n2)) != 0) {
			//uDebugLog() << "value mismatch: " << p_item1.info_enum_name(n1);
			return false;
		}
	}
	return true;
}

bool file_info::g_is_meta_subset_debug(const file_info& superset, const file_info& subset) {
	size_t total = subset.meta_get_count();
	bool rv = true;
	for (size_t walk = 0; walk < total; ++walk) {
		const char* name = subset.meta_enum_name(walk);
		const size_t idx = superset.meta_find(name);
		if (idx == SIZE_MAX) {
			rv = false;
			FB2K_console_formatter() << "Field " << name << " missing";
		} else if (!field_value_equals(superset, idx, subset, walk)) {
			rv = false;
			FB2K_console_formatter() << "Field " << name << " mismatch";
		}
	}
	return rv;
}

static bool is_valid_field_name_char(char p_char) {
	return p_char >= 32 && p_char < 127 && p_char != '=' && p_char != '%' && p_char != '<' && p_char != '>';
}

bool file_info::g_is_valid_field_name(const char * p_name,t_size p_length) {
	t_size walk;
	for(walk = 0; walk < p_length && p_name[walk] != 0; walk++) {
		if (!is_valid_field_name_char(p_name[walk])) return false;
	}
	return walk > 0;
}

void file_info::to_formatter(pfc::string_formatter& out) const {
	out << "File info dump:\n";
	if (get_length() > 0) out<< "Duration: " << pfc::format_time_ex(get_length(), 6) << "\n";
	pfc::string_formatter temp;
	for(t_size metaWalk = 0; metaWalk < meta_get_count(); ++metaWalk) {
		meta_format_entry(metaWalk, temp);
		out << "Meta: " << meta_enum_name(metaWalk) << " = " << temp << "\n";
	}
	for(t_size infoWalk = 0; infoWalk < info_get_count(); ++infoWalk) {
		out << "Info: " << info_enum_name(infoWalk) << " = " << info_enum_value(infoWalk) << "\n";
	}
    auto rg = this->get_replaygain();
    replaygain_info::t_text_buffer rgbuf;
    if (rg.format_track_gain(rgbuf)) out << "RG track gain: " << rgbuf << "\n";
    if (rg.format_track_peak(rgbuf)) out << "RG track peak: " << rgbuf << "\n";
    if (rg.format_album_gain(rgbuf)) out << "RG album gain: " << rgbuf << "\n";
    if (rg.format_album_peak(rgbuf)) out << "RG album peak: " << rgbuf << "\n";
}

void file_info::to_console() const {
	FB2K_console_formatter1() << "File info dump:";
	if (get_length() > 0) FB2K_console_formatter() << "Duration: " << pfc::format_time_ex(get_length(), 6);
	pfc::string_formatter temp;
	const auto numMeta = meta_get_count(), numInfo = info_get_count();
	if (numMeta == 0) {
		FB2K_console_formatter() << "Meta is blank";
	} else for(t_size metaWalk = 0; metaWalk < numMeta; ++metaWalk) {
		const char * name = meta_enum_name( metaWalk );
		const auto valCount = meta_enum_value_count( metaWalk );
		for ( size_t valWalk = 0; valWalk < valCount; ++valWalk ) {
			FB2K_console_formatter() << "Meta: " << name << " = " << meta_enum_value( metaWalk, valWalk );
		}

		/*
		meta_format_entry(metaWalk, temp);
		FB2K_console_formatter() << "Meta: " << meta_enum_name(metaWalk) << " = " << temp;
		*/
	}
	if (numInfo == 0) {
		FB2K_console_formatter() << "Info is blank";
	} else for(t_size infoWalk = 0; infoWalk < numInfo; ++infoWalk) {
		FB2K_console_formatter() << "Info: " << info_enum_name(infoWalk) << " = " << info_enum_value(infoWalk);
	}
}

void file_info::info_set_channels(uint32_t v) {
	this->info_set_int("channels", v);
}

void file_info::info_set_channels_ex(uint32_t channels, uint32_t mask) {
	info_set_channels(channels);
	info_set_wfx_chanMask(mask);
}

static bool parse_wfx_chanMask(const char* str, uint32_t& out) {
	try {
		if (pfc::strcmp_partial(str, "0x") != 0) return false;
		out = pfc::atohex<uint32_t>(str + 2, strlen(str + 2));
		return true;
	} catch (...) { return false; }
}

void file_info::info_tidy_channels() {
	const char * info = this->info_get(info_WAVEFORMATEXTENSIBLE_CHANNEL_MASK);
	if (info != nullptr) {
		bool keep = false;
		uint32_t v;
		if (parse_wfx_chanMask(info, v)) {
			if (v != 0 && v != 3 && v != 4) {
				// valid, not mono, not stereo
				keep = true;
			}
		}
		if (!keep) this->info_remove(info_WAVEFORMATEXTENSIBLE_CHANNEL_MASK);
	}
}

void file_info::info_set_wfx_chanMask(uint32_t val) {
	switch(val) {
	case 0:
	case 4:
	case 3:
		this->info_remove(info_WAVEFORMATEXTENSIBLE_CHANNEL_MASK);
		break;
	default:
		info_set (info_WAVEFORMATEXTENSIBLE_CHANNEL_MASK, pfc::format("0x", pfc::format_hex(val) ) );
		break;
	}
}

uint32_t file_info::info_get_wfx_chanMask() const {
	const char * str = this->info_get(info_WAVEFORMATEXTENSIBLE_CHANNEL_MASK);
	if (str == NULL) return 0;
	uint32_t ret;
	if (parse_wfx_chanMask(str, ret)) return ret;
	return 0;
}

bool file_info::field_is_person(const char * fieldName) {
	return field_name_equals(fieldName, "artist") ||
		field_name_equals(fieldName, "album artist") || 
		field_name_equals(fieldName, "composer") || 
		field_name_equals(fieldName, "performer") ||
		field_name_equals(fieldName, "conductor") ||
		field_name_equals(fieldName, "orchestra") ||
		field_name_equals(fieldName, "ensemble") ||
		field_name_equals(fieldName, "engineer");
}

bool file_info::field_is_title(const char * fieldName) {
	return field_name_equals(fieldName, "title") || field_name_equals(fieldName, "album");
}


void file_info::to_stream( stream_writer * stream, abort_callback & abort ) const {
	stream_writer_formatter<> out(* stream, abort );
	
	out << this->get_length();
	
	{
		const auto rg = this->get_replaygain();
		out << rg.m_track_gain << rg.m_album_gain << rg.m_track_peak << rg.m_album_peak;
	}

	
	{
		const uint32_t metaCount = pfc::downcast_guarded<uint32_t>( this->meta_get_count() );
		for(uint32_t metaWalk = 0; metaWalk < metaCount; ++metaWalk) {
			const char * name = this->meta_enum_name( metaWalk );
			if (*name) {
				out.write_string_nullterm( this->meta_enum_name( metaWalk ) );
				const size_t valCount = this->meta_enum_value_count( metaWalk );
				for(size_t valWalk = 0; valWalk < valCount; ++valWalk) {
					const char * value = this->meta_enum_value( metaWalk, valWalk );
					if (*value) {
						out.write_string_nullterm( value );
					}
				}
				out.write_int<char>(0);
			}
		}
		out.write_int<char>(0);
	}

	{
		const uint32_t infoCount = pfc::downcast_guarded<uint32_t>( this->info_get_count() );
		for(uint32_t infoWalk = 0; infoWalk < infoCount; ++infoWalk) {
			const char * name = this->info_enum_name( infoWalk );
			const char * value = this->info_enum_value( infoWalk );
			if (*name && *value) {
				out.write_string_nullterm(name); out.write_string_nullterm(value);
			}
		}
		out.write_int<char>(0);
	}
}

void file_info::from_stream( stream_reader * stream, abort_callback & abort ) {
	stream_reader_formatter<> in( *stream, abort );
	pfc::string_formatter tempName, tempValue;
	{
		double len; in >> len; this->set_length( len );
	}
	{
		replaygain_info rg;
		in >> rg.m_track_gain >> rg.m_album_gain >> rg.m_track_peak >> rg.m_album_peak;
	}

	{
		this->meta_remove_all();
		for(;;) {
			in.read_string_nullterm( tempName );
			if (tempName.length() == 0) break;
			size_t metaIndex = SIZE_MAX;
			for(;;) {
				in.read_string_nullterm( tempValue );
				if (tempValue.length() == 0) break;
				if (metaIndex == SIZE_MAX) metaIndex = this->meta_add( tempName, tempValue );
				else this->meta_add_value( metaIndex, tempValue );
			}
		}
	}
	{
		this->info_remove_all();
		for(;;) {
			in.read_string_nullterm( tempName );
			if (tempName.length() == 0) break;
			in.read_string_nullterm( tempValue );
			this->info_set( tempName, tempValue );
		}
	}
}

static const char * _readString( const uint8_t * & ptr, size_t & remaining ) {
	const char * rv = (const char*)ptr;
	for(;;) {
		if (remaining == 0) throw exception_io_data();
		uint8_t byte = *ptr++; --remaining;
		if (byte == 0) break;
	}
	return rv;
}

template<typename int_t> void _readInt( int_t & out, const uint8_t * &ptr, size_t & remaining) {
	if (remaining < sizeof(out)) throw exception_io_data();
	pfc::decode_little_endian( out, ptr ); ptr += sizeof(out); remaining -= sizeof(out);
}

template<typename float_t> static void _readFloat(float_t & out, const uint8_t * &ptr, size_t & remaining) {
	union {
		typename pfc::sized_int_t<sizeof(float_t)>::t_unsigned i;
		float_t f;
	} u;
	_readInt(u.i, ptr, remaining);
	out = u.f;
}

void file_info::from_mem( const void * memPtr, size_t memSize ) {
	size_t remaining = memSize;
	const uint8_t * walk = (const uint8_t*) memPtr;

	{
		double len; _readFloat(len, walk, remaining);
		this->set_length( len );
	}

	{
		replaygain_info rg;
		_readFloat(rg.m_track_gain, walk, remaining ); 
		_readFloat(rg.m_album_gain, walk, remaining );
		_readFloat(rg.m_track_peak, walk, remaining );
		_readFloat(rg.m_album_peak, walk, remaining );
		this->set_replaygain( rg );
	}

	{
		this->meta_remove_all();
		for(;;) {
			const char * metaName = _readString( walk, remaining );
			if (*metaName == 0) break;
			size_t metaIndex = SIZE_MAX;
			for(;;) {
				const char * metaValue = _readString( walk, remaining );
				if (*metaValue == 0) break;
				if (metaIndex == SIZE_MAX) metaIndex = this->meta_add( metaName, metaValue );
				else this->meta_add_value( metaIndex, metaName );
			}
		}
	}
	{
		this->info_remove_all();
		for(;;) {
			const char * infoName = _readString( walk, remaining );
			if (*infoName == 0) break;
			const char * infoValue = _readString( walk, remaining );
			this->info_set( infoName, infoValue );
		}
	}
}

void file_info::set_audio_chunk_spec(audio_chunk::spec_t s) {
	this->info_set_int("samplerate", s.sampleRate);
	this->info_set_int("channels", s.chanCount);
	uint32_t mask = 0;
	if (audio_chunk::g_count_channels(s.chanMask) == s.chanCount) {
		mask = s.chanMask;
	}
	this->info_set_wfx_chanMask(mask); // clears if zero or  one of trivial values
}

audio_chunk::spec_t file_info::audio_chunk_spec() const 
{
	audio_chunk::spec_t rv = {};
	rv.sampleRate = (uint32_t)this->info_get_int("samplerate");
	rv.chanCount = (uint32_t)this->info_get_int("channels");
	rv.chanMask = (uint32_t)this->info_get_wfx_chanMask();
	if (audio_chunk::g_count_channels( rv.chanMask ) != rv.chanCount ) {
		rv.chanMask = audio_chunk::g_guess_channel_config( rv.chanCount );
	}
	return rv;
}

bool file_info::field_value_equals(const file_info& i1, size_t meta1, const file_info& i2, size_t meta2) {
	const size_t c = i1.meta_enum_value_count(meta1);
	if (c != i2.meta_enum_value_count(meta2)) return false;
	for (size_t walk = 0; walk < c; ++walk) {
		if (strcmp(i1.meta_enum_value(meta1, walk), i2.meta_enum_value(meta2, walk)) != 0) return false;
	}
	return true;
}

bool file_info::unicode_normalize_C() {
	const size_t total = this->meta_get_count();
	bool changed = false;
	for (size_t mwalk = 0; mwalk < total; ++mwalk) {
		const size_t totalV = this->meta_enum_value_count(mwalk);
		for (size_t vwalk = 0; vwalk < totalV; ++vwalk) {
			const char* val = this->meta_enum_value(mwalk, vwalk);
			if (pfc::stringContainsFormD(val)) {
				auto norm = pfc::unicodeNormalizeC(val);
				if (strcmp(norm, val) != 0) {
					this->meta_modify_value(mwalk, vwalk, norm);
					changed = true;
				}
			}
		}
	}
	return changed;
}

void file_info::meta_enumerate(meta_enumerate_t cb) const {
	const size_t nMeta = this->meta_get_count();
	for (size_t metaWalk = 0; metaWalk < nMeta; ++metaWalk) {
		const char* name = this->meta_enum_name(metaWalk);
		const size_t nValue = this->meta_enum_value_count(metaWalk);
		for (size_t valueWalk = 0; valueWalk < nValue; ++valueWalk) {
			const char* value = this->meta_enum_value(metaWalk, valueWalk);
			cb(name, value);
		}
	}
}

#ifdef FOOBAR2000_MOBILE
#include "album_art.h"
#include "hasher_md5.h"

void file_info::info_set_pictures( const GUID * guids, size_t size ) {
    this->info_set("pictures", album_art_ids::ids_to_string(guids, size) );
}

pfc::array_t<GUID> file_info::info_get_pictures( ) const {
    return album_art_ids::string_to_ids( this->info_get( "pictures" ) );
}

uint64_t file_info::makeMetaHash() const {
	pfc::string_formatter temp;

	auto doMeta = [&] ( const char * meta ) {
		const char * p = meta_get(meta, 0);
		if (p != nullptr) temp << p;
		temp << "\n";
	};
	auto doMetaInt = [&] ( const char * meta ) {
		const char * p = meta_get(meta, 0);
		if (p != nullptr) {
			auto s = strchr(p, '/' ); if ( s != nullptr ) p = s+1;
			while(*p == '0') ++p;
			temp << p;
		}
		temp << "\n";
	};
	doMeta("title");
	doMeta("artist");
	doMeta("album");
	doMetaInt("tracknumber");
	doMetaInt("discnumber");

	if (temp.length() == 5) return 0;

	return hasher_md5::get()->process_single( temp.c_str(), temp.length( ) ).xorHalve();
}

#endif // FOOBAR2000_MOBILE
