#pragma once

//! Entrypoint class for adding items to Advanced Preferences page. \n
//! Implementations must derive from one of subclasses: advconfig_branch, advconfig_entry_checkbox, advconfig_entry_string. \n
//! Implementations are typically registered using static service_factory_single_t<myclass>, or using provided helper classes in case of standard implementations declared in this header.
class NOVTABLE advconfig_entry : public service_base {
public:
	virtual void get_name(pfc::string_base & p_out) = 0;
	virtual GUID get_guid() = 0;
	virtual GUID get_parent() = 0;
	virtual void reset() = 0;
	virtual double get_sort_priority() = 0;

	bool is_branch();
	t_uint32 get_preferences_flags_();

	static bool g_find(service_ptr_t<advconfig_entry>& out, const GUID & id);

	template<typename outptr> static bool g_find_t(outptr & out, const GUID & id) {
		service_ptr_t<advconfig_entry> temp;
		if (!g_find(temp, id)) return false;
		return temp->service_query_t(out);
	}

	static const GUID guid_root;
	static const GUID guid_branch_tagging,guid_branch_decoding,guid_branch_tools,guid_branch_playback,guid_branch_display,guid_branch_debug, guid_branch_tagging_general, guid_branch_converter;
	

	// \since 2.0
	static const GUID guid_branch_vis, guid_branch_general;

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(advconfig_entry);
};

//! Declares a new branch in Advanced Preferences. \n
//! Implementation: see advconfig_branch_impl / advconfig_branch_factory.
class NOVTABLE advconfig_branch : public advconfig_entry {
public:
	FB2K_MAKE_SERVICE_INTERFACE(advconfig_branch,advconfig_entry);
};

//! Declares a checkbox/radiocheckbox entry in Advanced Preferences. \n
//! The difference between checkboxes and radiocheckboxes is different icon (obviously) and that checking a radiocheckbox unchecks all other radiocheckboxes in the same branch. \n
//! Implementation: see advconfig_entry_checkbox_impl / advconfig_checkbox_factory_t.
class NOVTABLE advconfig_entry_checkbox : public advconfig_entry {
public:
	virtual bool get_state() = 0;
	virtual void set_state(bool p_state) = 0;
	virtual bool is_radio() = 0;

	bool get_default_state_();

	FB2K_MAKE_SERVICE_INTERFACE(advconfig_entry_checkbox,advconfig_entry);
};

//! Extension to advconfig_entry_checkbox, adds default state and preferences flags.
class NOVTABLE advconfig_entry_checkbox_v2 : public advconfig_entry_checkbox {
	FB2K_MAKE_SERVICE_INTERFACE(advconfig_entry_checkbox_v2, advconfig_entry_checkbox)
public:
	virtual bool get_default_state() = 0;
	virtual t_uint32 get_preferences_flags() {return 0;} //signals whether changing this setting should trigger playback restart or app restart; see: preferences_state::* constants
};

//! Declares a string/integer editbox entry in Advanced Preferences.\n
//! Implementation: see advconfig_entry_string_impl / advconfig_string_factory.
class NOVTABLE advconfig_entry_string : public advconfig_entry {
public:
	virtual void get_state(pfc::string_base & p_out) = 0;
	virtual void set_state(const char * p_string,t_size p_length = SIZE_MAX) = 0;
	virtual t_uint32 get_flags() = 0;

	void get_default_state_(pfc::string_base & out);

	enum {
		flag_is_integer		= 1 << 0, 
		flag_is_signed		= 1 << 1,
	};

	FB2K_MAKE_SERVICE_INTERFACE(advconfig_entry_string,advconfig_entry);
};

//! Extension to advconfig_entry_string, adds default state, validation and preferences flags.
class NOVTABLE advconfig_entry_string_v2 : public advconfig_entry_string {
	FB2K_MAKE_SERVICE_INTERFACE(advconfig_entry_string_v2, advconfig_entry_string)
public:
	virtual void get_default_state(pfc::string_base & out) = 0;
	virtual void validate(pfc::string_base & val) {}
	virtual t_uint32 get_preferences_flags() {return 0;} //signals whether changing this setting should trigger playback restart or app restart; see: preferences_state::* constants
};
