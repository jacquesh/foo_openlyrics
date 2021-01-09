#pragma once

#include <functional>

class groupname_comparator {
public:
	static int compare(pfc::stringp p_name1,pfc::stringp p_name2) {
		int temp = uStringCompare(p_name1,p_name2);
		if (temp != 0) return temp;
		return strcmp(p_name1,p_name2);
	}
};

struct propertyname_container {
	pfc::string m_name;
	double m_priority;
};

class propertyname_comparator {
public:
	static int compare(const propertyname_container & p_item1,const propertyname_container & p_item2) {
		int state = pfc::compare_t(p_item1.m_priority,p_item2.m_priority);
		if (state != 0) return state;
		return uStringCompare(p_item1.m_name.ptr(),p_item2.m_name.ptr());
	}
};

typedef pfc::map_t<propertyname_container,pfc::string,propertyname_comparator> property_group;

typedef pfc::map_t<pfc::string,property_group,groupname_comparator> t_property_group_list;

class track_property_callback_impl : public track_property_callback_v2 {
public:
	void set_property(const char * p_group,double p_sortpriority,const char * p_name,const char * p_value) override;
	bool is_group_wanted(const char * p_group) override;

	void merge( track_property_callback_impl const & other );

	t_property_group_list m_entries;

	bool m_cutMultiLine = false;
	typedef std::function<bool ( const char * ) > groupFilter_t;
	groupFilter_t m_groupFilter;
};

// Helper function to walk all track property providers in an optimized multithread manner
// Various *source arguments have been std::function'd so you can reference your own data structures gracefully
// If the function is aborted, it returns immediately - while actual worker threads may not yet have completed, and may still reference *source arguments. 
// You must ensure - by means of std::shared_ptr<> or such - that all of the *source arguments remain accessible even after enumerateTrackProperties() returns, until the std::functions are released.
// Legacy track property providers that do not support off main thread operation will be invoked via main_thread_callback in main thread, and the function will stall until they have returned (unless aborted).
void enumerateTrackProperties(track_property_callback_impl & callback, std::function< metadb_handle_list_cref() > itemsSource, std::function<track_property_provider_v3_info_source*()> infoSource, std::function<abort_callback& () > abortSource);
