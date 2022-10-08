#pragma once

template<typename float_t> inline void render_float(float_t* out, const audio_sample* in, size_t count) {
	if (sizeof(audio_sample) == sizeof(float_t)) {
		memcpy(out, in, count * sizeof(audio_sample));
	} else {
		for (size_t walk = 0; walk < count; ++walk) out[walk] = (float_t)in[walk];
	}
}

template<typename float_t> inline const float_t* render_float(mem_block_container& buffer, const audio_sample* in, size_t count) {
	if (sizeof(float_t) == sizeof(audio_sample)) {
		return reinterpret_cast<const float_t*>(in);
	}
	buffer.set_size(sizeof(float_t) * count);
	float_t* out = reinterpret_cast<float_t*>(buffer.get_ptr());
	render_float(out, in, count);
	return out;
}

inline const void* render_float_by_bps(unsigned bps, mem_block_container& buffer, const audio_sample* in, size_t count) {
	switch (bps) {
	case 32:
		return render_float<float>(buffer, in, count);
	case 64:
		return render_float<double>(buffer, in, count);
	default:
		throw exception_io_data();
	}
}