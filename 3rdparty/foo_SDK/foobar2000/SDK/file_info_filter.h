#pragma once
#include "tracks.h"

//! Implementing this class gives you direct control over which part of file_info gets altered during a tag update uperation. To be used with metadb_io_v2::update_info_async().
class NOVTABLE file_info_filter : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(file_info_filter, service_base);
public:
	//! Alters specified file_info entry; called as a part of tag update process. Specified file_info has been read from a file, and will be written back.\n
	//! WARNING: This will be typically called from another thread than main app thread (precisely, from thread created by tag updater). You should copy all relevant data to members of your file_info_filter instance in constructor and reference only member data in apply_filter() implementation.
	//! @returns True when you have altered file_info and changes need to be written back to the file; false if no changes have been made.
	virtual bool apply_filter(trackRef p_location, t_filestats p_stats, file_info & p_info) = 0;

    typedef std::function< bool (trackRef, t_filestats, file_info & ) > func_t;
    static file_info_filter::ptr create( func_t f );
};

//! Extended file_info_filter allowing the caller to do their own manipulation of the file before and after the metadata update takes place. \n
//! Respected by foobar2000 v1.5 and up; if metadb_io_v4 is supported, then file_info_filter_v2 is understood.
class NOVTABLE file_info_filter_v2 : public file_info_filter {
	FB2K_MAKE_SERVICE_INTERFACE(file_info_filter_v2, file_info_filter);
public:

	enum filterStatus_t {
		filterNoUpdate = 0,
		filterProceed,
		filterAlreadyUpdated
	};
	//! Called after just before rewriting metadata. The file is not yet opened for writing, but a file_lock has already been granted (so don't call it on your own). \n
	//! You can use this method to perform album art updates (via album_art_editor API) alongside metadata updates. \n
	//! Return value can be used to stop fb2k from proceeding with metadata update on this file. \n
	//! If your own operations on this file fail, just pass the exceptions to the caller and they will be reported just as other tag update errors.
	//! @param fileIfAlreadyOpened Reference to an already opened file object, if already opened by the caller. May be null.
	virtual filterStatus_t before_tag_update(const char * location, file::ptr fileIfAlreadyOpened, abort_callback & aborter) = 0;

	//! Called after metadata has been updated. \n
	//! If you wish to alter the file on your own, use before_tag_update() for this instead. \n
	//! If your own operations on this file fail, just pass the exceptions to the caller and they will be reported just as other tag update errors. \n
	//! The passed reader object can be used to read the properties of the updated file back. In most cases it will be the writer that was used to update the tags. Do not call tag writing methods on it from this function.
	virtual void after_tag_update(const char * location, service_ptr_t<class input_info_reader> reader, abort_callback & aborter) = 0;

	virtual void after_all_tag_updates(abort_callback & aborter) = 0;

	//! Allows you to do your own error logging.
	//! @returns True if the error has been noted by your code and does not need to be shown to the user.
	virtual bool filter_error(const char * location, const char * msg) = 0;
};
