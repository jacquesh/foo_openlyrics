#include "foobar2000.h"

#include "track_property.h"

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
