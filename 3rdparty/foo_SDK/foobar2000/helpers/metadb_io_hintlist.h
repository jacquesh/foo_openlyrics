#pragma once

#include "metadb_info_container_impl.h"
#include <SDK/input.h>
#include <SDK/filesystem.h>
#include <SDK/metadb.h>


// Obsolete, use metadb_hint_list instead when possible, wrapper provided for compatibility with old code

class metadb_io_hintlist {
public:
	void hint_reader(service_ptr_t<input_info_reader> p_reader, const char * p_path,abort_callback & p_abort) {
		init();
		m_hints->add_hint_reader( p_path, p_reader, p_abort );
		m_pendingCount += p_reader->get_subsong_count();
	}
	void add(metadb_handle_ptr const& h, const file_info& i, t_filestats2 const& s, bool f) {
		init();
		metadb_hint_list_v3::ptr v3;
		v3 ^= m_hints;

		auto infoObj = fb2k::service_new< metadb_info_container_const_impl >();
		infoObj->m_info = i; infoObj->m_stats = s;
		v3->add_hint_v3(h, infoObj, f);
		
		++m_pendingCount;
	}
	void add(metadb_handle_ptr const & p_handle,const file_info & p_info,t_filestats const & p_stats,bool p_fresh) {
		init();
		m_hints->add_hint( p_handle, p_info, p_stats, p_fresh );
		++m_pendingCount;
	}
	void run() {
		if ( m_hints.is_valid() ) {
			m_hints->on_done();
			m_hints.release();
		}
		m_pendingCount = 0;
	}
	size_t get_pending_count() const { return m_pendingCount; }
private:
	void init() {
		if ( m_hints.is_empty() ) {
			m_hints = metadb_io_v2::get()->create_hint_list();
		}
	}
	metadb_hint_list::ptr m_hints;
	size_t m_pendingCount = 0;
};
