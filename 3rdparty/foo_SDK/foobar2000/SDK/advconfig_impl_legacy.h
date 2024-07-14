#pragma once

// advconfig_impl_legacy.h : legacy (foobar2000 v1.x compatible) implementation of advconfig objects

#include "cfg_var_legacy.h"
using namespace cfg_var_legacy;

namespace fb2k {
	pfc::string8 advconfig_autoName(const GUID&);
}

#if FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE
#define ADVCONFIG_DOWNGRADE { this->get_static_instance().downgrade_set_name(configStoreName); }
#define ADVCONFIG_DOWNGRADE_AUTO { this->get_static_instance().downgrade_set_name( fb2k::advconfig_autoName(p_guid) ); }
#else
#define ADVCONFIG_DOWNGRADE {(void)configStoreName;}
#define ADVCONFIG_DOWNGRADE_AUTO
#endif

//! Standard implementation of advconfig_entry_checkbox. \n
class advconfig_entry_checkbox_impl : public advconfig_entry_checkbox_v2 {
public:
	advconfig_entry_checkbox_impl(const char* p_name, const GUID& p_guid, const GUID& p_parent, double p_priority, bool p_initialstate, bool isRadio = false, uint32_t flags = 0)
		: m_name(p_name), m_initialstate(p_initialstate), m_state(p_guid, p_initialstate), m_parent(p_parent), m_priority(p_priority), m_isRadio(isRadio), m_flags(flags) {}

	void get_name(pfc::string_base& p_out) override { p_out = m_name; }
	GUID get_guid() override { return m_state.get_guid(); }
	GUID get_parent() override { return m_parent; }
	void reset() override { m_state = m_initialstate; }
	bool get_state() override { return m_state; }
	void set_state(bool p_state) override { m_state = p_state; }
	bool is_radio() override { return m_isRadio; }
	double get_sort_priority() override { return m_priority; }
	bool get_default_state() override { return m_initialstate; }
	t_uint32 get_preferences_flags() override { return m_flags; }

	bool get_state_() const { return m_state; }
	bool get_default_state_() const { return m_initialstate; }
#if FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE
	void downgrade_set_name(const char * arg) { m_state.downgrade_set_name(arg); }
#endif
private:
	const pfc::string8 m_name;
	const bool m_initialstate;
	const bool m_isRadio;
	const uint32_t m_flags;
	cfg_bool m_state;
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

class advconfig_checkbox_factory_common : public service_factory_single_t<advconfig_entry_checkbox_impl> {
public:
	template<typename ... args_t> advconfig_checkbox_factory_common(args_t && ... args) : service_factory_single_t<advconfig_entry_checkbox_impl>(std::forward<args_t>(args) ...) {}

	bool get() const { return this->get_static_instance().get_state_(); }
	void set(bool val) { this->get_static_instance().set_state(val); }
	operator bool() const { return get(); }
	bool operator=(bool val) { set(val); return val; }
};

class advconfig_checkbox_factory : public advconfig_checkbox_factory_common {
public:
	advconfig_checkbox_factory(const char* p_name, const GUID& p_guid, const GUID& p_parent, double p_priority, bool p_initialstate, uint32_t flags = 0)
		: advconfig_checkbox_factory_common(p_name, p_guid, p_parent, p_priority, p_initialstate, false /* not radio */, flags) {
		ADVCONFIG_DOWNGRADE_AUTO;
	}
	advconfig_checkbox_factory(const char* p_name, const char * configStoreName, const GUID& p_guid, const GUID& p_parent, double p_priority, bool p_initialstate, uint32_t flags = 0)
		: advconfig_checkbox_factory_common(p_name, p_guid, p_parent, p_priority, p_initialstate, false /* not radio */, flags) {
		ADVCONFIG_DOWNGRADE;
	}

};

class advconfig_radio_factory : public advconfig_checkbox_factory_common {
public:
	advconfig_radio_factory(const char* p_name, const GUID& p_guid, const GUID& p_parent, double p_priority, bool p_initialstate, uint32_t flags = 0)
		: advconfig_checkbox_factory_common(p_name, p_guid, p_parent, p_priority, p_initialstate, true /* radio */, flags) {
		ADVCONFIG_DOWNGRADE_AUTO;
	}
	advconfig_radio_factory(const char* p_name, const char * configStoreName, const GUID& p_guid, const GUID& p_parent, double p_priority, bool p_initialstate, uint32_t flags = 0)
		: advconfig_checkbox_factory_common(p_name, p_guid, p_parent, p_priority, p_initialstate, true /* radio */, flags) {
		ADVCONFIG_DOWNGRADE;
	}
};

// OBSOLETE, use advconfig_checkbox_factory / advconfig_radio_factory
template<bool p_is_radio, uint32_t prefFlags = 0>
class advconfig_checkbox_factory_t : public advconfig_checkbox_factory_common {
public:
	advconfig_checkbox_factory_t(const char* p_name, const GUID& p_guid, const GUID& p_parent, double p_priority, bool p_initialstate)
		: advconfig_checkbox_factory_common(p_name, p_guid, p_parent, p_priority, p_initialstate, p_is_radio, prefFlags) {
		ADVCONFIG_DOWNGRADE_AUTO;
	}
	advconfig_checkbox_factory_t(const char* p_name, const char * configStoreName, const GUID& p_guid, const GUID& p_parent, double p_priority, bool p_initialstate)
		: advconfig_checkbox_factory_common(p_name, p_guid, p_parent, p_priority, p_initialstate, p_is_radio, prefFlags) {
		ADVCONFIG_DOWNGRADE;
	}
};

//! Special advconfig_entry_string implementation - implements integer entries. Use advconfig_integer_factory to register your own integer entries in Advanced Preferences instead of using this class directly.
template<typename int_t_>
class advconfig_entry_integer_impl_ : public advconfig_entry_string_v2 {
public:
	typedef int_t_ int_t;
	advconfig_entry_integer_impl_(const char* p_name, const GUID& p_guid, const GUID& p_parent, double p_priority, int_t p_initialstate, int_t p_min, int_t p_max, t_uint32 p_prefFlags)
		: m_name(p_name), m_parent(p_parent), m_priority(p_priority), m_initval(p_initialstate), m_min(p_min), m_max(p_max), m_state(p_guid, p_initialstate), m_prefFlags(p_prefFlags) {
		PFC_ASSERT(p_min < p_max);
	}
	void get_name(pfc::string_base& p_out) { p_out = m_name; }
	GUID get_guid() { return m_state.get_guid(); }
	GUID get_parent() { return m_parent; }
	void reset() { m_state = m_initval; }
	double get_sort_priority() { return m_priority; }
	void get_state(pfc::string_base& p_out) { format(p_out, m_state.get_value()); }
	void set_state(const char* p_string, t_size p_length) { set_state_int(myATOI(p_string, p_length)); }
	t_uint32 get_flags() { return advconfig_entry_string::flag_is_integer | (is_signed() ? flag_is_signed : 0); }

	int_t get_state_int() const { return m_state; }
	void set_state_int(int_t val) { m_state = pfc::clip_t<int_t>(val, m_min, m_max); }

	void get_default_state(pfc::string_base& out) {
		format(out, m_initval);
	}
	void validate(pfc::string_base& val) {
		format(val, pfc::clip_t<int_t>(myATOI(val, SIZE_MAX), m_min, m_max));
	}
	t_uint32 get_preferences_flags() { return m_prefFlags; }
#if FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE
	void downgrade_set_name(const char * arg) { m_state.downgrade_set_name(arg); }
#endif
private:
	static void format(pfc::string_base& out, int_t v) {
		if (is_signed()) out = pfc::format_int(v).get_ptr();
		else out = pfc::format_uint(v).get_ptr();
	}
	static int_t myATOI(const char* s, size_t l) {
		if (is_signed()) return pfc::atoi64_ex(s, l);
		else return pfc::atoui64_ex(s, l);
	}
	static bool is_signed() {
		return ((int_t)-1) < ((int_t)0);
	}
	cfg_int_t<int_t> m_state;
	const double m_priority;
	const int_t m_initval, m_min, m_max;
	const GUID m_parent;
	const pfc::string8 m_name;
	const t_uint32 m_prefFlags;
};

typedef advconfig_entry_integer_impl_<uint64_t> advconfig_entry_integer_impl;

//! Service factory helper around integer-specialized advconfig_entry_string implementation. Use this class to register your own integer entries in Advanced Preferences. \n
//! Usage: static advconfig_integer_factory myint(name, itemID, parentID, priority, initialValue, minValue, maxValue);
template<typename int_t_>
class advconfig_integer_factory_ : public service_factory_single_t<advconfig_entry_integer_impl_<int_t_> > {
public:
	typedef int_t_ int_t;
	advconfig_integer_factory_(const char* p_name, const GUID& p_guid, const GUID& p_parent, double p_priority, t_uint64 p_initialstate, t_uint64 p_min, t_uint64 p_max, t_uint32 p_prefFlags = 0)
		: service_factory_single_t<advconfig_entry_integer_impl_<int_t_> >(p_name, p_guid, p_parent, p_priority, p_initialstate, p_min, p_max, p_prefFlags) {
		ADVCONFIG_DOWNGRADE_AUTO;
	}

	advconfig_integer_factory_(const char* p_name, const char * configStoreName, const GUID& p_guid, const GUID& p_parent, double p_priority, t_uint64 p_initialstate, t_uint64 p_min, t_uint64 p_max, t_uint32 p_prefFlags = 0)
		: service_factory_single_t<advconfig_entry_integer_impl_<int_t_> >(p_name, p_guid, p_parent, p_priority, p_initialstate, p_min, p_max, p_prefFlags) {
		ADVCONFIG_DOWNGRADE;
	}

	int_t get() const { return this->get_static_instance().get_state_int(); }
	void set(int_t val) { this->get_static_instance().set_state_int(val); }

	operator int_t() const { return get(); }
	int_t operator=(int_t val) { set(val); return val; }
};

typedef advconfig_integer_factory_<uint64_t> advconfig_integer_factory;
typedef advconfig_integer_factory_<int64_t> advconfig_signed_integer_factory;


//! Standard advconfig_entry_string implementation
class advconfig_entry_string_impl : public advconfig_entry_string_v2 {
public:
	advconfig_entry_string_impl(const char* p_name, const GUID& p_guid, const GUID& p_parent, double p_priority, const char* p_initialstate, t_uint32 p_prefFlags)
		: m_name(p_name), m_parent(p_parent), m_priority(p_priority), m_initialstate(p_initialstate), m_state(p_guid, p_initialstate), m_prefFlags(p_prefFlags) {}
	void get_name(pfc::string_base& p_out) { p_out = m_name; }
	GUID get_guid() { return m_state.get_guid(); }
	GUID get_parent() { return m_parent; }
	void reset() {
		inWriteSync(m_sync);
		m_state = m_initialstate;
	}
	double get_sort_priority() { return m_priority; }
	void get_state(pfc::string_base& p_out) {
		inReadSync(m_sync);
		p_out = m_state;
	}
	void set_state(const char* p_string, t_size p_length = SIZE_MAX) {
		inWriteSync(m_sync);
		m_state.set_string(p_string, p_length);
	}
	t_uint32 get_flags() { return 0; }
	void get_default_state(pfc::string_base& out) { out = m_initialstate; }
	t_uint32 get_preferences_flags() { return m_prefFlags; }
#if FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE
	void downgrade_set_name(const char * arg) { m_state.downgrade_set_name(arg); }
#endif
private:
	const pfc::string8 m_initialstate, m_name;
	cfg_string m_state;
	pfc::readWriteLock m_sync;
	const double m_priority;
	const GUID m_parent;
	const t_uint32 m_prefFlags;
};

//! Service factory helper around standard advconfig_entry_string implementation. Use this class to register your own string entries in Advanced Preferences. \n
//! Usage: static advconfig_string_factory mystring(name, itemID, branchID, priority, initialValue);
class advconfig_string_factory : public service_factory_single_t<advconfig_entry_string_impl> {
public:
	advconfig_string_factory(const char* p_name, const GUID& p_guid, const GUID& p_parent, double p_priority, const char* p_initialstate, t_uint32 p_prefFlags = 0)
		: service_factory_single_t<advconfig_entry_string_impl>(p_name, p_guid, p_parent, p_priority, p_initialstate, p_prefFlags) {
		ADVCONFIG_DOWNGRADE_AUTO;
	}
	advconfig_string_factory(const char* p_name, const char * configStoreName, const GUID& p_guid, const GUID& p_parent, double p_priority, const char* p_initialstate, t_uint32 p_prefFlags = 0)
		: service_factory_single_t<advconfig_entry_string_impl>(p_name, p_guid, p_parent, p_priority, p_initialstate, p_prefFlags) {
		ADVCONFIG_DOWNGRADE;
	}

	void get(pfc::string_base& out) { get_static_instance().get_state(out); }
	void set(const char* in) { get_static_instance().set_state(in); }
};

// No more separate _MT versions, readWriteLock overhead is irrelevant
typedef advconfig_entry_string_impl advconfig_entry_string_impl_MT;
typedef advconfig_string_factory advconfig_string_factory_MT;


/*
  Advanced Preferences variable declaration examples

	static advconfig_string_factory mystring("name goes here",myguid,parentguid,0,"asdf");
	to retrieve state: pfc::string8 val; mystring.get(val);

	static advconfig_checkbox_factory mycheckbox("name goes here",myguid,parentguid,0,false);
	to retrieve state: mycheckbox.get();

	static advconfig_integer_factory myint("name goes here",myguid,parentguid,0,initialValue,minimumValue,maximumValue);
	to retrieve state: myint.get();
*/


#undef ADVCONFIG_DOWNGRADE
#undef ADVCONFIG_DOWNGRADE_AUTO
