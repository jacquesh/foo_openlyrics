#pragma once

// advconfig_impl.h : mainline (foobar2000 v2.0) implementation of advconfig objects

#include "advconfig.h"

//! Standard implementation of advconfig_branch. \n
//! Usage: no need to use this class directly - use advconfig_branch_factory instead.
class advconfig_branch_impl : public advconfig_branch {
public:
	advconfig_branch_impl(const char* p_name, const GUID& p_guid, const GUID& p_parent, double p_priority) : m_name(p_name), m_guid(p_guid), m_parent(p_parent), m_priority(p_priority) {}
	void get_name(pfc::string_base& p_out) override { p_out = m_name; }
	GUID get_guid() override { return m_guid; }
	GUID get_parent() override { return m_parent; }
	void reset() override {}
	double get_sort_priority() override { return m_priority; }
private:
	pfc::string8 m_name;
	GUID m_guid, m_parent;
	const double m_priority;
};


#if FOOBAR2000_TARGET_VERSION < 81
#include "advconfig_impl_legacy.h"
#else

#include "configStore.h"
#include "configCache.h"
#include "cfg_var_legacy.h" // cfg_var_reader

namespace fb2k {
	pfc::string8 advconfig_autoName(const GUID& id);
}


//! Standard implementation of advconfig_entry_checkbox. \n
class advconfig_entry_checkbox_impl : public advconfig_entry_checkbox_v2, private cfg_var_legacy::cfg_var_reader {
public:
	advconfig_entry_checkbox_impl(const char* p_name, const char * varName, const GUID& p_guid, const GUID& p_parent, double p_priority, bool p_initialstate, bool isRadio = false, uint32_t flags = 0)
		: cfg_var_reader(p_guid), m_name(p_name), m_varName(varName), m_guid(p_guid), m_initialstate(p_initialstate), m_parent(p_parent), m_priority(p_priority), m_isRadio(isRadio), m_flags(flags) {}

	void get_name(pfc::string_base& p_out) override { p_out = m_name; }
	GUID get_guid() override { return m_guid; }
	GUID get_parent() override { return m_parent; }
	void reset() override;
	bool get_state() override { return get_state_(); }
	void set_state(bool p_state) override;
	bool is_radio() override { return m_isRadio; }
	double get_sort_priority() override { return m_priority; }
	bool get_default_state() override { return m_initialstate; }
	t_uint32 get_preferences_flags() override { return m_flags; }

	bool get_state_() const;
	bool get_default_state_() const { return m_initialstate; }

	const char* varName() const { return m_varName; }
private:
#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
	// cfg_var_reader
	void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override;
#endif

	const pfc::string8 m_name, m_varName;
	const GUID m_guid;
	const bool m_initialstate;
	const bool m_isRadio;
	const uint32_t m_flags;

	const GUID m_parent;
	const double m_priority;
};

//! Service factory helper around standard advconfig_branch implementation. Use this class to register your own Advanced Preferences branches. \n
//! Usage: static advconfig_branch_factory mybranch(name, branchID, parentBranchID, priority);
class advconfig_branch_factory : public service_factory_single_t<advconfig_branch_impl> {
public:
	advconfig_branch_factory(const char* p_name, const GUID& p_guid, const GUID& p_parent, double p_priority)
		: service_factory_single_t<advconfig_branch_impl>(p_name, p_guid, p_parent, p_priority) {}
};

template<bool is_radio>
class advconfig_checkbox_factory_ : public service_factory_single_t<advconfig_entry_checkbox_impl> {
public:
	advconfig_checkbox_factory_(const char* name, const GUID& guid, const GUID& parent, double priority, bool initialstate, uint32_t flags = 0) : service_factory_single_t<advconfig_entry_checkbox_impl>(name, fb2k::advconfig_autoName(guid), guid, parent, priority, initialstate, is_radio, flags) {}
	advconfig_checkbox_factory_(const char* name, const char* varName, const GUID& guid, const GUID& parent, double priority, bool initial, uint32_t flags = 0) : service_factory_single_t<advconfig_entry_checkbox_impl>(name, varName, guid, parent, priority, initial, is_radio, flags) {}

	bool get() const { return this->get_static_instance().get_state_(); }
	void set(bool val) { this->get_static_instance().set_state(val); }
	operator bool() const { return get(); }
	bool operator=(bool val) { set(val); return val; }
	const char* varName() const { return this->get_static_instance().varName(); }
};

typedef advconfig_checkbox_factory_<false> advconfig_checkbox_factory;
typedef advconfig_checkbox_factory_<true> advconfig_radio_factory;

#if 0
// OBSOLETE, use advconfig_checkbox_factory / advconfig_radio_factory
template<bool p_is_radio, uint32_t prefFlags = 0>
class advconfig_checkbox_factory_t : public advconfig_checkbox_factory_<p_is_radio> {
public:
	advconfig_checkbox_factory_t(const char* p_name, const GUID& p_guid, const GUID& p_parent, double p_priority, bool p_initialstate) : advconfig_checkbox_factory_<p_is_radio>(p_name, fb2k::advconfig_autoName(p_guid), p_guid, p_parent, p_priority, p_initialstate, prefFlags) {}
};
#endif

//! Standard advconfig_entry_string implementation. Use advconfig_string_factory to register your own string entries in Advanced Preferences instead of using this class directly.
class advconfig_entry_string_impl : public advconfig_entry_string_v2, private cfg_var_legacy::cfg_var_reader {
public:
	advconfig_entry_string_impl(const char* p_name, const char * p_varName, const GUID& p_guid, const GUID& p_parent, double p_priority, const char* p_initialstate, t_uint32 p_prefFlags)
		: cfg_var_reader(p_guid), m_name(p_name), m_varName(p_varName), m_guid(p_guid), m_parent(p_parent), m_initialstate(p_initialstate), m_priority(p_priority), m_prefFlags(p_prefFlags) {}
	void get_name(pfc::string_base& p_out) override { p_out = m_name; }
	GUID get_guid() override { return m_guid; }
	GUID get_parent() override { return m_parent; }
	void reset() override;
	double get_sort_priority() override { return m_priority; }
	void get_state(pfc::string_base& p_out) override;
	void set_state(const char* p_string, t_size p_length = SIZE_MAX) override;
	t_uint32 get_flags() override { return 0; }
	void get_default_state(pfc::string_base& out)  override{ out = m_initialstate; }
	t_uint32 get_preferences_flags() override { return m_prefFlags; }
private:
#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
	// cfg_var_reader
	void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override;
#endif

	const pfc::string8 m_initialstate, m_name, m_varName;
	const GUID m_guid, m_parent;

	const double m_priority;
	const t_uint32 m_prefFlags;
};

//! Service factory helper around standard advconfig_entry_string implementation. Use this class to register your own string entries in Advanced Preferences. \n
//! Usage: static advconfig_string_factory mystring(name, itemID, branchID, priority, initialValue);
class advconfig_string_factory : public service_factory_single_t<advconfig_entry_string_impl> {
public:
	advconfig_string_factory(const char* p_name, const char * varName, const GUID& p_guid, const GUID& p_parent, double p_priority, const char* p_initialstate, t_uint32 p_prefFlags = 0) : service_factory_single_t<advconfig_entry_string_impl>(p_name, varName, p_guid, p_parent, p_priority, p_initialstate, p_prefFlags) {}
	advconfig_string_factory(const char* p_name, const GUID& p_guid, const GUID& p_parent, double p_priority, const char* p_initialstate, t_uint32 p_prefFlags = 0) : service_factory_single_t<advconfig_entry_string_impl>(p_name, fb2k::advconfig_autoName(p_guid), p_guid, p_parent, p_priority, p_initialstate, p_prefFlags) {}

	void get(pfc::string_base& out) { get_static_instance().get_state(out); }
	pfc::string8 get() { pfc::string8 temp; get(temp); return temp; }
	void set(const char* in) { get_static_instance().set_state(in); }
};

// Thread hacks no longer needed
typedef advconfig_string_factory advconfig_string_factory_MT;

//! Special advconfig_entry_string implementation - implements integer entries. Use advconfig_integer_factory to register your own integer entries in Advanced Preferences instead of using this class directly.
template<typename int_t_>
class advconfig_entry_integer_impl_ : public advconfig_entry_string_v2, private cfg_var_legacy::cfg_var_reader {
public:
	typedef int_t_ int_t;
	advconfig_entry_integer_impl_(const char* p_name, const char * p_varName, const GUID& p_guid, const GUID& p_parent, double p_priority, int_t p_initialstate, int_t p_min, int_t p_max, t_uint32 p_prefFlags)
		: cfg_var_reader(p_guid), m_name(p_name), m_varName(p_varName), m_guid(p_guid), m_parent(p_parent), m_priority(p_priority), m_initval(p_initialstate), m_min(p_min), m_max(p_max), m_prefFlags(p_prefFlags) {
		PFC_ASSERT(p_min < p_max);
	}
	void get_name(pfc::string_base& p_out) override { p_out = m_name; }
	GUID get_guid() override { return m_guid; }
	GUID get_parent() override { return m_parent; }
    void reset() override {fb2k::configStore::get()->deleteConfigBool(m_varName);}
	double get_sort_priority() override { return m_priority; }
    void get_state(pfc::string_base& p_out) override { p_out = pfc::format_int( get_state_int() ); }
    void set_state(const char* p_string, t_size p_length) override {set_state_int(pfc::atoi64_ex(p_string, p_length));}
	t_uint32 get_flags() override { return advconfig_entry_string::flag_is_integer | (is_signed() ? flag_is_signed : 0); }

    int_t get_state_int() const {return fb2k::configStore::get()->getConfigInt(m_varName, m_initval);}
    void set_state_int(int_t val) { val = pfc::clip_t<int_t>(val, m_min, m_max); fb2k::configStore::get()->setConfigInt(m_varName, val); }

	void get_default_state(pfc::string_base& out) override {
		format(out, m_initval);
	}
	void validate(pfc::string_base& val) override {
		format(val, pfc::clip_t<int_t>(pfc::atoi64_ex(val, SIZE_MAX), m_min, m_max));
	}
	t_uint32 get_preferences_flags() override { return m_prefFlags; }
private:
#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
	// cfg_var_reader
	void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override {
		switch (p_sizehint) {
		case 4:
		{ int32_t v; p_stream->read_lendian_t(v, p_abort); this->set_state_int(v); } break;
		case 8:
		{ int64_t v; p_stream->read_lendian_t(v, p_abort); this->set_state_int(v); } break;
		}
	}

#endif

	void format(pfc::string_base& out, int_t v) const {
		if (is_signed()) out = pfc::format_int(v);
		else out = pfc::format_uint(v);
	}
	bool is_signed() const {
		return m_min < 0;
	}

	const double m_priority;
	const int_t m_initval, m_min, m_max;
	const GUID m_guid, m_parent;
	const pfc::string8 m_name, m_varName;
	const t_uint32 m_prefFlags;
};

typedef advconfig_entry_integer_impl_<uint64_t> advconfig_entry_integer_impl;

//! Service factory helper around integer-specialized advconfig_entry_string implementation. Use this class to register your own integer entries in Advanced Preferences. \n
//! Usage: static advconfig_integer_factory myint(name, itemID, parentID, priority, initialValue, minValue, maxValue);
template<typename int_t_>
class advconfig_integer_factory_ : public service_factory_single_t<advconfig_entry_integer_impl_<int_t_>> {
public:
	typedef int_t_ int_t;
	advconfig_integer_factory_(const char* p_name, const GUID& p_guid, const GUID& p_parent, double p_priority, t_uint64 p_initialstate, t_uint64 p_min, t_uint64 p_max, t_uint32 p_prefFlags = 0) : service_factory_single_t<advconfig_entry_integer_impl>(p_name, fb2k::advconfig_autoName(p_guid), p_guid, p_parent, p_priority, p_initialstate, p_min, p_max, p_prefFlags) {}
	advconfig_integer_factory_(const char* p_name, const char * p_varName, const GUID& p_guid, const GUID& p_parent, double p_priority, int_t p_initialstate, int_t p_min, int_t p_max, t_uint32 p_prefFlags = 0) : service_factory_single_t<advconfig_entry_integer_impl_<int_t_> >(p_name, p_varName, p_guid, p_parent, p_priority, p_initialstate, p_min, p_max, p_prefFlags) {}

	int_t get() const { return this->get_static_instance().get_state_int(); }
	void set(int_t val) { this->get_static_instance().set_state_int(val); }

	operator int_t() const { return get(); }
	int_t operator=(int_t val) { set(val); return val; }
};

typedef advconfig_integer_factory_<uint64_t> advconfig_integer_factory;
typedef advconfig_integer_factory_<int64_t> advconfig_signed_integer_factory;


class advconfig_integer_factory_cached : public advconfig_integer_factory {
public:
	using advconfig_integer_factory::int_t;
	advconfig_integer_factory_cached(const char* p_name, const char* p_varName, const GUID& p_guid, const GUID& p_parent, double p_priority, int_t p_initialstate, int_t p_min, int_t p_max, t_uint32 p_prefFlags = 0)
		: advconfig_integer_factory(p_name, p_varName, p_guid, p_parent, p_priority, p_initialstate, p_min, p_max, p_prefFlags),
		m_cache(p_varName, p_initialstate) {}

	int_t get() { return m_cache.get(); }
	void set(int_t v) { m_cache.set(v); }
	operator int_t() { return get(); }
private:
	fb2k::configIntCache m_cache;
};

template<bool is_radio>
class advconfig_checkbox_factory_cached_ : public advconfig_checkbox_factory_<is_radio> {
public:
	advconfig_checkbox_factory_cached_(const char* name, const char* varName, const GUID& guid, const GUID& parent, double priority, bool initial, uint32_t flags = 0) 
		: advconfig_checkbox_factory_<is_radio>(name, varName, guid, parent, priority, initial, flags),
		m_cache(varName, initial) {}

	bool get() { return m_cache.get(); }
    void set(bool v) { m_cache.set(v); }
	operator bool() { return get(); }
private:
	fb2k::configBoolCache m_cache;
};

typedef advconfig_checkbox_factory_cached_<false> advconfig_checkbox_factory_cached;
typedef advconfig_checkbox_factory_cached_<true> advconfig_radio_factory_cached;

/*
  Advanced Preferences variable declaration examples

	static advconfig_string_factory mystring("name goes here","configStore var name goes here", myguid,parentguid,0,"asdf");
	to retrieve state: pfc::string8 val; mystring.get(val);

	static advconfig_checkbox_factory mycheckbox("name goes here","configStore var name goes here", myguid,parentguid,0,false);
	to retrieve state: mycheckbox.get();

	static advconfig_integer_factory myint("name goes here","configStore var name goes herE", myguid,parentguid,0,initialValue,minimumValue,maximumValue);
	to retrieve state: myint.get();
*/
#endif
