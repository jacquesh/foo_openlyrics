#pragma once

#include "config_object.h"
#include "cfg_var_legacy.h"
//template function bodies from config_object class

template<class T>
void config_object::get_data_struct_t(T& p_out) {
	if (get_data_raw(&p_out,sizeof(T)) != sizeof(T)) throw exception_io_data_truncation();
}

template<class T>
void config_object::set_data_struct_t(const T& p_in) {
	return set_data_raw(&p_in,sizeof(T));
}

template<class T>
void config_object::g_get_data_struct_t(const GUID & p_guid,T & p_out) {
	service_ptr_t<config_object> ptr;
	if (!g_find(ptr,p_guid)) throw exception_service_not_found();
	return ptr->get_data_struct_t<T>(p_out);
}

template<class T>
void config_object::g_set_data_struct_t(const GUID & p_guid,const T & p_in) {
	service_ptr_t<config_object> ptr;
	if (!g_find(ptr,p_guid)) throw exception_service_not_found();
	return ptr->set_data_struct_t<T>(p_in);
}

#if FOOBAR2020
class config_object_impl : public config_object, private cfg_var_legacy::cfg_var_reader
{
public:
	GUID get_guid() const override {return cfg_var_reader::m_guid;}
	void get_data(stream_writer * p_stream,abort_callback & p_abort) const override;
	void set_data(stream_reader * p_stream,abort_callback & p_abort,bool p_notify) override;

	config_object_impl(const GUID & p_guid,const void * p_data,t_size p_bytes);
private:
#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
	void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort) override {set_data(p_stream,p_abort,false);}
#endif

	pfc::string8 formatName() const;
	
	fb2k::memBlockRef m_initial;
};
#else
class config_object_impl : public config_object, private cfg_var_legacy::cfg_var
{
public:
	GUID get_guid() const { return cfg_var::get_guid(); }
	void get_data(stream_writer* p_stream, abort_callback& p_abort) const;
	void set_data(stream_reader* p_stream, abort_callback& p_abort, bool p_notify);

	config_object_impl(const GUID& p_guid, const void* p_data, t_size p_bytes);
private:

	//cfg_var methods
	void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) { get_data(p_stream, p_abort); }
	void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) { set_data(p_stream, p_abort, false); }

	mutable pfc::readWriteLock m_sync;
	pfc::array_t<t_uint8> m_data;
};
#endif

typedef service_factory_single_transparent_t<config_object_impl> config_object_factory;

class config_object_bool_factory : public config_object_factory {
public:
	config_object_bool_factory(const GUID& id, bool def) : config_object_factory(id, &def, 1) {}
};

class config_object_string_factory : public config_object_factory {
public:
	config_object_string_factory(const GUID& id, const char * def) : config_object_factory(id, def, strlen(def)) {}
};


class config_object_notify_impl_simple : public config_object_notify
{
public:
	t_size get_watched_object_count() {return 1;}
	GUID get_watched_object(t_size p_index) {return m_guid;}
	void on_watched_object_changed(const service_ptr_t<config_object> & p_object) {m_func(p_object);}
	
	typedef void (*t_func)(const service_ptr_t<config_object> &);

	config_object_notify_impl_simple(const GUID & p_guid,t_func p_func) : m_guid(p_guid), m_func(p_func) {}
private:
	GUID m_guid;
	t_func m_func;	
};

typedef service_factory_single_transparent_t<config_object_notify_impl_simple> config_object_notify_simple_factory;
