#pragma once
#include "mem_block_container.h"


//! \since 1.1
//! metadb_index_manager hash, currently a 64bit int, typically made from halving MD5 hash.
typedef t_uint64 metadb_index_hash;


//! \since 1.1
//! A class that transforms track information (location+metadata) to a hash for metadb_index_manager. \n
//! You need to implement your own when using metadb_index_manager to pin your data to user's tracks. \n
//! Possible routes to take when implementing: \n
//! Rely on location only - pinning lost when user moves, but survives editing tags\n
//! Rely on metadata - pinning survives moving files, but lost when editing tags\n
//! If you do the latter, you can implement metadb_io_edit_callback to respond to tag edits and avoid data loss.
class NOVTABLE metadb_index_client : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(metadb_index_client, service_base)
public:
	virtual metadb_index_hash transform(const file_info& info, const playable_location& location) = 0;

	bool hashHandle(metadb_handle_ptr const& h, metadb_index_hash& out) {
		metadb_info_container::ptr info;
		if (!h->get_info_ref(info)) return false;
		out = transform(info->info(), h->get_location());
		return true;
	}

	static metadb_index_hash from_md5(hasher_md5_result const& in) { return in.xorHalve(); }
};

//! \since 1.1
//! This service lets you pin your data to user's music library items, typically to be presented as title formatting %fields% via metadb_display_field_provider. \n
//! Implement metadb_index_client to define how your data gets pinned to the songs.
class NOVTABLE metadb_index_manager : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(metadb_index_manager)
public:
	//! Install a metadb_index_client. \n
	//! This is best done from init_stage_callback::on_init_stage(init_stages::before_config_read) to avoid hammering already loaded UI & playlists with refresh requests. \n
	//! If you provide your own title formatting fields, call dispatch_global_refresh() after a successful add() to signal all components to refresh all tracks \n
	//! - which is expensive, hence it should be done in early app init phase for minimal performance penalty. \n
	//! Always put a try/catch around add() as it may fail with an exception in an unlikely scenario of corrupted files holding your previously saved data. \n
	//! @param client your metadb_index_client object.
	//! @param index_id Your GUID that you will pass to other methods when referring to your index.
	//! @param userDataRetentionPeriod Time for which the data should be retained if no matching tracks are present. \n
	//! If this was set to zero, the data could be lost immediately if a library folder disappers for some reason. \n
	//! Hint: use system_time_periods::* constants, for an example, system_time_periods::week.
	virtual void add(metadb_index_client::ptr client, const GUID& index_id, t_filetimestamp userDataRetentionPeriod) = 0;
	//! Uninstalls a previously installed index.
	virtual void remove(const GUID& index_id) = 0;
	//! Sets your data for the specified index+hash.
	virtual void set_user_data(const GUID& index_id, const metadb_index_hash& hash, const void* data, t_size dataSize) = 0;
	//! Gets your data for the specified index+hash.
	virtual void get_user_data(const GUID& index_id, const metadb_index_hash& hash, mem_block_container& out) = 0;


	//! Helper
	template<typename t_array> void get_user_data_t(const GUID& index_id, const metadb_index_hash& hash, t_array& out) {
		mem_block_container_ref_impl<t_array> ref(out);
		get_user_data(index_id, hash, ref);
	}

	//! Helper
	t_size get_user_data_here(const GUID& index_id, const metadb_index_hash& hash, void* out, t_size outSize) {
		mem_block_container_temp_impl ref(out, outSize);
		get_user_data(index_id, hash, ref);
		return ref.get_size();
	}

	//! Signals all components that your data for the tracks matching the specified hashes has been altered; this will redraw the affected tracks in playlists and such.
	virtual void dispatch_refresh(const GUID& index_id, const pfc::list_base_const_t<metadb_index_hash>& hashes) = 0;

	//! Helper
	void dispatch_refresh(const GUID& index_id, const metadb_index_hash& hash) {
		pfc::list_single_ref_t<metadb_index_hash> l(hash);
		dispatch_refresh(index_id, l);
	}

	//! Dispatches a global refresh, asks all components to refresh all tracks. To be calling after adding/removing indexes. Expensive!
	virtual void dispatch_global_refresh() = 0;

	//! Efficiently retrieves metadb_handles of items present in the Media Library matching the specified index value. \n
	//! This can be called from the app main thread only (interfaces with the library_manager API).
	virtual void get_ML_handles(const GUID& index_id, const metadb_index_hash& hash, metadb_handle_list_ref out) = 0;

	//! Retrieves all known hash values for this index.
	virtual void get_all_hashes(const GUID& index_id, pfc::list_base_t<metadb_index_hash>& out) = 0;

	//! Determines whether a no longer needed user data file for this index exists. \n
	//! For use with index IDs that are not currently registered only.
	virtual bool have_orphaned_data(const GUID& index_id) = 0;

	//! Deletes no longer needed index user data files. \n
	//! For use with index IDs that are not currently registered only.
	virtual void erase_orphaned_data(const GUID& index_id) = 0;

	//! Saves index user data file now. You normally don't need to call this; it's done automatically when saving foobar2000 configuration. \n
	//! This will throw exceptions in case of a failure (out of disk space etc).
	virtual void save_index_data(const GUID& index_id) = 0;
};

//! \since 2.0
//! Call this instead of metadb_index_manager::set_user_data() for many updates in a row
class NOVTABLE metadb_index_transaction : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(metadb_index_transaction, service_base)
public:
	virtual void set_user_data(const GUID& index_id, const metadb_index_hash& hash, const void* data, t_size dataSize) = 0;
	virtual void commit() = 0;
};

//! \since 2.0
class NOVTABLE metadb_index_manager_v2 : public metadb_index_manager {
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(metadb_index_manager_v2, metadb_index_manager)
public:
	//! Call this instead of metadb_index_manager::set_user_data() for many updates in a row
	virtual metadb_index_transaction::ptr begin_transaction() = 0;
};
