#pragma once

#include "tracks.h"

//! Callback interface for track_property_provider::enumerate_properties().
class NOVTABLE track_property_callback {
public:
	//! Sets a property list entry to display. Called by track_property_provider::enumerate_properties() implementation.
	//! @param p_group Name of group to put the entry in, case-sensitive. Note that non-standard groups are sorted alphabetically.
	//! @param p_sortpriority Sort priority of the property inside its group (smaller value means earlier in the list), pass 0 if you don't care (alphabetic order by name used when more than one item has same priority).
	//! @param p_name Name of the property.
	//! @param p_value Value of the property.
	virtual void set_property(const char * p_group,double p_sortpriority,const char * p_name,const char * p_value) = 0;
protected:
	track_property_callback() {}
	~track_property_callback() {}
	track_property_callback(track_property_callback const &) {};
	void operator=(track_property_callback const &) {};
};

//! Extended version of track_property_callback
class NOVTABLE track_property_callback_v2 : public track_property_callback {
public:
	//! Returns a boolean value indicating whether the specified group is wanted; can be used to suppress expensive processing of information that will not be actually shown. \n
	//! See also set_property() p_group parameter.
	virtual bool is_group_wanted(const char * p_group) = 0;
protected:
	~track_property_callback_v2() {}
};

//! \since 1.3
class NOVTABLE track_property_provider_v3_info_source {
public:
	virtual trackInfoContainer::ptr get_info(size_t index) = 0;

protected:
	track_property_provider_v3_info_source() {}
	~track_property_provider_v3_info_source() {}
	track_property_provider_v3_info_source( const track_property_provider_v3_info_source & ) {};
	void operator=( const track_property_provider_v3_info_source & ) {};
};

//! Service for adding custom entries in "Properties" tab of the properties dialog.
class NOVTABLE track_property_provider : public service_base {
public:
	//! Enumerates properties of specified track list.
	//! @param p_tracks List of tracks to enumerate properties on.
	//! @param p_out Callback interface receiving enumerated properties.
	virtual void enumerate_properties(trackListRef p_tracks, track_property_callback & p_out) = 0;
	//! Returns whether specified tech info filed is processed by our service and should not be displayed among unknown fields.
	//! @param p_name Name of tech info field being queried.
	//! @returns True if the field is among fields processed by this track_property_provider implementation and should not be displayed among unknown fields, false otherwise.
	virtual bool is_our_tech_info(const char * p_name) = 0;


	//! Helper; calls modern versions of this API where appropriate.
	//! @param items List of tracks to enumerate properties on.
	//! @param info Callback object to fetch info from. Pass null to use a generic implementation querying the metadb.
	//! @param callback Callback interface receiving enumerated properties.
	//! @param abort The aborter for this operation.
	void enumerate_properties_helper(trackListRef items, track_property_provider_v3_info_source * info, track_property_callback_v2 & callback, abort_callback & abort);

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(track_property_provider);
};

class NOVTABLE track_property_provider_v2 : public track_property_provider {
	FB2K_MAKE_SERVICE_INTERFACE(track_property_provider_v2,track_property_provider)
public:
	virtual void enumerate_properties_v2(trackListRef p_tracks, track_property_callback_v2 & p_out) = 0;
};


//! \since 1.3
class NOVTABLE track_property_provider_v3 : public track_property_provider_v2 {
	FB2K_MAKE_SERVICE_INTERFACE(track_property_provider_v3,track_property_provider_v2)
public:
	virtual void enumerate_properties_v3(trackListRef items, track_property_provider_v3_info_source & info, track_property_callback_v2 & callback) = 0;

	void enumerate_properties(trackListRef p_tracks, track_property_callback & p_out) override;
	void enumerate_properties_v2(trackListRef p_tracks, track_property_callback_v2 & p_out) override;
};

template<typename T>
class track_property_provider_factory_t : public service_factory_single_t<T> {};

//! \since 1.5
//! Adds abortability on top of track_property_provider_v3 interface. \n
//! The previous revisions of this API were only legal to call from the main thread. \n
//! track_property_provider_v4 implementers should make NO ASSUMPTIONS about the thread they are in. \n
//! Implementing track_property_provider_v4 declares your class as safe to call from any thread. \n
//! If called via enumerate_properties_v4() method or off-main-thread, the implementation can assume the info source object to be thread-safe.
class NOVTABLE track_property_provider_v4 : public track_property_provider_v3 {
	FB2K_MAKE_SERVICE_INTERFACE(track_property_provider_v4, track_property_provider_v3 );
public:
	void enumerate_properties_v3(trackListRef items, track_property_provider_v3_info_source & info, track_property_callback_v2 & callback) override;

	virtual void enumerate_properties_v4(trackListRef items, track_property_provider_v3_info_source & info, track_property_callback_v2 & callback, abort_callback & abort) = 0;
};
