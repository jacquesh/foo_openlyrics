#pragma once
#include <SDK/cfg_var.h>
#include <SDK/dsp.h>

#if FOOBAR2020
class cfg_dsp_chain_config : public cfg_blob {
public:
	void reset() {
		cfg_blob::set(nullptr);
	}
	cfg_dsp_chain_config(const GUID& p_guid) : cfg_blob(p_guid) {}

	void get_data(dsp_chain_config& p_data) {
		p_data.from_blob(cfg_blob::get());
	}
	void set_data(const dsp_chain_config& p_data) {
		cfg_blob::set(p_data.to_blob());
	}
};

typedef cfg_dsp_chain_config cfg_dsp_chain_config_mt;
#else
class cfg_dsp_chain_config : public cfg_var {
protected:
	void get_data_raw(stream_writer * p_stream,abort_callback & p_abort) {m_data.contents_to_stream(p_stream,p_abort);}
	void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort) {m_data.contents_from_stream(p_stream,p_abort);}
public:
	void reset() { m_data.remove_all(); }
	inline cfg_dsp_chain_config(const GUID & p_guid) : cfg_var(p_guid) {}
	t_size get_count() const {return m_data.get_count();}
	const dsp_preset & get_item(t_size p_index) const {return m_data.get_item(p_index);}
	void get_data(dsp_chain_config & p_data) const {p_data.copy(m_data);}
	void set_data(const dsp_chain_config & p_data) {m_data.copy(p_data);}
	dsp_chain_config_impl & _data() {return m_data; }
private:
	dsp_chain_config_impl m_data;
};

class cfg_dsp_chain_config_mt : private cfg_var {
public:
	cfg_dsp_chain_config_mt( const GUID & id ) : cfg_var(id) {}
	void reset() { dsp_chain_config_impl dummy; set_data(dummy); }
	void get_data(dsp_chain_config & p_data) { inReadSync( m_sync ); p_data.copy(m_data); }
	template<typename arg_t>
	void set_data(arg_t && p_data) { inWriteSync( m_sync ); m_data = std::forward<arg_t>(p_data);}
protected:
	void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) {
		dsp_chain_config_impl temp;
		get_data( temp );
		temp.contents_to_stream( p_stream, p_abort );
	}
	void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) {
		dsp_chain_config_impl temp; temp.contents_from_stream( p_stream, p_abort );
		set_data( temp );
	}
private:
	pfc::readWriteLock m_sync;
	dsp_chain_config_impl m_data;
};
#endif
