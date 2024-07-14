#include "shared.h"

//#define AUDIO_MATH_NOASM

// NOTE: SSE4.1 int16 ops code determined MUCH SLOWER than SSE2 on Sandy Bridge era Xeon and therefore disabled for now
// #define SUPPORT_SSE41

#ifdef SUPPORT_SSE41
static const bool g_have_sse41 = pfc::query_cpu_feature_set(pfc::CPU_HAVE_SSE41);

inline static void convert_from_int16_noopt(const t_int16 * p_source,t_size p_count,audio_sample * p_output,float p_scale)
{
	t_size num = p_count;
	for(;num;num--)
		*(p_output++) = (audio_sample)*(p_source++) * p_scale;
}

__declspec(naked) static void __fastcall convert_from_int16_sse41_8word(const t_int16 * p_source,t_size p_count,audio_sample * p_output,float p_scale) {
	__asm {
		// ecx = source, edx = count, [esp + 4] = output, [esp + 8] = scale
		movss xmm7, [esp + 8]
		test edx, edx
		mov eax, [esp + 4]
		pshufd xmm7, xmm7, 0
		jz loopend
loopbegin:
		PMOVSXWD xmm0, mmword ptr [ecx]
		PMOVSXWD xmm1, mmword ptr [ecx+8]
		CVTDQ2PS xmm0, xmm0
		CVTDQ2PS xmm1, xmm1
		add ecx, 16
		mulps xmm0, xmm7
		mulps xmm1, xmm7
		dec edx
		movups [eax], xmm0
		movups [eax + 16], xmm1
		lea eax, [eax + 32]
		jnz loopbegin
loopend:
		ret 8
	}
}


__declspec(naked) static void __fastcall convert_from_int16_sse41_8word_aligned(const t_int16 * p_source,t_size p_count,audio_sample * p_output,float p_scale) {
	__asm {
		// ecx = source, edx = count, [esp + 4] = output, [esp + 8] = scale
		movss xmm7, [esp + 8]
		test edx, edx
		mov eax, [esp + 4]
		pshufd xmm7, xmm7, 0
		jz loopend
loopbegin:
		PMOVSXWD xmm0, mmword ptr [ecx]
		PMOVSXWD xmm1, mmword ptr [ecx+8]
		CVTDQ2PS xmm0, xmm0
		CVTDQ2PS xmm1, xmm1
		add ecx, 16
		mulps xmm0, xmm7
		mulps xmm1, xmm7
		dec edx
		movaps [eax], xmm0
		movaps [eax + 16], xmm1
		lea eax, [eax + 32]
		jnz loopbegin
loopend:
		ret 8
	}
}
#endif



#ifdef audio_math 
#undef audio_math
#endif
namespace audio_math {

	void SHARED_EXPORT scale(const audio_sample * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale)
	{
		::pfc::audio_math::scale(p_source, p_count, p_output, p_scale);
	}
	
	void SHARED_EXPORT convert_to_int16(const audio_sample * p_source,t_size p_count,t_int16 * p_output,audio_sample p_scale)
	{
		::pfc::audio_math::convert_to_int16(p_source, p_count, p_output, p_scale);
	}

	audio_sample SHARED_EXPORT convert_to_int16_calculate_peak(const audio_sample * p_source,t_size p_count,t_int16 * p_output,audio_sample p_scale)
	{
		convert_to_int16(p_source,p_count,p_output,p_scale);
		return p_scale * calculate_peak(p_source,p_count);
	}

	void SHARED_EXPORT convert_from_int16(const t_int16 * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale)
	{
#ifdef SUPPORT_SSE41
		if (g_have_sse41) {
			audio_sample scale = (audio_sample)(p_scale / (double)0x8000);
			convert_from_int16_sse41_8word(p_source, p_count >> 3, p_output, scale);
			convert_from_int16_noopt(p_source + (p_count & ~7), p_count & 7, p_output + (p_count & ~7), scale);
			return;
		}
#endif
		::pfc::audio_math::convert_from_int16(p_source, p_count, p_output, p_scale);
	}

	void SHARED_EXPORT convert_to_int32(const audio_sample * p_source,t_size p_count,t_int32 * p_output,audio_sample p_scale)
	{
		return ::pfc::audio_math::convert_to_int32(p_source, p_count, p_output, p_scale);
	}

	audio_sample SHARED_EXPORT convert_to_int32_calculate_peak(const audio_sample * p_source,t_size p_count,t_int32 * p_output,audio_sample p_scale)
	{
		convert_to_int32(p_source,p_count,p_output,p_scale);
		return p_scale * calculate_peak(p_source,p_count);
	}

	void SHARED_EXPORT convert_from_int32(const t_int32 * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale)
	{
		::pfc::audio_math::convert_from_int32(p_source, p_count, p_output, p_scale);
	}


	audio_sample SHARED_EXPORT calculate_peak(const audio_sample * p_source,t_size p_count) {
		return ::pfc::audio_math::calculate_peak(p_source, p_count);
	}

	void SHARED_EXPORT kill_denormal(audio_sample * p_buffer,t_size p_count) {
		::pfc::audio_math::remove_denormals(p_buffer, p_count);
	}

	void SHARED_EXPORT add_offset(audio_sample * p_buffer,audio_sample p_delta,t_size p_count) {
		::pfc::audio_math::add_offset(p_buffer, p_delta, p_count);
	}

}
