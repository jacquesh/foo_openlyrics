#pragma once

#include "search_tools.h"

//! OBSOLETE, DO NOT USE \n
//! Please use search_index instead.
class NOVTABLE library_index : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(library_index);
public:
	virtual void hit_test(search_filter::ptr pattern, metadb_handle_list_cref items, bool* out, abort_callback & a) = 0;

	enum {
		flag_sort = 1 << 0,
	};

	virtual fb2k::arrayRef search(search_filter::ptr pattern, uint32_t flags, abort_callback& abort) = 0;
};