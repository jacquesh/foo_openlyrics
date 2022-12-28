#pragma once

#include "file_info_const_impl.h"

class metadb_info_container_const_impl : public metadb_info_container_v2 {
public:
	file_info const& info() override { return m_info; }
	t_filestats const& stats() override { return m_stats.as_legacy(); }
	bool isInfoPartial() override { return false; }
	t_filestats2 const& stats2() override { return m_stats; }
	file_info_const_impl m_info;
	t_filestats2 m_stats;
};
