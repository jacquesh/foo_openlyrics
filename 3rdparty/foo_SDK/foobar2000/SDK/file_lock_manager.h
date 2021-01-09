#pragma once

/*
File lock management API
Historical note: while this API was first published in a 2018 release of the SDK, it had been around for at least a decade and is supported in all fb2k versions from 0.9 up.
The semantics are similar to those of blocking POSIX flock().
Since read locks are expected to be held for a long period of time - for an example, when playing an audio track, a holder of such lock can query whether someone else is waiting for this lock to be released.
Various audio file operations performed by fb2k core use file locks to synchronize access to the files - in particular, to allow graceful updates of tags on the currently playing track.

Usage examples:
If you want to write tags to an audio file, using low level methods such as input or album_art_editor APIs-
  Obtain a write lock before accessing your file and release it when done.
If you want to keep an audio file open for an extended period of time and allow others to write to it-
  Obtain a read lock, check is_release_requested() on it periodically; when it returns true, close the file, release the lock, obtain a new lock (which will block you until the peding write operation has completed), reopen the file and resume decoding where you left.

Final note:
Majority of the fb2k components will never need this API. 
If you carry out tag updates via metadb_io, the locking is already dealt with by fb2k core.
*/

//! An instance of a file lock object. Use file_lock_manager to instantiate.
class NOVTABLE file_lock : public service_base {
public:
	//! Returns whether we're blocking other attempts to access this file. A read lock does not block other read locks, but a write lock requires exclusive access.\n
	//! Typically, time consuming read operations check this periodically and close the file / release the lock / reacquire the lock / reopen the file when signaled.
	virtual bool is_release_requested() = 0;

	FB2K_MAKE_SERVICE_INTERFACE(file_lock, service_base);
};

typedef service_ptr_t<file_lock> file_lock_ptr;

//! \since 1.5
//! Modern version of file locking. \n
//! A read lock can be interrupted by a write lock request, from the thread that requested writing. \n
class NOVTABLE file_lock_interrupt : public service_base {
    FB2K_MAKE_SERVICE_INTERFACE(file_lock_interrupt, service_base);
public:
    //! Please note that interrupt() is called outside any sync scopes and may be called after lock reference has been released. \n
    //! It is implementer's responsibility to safeguard against such. \n
    //! The interrupt() function must *never* fail, unless aborted by calling context - which means that whoever asked for write access is aborting whatever they're doing. \n
    //! This function may block for as long as it takes to release the owned resources, but must be able to abort cleanly if doing so. \n
    //! If the function was aborted, it may be called again on the same object. \n
    //! If the function succeeded, it will not be called again on the same object; the object will be released immediately after.
    virtual void interrupt( abort_callback & aborter ) = 0;
    
    static file_lock_interrupt::ptr create( std::function< void (abort_callback&)> );
};

//! Entry point class for obtaining file_lock objects.
class NOVTABLE file_lock_manager : public service_base {
public:
	enum t_mode {
		mode_read = 0,
		mode_write
	};
	//! Acquires a read or write lock for this file path. \n
	//! If asked for read access, waits until nobody else holds a write lock for this path (but others may read at the same time).
	//! If asked for write access, access until nobody else holds a read or write lock for this path. \n
	//! The semantics are similar to those of blocking POSIX flock().
	virtual file_lock_ptr acquire(const char * p_path, t_mode p_mode, abort_callback & p_abort) = 0;

	//! Helper, calls acquire() with mode_read.
	file_lock_ptr acquire_read(const char * p_path, abort_callback & p_abort) { return acquire(p_path, mode_read, p_abort); }
	//! Helper, calls acquire() with mode_write.
	file_lock_ptr acquire_write(const char * p_path, abort_callback & p_abort) { return acquire(p_path, mode_write, p_abort); }


	FB2K_MAKE_SERVICE_COREAPI(file_lock_manager);
};

// \since 1.5
class NOVTABLE file_lock_manager_v2 : public file_lock_manager {
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION( file_lock_manager_v2, file_lock_manager );
public:
	virtual fb2k::objRef acquire_read_v2(const char * p_path, file_lock_interrupt::ptr interruptHandler, abort_callback & p_abort) = 0;
};
