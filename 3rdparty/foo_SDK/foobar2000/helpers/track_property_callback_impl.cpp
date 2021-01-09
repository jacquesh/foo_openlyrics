#include "StdAfx.h"

#include <list>
#include <memory>

#include "track_property_callback_impl.h"


void track_property_callback_impl::set_property(const char * p_group, double p_sortpriority, const char * p_name, const char * p_value) {
	propertyname_container temp;
	temp.m_name = p_name;
	temp.m_priority = p_sortpriority;

	pfc::string8 fixEOL;
	if (m_cutMultiLine && strchr(p_value, '\n') != nullptr) {
		fixEOL = p_value; fixEOL.fix_eol(); p_value = fixEOL;
	}

	m_entries.find_or_add(p_group).set(temp, p_value);
}

bool track_property_callback_impl::is_group_wanted(const char * p_group) {
	if (m_groupFilter) return m_groupFilter(p_group);
	return true;
}

void track_property_callback_impl::merge(track_property_callback_impl const & other) {
	for (auto iterGroup = other.m_entries.first(); iterGroup.is_valid(); ++iterGroup) {
		auto & in = iterGroup->m_value;
		auto & out = m_entries[iterGroup->m_key];
		for (auto iterEntry = in.first(); iterEntry.is_valid(); ++iterEntry) {
			out.set(iterEntry->m_key, iterEntry->m_value);
		}
	}
}

static bool is_filtered_info_field(const char * p_name) {
	service_ptr_t<track_property_provider> ptr;
	service_enum_t<track_property_provider> e;
	while (e.next(ptr)) {
		if (ptr->is_our_tech_info(p_name)) return true;
	}
	return false;
}

static const char strGroupOther[] = "Other";

static void enumOtherHere(track_property_callback_impl & callback, metadb_info_container::ptr info_) {
	const file_info * infoptr = &info_->info();
	for (t_size n = 0, m = infoptr->info_get_count(); n < m; n++) {
		const char * name = infoptr->info_enum_name(n);
		if (!is_filtered_info_field(name)) {
			pfc::string_formatter temp;
			temp << "<";
			uAddStringUpper(temp, name);
			temp << ">";
			callback.set_property("Other", 0, temp, infoptr->info_enum_value(n));
		}
	}
}

static void enumOther( track_property_callback_impl & callback, metadb_handle_list_cref items, track_property_provider_v3_info_source * infoSource ) {
	if (items.get_count() == 1 ) {
		enumOtherHere(callback, infoSource->get_info(0) );
	}
}

void enumerateTrackProperties( track_property_callback_impl & callback, std::function< metadb_handle_list_cref () > itemsSource, std::function<track_property_provider_v3_info_source*()> infoSource, std::function<abort_callback& () > abortSource) {
	

	if ( core_api::is_main_thread() ) {
		// should not get here like this
		// but that does make our job easier
		auto & items = itemsSource();
		auto info = infoSource();
		track_property_provider::ptr ptr;
		service_enum_t<track_property_provider> e;
		while (e.next(ptr)) {
			ptr->enumerate_properties_helper(items, info, callback, abortSource() );
		}
		if ( callback.is_group_wanted( strGroupOther ) ) {
			enumOther(callback, items, info );
		}
		return;
	}

	std::list<std::shared_ptr<pfc::event> > lstWaitFor;
	std::list<std::shared_ptr< track_property_callback_impl > > lstMerge;
	track_property_provider::ptr ptr;
	service_enum_t<track_property_provider> e;
	while (e.next(ptr)) {
		auto evt = std::make_shared<pfc::event>();
		auto cb = std::make_shared< track_property_callback_impl >(callback); // clone watched group info
		auto work = [ptr, itemsSource, evt, cb, infoSource, abortSource] {
			try {
				ptr->enumerate_properties_helper(itemsSource(), infoSource(), *cb, abortSource());
			} catch (...) {}
			evt->set_state(true);
		};

		track_property_provider_v4::ptr v4;
		if (v4 &= ptr) {
			// Supports v4 = split a worker thread, work in parallel
			pfc::splitThread(work);
		} else {
			// No v4 = delegate to main thread. Ugly but gets the job done.
			fb2k::inMainThread(work);
		}

		lstWaitFor.push_back(std::move(evt));
		lstMerge.push_back(std::move(cb));
	}

	if (callback.is_group_wanted(strGroupOther)) {
		enumOther(callback, itemsSource(), infoSource());
	}

	for (auto i = lstWaitFor.begin(); i != lstWaitFor.end(); ++i) {
		abortSource().waitForEvent(**i, -1);
	}
	for (auto i = lstMerge.begin(); i != lstMerge.end(); ++i) {
		callback.merge(** i);
	}
}