#pragma once
#include "cfg_objList.h"
#include <pfc/bsearch_inline.h>

class cfg_guidlist : public cfg_objList<GUID> {
public:
	cfg_guidlist(const GUID& p_guid) : cfg_objList(p_guid) {}

	void sort() {
		auto v = this->get();
		pfc::sort_t(v, pfc::guid_compare, v.size());
		set(std::move(v));
	}

	bool have_item_bsearch(const GUID & p_item) {
		size_t dummy;
		return pfc::bsearch_simple_inline_t(*this, size(), p_item, pfc::guid_compare, dummy);
	}
	bool bsearch(const GUID& item, size_t & idx) {
		return pfc::bsearch_simple_inline_t(*this, size(), item, pfc::guid_compare, idx);
	}
	size_t bsearch(const GUID& item) {
		size_t ret = SIZE_MAX;
		pfc::bsearch_simple_inline_t(*this, size(), item, pfc::guid_compare, ret);
		return ret;
	}
};
