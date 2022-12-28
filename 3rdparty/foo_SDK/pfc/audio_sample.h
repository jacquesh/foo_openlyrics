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
		static void convert_to_int16(const float* p_source, size_t p_count, int16_t * p_output, float p_scale);
		static void convert_to_int16(const double* p_source, size_t p_count, int16_t* p_output, double p_scale);
		static void convert_to_int32(const float* p_source, size_t p_count, int32_t * p_output, float p_scale);
		static void convert_to_int32(const double* p_source, size_t p_count, int32_t* p_output, double p_scale);
		static void convert_from_int16(const int16_t * p_source, size_t p_count, float * p_output, float p_scale);
		static void convert_from_int16(const int16_t* p_source, size_t p_count, double * p_output, double p_scale);
		static void convert_from_int32(const int32_t* p_source, size_t p_count, float* p_output, float p_scale);
		static void convert_from_int32(const int32_t* p_source, size_t p_count, double* p_output, double p_scale);

		static float calculate_peak(const float * p_source, size_t p_count);
		static double calculate_peak(const double * p_source, size_t p_count);
		
		static void remove_denormals(float * p_buffer, size_t p_count);
		static void remove_denormals(double * p_buffer, size_t p_count);

		
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

		inline static int64_t rint64(float val) { return (int64_t)llroundf(val); }
		inline static int32_t rint32(float val) { return (int32_t)lroundf(val); }
		inline static int64_t rint64(double val) { return (int64_t)llround(val); }
		inline static int32_t rint32(double val) { return (int32_t)lround(val); }

		static inline uint64_t time_to_samples(double p_time, uint32_t p_sample_rate) {
			return (uint64_t)pfc::rint64((double)p_sample_rate * p_time);
		}

		static inline double samples_to_time(uint64_t p_samples, uint32_t p_sample_rate) {
			PFC_ASSERT(p_sample_rate > 0);
			return (double)p_samples / (double)p_sample_rate;
		}

		static inline double gain_to_scale(double p_gain) { return pow(10.0, p_gain / 20.0); }
		static inline double scale_to_gain(double scale) { return 20.0*log10(scale); }

		static constexpr float float16scale = 65536.f;

		static float decodeFloat24ptr(const void * sourcePtr);
		static float decodeFloat24ptrbs(const void * sourcePtr);
		static float decodeFloat16(uint16_t source);

		static unsigned bitrate_kbps( uint64_t fileSize, double duration );
	}; // class audio_math

} // namespace pfc
