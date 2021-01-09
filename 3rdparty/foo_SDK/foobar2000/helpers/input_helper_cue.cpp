#include "stdafx.h"
#include "input_helper_cue.h"
#include "../SDK/mem_block_container.h"


namespace {
	class input_dec_binary : public input_decoder_v2 {
		enum {
			m_rate = 44100,
			m_bps = 16,
			m_channels = 2,
			m_channelMask = audio_chunk::channel_config_stereo,
			m_sampleBytes = (m_bps/8)*m_channels,
			m_readAtOnce = 588,
			m_readAtOnceBytes = m_readAtOnce * m_sampleBytes
		};
	public:
		input_dec_binary( file::ptr f ) : m_file(f) {}
		t_uint32 get_subsong_count() override {return 0;}
		t_uint32 get_subsong(t_uint32 p_index) override {return 0;}
	
		void get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort) override {
			p_info.reset();
			p_info.info_set_int("samplerate", m_rate);
			p_info.info_set_int("channels", m_channels);
			p_info.info_set_int("bitspersample", m_bps);
			p_info.info_set("encoding","lossless");
			p_info.info_set_bitrate((m_bps * m_channels * m_rate + 500 /* rounding for bps to kbps*/ ) / 1000 /* bps to kbps */);
			p_info.info_set("codec", "PCM");

			try {
				auto stats = get_file_stats(p_abort);
				if ( stats.m_size != filesize_invalid ) {
					p_info.set_length( audio_math::samples_to_time( stats.m_size / 4, 44100 ) );
				}
			} catch(exception_io) {}
		}

		
		t_filestats get_file_stats(abort_callback & p_abort) override {
			return m_file->get_stats(p_abort);
		}
		void initialize(t_uint32 p_subsong,unsigned p_flags,abort_callback & p_abort) override {
			m_file->reopen( p_abort );
		}
		bool run(audio_chunk & p_chunk,abort_callback & p_abort) override {
			mem_block_container_impl stfu;
			return run_raw(p_chunk, stfu, p_abort);
		}
		bool run_raw(audio_chunk & out, mem_block_container & outRaw, abort_callback & abort) override {
			size_t bytes = m_readAtOnceBytes;
			outRaw.set_size( bytes );
			size_t got = m_file->read(outRaw.get_ptr(), bytes, abort);
			got -= got % m_sampleBytes;
			if ( got == 0 ) return false;
			if ( got < bytes ) outRaw.set_size( got );
			out.set_data_fixedpoint_signed( outRaw.get_ptr(), got, m_rate, m_channels, m_bps, m_channelMask);
			return true;
		}
		void seek(double p_seconds,abort_callback & p_abort) override {
			m_file->seek( audio_math::time_to_samples( p_seconds, m_rate ) * m_sampleBytes, p_abort );
		}
		bool can_seek() override {
			return m_file->can_seek();
		}
		bool get_dynamic_info(file_info & p_out, double & p_timestamp_delta) override {return false;}
		bool get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) override {return false;}
		void on_idle(abort_callback & p_abort) override {}
		void set_logger(event_logger::ptr ptr) override {}
	private:
		const file::ptr m_file;
	};
}


void input_helper_cue::get_info_binary( const char * path, file_info & out, abort_callback & abort ) {
	auto f = fileOpenReadExisting( path, abort );
	auto obj = fb2k::service_new< input_dec_binary > ( f );
	obj->get_info( 0, out, abort );
}

void input_helper_cue::open(service_ptr_t<file> p_filehint,const playable_location & p_location,unsigned p_flags,abort_callback & p_abort,double p_start,double p_length, bool binary) {
	p_abort.check();

	m_start = p_start;
	m_position = 0;
	m_dynamic_info_trigger = false;
	m_dynamic_info_track_trigger = false;
	
	if ( binary ) {
	{
		const char * path = p_location.get_path();
		auto f = fileOpenReadExisting( path, p_abort );
		auto obj = fb2k::service_new< input_dec_binary > ( f );
		
		m_input.attach( obj, path );
		m_input.open_decoding( 0, p_flags, p_abort );
	}
	} else {
		m_input.open(p_filehint,p_location,p_flags,p_abort,true,true);
	}
	
	
	if (!m_input.can_seek()) throw exception_io_object_not_seekable();

	if (m_start > 0) {
		m_input.seek(m_start,p_abort);
	}

	if (p_length > 0) {
		m_length = p_length;
	} else {
		file_info_impl temp;
		m_input.get_info(0,temp,p_abort);
		double ref_length = temp.get_length();
		if (ref_length <= 0) throw exception_io_data();
		m_length = ref_length - m_start + p_length /* negative or zero */;
		if (m_length <= 0) throw exception_io_data();
	}
}

void input_helper_cue::close() {m_input.close();}
bool input_helper_cue::is_open() {return m_input.is_open();}

bool input_helper_cue::_m_input_run(audio_chunk & p_chunk, mem_block_container * p_raw, abort_callback & p_abort) {
	if (p_raw == NULL) {
		return m_input.run(p_chunk, p_abort);
	} else {
		return m_input.run_raw(p_chunk, *p_raw, p_abort);
	}
}
bool input_helper_cue::_run(audio_chunk & p_chunk, mem_block_container * p_raw, abort_callback & p_abort) {
	p_abort.check();
	
	if (m_length > 0) {
		if (m_position >= m_length) return false;

		if (!_m_input_run(p_chunk, p_raw, p_abort)) return false;

		m_dynamic_info_trigger = true;
		m_dynamic_info_track_trigger = true;

		t_uint64 max = (t_uint64)audio_math::time_to_samples(m_length - m_position, p_chunk.get_sample_rate());
		if (max == 0)
		{//handle rounding accidents, this normally shouldn't trigger
			m_position = m_length;
			return false;
		}

		t_size samples = p_chunk.get_sample_count();
		if ((t_uint64)samples > max)
		{
			p_chunk.set_sample_count((unsigned)max);
			if (p_raw != NULL) {
				const t_size rawSize = p_raw->get_size();
				PFC_ASSERT(rawSize % samples == 0);
				p_raw->set_size((t_size)((t_uint64)rawSize * max / samples));
			}
			m_position = m_length;
		}
		else
		{
			m_position += p_chunk.get_duration();
		}
		return true;
	}
	else
	{
		if (!_m_input_run(p_chunk, p_raw, p_abort)) return false;
		m_position += p_chunk.get_duration();
		return true;
	}
}
bool input_helper_cue::run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort) {
	return _run(p_chunk, &p_raw, p_abort);
}

bool input_helper_cue::run(audio_chunk & p_chunk, abort_callback & p_abort) {
	return _run(p_chunk, NULL, p_abort);
}

void input_helper_cue::seek(double p_seconds, abort_callback & p_abort)
{
	m_dynamic_info_trigger = false;
	m_dynamic_info_track_trigger = false;
	if (m_length <= 0 || p_seconds < m_length) {
		m_input.seek(p_seconds + m_start, p_abort);
		m_position = p_seconds;
	}
	else {
		m_position = m_length;
	}
}

bool input_helper_cue::can_seek() { return true; }

void input_helper_cue::on_idle(abort_callback & p_abort) { m_input.on_idle(p_abort); }

bool input_helper_cue::get_dynamic_info(file_info & p_out, double & p_timestamp_delta) {
	if (m_dynamic_info_trigger) {
		m_dynamic_info_trigger = false;
		return m_input.get_dynamic_info(p_out, p_timestamp_delta);
	}
	else {
		return false;
	}
}

bool input_helper_cue::get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) {
	if (m_dynamic_info_track_trigger) {
		m_dynamic_info_track_trigger = false;
		return m_input.get_dynamic_info_track(p_out, p_timestamp_delta);
	}
	else {
		return false;
	}
}

const char * input_helper_cue::get_path() const { return m_input.get_path(); }

void input_helper_cue::get_info(t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort) { m_input.get_info(p_subsong, p_info, p_abort); }
