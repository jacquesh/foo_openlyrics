#pragma once

// Obsolete features

//! Special hack to ensure errors when someone tries to ->service_add_ref()/->service_release() on a service_ptr_t
template<typename T> class service_obscure_refcounting : public T {
private:
	int service_add_ref() throw();
	int service_release() throw();
};

//! Converts a service interface pointer to a pointer that obscures service counter functionality.
template<typename T> static inline service_obscure_refcounting<T>* service_obscure_refcounting_cast(T * p_source) throw() {return static_cast<service_obscure_refcounting<T>*>(p_source);}

template<typename T, template<typename> class t_alloc = pfc::alloc_fast>
class service_list_t : public pfc::list_t<service_ptr_t<T>, t_alloc >
{
};

//! Helper; simulates array with instance of each available implementation of given service class.
template<typename T> class service_instance_array_t {
public:
	typedef service_ptr_t<T> t_ptr;
	service_instance_array_t() {
		service_class_helper_t<T> helper;
		const t_size count = helper.get_count();
		m_data.set_size(count);
		for(t_size n=0;n<count;n++) m_data[n] = helper.create(n);
	}

	t_size get_size() const {return m_data.get_size();}
	const t_ptr & operator[](t_size p_index) const {return m_data[p_index];}

	//nonconst version to allow sorting/bsearching; do not abuse
	t_ptr & operator[](t_size p_index) {return m_data[p_index];}
private:
	pfc::array_t<t_ptr> m_data;
};
