#pragma once
#include <vector>
#include <algorithm>
#include <SDK/callback_merit.h>

namespace fb2k {
	// Call global callbacks in correct order
	// INTENDED FOR CORE USE ONLY
	// Not aware of dynamically registered callbacks, if there are such for the same event type
	// Usage: for_each_callback_by_merit< metadb_io_edit_callback > ( [] ( metadb_io_edit_callback::ptr ) { ...} );
	template<typename class_t>
	void for_each_callback_by_merit(std::function<void(service_ptr_t<class_t>)> f) {
		struct rec_t {
			class_t* obj; // non-autoptr INTENDED, avoid destruction order bugs on shutdown
			fb2k::callback_merit_t merit;
		};
		auto generator = [] {
			std::vector<rec_t> ret; ret.reserve(64);
			for (auto ptr : class_t::enumerate()) { 
				rec_t r;
				r.merit = fb2k::callback_merit_of(ptr);
				r.obj = ptr.detach();
				ret.push_back( std::move(r) );
			}
			std::sort(ret.begin(), ret.end(), [](rec_t const& e1, rec_t const& e2) { return e1.merit > e2.merit; });
			return ret;
		};
		static std::vector<rec_t> cache = generator();
		for (auto& walk : cache) f(walk.obj);
	}
}
