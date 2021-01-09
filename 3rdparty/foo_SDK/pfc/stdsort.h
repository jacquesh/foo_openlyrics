#pragma once

// OPTIONAL pfc feature, include on need to use basis
// std sort interop methods

#include <algorithm>
#include <functional>
#include <vector>

namespace pfc {

	std::vector<size_t> sort_identity( size_t count ) {
		std::vector<size_t> ret; ret.resize(count);
		for( size_t walk = 0; walk < ret.size(); ++ walk) ret[walk] = walk;
		return ret;
	}

	template<typename iterator_t, typename predicate_t>
	std::vector<size_t> sort_get_order(iterator_t i1, iterator_t i2, predicate_t pred ) {
		auto ret = sort_identity( i2 - i1 );
		auto pred2 = [pred, i1] (size_t idx1, size_t idx2) {
			return pred( *(i1+idx1), *(i1+idx2));
		};
		std::sort(ret.begin(), ret.end(), pred2);
		return ret;
	}

}
