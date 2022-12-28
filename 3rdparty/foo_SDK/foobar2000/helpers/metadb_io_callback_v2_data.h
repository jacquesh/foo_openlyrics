#pragma once

#pragma once
#include <mutex>
#include <vector>
#include <SDK/metadb_callbacks.h>

class metadb_io_callback_v2_data_impl : public metadb_io_callback_v2_data {
	metadb_handle_list_cref m_items;
	pfc::array_t< metadb_v2_rec_t> m_data;
	std::once_flag m_once;
public:
	metadb_io_callback_v2_data_impl(metadb_handle_list_cref items) : m_items(items) {}
	metadb_v2_rec_t get(size_t idxInList) override {
		// Late init, don't hammer metadb if nobody cares
		// Maybe this should be offthread somehow?? Kick off early, stall till done once asked?
		std::call_once(m_once, [&] { m_data = metadb_v2::get()->queryMultiSimple(m_items); });
		PFC_ASSERT(m_data.size() == m_items.get_size());
		PFC_ASSERT(idxInList < m_data.size());
		return m_data[idxInList];
	}
};

class metadb_io_callback_v2_data_wrap : public metadb_io_callback_v2_data {
public:
	metadb_io_callback_v2_data_wrap(metadb_io_callback_v2_data& chain) : m_chain(chain) {}
	metadb_io_callback_v2_data& m_chain;
	std::vector<size_t> m_mapping;

	metadb_v2_rec_t get(size_t idxInList) override {
		PFC_ASSERT(idxInList < m_mapping.size());
		return m_chain[m_mapping[idxInList]];
	}
};

class metadb_io_callback_v2_data_mirror : public metadb_io_callback_v2_data {
public:
	metadb_v2_rec_t get(size_t idxInList) override {return m_data[idxInList];}

	metadb_io_callback_v2_data_mirror() {}
	metadb_io_callback_v2_data_mirror(metadb_io_callback_v2_data& source, size_t size) {
		init(source, size);
	}
	void init(metadb_io_callback_v2_data& source, size_t size) {
		m_data.set_size_discard(size);
		for (size_t w = 0; w < size; ++w) m_data[w] = source[w];
	}
private:
	pfc::array_t< metadb_v2_rec_t> m_data;
};