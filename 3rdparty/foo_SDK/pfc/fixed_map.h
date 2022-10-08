#pragma once

#include <map>
#include <algorithm>
#include <vector>

template<typename key_t, typename value_t> class fixed_map {
public:
	typedef std::map<key_t, value_t> source_t;

	void initialize(source_t&& src) {
		const size_t size = src.size();
		m_keys.resize(size); m_values.resize(size);
		size_t walk = 0;
		for (auto iter = src.begin(); iter != src.end(); ++iter) {
			m_keys[walk] = iter->first;
			m_values[walk] = std::move(iter->second);
			++walk;
		}
		src.clear();
	}

	value_t query(key_t key) const {
		auto iter = std::lower_bound(m_keys.begin(), m_keys.end(), key);
		if (iter == m_keys.end() || *iter != key) return 0;
		return m_values[iter - m_keys.begin()];
	}

	const value_t* query_ptr(key_t key) const {
		auto iter = std::lower_bound(m_keys.begin(), m_keys.end(), key);
		if (iter == m_keys.end() || *iter != key) return nullptr;
		return &m_values[iter - m_keys.begin()];
	}
private:
	std::vector<key_t> m_keys;
	std::vector<value_t> m_values;
};