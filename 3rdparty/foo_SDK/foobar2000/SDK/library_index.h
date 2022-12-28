#pragma once

#include "search_tools.h"

//! \since 2.0
//! Provides access to indexed searches of media library content, \n
//! which in typical scenarios are much faster than running whole library content via search filters. \n
//! Not every search query is supported by library index - in particular, queries relying on title formatting cannot be optimized. \n
//! In such cases, search() will fall through to legacy search filter use. \n
//! Note that if your query looks like: cond1 AND cond2 AND cond3 (...), and only some of the conditions cannot be searched using index, \n
//! search() will use the supported subset of your query to only feed relevant subset of your library to slow search filters.
class NOVTABLE library_index : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(library_index);
public:
	//! Same as using search filters, provided for consistency. Use search() instead.
	virtual void hit_test(search_filter::ptr pattern, metadb_handle_list_cref items, bool* out, abort_callback & a) = 0;

	enum {
		flag_sort = 1 << 0,
	};

	//! @returns list of metadb_handles. Safe to use arr->as_list_of<metadb_handle>() to get a pfc::list_base_const_t<metadb_handle_ptr>
	virtual fb2k::arrayRef search(search_filter::ptr pattern, uint32_t flags, abort_callback& abort) = 0;
};