#pragma once

class metadb_handle_array : public fb2k::array {
public:
	metadb_handle_array() {}
	metadb_handle_array(metadb_handle_list&& lst) : m_items(std::move(lst)) {}
	size_t count() const override { return m_items.get_size(); }
	fb2k::objRef itemAt(size_t index) const override { return m_items[index]; }

	t_size get_count() const override { return m_items.get_size(); }
	void get_item_ex(service_ptr& p_out, t_size n) const override { p_out = m_items[n]; }

	metadb_handle_list m_items;
};

