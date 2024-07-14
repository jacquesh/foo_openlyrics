#pragma once

#include <functional>
#include "metadb_handle.h"

class file_info_filter; // forward decl; file_info_filter moved to file_info_filter.h

class metadb_io_callback_dynamic; class metadb_io_callback_v2_dynamic;


//! API for tag read/write operations. Legal to call from main thread only, except for hint_multi_async() / hint_async() / hint_reader().\n
//! Implemented only by core, do not reimplement.\n
//! Use static_api_ptr_t template to access metadb_io methods.\n
//! WARNING: Methods that perform file access (tag reads/writes) run a modal message loop. They SHOULD NOT be called from global callbacks and such.
class NOVTABLE metadb_io : public service_base
{
public:
	enum t_load_info_type {
		load_info_default,
		load_info_force,
		load_info_check_if_changed
	};

	enum t_update_info_state {
		update_info_success,
		update_info_aborted,
		update_info_errors,		
	};
	
	enum t_load_info_state {
		load_info_success,
		load_info_aborted,
		load_info_errors,		
	};

	//! No longer used - returns false always.
    FB2K_DEPRECATED virtual bool is_busy() = 0;
	//! No longer used - returns false always.
    FB2K_DEPRECATED virtual bool is_updating_disabled() = 0;
	//! No longer used - returns false always.
    FB2K_DEPRECATED virtual bool is_file_updating_blocked() = 0;
	//! No longer used.
    FB2K_DEPRECATED virtual void highlight_running_process() = 0;
	//! Loads tags from multiple items. Use the async version in metadb_io_v2 instead if possible.
    FB2K_DEPRECATED virtual t_load_info_state load_info_multi(metadb_handle_list_cref p_list,t_load_info_type p_type,fb2k::hwnd_t p_parent_window,bool p_show_errors) = 0;
	//! Updates tags on multiple items. Use the async version in metadb_io_v2 instead if possible.
    FB2K_DEPRECATED virtual t_update_info_state update_info_multi(metadb_handle_list_cref p_list,const pfc::list_base_const_t<file_info*> & p_new_info,fb2k::hwnd_t p_parent_window,bool p_show_errors) = 0;
	//! Rewrites tags on multiple items. Use the async version in metadb_io_v2 instead if possible.
    FB2K_DEPRECATED virtual t_update_info_state rewrite_info_multi(metadb_handle_list_cref p_list,fb2k::hwnd_t p_parent_window,bool p_show_errors) = 0;
	//! Removes tags from multiple items. Use the async version in metadb_io_v2 instead if possible.
    FB2K_DEPRECATED virtual t_update_info_state remove_info_multi(metadb_handle_list_cref p_list,fb2k::hwnd_t p_parent_window,bool p_show_errors) = 0;

	virtual void hint_multi(metadb_handle_list_cref p_list,const pfc::list_base_const_t<const file_info*> & p_infos,const pfc::list_base_const_t<t_filestats> & p_stats,const bit_array & p_fresh_mask) = 0;

	virtual void hint_multi_async(metadb_handle_list_cref p_list,const pfc::list_base_const_t<const file_info*> & p_infos,const pfc::list_base_const_t<t_filestats> & p_stats,const bit_array & p_fresh_mask) = 0;

	virtual void hint_reader(service_ptr_t<class input_info_reader> p_reader,const char * p_path,abort_callback & p_abort) = 0;

	//! For internal use only.
	virtual void path_to_handles_simple(const char * p_path, metadb_handle_list_ref p_out) = 0;

	//! Dispatches metadb_io_callback calls with specified items. To be used with metadb_display_field_provider when your component needs specified items refreshed.
	virtual void dispatch_refresh(metadb_handle_list_cref p_list) = 0;

	void dispatch_refresh(metadb_handle_ptr const & handle) {dispatch_refresh(pfc::list_single_ref_t<metadb_handle_ptr>(handle));}

	void hint_async(metadb_handle_ptr p_item,const file_info & p_info,const t_filestats & p_stats,bool p_fresh);

    FB2K_DEPRECATED t_load_info_state load_info(metadb_handle_ptr p_item,t_load_info_type p_type,fb2k::hwnd_t p_parent_window,bool p_show_errors);
    FB2K_DEPRECATED t_update_info_state update_info(metadb_handle_ptr p_item,file_info & p_info,fb2k::hwnd_t p_parent_window,bool p_show_errors);
	
	FB2K_MAKE_SERVICE_COREAPI(metadb_io);
};

//! Advanced interface for passing infos read from files to metadb backend. Use metadb_io_v2::create_hint_list() to instantiate. \n
//! Thread safety: all methods other than on_done() are intended for worker threads. Instantiate and use the object in a worker thread, call on_done() in main thread to finalize. \n
//! Typical usage pattern: create a hint list (in any thread), hand infos to it from files that you work with (in a worker thread), call on_done() in main thread. \n
class NOVTABLE metadb_hint_list : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(metadb_hint_list,service_base);
public:
	//! Helper.
	static metadb_hint_list::ptr create();
	//! Adds a hint to the list.
	//! @param p_location Location of the item the hint applies to.
	//! @param p_info file_info object describing the item.
	//! @param p_stats Information about the file containing item the hint applies to.
	//! @param p_freshflag Set to true if the info has been directly read from the file, false if it comes from another source such as a playlist file.
	virtual void add_hint(metadb_handle_ptr const & p_location,const file_info & p_info,const t_filestats & p_stats,bool p_freshflag) = 0;
	//! Reads info from specified info reader instance and adds hints. May throw an exception in case info read has failed. \n
	//! If the file has multiple subsongs, info from all the subsongs will be read and pssed to add_hint(). \n
	//! Note that an input_info_writer is a subclass of input_info_reader - so any input_info_reader OR input_info_writer is a valid argument for add_hint_reader(). \n
	//! This method is often called with your input_info_writer instance after committing tag updates, to notify metadb about altered tags.
	virtual void add_hint_reader(const char * p_path,service_ptr_t<input_info_reader> const & p_reader,abort_callback & p_abort) = 0;
	//! Call this when you're done working with this metadb_hint_list instance, to apply hints and dispatch callbacks. \n
	//! If you don't call this, all added hints will be ignored. \n
	//! As a general rule, you should add as many infos as possible - such as all the tracks involved in some operation that you perform - then call on_done() once. \n
	//! on_done() is expensive because it not only updates the metadb, but tells all components about the changes made - refreshes playlists/autoplaylists, library viewers, etc. \n
	//! Calling on_done() repeatedly is inefficient and should be avoided.
	virtual void on_done() = 0;
};

//! \since 1.0
//! To obtain metadb_hint_list_v2, use service_query on a metadb_hint_list object. \n
//! Simplified: metadb_hint_list_v2::ptr v2; v2 ^= myHintList;  ( causes bugcheck if old fb2k / no interface ).
class NOVTABLE metadb_hint_list_v2 : public metadb_hint_list {
	FB2K_MAKE_SERVICE_INTERFACE(metadb_hint_list_v2, metadb_hint_list);
public:
	//! Helper.
	static metadb_hint_list_v2::ptr create();
	//! Hint with browse info. \n
	//! See: metadb_handle::get_browse_info() for browse info rationale.
	//! @param p_location Location for which we're providing browse info.
	//! @param p_info Browse info for this location.
	//! @param browseTS timestamp of the browse info - such as last-modified time of the playlist file providing browse info.
	virtual void add_hint_browse(metadb_handle_ptr const & p_location,const file_info & p_info, t_filetimestamp browseTS) = 0;
};

//! \since 1.3
//! To obtain metadb_hint_list_v3, use service_query on a metadb_hint_list object. \n
//! Simplified: metadb_hint_list_v3::ptr v3; v3 ^= myHintList;  ( causes bugcheck if old fb2k / no interface ).
class NOVTABLE metadb_hint_list_v3 : public metadb_hint_list_v2 {
	FB2K_MAKE_SERVICE_INTERFACE(metadb_hint_list_v3, metadb_hint_list_v2);
public:
	//! Helper.
	static metadb_hint_list_v3::ptr create();
	//! Hint primary info with a metadb_info_container.
	virtual void add_hint_v3(metadb_handle_ptr const & p_location, metadb_info_container::ptr info,bool p_freshflag) = 0;
	//! Hint browse info with a metadb_info_container.
	virtual void add_hint_browse_v3(metadb_handle_ptr const & p_location,metadb_info_container::ptr info) = 0;

	//! Add a forced hint.\n
	//! A normal hint may or may not cause metadb update - metadb is not updated if the file has not changed according to last modified time. \n
	//! A forced hint always updates metadb regardless of timestamps.
	virtual void add_hint_forced(metadb_handle_ptr const & p_location, const file_info & p_info,const t_filestats & p_stats,bool p_freshflag) = 0;
	//! Add a forced hint, with metadb_info_container. \n
	//! Forced hint rationale - see add_hint_forced().
	virtual void add_hint_forced_v3(metadb_handle_ptr const & p_location, metadb_info_container::ptr info,bool p_freshflag) = 0;
	//! Adds a forced hint, with an input_info_reader. \n
	//! Forced hint rationale - see add_hint_forced(). \n
	//! Info reader use rationale - see add_hint_reader(). 
	virtual void add_hint_forced_reader(const char * p_path,service_ptr_t<input_info_reader> const & p_reader,abort_callback & p_abort) = 0;
};

//! \since 2.0
//! Allows dispatching of metadb_io_edit_callback from your code.
class NOVTABLE metadb_hint_list_v4 : public metadb_hint_list_v3 {
	FB2K_MAKE_SERVICE_INTERFACE( metadb_hint_list_v4, metadb_hint_list_v3 );
public:
	static metadb_hint_list_v4::ptr create();

	virtual void before_edit( const char * path, service_ptr_t<input_info_reader> reader, abort_callback & a ) = 0;
	virtual void after_edit( const char * path, service_ptr_t<input_info_reader> reader, abort_callback & a ) = 0;

};


//! New in 0.9.3. Extends metadb_io functionality with nonblocking versions of tag read/write functions, and some other utility features.
class NOVTABLE metadb_io_v2 : public metadb_io {
public:
	enum {
		//! By default, when some part of requested operation could not be performed for reasons other than user abort, a popup dialog with description of the problem is spawned.\n
		//! Set this flag to disable error notification.
		op_flag_no_errors		= 1 << 0,
		//! Set this flag to make the progress dialog not steal focus on creation.
		op_flag_background		= 1 << 1,
		//! Set this flag to delay the progress dialog becoming visible, so it does not appear at all during short operations. Also implies op_flag_background effect.
		op_flag_delay_ui		= 1 << 2,

		//! \since 1.3
		//! Indicates that the caller is aware of the metadb partial info feature introduced at v1.3.
		//! When not specified, affected info will be quietly preserved when updating tags.
		//! Obsolete in 2.0
		op_flag_partial_info_aware = 1 << 3,

		//! \since 2.0
		//! Do not show any user interface.
		op_flag_silent		= 1 << 4,
        
        //! \since 2.0
        op_flag_detect_rechapter = 1 << 5,
	};

	//! Preloads information from the specified tracks. \n
	//! Use from main thread only (starts a threaded_process to show a progress dialog).
	//! @param p_list List of items to process.
	//! @param p_op_flags Can be null, or one or more of op_flag_* enum values combined, altering behaviors of the operation.
	//! @param p_notify Called when the task is completed. Status code is one of t_load_info_state values. Can be null if caller doesn't care.
	virtual void load_info_async(metadb_handle_list_cref p_list,t_load_info_type p_type,fb2k::hwnd_t p_parent_window,t_uint32 p_op_flags,completion_notify_ptr p_notify) = 0;
	//! Updates tags of the specified tracks. \n
	//! Use from main thread only (starts a threaded_process to show a progress dialog).
	//! @param p_list List of items to process.
	//! @param p_op_flags Can be null, or one or more of op_flag_* enum values combined, altering behaviors of the operation.
	//! @param p_notify Called when the task is completed. Status code is one of t_update_info values. Can be null if caller doesn't care.
	//! @param p_filter Callback handling actual file_info alterations. Typically used to replace entire meta part of file_info, or to alter something else such as ReplayGain while leaving meta intact.
	virtual void update_info_async(metadb_handle_list_cref p_list,service_ptr_t<file_info_filter> p_filter,fb2k::hwnd_t p_parent_window,t_uint32 p_op_flags,completion_notify_ptr p_notify) = 0;
	//! Rewrites tags of the specified tracks; similar to update_info_async() but using last known/cached file_info values rather than values passed by caller. \n
	//! Use from main thread only (starts a threaded_process to show a progress dialog).
	//! @param p_list List of items to process.
	//! @param p_op_flags Can be null, or one or more of op_flag_* enum values combined, altering behaviors of the operation.
	//! @param p_notify Called when the task is completed. Status code is one of t_update_info values. Can be null if caller doesn't care.
	virtual void rewrite_info_async(metadb_handle_list_cref p_list,fb2k::hwnd_t p_parent_window,t_uint32 p_op_flags,completion_notify_ptr p_notify) = 0;
	//! Strips all tags / metadata fields from the specified tracks. \n
	//! Use from main thread only (starts a threaded_process to show a progress dialog).
	//! @param p_list List of items to process.
	//! @param p_op_flags Can be null, or one or more of op_flag_* enum values combined, altering behaviors of the operation.
	//! @param p_notify Called when the task is completed. Status code is one of t_update_info values. Can be null if caller doesn't care.
	virtual void remove_info_async(metadb_handle_list_cref p_list,fb2k::hwnd_t p_parent_window,t_uint32 p_op_flags,completion_notify_ptr p_notify) = 0;

	//! Creates a metadb_hint_list object. \n
	//! Contrary to other metadb_io methods, this can be safely called in a worker thread. You only need to call the hint list's on_done() method in main thread to finalize.
	virtual metadb_hint_list::ptr create_hint_list() = 0;

	//! Updates tags of the specified tracks. Helper; uses update_info_async internally. \n
	//! Use from main thread only (starts a threaded_process to show a progress dialog).
	//! @param p_list List of items to process.
	//! @param p_op_flags Can be null, or one or more of op_flag_* enum values combined, altering behaviors of the operation.
	//! @param p_notify Called when the task is completed. Status code is one of t_update_info values. Can be null if caller doesn't care.
	//! @param p_new_info New infos to write to specified items.
	void update_info_async_simple(metadb_handle_list_cref p_list,const pfc::list_base_const_t<const file_info*> & p_new_info, fb2k::hwnd_t p_parent_window,t_uint32 p_op_flags,completion_notify_ptr p_notify);

	//! Helper to be called after a file has been rechaptered. \n
	//! Forcibly reloads info then tells playlist_manager to update all affected playlists. \n
	//! Call from main thread only.
	void on_file_rechaptered( const char * path, metadb_handle_list_cref newItems );
	//! Helper to be called after a file has been rechaptered. \n
	//! Forcibly reloads info then tells playlist_manager to update all affected playlists. \n
	//! Call from main thread only.
	void on_files_rechaptered( metadb_handle_list_cref newHandles );

	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(metadb_io_v2,metadb_io);
};


//! \since 0.9.5
class NOVTABLE metadb_io_v3 : public metadb_io_v2 {
public:
	//! Registers a callback object to receive notifications about metadb_io operations. \n
	//! See: metadb_io_callback_dynamic \n
	//! Call from main thread only.
	virtual void register_callback(metadb_io_callback_dynamic * p_callback) = 0;
	//! Unregisters a callback object to receive notifications about metadb_io operations. \n
	//! See: metadb_io_callback_dynamic \n
	//! Call from main thread only.
	virtual void unregister_callback(metadb_io_callback_dynamic * p_callback) = 0;

	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(metadb_io_v3,metadb_io_v2);
};


class threaded_process_callback;

//! \since 1.5
class NOVTABLE metadb_io_v4 : public metadb_io_v3 {
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(metadb_io_v4, metadb_io_v3);
public:
	//! Creates an update-info task, that can be either fed to threaded_process API, or invoked by yourself respecting threaded_process semantics. \n
	//! May return null pointer if the operation has been refused (by user settings or such). \n
	//! Useful for performing the operation with your own in-dialog progress display instead of the generic progress popup. \n
	//! Main thread only.
	virtual service_ptr_t<threaded_process_callback> spawn_update_info( metadb_handle_list_cref items, service_ptr_t<file_info_filter> p_filter, uint32_t opFlags, completion_notify_ptr reply ) = 0;
	//! Creates an remove-info task, that can be either fed to threaded_process API, or invoked by yourself respecting threaded_process semantics. \n
	//! May return null pointer if the operation has been refused (by user settings or such). \n
	//! Useful for performing the operation with your own in-dialog progress display instead of the generic progress popup. \n
	//! Main thread only.
	virtual service_ptr_t<threaded_process_callback> spawn_remove_info( metadb_handle_list_cref items, uint32_t opFlags, completion_notify_ptr reply) = 0;
	//! Creates an load-info task, that can be either fed to threaded_process API, or invoked by yourself respecting threaded_process semantics. \n
	//! May return null pointer if the operation has been refused (for an example no loading is needed for these items). \n
	//! Useful for performing the operation with your own in-dialog progress display instead of the generic progress popup. \n
	//! Main thread only.
	virtual service_ptr_t<threaded_process_callback> spawn_load_info( metadb_handle_list_cref items, t_load_info_type opType, uint32_t opFlags, completion_notify_ptr reply) = 0;
};

// \since 2.0
class NOVTABLE metadb_io_v5 : public metadb_io_v4 {
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(metadb_io_v5, metadb_io_v4);
public:
	//! Register a metadb_io_callback_v2_dynamic object to receive notifications about metadb_io events. \n
	//! Main thread only.
	virtual void register_callback_v2(metadb_io_callback_v2_dynamic*) = 0;
	//! Unregister a metadb_io_callback_v2_dynamic object. \n
	//! Main thread only.
	virtual void unregister_callback_v2(metadb_io_callback_v2_dynamic*) = 0;
};


//! Entrypoint service for metadb_handle related operations.\n
//! Implemented only by core, do not reimplement.\n
//! Use metadb::get() to obtain an instance.
class NOVTABLE metadb : public service_base
{
protected:	
	//! OBSOLETE, DO NOT CALL
	virtual void database_lock()=0;
	//! OBSOLETE, DO NOT CALL
	virtual void database_unlock()=0;
public:
	
	//! Returns a metadb_handle object referencing the specified location. If one doesn't exist yet a new one is created. There can be only one metadb_handle object referencing specific location. \n
	//! This function should never fail unless there's something critically wrong (can't allocate memory for the new object, etc). \n
	//! Speed: O(log(n)) to total number of metadb_handles present. It's recommended to pass metadb_handles around whenever possible rather than pass playable_locations then retrieve metadb_handles on demand when needed.
	//! @param p_out Receives the metadb_handle pointer.
	//! @param p_location Location to create a metadb_handle for.
	virtual void handle_create(metadb_handle_ptr & p_out,const playable_location & p_location)=0;

	void handle_create_replace_path_canonical(metadb_handle_ptr & p_out,const metadb_handle_ptr & p_source,const char * p_new_path);
	void handle_replace_path_canonical(metadb_handle_ptr & p_out,const char * p_new_path);
	void handle_create_replace_path(metadb_handle_ptr & p_out,const metadb_handle_ptr & p_source,const char * p_new_path);

	//! Helper function; attempts to retrieve a handle to any known playable location to be used for e.g. titleformatting script preview.\n
	//! @returns True on success; false on failure (no known playable locations).
	static bool g_get_random_handle(metadb_handle_ptr & p_out);

	enum {case_sensitive = playable_location::case_sensitive};
	typedef playable_location::path_comparator path_comparator;

	inline static int path_compare_ex(const char * p1,t_size len1,const char * p2,t_size len2) {return case_sensitive ? pfc::strcmp_ex(p1,len1,p2,len2) : stricmp_utf8_ex(p1,len1,p2,len2);}
	inline static int path_compare_nc(const char * p1, size_t len1, const char * p2, size_t len2) {return case_sensitive ? pfc::strcmp_nc(p1,len1,p2,len2) : stricmp_utf8_ex(p1,len1,p2,len2);}
	inline static int path_compare(const char * p1,const char * p2) {return case_sensitive ? strcmp(p1,p2) : stricmp_utf8(p1,p2);}
	inline static int path_compare_metadb_handle(const metadb_handle_ptr & p1,const metadb_handle_ptr & p2) {return path_compare(p1->get_path(),p2->get_path());}

	metadb_handle_ptr handle_create(playable_location const & l) {metadb_handle_ptr temp; handle_create(temp, l); return temp;}
	metadb_handle_ptr handle_create(const char * path, uint32_t subsong) {return handle_create(make_playable_location(path, subsong));}

	FB2K_MAKE_SERVICE_COREAPI(metadb);
};




//! \since 2.0
class metadb_v2 : public metadb {
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(metadb_v2, metadb);
public:
	typedef metadb_v2_rec_t rec_t;
    
    //! Query info record by location, bypassing metadb_handle layer.
	virtual rec_t query(playable_location const& loc) = 0;

    //! Callback class for queryMulti(). See metadb_v2::queryMulti().
	class queryMultiCallback_t {
	public:
		virtual void onInfo(size_t idx, const rec_t& rec) = 0;
	};
    //! Callback class for queryMultiParallel(). See metadb_v2::queryMultiParallel().
	class queryMultiParallelCallback_t {
	public:
		virtual void* initThreadContext() { return nullptr; }
		virtual void onInfo(size_t idx, const rec_t& rec, void * ctx) = 0;
		virtual void clearThreadContext(void*) {}
	};
	
    //! Optimized multi-item metadb info query. Supply a callback to receive info records for all requested tracks. \n
    //! This is considerably faster than reading info records one by one, batch database queries are used to speed operation up. \n
    //! The infos may come in different order than requested - pay attention to idx argument of callback's onInfo(). \n
    //! See also: queryMulti_(), using a lambda instead of a callback object.
	virtual void queryMulti(metadb_handle_list_cref items, queryMultiCallback_t& cb) = 0;
	
    //! Multi-thread optimized version of queryMulti(). \n
    //! Faster if used with thousands of items, needs the callback to handle concurrent calls from many threads. \n
    //! See also: queryMultiParallel_() and queryMultiParallelEx_(), helpers using lambdas and classes to implement the callback for you.
	virtual void queryMultiParallel(metadb_handle_list_cref items, queryMultiParallelCallback_t& cb) = 0;
	
    //! Format title without database access, use preloaded metadb v2 record.
	virtual void formatTitle_v2( const metadb_handle_ptr & handle, const rec_t & rec, titleformat_hook* p_hook, pfc::string_base& p_out, const service_ptr_t<titleformat_object>& p_script, titleformat_text_filter* p_filter) = 0;

    //! Helper around queryMulti(). Implements callback for you using passed lambda.
	void queryMulti_(metadb_handle_list_cref items, std::function< void(size_t idx, const rec_t& rec) > f) {
		class qmc_impl : public queryMultiCallback_t {
		public:
			void onInfo(size_t idx, const rec_t& rec) override { m_f(idx, rec); }
			decltype(f) m_f;
		};
		qmc_impl cb; cb.m_f = f;
		this->queryMulti(items, cb);
	}

    //! Simplified helper around queryMultiParallel(). No per-thread data object used. Implements callback for you using passed labmda.
	void queryMultiParallel_(metadb_handle_list_cref items, std::function< void(size_t, const rec_t&) > f) {
		class qmc_impl : public queryMultiParallelCallback_t {
		public:
			void onInfo(size_t idx, const rec_t& rec, void* ctx) override {m_f(idx, rec);}

			decltype(f) m_f;
		};
		qmc_impl cb; cb.m_f = f;
		this->queryMultiParallel(items, cb);
	}
    
    //! Simplified helper around queryMultiParallel(). \n
    //! instance_t implements per-thread context data, one will be created in each worker threads. \n
    //! While lambda itself will be called from many threads at once, only one instance_t will be used in each thread, so instance_t can be accessed without thread safety measures.
	template<typename instance_t> void queryMultiParallelEx_(metadb_handle_list_cref items, std::function<void(size_t, const rec_t&, instance_t&)> f) {
		class qmc_impl : public queryMultiParallelCallback_t {
		public:
			void* initThreadContext() override {return reinterpret_cast<void*>(new instance_t);}
			void onInfo(size_t idx, const rec_t& rec, void* ctx) override { m_f(idx, rec, * reinterpret_cast<instance_t*>(ctx)); }
			void clearThreadContext(void* ctx) override { delete reinterpret_cast<instance_t*>(ctx); }

			decltype(f) m_f;
		};
		qmc_impl cb; cb.m_f = f;
		this->queryMultiParallel(items, cb);
	}
    
    //! Simplified helper for retrieving info of multiple tracks efficiently. \n
    //! Uses the fastest way to pull info from thousands of tracks - queryMultiParallel(). \n
    //! Keep in mind that it results in all affected info becoming loaded into application memory at once. \n
    //! If possible, use methods with callbacks/lambdas instead and process info in callbacks instead of keeping it.
	pfc::array_t<rec_t> queryMultiSimple(metadb_handle_list_cref items) {
		pfc::array_t<rec_t> ret;
		ret.resize(items.get_count());
		this->queryMultiParallel_(items, [&](size_t idx, const rec_t& rec) {ret[idx] = rec;});
		return ret;
	}
};
