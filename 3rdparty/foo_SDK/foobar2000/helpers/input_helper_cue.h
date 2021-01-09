#pragma once

#include "input_helpers.h"


class input_helper_cue {
public:
	void open(service_ptr_t<file> p_filehint,const playable_location & p_location,unsigned p_flags,abort_callback & p_abort,double p_start,double p_length, bool binary = false);
	static void get_info_binary( const char * path, file_info & out, abort_callback & abort );

	void close();
	bool is_open();
	bool run(audio_chunk & p_chunk,abort_callback & p_abort);
	bool run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort);
	void seek(double seconds,abort_callback & p_abort);
	bool can_seek();
	void on_idle(abort_callback & p_abort);
	bool get_dynamic_info(file_info & p_out,double & p_timestamp_delta);
	bool get_dynamic_info_track(file_info & p_out,double & p_timestamp_delta);
	void set_logger(event_logger::ptr ptr) {m_input.set_logger(ptr);}

	const char * get_path() const;
	
	void get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort);

private:
	bool _run(audio_chunk & p_chunk, mem_block_container * p_raw, abort_callback & p_abort);
	bool _m_input_run(audio_chunk & p_chunk, mem_block_container * p_raw, abort_callback & p_abort);
	input_helper m_input;
	double m_start,m_length,m_position;
	bool m_dynamic_info_trigger,m_dynamic_info_track_trigger;
	bool m_binary;
};
