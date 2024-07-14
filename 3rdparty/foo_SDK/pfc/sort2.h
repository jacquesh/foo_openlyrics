#pragma once

#include "sort.h"

// 2023 additions


namespace pfc {

	typedef array_t<size_t> permutation_t;

	permutation_t make_identitiy(size_t);

	template<typename container_t, typename compare_t>
	permutation_t sort_get_permutation(container_t const& data, compare_t compare) {
		const size_t count = std::size(data);
		auto ret = make_identitiy( count );
		if ( count > 0 ) sort_get_permutation_t(data, compare, count, ret.get_ptr() );
		return ret;
	}
	template<typename container_t, typename compare_t>
	permutation_t sort_stable_get_permutation(container_t const& data, compare_t compare) {
		const size_t count = std::size(data);
		auto ret = make_identitiy( count );
		if ( count > 0 ) sort_stable_get_permutation_t(data, compare, count, ret.get_ptr() );
		return ret;
	}

	template<typename container_t>
	void reorder(container_t& data, permutation_t const& order) {
		PFC_ASSERT( std::size(data) == std::size(order) );
		reorder_t( data, order.get_ptr(), order.get_size() );
	}
}