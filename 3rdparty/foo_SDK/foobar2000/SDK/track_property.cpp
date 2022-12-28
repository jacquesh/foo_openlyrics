#include "foobar2000-sdk-pch.h"

#include "track_property.h"
#include "metadb_info_container_impl.h"

namespace {
	class track_property_provider_v3_info_source_impl : public track_property_provider_v3_info_source {
	public:
		track_property_provider_v3_info_source_impl(trackListRef items) : m_items(items) {}
		trackInfoContainer::ptr get_info(size_t index) { 
			return trackGetInfoRef(m_items, index);
		}
	private:
		trackListRef m_items;
	};

	class track_property_callback_v2_proxy : public track_property_callback_v2 {
	public:
		track_property_callback_v2_proxy(track_property_callback & callback) : m_callback(callback) {}
		void set_property(const char * p_group, double p_sortpriority, const char * p_name, const char * p_value) { m_callback.set_property(p_group, p_sortpriority, p_name, p_value); }
		bool is_group_wanted(const char*) { return true; }

	private:
		track_property_callback & m_callback;
	};


}

void track_property_provider_v3::enumerate_properties(trackListRef p_tracks, track_property_callback & p_out) { 
	track_property_provider_v3_info_source_impl src(p_tracks); track_property_callback_v2_proxy cb(p_out); enumerate_properties_v3(p_tracks, src, cb); 
}

void track_property_provider_v3::enumerate_properties_v2(trackListRef p_tracks, track_property_callback_v2 & p_out) {
	track_property_provider_v3_info_source_impl src(p_tracks); enumerate_properties_v3(p_tracks, src, p_out); 
}

void track_property_provider_v4::enumerate_properties_v3(trackListRef items, track_property_provider_v3_info_source & info, track_property_callback_v2 & callback) {
	this->enumerate_properties_v4(items, info, callback, fb2k::noAbort );
}
namespace {
	class track_property_provider_v5_info_source_wrap : public track_property_provider_v5_info_source {
	public:
		track_property_provider_v5_info_source_wrap(track_property_provider_v3_info_source& chain, trackListRef items) : m_chain(chain), m_items(items) {}
		track_property_provider_v3_info_source & m_chain;
		trackListRef m_items;
		
		// Not very efficient but gets the job done
		metadb_v2_rec_t get_info(size_t index) override {
			auto ret = m_items[index]->query_v2_();
			ret.info = m_chain.get_info(index);
			return ret;
		}
	};
}
void track_property_provider_v5::enumerate_properties_v4(trackListRef items, track_property_provider_v3_info_source& info, track_property_callback_v2& callback, abort_callback& abort) {
	track_property_provider_v5_info_source_wrap wrap(info, items);
	this->enumerate_properties_v5(items, wrap, callback, abort);
}

namespace {
	class track_property_provider_v3_info_source_wrap_v5 : public track_property_provider_v3_info_source {
	public:
		track_property_provider_v5_info_source* chain;
		trackInfoContainer::ptr get_info(size_t index) override {
			auto ret = chain->get_info(index).info;
			if (ret.is_empty()) {
				ret = m_blank;
			}
			return ret;
		}
		metadb_info_container::ptr m_blank = fb2k::service_new<metadb_info_container_const_impl>();
	};
}
metadb_v2_rec_t track_property_provider_v5_info_source_impl::get_info(size_t index) {
#if FOOBAR2000_TARGET_VERSION >= 81
	return m_infos[index];
#else
	if (index < m_infos.get_size()) return m_infos[index];
	else if (index < m_tracks.get_size()) return m_tracks[index]->query_v2_();
	else uBugCheck();
#endif
}

track_property_provider_v5_info_source_impl::track_property_provider_v5_info_source_impl(trackListRef items, abort_callback & a) {
	
#if FOOBAR2000_TARGET_VERSION >= 81
	m_infos.resize(items.get_count()); 
	metadb_v2::get()->queryMultiParallel_(items, [this, &a](size_t idx, metadb_v2_rec_t const& rec) {a.check(); this->m_infos[idx] = rec; });
#else
	auto api = metadb_v2::tryGet();
	if (api.is_valid()) {
		m_infos.resize(items.get_count());
		api->queryMultiParallel_(items, [this](size_t idx, metadb_v2_rec_t const& rec) {this->m_infos[idx] = rec; });
		return;
	}
	// pre-2.0 metadb, talking to metadb directly is OK, no need to get info preemptively
	m_tracks = items;
#endif
}

void track_property_provider::enumerate_properties_helper(trackListRef items, track_property_provider_v5_info_source* info, track_property_callback_v2& callback, abort_callback& abort) {

	if (info == nullptr) {
		this->enumerate_properties_helper(items, nullptr, callback, abort);
	} else {
		{
			track_property_provider_v5::ptr v5;
			if (v5 &= this) {
				v5->enumerate_properties_v5(items, *info, callback, abort);
				return;
			}
		}

		track_property_provider_v3_info_source_wrap_v5 wrap;
		wrap.chain = info;
		this->enumerate_properties_helper(items, &wrap, callback, abort);
	}
}

void track_property_provider::enumerate_properties_helper(trackListRef items, std::nullptr_t, track_property_callback_v2& callback, abort_callback& abort) {
	track_property_provider_v5::ptr v5;
	if (v5 &= this) {
		track_property_provider_v5_info_source_impl infoImpl(items, abort);
		v5->enumerate_properties_v5(items, infoImpl, callback, abort);
		return;
	}
	track_property_provider_v3_info_source* dummy = nullptr;
	this->enumerate_properties_helper(items, dummy, callback, abort);
}

void track_property_provider::enumerate_properties_helper(trackListRef items, track_property_provider_v3_info_source * info, track_property_callback_v2 & callback, abort_callback & abort) {

	abort.check();

	track_property_provider_v2::ptr v2;
	if ( ! this->cast( v2 ) ) {
		// no v2
		PFC_ASSERT(core_api::is_main_thread());
		this->enumerate_properties( items, callback ); return;
	}

	track_property_provider_v3::ptr v3;
	if ( ! (v3 &= v2 ) ) {
		// no v3
		PFC_ASSERT(core_api::is_main_thread());
		v2->enumerate_properties_v2( items, callback ); return;
	}

	track_property_provider_v3_info_source_impl infoFallback ( items );
	if ( info == nullptr ) info = & infoFallback;

	track_property_provider_v4::ptr v4;
	if (! ( v4 &= v3 ) ) {
		// no v4
		PFC_ASSERT( core_api::is_main_thread() );
		v3->enumerate_properties_v3( items, *info, callback );
	} else {
		v4->enumerate_properties_v4( items, *info, callback, abort );
	}
}
