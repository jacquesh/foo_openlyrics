#include "StdAfx.h"

#include <list>
#include <memory>

#include "track_property_callback_impl.h"
#include <SDK/threadPool.h>
#include <SDK/track_property.h>

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
	for (auto ptr : track_property_provider::enumerate()) {
		if (ptr->is_our_tech_info(p_name)) return true;
	}
	return false;
}

static const char strGroupOther[] = "Other";

static pfc::string8 encloseInfoName(const char* name) {
	pfc::string8 temp;
	temp << "<";
	uAddStringUpper(temp, name);
	temp << ">";
	return temp;
}

static void enumOtherHere(track_property_callback_impl & callback, metadb_info_container::ptr info_) {
	if (info_.is_empty()) return;
	const file_info * infoptr = &info_->info();
	for (t_size n = 0, m = infoptr->info_get_count(); n < m; n++) {
		const char * name = infoptr->info_enum_name(n);
		if (!is_filtered_info_field(name)) {
			callback.set_property(strGroupOther, 0, encloseInfoName(name), infoptr->info_enum_value(n));
		}
	}
}

static trackInfoContainer::ptr getInfoHelper(track_property_provider_v3_info_source* source, size_t idx) {
	return source->get_info(idx);
}

static trackInfoContainer::ptr getInfoHelper(track_property_provider_v5_info_source* source, size_t idx) {
	return source->get_info(idx).info;
}

template<typename infoSource_t>
static void enumOther( track_property_callback_impl & callback, metadb_handle_list_cref items, infoSource_t* infoSource ) {

	const size_t itemCount = items.get_count();
	if (itemCount == 1 ) {
		enumOtherHere(callback, getInfoHelper(infoSource,0) );
		return;
	}

	typedef file_info::field_name_comparator field_name_comparator_t;
	typedef pfc::comparator_stricmp_ascii value_comparator_t;

	typedef pfc::avltree_t< pfc::string8, field_name_comparator_t > field_mask_t;

	struct stats_t {
		size_t count = 0;
		double totalDuration = 0;
	};
	typedef pfc::map_t<pfc::string8,stats_t,value_comparator_t > field_results_t;
	typedef pfc::map_t<pfc::string8, field_results_t, field_name_comparator_t> results_t;
	results_t results;

	field_mask_t fieldsIgnore, fieldsUse;
	bool useDuration = true;
	double totalDuration = 0;
	
	for (size_t itemWalk = 0; itemWalk < itemCount; ++itemWalk) {
		auto info_ = getInfoHelper(infoSource, itemWalk);
		if (info_.is_empty()) continue;
		const file_info * infoptr = &info_->info();
		const size_t numInfo = infoptr->info_get_count();
		const double duration = infoptr->get_length();
		if (duration > 0) totalDuration += duration;
		else useDuration = false;
		for (size_t infoWalk = 0; infoWalk < numInfo; ++infoWalk) {
			const char * name = infoptr->info_enum_name(infoWalk);
			if ( fieldsIgnore.contains( name )) continue;
			if (!fieldsUse.contains(name)) {
				const bool bUse = !is_filtered_info_field( name );
				if ( bUse ) fieldsUse += name;
				else { fieldsIgnore += name; continue; }
			}

			const char * value = infoptr->info_enum_value(infoWalk);
			auto & stats = results[name][value];
			++ stats.count;
			if ( duration > 0 ) {
				stats.totalDuration += duration;
			}
		}
	}

	for (auto iter = results.first(); iter.is_valid(); ++iter) {
		const auto key = encloseInfoName(iter->m_key);
		pfc::string8 out;
		if (iter->m_value.get_count() == 1 && iter->m_value.first()->m_value.count == itemCount) {
			out = iter->m_value.first()->m_key;
		} else {
			for (auto iterValue = iter->m_value.first(); iterValue.is_valid(); ++iterValue) {
				double percentage;
				if ( useDuration ) percentage = iterValue->m_value.totalDuration / totalDuration;
				else percentage = (double) iterValue->m_value.count / (double) itemCount;
				if (!out.is_empty()) out << "; ";
				out << iterValue->m_key << " (" << pfc::format_fixedpoint( pfc::rint64( percentage * 1000.0), 1) << "%)";
			}
		}
		callback.set_property(strGroupOther, 0, key, out);
	}
}

template<typename source_t> static void enumerateTrackProperties_(track_property_callback_impl& callback, std::function< metadb_handle_list_cref() > itemsSource, source_t infoSource, std::function<abort_callback& () > abortSource) {
	if (core_api::is_main_thread()) {
		// should not get here like this
		// but that does make our job easier
		auto& items = itemsSource();
		auto info = infoSource();
		for (auto ptr : track_property_provider::enumerate()) {
			ptr->enumerate_properties_helper(items, info, callback, abortSource());
		}
		if (callback.is_group_wanted(strGroupOther)) {
			enumOther(callback, items, info);
		}
		return;
	}

	std::list<std::shared_ptr<pfc::event> > lstWaitFor;
	std::list<std::shared_ptr< track_property_callback_impl > > lstMerge;
	for (auto ptr : track_property_provider::enumerate()) {
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
			fb2k::inCpuWorkerThread(work);
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

	for (auto& i : lstWaitFor) {
		abortSource().waitForEvent(*i, -1);
	}
	for (auto& i : lstMerge) {
		callback.merge(*i);
	}
}

void enumerateTrackProperties(track_property_callback_impl& callback, std::function< metadb_handle_list_cref() > itemsSource, std::function<track_property_provider_v3_info_source* ()> infoSource, std::function<abort_callback& () > abortSource) {
	enumerateTrackProperties_(callback, itemsSource, infoSource, abortSource);
}

void enumerateTrackProperties_v5(track_property_callback_impl& callback, std::function< metadb_handle_list_cref() > itemsSource, std::function<track_property_provider_v5_info_source* ()> infoSource, std::function<abort_callback& () > abortSource) {
	enumerateTrackProperties_(callback, itemsSource, infoSource, abortSource);
}