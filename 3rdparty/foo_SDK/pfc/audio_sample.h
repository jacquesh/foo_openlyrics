#pragma once
#include <math.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif


namespace pfc {
	// made a class so it can be redirected to an alternate class more easily than with namespacing
	// in win desktop fb2k these are implemented in a DLL
	class audio_math {
	public:

		//! p_source/p_output can point to same buffer
		static void convert_to_int16(const float* p_source, t_size p_count, t_int16 * p_output, float p_scale);
		static void convert_to_int16(const double* p_source, t_size p_count, t_int16* p_output, double p_scale);
		static void convert_to_int32(const float* p_source, t_size p_count, t_int32 * p_output, float p_scale);
		static void convert_to_int32(const double* p_source, t_size p_count, t_int32* p_output, double p_scale);
		static void convert_from_int16(const t_int16 * p_source, t_size p_count, float * p_output, float p_scale);
		static void convert_from_int16(const t_int16* p_source, t_size p_count, double * p_output, double p_scale);
		static void convert_from_int32(const t_int32* p_source, t_size p_count, float* p_output, float p_scale);
		static void convert_from_int32(const t_int32* p_source, t_size p_count, double* p_output, double p_scale);

		static float calculate_peak(const float * p_source, t_size p_count);
		static double calculate_peak(const double * p_source, t_size p_count);
		
		static void remove_denormals(float * p_buffer, t_size p_count);
		static void remove_denormals(double * p_buffer, t_size p_count);

		
		static void add_offset(float * p_buffer, float p_delta, size_t p_count);
		static void add_offset(double * p_buffer, double p_delta, size_t p_count);

		static void scale(const float* p_source, size_t p_count, float * p_output, float p_scale);
		static void scale(const double* p_source, size_t p_count, double * p_output, double p_scale);

		static void convert(const float* in, float* out, size_t count);
		static void convert(const float* in, float* out, size_t count, float scale);
		static void convert(const float* in, double* out, size_t count);
		static void convert(const float* in, double* out, size_t count, double scale);
		static void convert(const double* in, float* out, size_t count);
		static void convert(const double* in, float* out, size_t count, double scale);
		static void convert(const double* in, double* out, size_t count);
		static void convert(const double* in, double* out, size_t count, double scale);

		inline static t_int64 rint64(float val) { return (t_int64)llroundf(val); }
		inline static t_int32 rint32(float val) { return (t_int32)lroundf(val); }
		inline static t_int64 rint64(double val) { return (t_int64)llround(val); }
		inline static t_int32 rint32(double val) { return (t_int32)lround(val); }

		static inline t_uint64 time_to_samples(double p_time, t_uint32 p_sample_rate) {
			return (t_uint64)pfc::rint64((double)p_sample_rate * p_time);
		}

		static inline double samples_to_time(t_uint64 p_samples, t_uint32 p_sample_rate) {
			PFC_ASSERT(p_sample_rate > 0);
			return (double)p_samples / (double)p_sample_rate;
		}

		static inline double gain_to_scale(double p_gain) { return pow(10.0, p_gain / 20.0); }
		static inline double scale_to_gain(double scale) { return 20.0*log10(scale); }

		static const float float16scale;

		static float decodeFloat24ptr(const void * sourcePtr);
		static float decodeFloat24ptrbs(const void * sourcePtr);
		static float decodeFloat16(uint16_t source);

		static unsigned bitrate_kbps( uint64_t fileSize, double duration );
	}; // class audio_math

} // namespace pfc
