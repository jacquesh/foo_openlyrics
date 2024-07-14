#include "foobar2000-sdk-pch.h"
#include "audio_chunk.h"

#ifdef _WIN32
#include <ks.h>
#include <ksmedia.h>

#if 0
#define SPEAKER_FRONT_LEFT             0x1
#define SPEAKER_FRONT_RIGHT            0x2
#define SPEAKER_FRONT_CENTER           0x4
#define SPEAKER_LOW_FREQUENCY          0x8
#define SPEAKER_BACK_LEFT              0x10
#define SPEAKER_BACK_RIGHT             0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER   0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
#define SPEAKER_BACK_CENTER            0x100
#define SPEAKER_SIDE_LEFT              0x200
#define SPEAKER_SIDE_RIGHT             0x400
#define SPEAKER_TOP_CENTER             0x800
#define SPEAKER_TOP_FRONT_LEFT         0x1000
#define SPEAKER_TOP_FRONT_CENTER       0x2000
#define SPEAKER_TOP_FRONT_RIGHT        0x4000
#define SPEAKER_TOP_BACK_LEFT          0x8000
#define SPEAKER_TOP_BACK_CENTER        0x10000
#define SPEAKER_TOP_BACK_RIGHT         0x20000

static struct {DWORD m_wfx; unsigned m_native; } const g_translation_table[] =
{
	{SPEAKER_FRONT_LEFT,			audio_chunk::channel_front_left},
	{SPEAKER_FRONT_RIGHT,			audio_chunk::channel_front_right},
	{SPEAKER_FRONT_CENTER,			audio_chunk::channel_front_center},
	{SPEAKER_LOW_FREQUENCY,			audio_chunk::channel_lfe},
	{SPEAKER_BACK_LEFT,				audio_chunk::channel_back_left},
	{SPEAKER_BACK_RIGHT,			audio_chunk::channel_back_right},
	{SPEAKER_FRONT_LEFT_OF_CENTER,	audio_chunk::channel_front_center_left},
	{SPEAKER_FRONT_RIGHT_OF_CENTER,	audio_chunk::channel_front_center_right},
	{SPEAKER_BACK_CENTER,			audio_chunk::channel_back_center},
	{SPEAKER_SIDE_LEFT,				audio_chunk::channel_side_left},
	{SPEAKER_SIDE_RIGHT,			audio_chunk::channel_side_right},
	{SPEAKER_TOP_CENTER,			audio_chunk::channel_top_center},
	{SPEAKER_TOP_FRONT_LEFT,		audio_chunk::channel_top_front_left},
	{SPEAKER_TOP_FRONT_CENTER,		audio_chunk::channel_top_front_center},
	{SPEAKER_TOP_FRONT_RIGHT,		audio_chunk::channel_top_front_right},
	{SPEAKER_TOP_BACK_LEFT,			audio_chunk::channel_top_back_left},
	{SPEAKER_TOP_BACK_CENTER,		audio_chunk::channel_top_back_center},
	{SPEAKER_TOP_BACK_RIGHT,		audio_chunk::channel_top_back_right},
};

#endif
#endif



static constexpr unsigned g_audio_channel_config_table[] = 
{
	0,
	audio_chunk::channel_config_mono,
	audio_chunk::channel_config_stereo,
	audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_lfe,
	audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_left | audio_chunk::channel_back_right,
	audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_left | audio_chunk::channel_back_right | audio_chunk::channel_lfe,
	audio_chunk::channel_config_5point1,
	audio_chunk::channel_config_5point1_side | audio_chunk::channel_back_center,
	audio_chunk::channel_config_7point1,
	0,
	audio_chunk::channel_config_7point1 | audio_chunk::channel_front_center_right | audio_chunk::channel_front_center_left,
};

unsigned audio_chunk::g_guess_channel_config(unsigned count)
{
	if (count == 0) return 0;
	if (count > 32) throw exception_io_data();
	unsigned ret = 0;
	if (count < PFC_TABSIZE(g_audio_channel_config_table)) ret = g_audio_channel_config_table[count];
	if (ret == 0) {
		ret = (1 << count) - 1;
	}
	PFC_ASSERT(g_count_channels(ret) == count);
	return ret;	
}

unsigned audio_chunk::g_guess_channel_config_xiph(unsigned count) {
	switch (count) {
	case 3:
		return audio_chunk::channel_front_left | audio_chunk::channel_front_center | audio_chunk::channel_front_right;
	case 5:
		return audio_chunk::channel_front_left | audio_chunk::channel_front_center | audio_chunk::channel_front_right | audio_chunk::channel_back_left | audio_chunk::channel_back_right;
	default:
		return g_guess_channel_config(count);
	}	
}

unsigned audio_chunk::g_channel_index_from_flag(unsigned p_config,unsigned p_flag) {
	if (p_config & p_flag) {
		unsigned index = 0;

		for (unsigned walk = 0; walk < 32; walk++) {
			unsigned query = 1 << walk;
			if (p_flag & query) return index;
			if (p_config & query) index++;
		}
	}
	return (unsigned)(-1);
}

unsigned audio_chunk::g_extract_channel_flag(unsigned p_config,unsigned p_index)
{
	unsigned toskip = p_index;
	unsigned flag = 1;
	while(flag)
	{
		if (p_config & flag)
		{
			if (toskip == 0) break;
			toskip--;
		}
		flag <<= 1;
	}
	return flag;
}


static const char * const chanNames[] = {
	"FL", //channel_front_left			= 1<<0,
	"FR", //channel_front_right			= 1<<1,
	"FC", //channel_front_center		= 1<<2,
	"LFE", //channel_lfe					= 1<<3,
	"BL", //channel_back_left			= 1<<4,
	"BR", //channel_back_right			= 1<<5,
	"FCL", //channel_front_center_left	= 1<<6,
	"FCR", //channel_front_center_right	= 1<<7,
	"BC", //channel_back_center			= 1<<8,
	"SL", //channel_side_left			= 1<<9,
	"SR", //channel_side_right			= 1<<10,
	"TC", //channel_top_center			= 1<<11,
	"TFL", //channel_top_front_left		= 1<<12,
	"TFC", //channel_top_front_center	= 1<<13,
	"TFR", //channel_top_front_right		= 1<<14,
	"TBL", //channel_top_back_left		= 1<<15,
	"TBC", //channel_top_back_center		= 1<<16,
	"TBR", //channel_top_back_right		= 1<<17,
};

unsigned audio_chunk::g_find_channel_idx(unsigned p_flag) {
	unsigned rv = 0;
	if ((p_flag & 0xFFFF) == 0) {
		rv += 16; p_flag >>= 16;
	}
	if ((p_flag & 0xFF) == 0) {
		rv += 8; p_flag >>= 8;
	}
	if ((p_flag & 0xF) == 0) {
		rv += 4; p_flag >>= 4;
	}
	if ((p_flag & 0x3) == 0) {
		rv += 2; p_flag >>= 2;
	}
	if ((p_flag & 0x1) == 0) {
		rv += 1; p_flag >>= 1;
	}
	PFC_ASSERT( p_flag & 1 );
	return rv;
}

const char * audio_chunk::g_channel_name(unsigned p_flag) {
	return g_channel_name_byidx(g_find_channel_idx(p_flag));
}

const char * audio_chunk::g_channel_name_byidx(unsigned p_index) {
	if (p_index < PFC_TABSIZE(chanNames)) return chanNames[p_index];
	else return "?";
}

pfc::string8 audio_chunk::g_formatChannelMaskDesc(unsigned flags) {
	pfc::string8 temp; g_formatChannelMaskDesc(flags, temp); return temp;
}
void audio_chunk::g_formatChannelMaskDesc(unsigned flags, pfc::string_base & out) {
	out.reset();
	unsigned idx = 0;
	while(flags) {
		if (flags & 1) {
			if (!out.is_empty()) out << " ";
			out << g_channel_name_byidx(idx);
		}
		flags >>= 1;
		++idx;
	}
}

namespace {
	struct maskDesc_t {
		const char* name;
		unsigned mask;
	};
	static constexpr maskDesc_t maskDesc[] = {
		{"mono", audio_chunk::channel_config_mono},
		{"stereo", audio_chunk::channel_config_stereo},
		{"stereo (rear)", audio_chunk::channel_back_left | audio_chunk::channel_back_right},
		{"stereo (side)", audio_chunk::channel_side_left | audio_chunk::channel_side_right},
		{"2.1", audio_chunk::channel_config_2point1},
		{"3.0", audio_chunk::channel_config_3point0},
		{"4.0", audio_chunk::channel_config_4point0},
		{"4.1", audio_chunk::channel_config_4point1},
		{"5.0", audio_chunk::channel_config_5point0},
		{"5.1", audio_chunk::channel_config_5point1},
		{"5.1 (side)", audio_chunk::channel_config_5point1_side},
		{"6.1", audio_chunk::channel_config_5point1 | audio_chunk::channel_back_center},
		{"6.1 (side)", audio_chunk::channel_config_5point1_side | audio_chunk::channel_back_center},
		{"7.1", audio_chunk::channel_config_7point1},
	};
}

const char* audio_chunk::g_channelMaskName(unsigned flags) {
	for (auto& walk : maskDesc) {
		if (flags == walk.mask) return walk.name;
	}
	return nullptr;
}

static_assert( pfc::countBits32(audio_chunk::channel_config_mono) == 1 );
static_assert( pfc::countBits32(audio_chunk::channel_config_stereo) == 2 );
static_assert( pfc::countBits32(audio_chunk::channel_config_4point0) == 4 );
static_assert( pfc::countBits32(audio_chunk::channel_config_4point0_side) == 4 );
static_assert( pfc::countBits32(audio_chunk::channel_config_4point1) == 5 );
static_assert( pfc::countBits32(audio_chunk::channel_config_5point0) == 5 );
static_assert( pfc::countBits32(audio_chunk::channel_config_5point1) == 6 );
static_assert( pfc::countBits32(audio_chunk::channel_config_5point1_side) == 6 );
static_assert( pfc::countBits32(audio_chunk::channel_config_6point0) == 6);
static_assert( pfc::countBits32(audio_chunk::channel_config_7point1) == 8 );
