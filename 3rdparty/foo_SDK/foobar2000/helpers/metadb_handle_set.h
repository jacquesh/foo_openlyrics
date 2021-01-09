#pragma once
#include <set>
class metadb_handle;

// Roughly same as pfc::avltree_t<metadb_handle_ptr> or std::set<metadb_handle_ptr>, but optimized for use with large amounts of items
class metadb_handle_set {
public:
	metadb_handle_set() {}
	template<typename ptr_t>
	bool add_item_check(ptr_t const & item) { return add_item_check_(&*item); }
	template<typename ptr_t>
	bool remove_item(ptr_t const & item) { return remove_item_(&*item); }

	bool add_item_check_(metadb_handle * p) {
		bool rv = m_content.insert(p).second;
		if (rv) p->service_add_ref();
		return rv;
	}
	bool remove_item_(metadb_handle * p) {
		bool rv = m_content.erase(p) != 0;
		if (rv) p->service_release();
		return rv;
	}

	size_t get_count() const {
		return m_content.size();
	}
	template<typename ptr_t>
	bool contains(ptr_t const & item) const {
		return m_content.count(&*item) != 0;
	}
	template<typename ptr_t>
	bool have_item(ptr_t const & item) const {
		return m_content.count(&*item) != 0;
	}
	void operator+=(metadb_handle::ptr const & item) {
		add_item_check_(item.get_ptr());
	}
	void operator-=(metadb_handle::ptr const & item) {
		remove_item_(item.get_ptr());
	}
	void operator+=(metadb_handle::ptr && item) {
		auto p = item.detach();
		bool added = m_content.insert(p).second;
		if (!added) p->service_release();
	}
	void remove_all() {
		for (auto iter = m_content.begin(); iter != m_content.end(); ++iter) {
			metadb_handle * p = (*iter);
			p->service_release();
		}
		m_content.clear();
	}
	template<typename callback_t>
	void enumerate(callback_t & cb) const {
		for (auto iter = m_content.begin(); iter != m_content.end(); ++iter) {
			cb(*iter);
		}
	}
	typedef std::set<metadb_handle*> impl_t;
	typedef impl_t::const_iterator const_iterator;
	const_iterator begin() const { return m_content.begin(); }
	const_iterator end() const { return m_content.end(); }
private:

	std::set<metadb_handle*> m_content;

private:
	metadb_handle_set(const metadb_handle_set &) = delete;
	void operator=(const metadb_handle_set&) = delete;
};
