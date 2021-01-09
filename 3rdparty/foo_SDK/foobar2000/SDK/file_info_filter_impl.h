#pragma once

//! Generic implementation of file_info_filter_impl.
class file_info_filter_impl : public file_info_filter {
public:
	file_info_filter_impl(const pfc::list_base_const_t<metadb_handle_ptr> & p_list, const pfc::list_base_const_t<const file_info*> & p_new_info) {
		FB2K_DYNAMIC_ASSERT(p_list.get_count() == p_new_info.get_count());
		pfc::array_t<t_size> order;
		order.set_size(p_list.get_count());
		order_helper::g_fill(order.get_ptr(), order.get_size());
		p_list.sort_get_permutation_t(pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, order.get_ptr());
		m_handles.set_count(order.get_size());
		m_infos.set_size(order.get_size());
		for (t_size n = 0; n < order.get_size(); n++) {
			m_handles[n] = p_list[order[n]];
			m_infos[n] = *p_new_info[order[n]];
		}
	}

	bool apply_filter(metadb_handle_ptr p_location, t_filestats p_stats, file_info & p_info) {
		t_size index;
		if (m_handles.bsearch_t(pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, p_location, index)) {
			p_info = m_infos[index];
			return true;
		}
		else {
			return false;
		}
	}
private:
	metadb_handle_list m_handles;
	pfc::array_t<file_info_impl> m_infos;
};
