#include "foobar2000-sdk-pch.h"
#include "advconfig.h"
#include "advconfig_impl.h"

bool advconfig_entry::is_branch() {
	advconfig_branch::ptr branch;
	return branch &= this;
}

bool advconfig_entry::g_find(service_ptr_t<advconfig_entry>& out, const GUID & id) {
	for (auto ptr : advconfig_entry::enumerate()) {
		if (ptr->get_guid() == id) { out = ptr; return true; }
	}
	return false;
}

t_uint32 advconfig_entry::get_preferences_flags_() {
	{
		advconfig_entry_string_v2::ptr ex;
		if (service_query_t(ex)) return ex->get_preferences_flags();
	}
	{
		advconfig_entry_checkbox_v2::ptr ex;
		if (service_query_t(ex)) return ex->get_preferences_flags();
	}
	return 0;
}

bool advconfig_entry_checkbox::get_default_state_() {
	{
		advconfig_entry_checkbox_v2::ptr ex;
		if (service_query_t(ex)) return ex->get_default_state();
	}

	bool backup = get_state();
	reset();
	bool rv = get_state();
	set_state(backup);
	return rv;
}

void advconfig_entry_string::get_default_state_(pfc::string_base & out) {
	{
		advconfig_entry_string_v2::ptr ex;
		if (service_query_t(ex)) {ex->get_default_state(out); return;}
	}
	pfc::string8 backup;
	get_state(backup);
	reset();
	get_state(out);
	set_state(backup);
}


#if FOOBAR2000_TARGET_VERSION >= 81
// advconfig_impl.h functionality

void advconfig_entry_checkbox_impl::reset() {
	fb2k::configStore::get()->deleteConfigBool(m_varName);
}

void advconfig_entry_checkbox_impl::set_state(bool p_state) {
	fb2k::configStore::get()->setConfigBool(m_varName, p_state);
}

bool advconfig_entry_checkbox_impl::get_state_() const {
	return fb2k::configStore::get()->getConfigBool(m_varName, m_initialstate);
}

#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
void advconfig_entry_checkbox_impl::set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) {
	uint8_t v;
	if (p_stream->read(&v, 1, p_abort) == 1) {
		set_state(v != 0);
	}
}
#endif

void advconfig_entry_string_impl::reset() {
	fb2k::configStore::get()->deleteConfigString(m_varName);
}
void advconfig_entry_string_impl::get_state(pfc::string_base& p_out) {
	p_out = fb2k::configStore::get()->getConfigString(m_varName, m_initialstate)->c_str();
}
void advconfig_entry_string_impl::set_state(const char* p_string, t_size p_length) {
	pfc::string8 asdf;
	if (p_length != SIZE_MAX) {
		asdf.set_string(p_string, p_length);
		p_string = asdf;
	}
	fb2k::configStore::get()->setConfigString(m_varName, p_string);
}

#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
void advconfig_entry_string_impl::set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) {
	pfc::string8_fastalloc temp;
	p_stream->read_string_raw(temp, p_abort);
	this->set_state(temp);
}
#endif

#endif // FOOBAR2000_TARGET_VERSION >= 81
