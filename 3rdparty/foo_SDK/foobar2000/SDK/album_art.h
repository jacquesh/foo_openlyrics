#pragma once

#include <functional>

//! Common class for handling picture data. \n
//! Type of contained picture data is unknown and to be determined according to memory block contents by code parsing/rendering the picture. Commonly encountered types are: BMP, PNG, JPEG and GIF. \n
//! Implementation: use album_art_data_impl.
class NOVTABLE album_art_data : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(album_art_data, service_base);
public:
	//! Retrieves a pointer to a memory block containing the picture.
	virtual const void * get_ptr() const = 0;
	//! Retrieves size of the memory block containing the picture.
	virtual t_size get_size() const = 0;

	//! New code compat
	size_t size() const { return get_size(); }
	const void * data() const { return get_ptr(); }

	//! Determine whether two album_art_data objects store the same picture data.
	static bool equals(album_art_data const & v1, album_art_data const & v2) {
		const t_size s = v1.get_size();
		if (s != v2.get_size()) return false;
		return memcmp(v1.get_ptr(), v2.get_ptr(),s) == 0;
	}
	static bool equals(ptr const& v1, ptr const& v2) {
		if (v1.is_valid() != v2.is_valid()) return false;
		if (v1.is_empty() && v2.is_empty()) return true;
		return equals(*v1, *v2);
	}

	bool operator==(const album_art_data & other) const {return equals(*this,other);}
	bool operator!=(const album_art_data & other) const {return !equals(*this,other);}
};

typedef service_ptr_t<album_art_data> album_art_data_ptr;
namespace fb2k {
	typedef album_art_data_ptr memBlockRef;
}

//! Namespace containing identifiers of album art types.
namespace album_art_ids {
	//! Front cover.
	static const GUID cover_front = { 0xf1e66f4e, 0xfe09, 0x4b94, { 0x91, 0xa3, 0x67, 0xc2, 0x3e, 0xd1, 0x44, 0x5e } };
	//! Back cover.
	static const GUID cover_back = { 0xcb552d19, 0x86d5, 0x434c, { 0xac, 0x77, 0xbb, 0x24, 0xed, 0x56, 0x7e, 0xe4 } };
	//! Picture of a disc or other storage media.
	static const GUID disc = { 0x3dba9f36, 0xf928, 0x4fa4, { 0x87, 0x9c, 0xd3, 0x40, 0x47, 0x59, 0x58, 0x7e } };
	//! Album-specific icon (NOT a file type icon).
	static const GUID icon = { 0x74cdf5b4, 0x7053, 0x4b3d, { 0x9a, 0x3c, 0x54, 0x69, 0xf5, 0x82, 0x6e, 0xec } };
	//! Artist picture.
	static const GUID artist = { 0x9a654042, 0xacd1, 0x43f7, { 0xbf, 0xcf, 0xd3, 0xec, 0xf, 0xfe, 0x40, 0xfa } };

	size_t num_types();
	GUID query_type( size_t );
	// returns lowercase name
	const char * query_name( size_t );
	const char * name_of( const GUID & );
	// returns Capitalized name
	const char * query_capitalized_name( size_t );
	const char * capitalized_name_of( const GUID & );
};

PFC_DECLARE_EXCEPTION(exception_album_art_not_found,exception_io_not_found,"Attached picture not found");
PFC_DECLARE_EXCEPTION(exception_album_art_unsupported_entry,exception_io_data,"Unsupported attached picture entry");

PFC_DECLARE_EXCEPTION(exception_album_art_unsupported_format,exception_io_data,"Attached picture operations not supported for this file format");

//! Class encapsulating access to album art stored in a media file. Use album_art_extractor class obtain album_art_extractor_instance referring to specified media file.
class NOVTABLE album_art_extractor_instance : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(album_art_extractor_instance,service_base);
public:
	//! Throws exception_album_art_not_found when the requested album art entry could not be found in the referenced media file.
	virtual album_art_data_ptr query(const GUID & p_what,abort_callback & p_abort) = 0;

	bool have_entry( const GUID & what, abort_callback & abort );
	bool query(const GUID & what, album_art_data::ptr & out, abort_callback & abort);
};

//! Class encapsulating access to album art stored in a media file. Use album_art_editor class to obtain album_art_editor_instance referring to specified media file.
class NOVTABLE album_art_editor_instance : public album_art_extractor_instance {
	FB2K_MAKE_SERVICE_INTERFACE(album_art_editor_instance,album_art_extractor_instance);
public:
	//! Throws exception_album_art_unsupported_entry when the file format we're dealing with does not support specific entry.
	virtual void set(const GUID & p_what,album_art_data_ptr p_data,abort_callback & p_abort) = 0;

	//! Removes the requested entry. Fails silently when the entry doesn't exist.
	virtual void remove(const GUID & p_what) = 0;

	//! Finalizes file tag update operation.
	virtual void commit(abort_callback & p_abort) = 0;

	//! Helper; see album_art_editor_instance_v2::remove_all();
	void remove_all_();
};

class NOVTABLE album_art_editor_instance_v2 : public album_art_editor_instance {
	FB2K_MAKE_SERVICE_INTERFACE(album_art_editor_instance_v2, album_art_editor_instance);
public:
	//! Tells the editor to remove all entries, including unsupported picture types that do not translate to fb2k ids.
	virtual void remove_all() = 0;
};

typedef service_ptr_t<album_art_extractor_instance> album_art_extractor_instance_ptr;
typedef service_ptr_t<album_art_editor_instance> album_art_editor_instance_ptr;

//! Entrypoint class for accessing album art extraction functionality. Register your own implementation to allow album art extraction from your media file format. \n
//! If you want to extract album art from a media file, it's recommended that you use album_art_manager API instead of calling album_art_extractor directly.
class NOVTABLE album_art_extractor : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(album_art_extractor);
public:
	//! Returns whether the specified file is one of formats supported by our album_art_extractor implementation.
	//! @param p_path Path to file being queried.
	//! @param p_extension Extension of file being queried (also present in p_path parameter) - provided as a separate parameter for performance reasons.
	virtual bool is_our_path(const char * p_path,const char * p_extension) = 0;
	
	//! Instantiates album_art_extractor_instance providing access to album art stored in a specified media file. \n
	//! Throws one of I/O exceptions on failure; exception_album_art_not_found when the file has no album art record at all.
	//! @param p_filehint Optional; specifies a file interface to use for accessing the specified file; can be null - in that case, the implementation will open and close the file internally.
	virtual album_art_extractor_instance_ptr open(file_ptr p_filehint,const char * p_path,abort_callback & p_abort) = 0;

	static bool g_get_interface(service_ptr_t<album_art_extractor> & out,const char * path);
	static bool g_is_supported_path(const char * path);
	static album_art_extractor_instance_ptr g_open(file_ptr p_filehint,const char * p_path,abort_callback & p_abort);
	static album_art_extractor_instance_ptr g_open_allowempty(file_ptr p_filehint,const char * p_path,abort_callback & p_abort);

	//! Returns GUID of the corresponding input class. Null GUID if none.
	GUID get_guid(); 
};

//! \since 1.5
class NOVTABLE album_art_extractor_v2 : public album_art_extractor {
	FB2K_MAKE_SERVICE_INTERFACE(album_art_extractor_v2 , album_art_extractor);
public:
	//! Returns GUID of the corresponding input class. Null GUID if none.
	virtual GUID get_guid() = 0;
};

//! Entrypoint class for accessing album art editing functionality. Register your own implementation to allow album art editing on your media file format.
class NOVTABLE album_art_editor : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(album_art_editor);
public:
	//! Returns whether the specified file is one of formats supported by our album_art_editor implementation.
	//! @param p_path Path to file being queried.
	//! @param p_extension Extension of file being queried (also present in p_path parameter) - provided as a separate parameter for performance reasons.
	virtual bool is_our_path(const char * p_path,const char * p_extension) = 0;

	//! Instantiates album_art_editor_instance providing access to album art stored in a specified media file. \n
	//! @param p_filehint Optional; specifies a file interface to use for accessing the specified file; can be null - in that case, the implementation will open and close the file internally.
	virtual album_art_editor_instance_ptr open(file_ptr p_filehint,const char * p_path,abort_callback & p_abort) = 0;

	//! Helper; attempts to retrieve an album_art_editor service pointer that supports the specified file.
	//! @returns True on success, false on failure (no registered album_art_editor supports this file type).
	static bool g_get_interface(service_ptr_t<album_art_editor> & out,const char * path);
	//! Helper; returns whether one of registered album_art_editor implementations is capable of opening the specified file.
	static bool g_is_supported_path(const char * path);

	static album_art_editor_instance_ptr g_open(file_ptr p_filehint,const char * p_path,abort_callback & p_abort);

	//! Returns GUID of the corresponding input class. Null GUID if none.
	GUID get_guid();
};

//! \since 1.5
class NOVTABLE album_art_editor_v2 : public album_art_editor {
	FB2K_MAKE_SERVICE_INTERFACE( album_art_editor_v2, album_art_editor )
public:
	//! Returns GUID of the corresponding input class. Null GUID if none.
	virtual GUID get_guid() = 0;
};

//! \since 0.9.5
//! Helper API for extracting album art from APEv2 tags.
class NOVTABLE tag_processor_album_art_utils : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(tag_processor_album_art_utils)
public:

	//! Throws one of I/O exceptions on failure; exception_album_art_not_found when the file has no album art record at all.
	virtual album_art_extractor_instance_ptr open(file_ptr p_file,abort_callback & p_abort) = 0;

	//! \since 1.1.6
	//! Throws exception_not_implemented on earlier than 1.1.6.
	virtual album_art_editor_instance_ptr edit(file_ptr p_file,abort_callback & p_abort) = 0;
};


//! Album art path list - see album_art_extractor_instance_v2
class NOVTABLE album_art_path_list : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(album_art_path_list, service_base)
public:
	virtual const char * get_path(t_size index) const = 0;
	virtual t_size get_count() const = 0;

	static bool equals(album_art_path_list const& v1, album_art_path_list const& v2);
	static bool equals(ptr const& v1, ptr const& v2);
};

//! album_art_extractor_instance extension; lets the frontend query referenced file paths (eg. when using external album art).
class NOVTABLE album_art_extractor_instance_v2 : public album_art_extractor_instance {
	FB2K_MAKE_SERVICE_INTERFACE(album_art_extractor_instance_v2, album_art_extractor_instance)
public:
	virtual album_art_path_list::ptr query_paths(const GUID & p_what, abort_callback & p_abort) = 0;
};


//! \since 1.0
//! Provides methods for interfacing with the foobar2000 core album art loader. \n
//! Use this when you need to load album art for a specific group of tracks.
class NOVTABLE album_art_manager_v2 : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(album_art_manager_v2)
public:
	//! Instantiates an album art extractor object for the specified group of items.
	virtual album_art_extractor_instance_v2::ptr open(metadb_handle_list_cref items, pfc::list_base_const_t<GUID> const & ids, abort_callback & abort) = 0;
	
	//! Instantiates an album art extractor object that retrieves stub images.
	virtual album_art_extractor_instance_v2::ptr open_stub(abort_callback & abort) = 0;
};


//! \since 1.0
//! Called when no other album art source (internal, external, other registered fallbacks) returns relevant data for the specified items. \n
//! Can be used to implement online lookup and such.
class NOVTABLE album_art_fallback : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(album_art_fallback)
public:
	virtual album_art_extractor_instance_v2::ptr open(metadb_handle_list_cref items, pfc::list_base_const_t<GUID> const & ids, abort_callback & abort) = 0;
};

//! \since 1.1.7
class NOVTABLE album_art_manager_config : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(album_art_manager_config, service_base)
public:
	virtual bool get_external_pattern(pfc::string_base & out, const GUID & type) = 0;
	virtual bool use_embedded_pictures() = 0;
	virtual bool use_fallbacks() = 0;
};

//! \since 1.1.7
class NOVTABLE album_art_manager_v3 : public album_art_manager_v2 {
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(album_art_manager_v3, album_art_manager_v2)
public:
	//! @param config An optional album_art_manager_config object to override global settings. Pass null to use global settings.
	virtual album_art_extractor_instance_v2::ptr open_v3(metadb_handle_list_cref items, pfc::list_base_const_t<GUID> const & ids, album_art_manager_config::ptr config, abort_callback & abort) = 0;
};

//! \since 1.4
//! A notification about a newly loaded album art being ready to display. \n
//! See: now_playing_album_art_notify_manager.
class NOVTABLE now_playing_album_art_notify {
public:
	//! Called when album art has finished loading for the now playing track.
	//! @param data The newly loaded album art. Never a null object - the callbacks are simply not called when there is nothing to show.
	virtual void on_album_art( album_art_data::ptr data ) = 0;
};

//! \since 1.4
//! Since various components require the album art of the now-playing track, a centralized loader has been provided, so the file isn't hammered independently by different components. \n
//! Use this in conjunction with play_callback notifications to render now-playing track information.
class NOVTABLE now_playing_album_art_notify_manager : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(now_playing_album_art_notify_manager)
public:
	//! Register a notification to be told when the album art has been loaded.
	virtual void add(now_playing_album_art_notify*) = 0;
	//! Unregister a previously registered notification.
	virtual void remove(now_playing_album_art_notify*) = 0;
	//! Retrieves the album art for the currently playing track.
	//! @returns The current album art (front cover), or null if there is no art or the art is being loaded and is not yet available.
	virtual album_art_data::ptr current() = 0;

	//! Helper; register a lambda notification. Pass the returned obejct to remove() to unregister.
	now_playing_album_art_notify* add( std::function<void (album_art_data::ptr) > );
};

//! \since 1.6.6
class NOVTABLE now_playing_album_art_notify_manager_v2 : public now_playing_album_art_notify_manager {
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(now_playing_album_art_notify_manager_v2, now_playing_album_art_notify_manager);
public:
	struct info_t {
		album_art_data::ptr data;
		album_art_path_list::ptr paths;

		static bool equals(const info_t& v1, const info_t& v2) {
			return album_art_data::equals(v1.data, v2.data) && album_art_path_list::equals(v1.paths, v2.paths);
		}
		bool operator==(const info_t& other) const { return equals(*this, other); }
		bool operator!=(const info_t& other) const { return !equals(*this, other); }

		void clear() { *this = {}; }
		bool is_valid() const { return data.is_valid(); }
		operator bool() const { return is_valid(); }
	};
	virtual info_t current_v2() = 0;
};