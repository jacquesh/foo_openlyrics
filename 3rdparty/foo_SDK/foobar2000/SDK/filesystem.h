#pragma once

#include "file.h"
#include "fsitem.h"
#include <functional>

#ifndef _WIN32
struct stat;
#endif

//! Contains various I/O related structures and interfaces.
namespace foobar2000_io
{

	//! Disk space info struct
	struct drivespace_t {
		//! Free space, in bytes
		t_filesize m_free = filesize_invalid;
		//! Total size, in bytes
		t_filesize m_total = filesize_invalid;
		//! Free space available to caller, in bytes \n
		//! If not specifically supported by filesystem, same as m_free.
		t_filesize m_avail = filesize_invalid;
	};



	class filesystem;

	class NOVTABLE directory_callback {
	public:
		//! @returns true to continue enumeration, false to abort.
		virtual bool on_entry(filesystem * p_owner,abort_callback & p_abort,const char * p_url,bool p_is_subdirectory,const t_filestats & p_stats)=0;
	};

	class NOVTABLE directory_callback_v3 {
	public:
		virtual bool on_entry(filesystem* owner, const char* URL, t_filestats2 const& stats, abort_callback& abort) = 0;
	};

	//! Entrypoint service for all filesystem operations.\n
	//! Implementation: standard implementations for local filesystem etc are provided by core.\n
	//! Instantiation: use static helper functions rather than calling filesystem interface methods directly, e.g. filesystem::g_open() to open a file.
	class NOVTABLE filesystem : public service_base {
		FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(filesystem);
	public:
		//! Enumeration specifying how to open a file. See: filesystem::open(), filesystem::g_open().
		typedef uint32_t t_open_mode;
		enum {
			//! Opens an existing file for reading; if the file does not exist, the operation will fail.
			open_mode_read,
			//! Opens an existing file for writing; if the file does not exist, the operation will fail.
			open_mode_write_existing,
			//! Opens a new file for writing; if the file exists, its contents will be wiped.
			open_mode_write_new,

			open_mode_mask = 0xFF,
            open_shareable = 0x100,
		};

		virtual bool get_canonical_path(const char * p_path,pfc::string_base & p_out)=0;
		virtual bool is_our_path(const char * p_path)=0;
		virtual bool get_display_path(const char * p_path,pfc::string_base & p_out)=0;

		virtual void open(service_ptr_t<file> & p_out,const char * p_path, t_open_mode p_mode,abort_callback & p_abort)=0;
		virtual void remove(const char * p_path,abort_callback & p_abort)=0;
		//! Moves/renames a file. Will fail if the destination file already exists. \n
		//! Note that this function may throw exception_io_denied instead of exception_io_sharing_violation when the file is momentarily in use, due to bugs in Windows MoveFile() API. There is no legitimate way for us to distinguish between the two scenarios.
		virtual void move(const char * p_src,const char * p_dst,abort_callback & p_abort)=0;
		//! Queries whether a file at specified path belonging to this filesystem is a remote object or not.
		virtual bool is_remote(const char * p_src) = 0;

		//! Retrieves stats of a file at specified path.
		virtual void get_stats(const char * p_path,t_filestats & p_stats,bool & p_is_writeable,abort_callback & p_abort) = 0;
		//! Helper
		t_filestats get_stats( const char * path, abort_callback & abort );
		
		virtual bool relative_path_create(const char * file_path,const char * playlist_path,pfc::string_base & out) {return false;}
		virtual bool relative_path_parse(const char * relative_path,const char * playlist_path,pfc::string_base & out) {return false;}

		//! Creates a directory.
		virtual void create_directory(const char * p_path,abort_callback & p_abort) = 0;

		virtual void list_directory(const char * p_path,directory_callback & p_out,abort_callback & p_abort)=0;

		//! Hint; returns whether this filesystem supports mime types. \n 
		//! When this returns false, all file::get_content_type() calls on files opened thru this filesystem implementation will return false; otherwise, file::get_content_type() calls may return true depending on the file.
		virtual bool supports_content_types() = 0;

		static void g_get_canonical_path(const char * path,pfc::string_base & out);
		static void g_get_display_path(const char * path,pfc::string_base & out);
        //! Retrieves a shortened display name for this file. By default this is implemented by returning filename.ext portion of the path.
        static bool g_get_display_name_short( const char * path, pfc::string_base & out );
        //! Extracts the native filesystem path, sets out to the input path if native path cannot be extracted so the output is always set.
        //! @returns True if native path was extracted successfully, false otherwise (but output is set anyway).
        static bool g_get_native_path( const char * path, pfc::string_base & out, abort_callback & a = fb2k::noAbort);
        static pfc::string8 g_get_native_path( const char * path );

		static bool g_get_interface(service_ptr_t<filesystem> & p_out,const char * path);//path is AFTER get_canonical_path
		static filesystem::ptr g_get_interface(const char * path);// throws exception_io_no_handler_for_path on failure
		static filesystem::ptr get( const char * path ) { return g_get_interface(path); } // shortened
		static filesystem::ptr tryGet(const char* path);
		static bool g_is_remote(const char * p_path);//path is AFTER get_canonical_path
		static bool g_is_recognized_and_remote(const char * p_path);//path is AFTER get_canonical_path
		static bool g_is_remote_safe(const char * p_path) {return g_is_recognized_and_remote(p_path);}
		static bool g_is_remote_or_unrecognized(const char * p_path);
		static bool g_is_recognized_path(const char * p_path);
		
		//! Opens file at specified path, with specified access privileges.
		static void g_open(service_ptr_t<file> & p_out,const char * p_path,t_open_mode p_mode,abort_callback & p_abort);
		//! Attempts to open file at specified path; if the operation fails with sharing violation error, keeps retrying (with short sleep period between retries) for specified amount of time.
		static void g_open_timeout(service_ptr_t<file> & p_out,const char * p_path,t_open_mode p_mode,double p_timeout,abort_callback & p_abort);
		static void g_open_write_new(service_ptr_t<file> & p_out,const char * p_path,abort_callback & p_abort);
		static void g_open_read(service_ptr_t<file> & p_out,const char * path,abort_callback & p_abort) {return g_open(p_out,path,open_mode_read,p_abort);}
		static void g_open_precache(service_ptr_t<file> & p_out,const char * path,abort_callback & p_abort);//open only for precaching data (eg. will fail on http etc)
		static bool g_exists(const char * p_path,abort_callback & p_abort);
		static bool g_exists_writeable(const char * p_path,abort_callback & p_abort);
		//! Removes file at specified path.
		static void g_remove(const char * p_path,abort_callback & p_abort);
		//! Attempts to remove file at specified path; if the operation fails with a sharing violation error, keeps retrying (with short sleep period between retries) for specified amount of time.
		static void g_remove_timeout(const char * p_path,double p_timeout,abort_callback & p_abort);
		//! Moves file from one path to another.
		//! Note that this function may throw exception_io_denied instead of exception_io_sharing_violation when the file is momentarily in use, due to bugs in Windows MoveFile() API. There is no legitimate way for us to distinguish between the two scenarios.
		static void g_move(const char * p_src,const char * p_dst,abort_callback & p_abort);
		//! Attempts to move file from one path to another; if the operation fails with a sharing violation error, keeps retrying (with short sleep period between retries) for specified amount of time.
		static void g_move_timeout(const char * p_src,const char * p_dst,double p_timeout,abort_callback & p_abort);

		static void g_link(const char * p_src,const char * p_dst,abort_callback & p_abort);
		static void g_link_timeout(const char * p_src,const char * p_dst,double p_timeout,abort_callback & p_abort);

		static void g_copy(const char * p_src,const char * p_dst,abort_callback & p_abort);//needs canonical path
		static void g_copy_timeout(const char * p_src,const char * p_dst,double p_timeout,abort_callback & p_abort);//needs canonical path
		static void g_copy_directory(const char * p_src,const char * p_dst,abort_callback & p_abort);//needs canonical path
		static void g_get_stats(const char * p_path,t_filestats & p_stats,bool & p_is_writeable,abort_callback & p_abort);
		static bool g_relative_path_create(const char * p_file_path,const char * p_playlist_path,pfc::string_base & out);
		static bool g_relative_path_parse(const char * p_relative_path,const char * p_playlist_path,pfc::string_base & out);
		
		static void g_create_directory(const char * p_path,abort_callback & p_abort);

		static void g_open_temp(service_ptr_t<file> & p_out,abort_callback & p_abort);
		static void g_open_tempmem(service_ptr_t<file> & p_out,abort_callback & p_abort);
		static file::ptr g_open_tempmem();

		static void g_list_directory(const char * p_path,directory_callback & p_out,abort_callback & p_abort);// path must be canonical

		static bool g_is_valid_directory(const char * path,abort_callback & p_abort);
		static bool g_is_empty_directory(const char * path,abort_callback & p_abort);

		void remove_object_recur(const char * path, abort_callback & abort);
		void remove_directory_content(const char * path, abort_callback & abort);
		static void g_remove_object_recur(const char * path, abort_callback & abort);
		static void g_remove_object_recur_timeout(const char * path, double timeout, abort_callback & abort);

		// Presumes both source and destination belong to this filesystem.
		void copy_directory(const char * p_src, const char * p_dst, abort_callback & p_abort);
		void copy_directory_contents(const char* p_src, const char* p_dst, abort_callback& p_abort);

		//! Moves/renames a file, overwriting the destination atomically if exists. \n
		//! Note that this function may throw exception_io_denied instead of exception_io_sharing_violation when the file is momentarily in use, due to bugs in Windows MoveFile() API. There is no legitimate way for us to distinguish between the two scenarios.
		void move_overwrite( const char * src, const char * dst, abort_callback & abort);
		//! Moves/renames a file, overwriting the destination atomically if exists. \n
		//! Meant to retain destination attributes if feasible. Otherwise identical to move_overwrite(). \n
		//! Note that this function may throw exception_io_denied instead of exception_io_sharing_violation when the file is momentarily in use, due to bugs in Windows MoveFile() API. There is no legitimate way for us to distinguish between the two scenarios.
		void replace_file(const char * src, const char * dst, abort_callback & abort);
		//! Create a directory, without throwing an exception if it already exists.
		//! @param didCreate bool flag indicating whether a new directory was created or not. \n
		//! This should be a retval, but because it's messy to obtain this information with certain APIs, the caller can opt out of receiving this information,.
		void make_directory( const char * path, abort_callback & abort, bool * didCreate = nullptr );
		//! Bool retval version of make_directory().
		bool make_directory_check( const char * path, abort_callback & abort );

		//! Returns whether a directory exists at path, false if doesn't exist or not a directory.
		bool directory_exists(const char * path, abort_callback & abort);
		//! Returns whether a file exists at path, false if doesn't exist or not a file.
		bool file_exists( const char * path, abort_callback & abort );
		//! Returns whether either a file or a directory exists at path. Effectively directory_exists() || file_exists(), but somewhat more efficient.
		bool exists(const char* path, abort_callback& a);

		char pathSeparator();
		//! Extracts the filename.ext portion of the path. \n
		//! The filename is ready to be presented to the user - URL decoding and such (similar to get_display_path()) is applied.
		void extract_filename_ext(const char * path, pfc::string_base & outFN);
		pfc::string8 extract_filename_ext(const char* path);
		pfc::string8 get_extension(const char* path);
		static pfc::string8 g_get_extension(const char* path);
		//! Retrieves the parent path.
		bool get_parent_path(const char * path, pfc::string_base & out);
		//! Retrieves the parent path, alternate version.
		fb2k::stringRef parentPath(const char* path);

		file::ptr openWriteNew( const char * path, abort_callback & abort, double timeout );
		file::ptr openWriteExisting(const char * path, abort_callback & abort, double timeout);
		file::ptr openRead( const char * path, abort_callback & abort, double timeout);
		file::ptr openEx( const char * path, t_open_mode mode, abort_callback & abort, double timeout);
		void remove_(const char* path, abort_callback& a, double timeout);

		//! Read whole file into a mem_block_container
		void read_whole_file(const char * path, mem_block_container & out, pfc::string_base & outContentType, size_t maxBytes, abort_callback & abort );
		//! Alternate read whole file, fb2k mobile style
		fb2k::memBlockRef readWholeFile(const char* path, size_t maxBytes, abort_callback& abort);
        static fb2k::memBlockRef g_readWholeFile(const char * path, size_t sizeSanity, abort_callback & aborter);

		bool is_transacted();
		bool commit_if_transacted(abort_callback &abort);

		//! Full file rewrite helper that automatically does the right thing to ensure atomic update. \n
		//! If this is a transacted filesystem, a simple in-place rewrite is performed. \n
		//! If this is not a transacted filesystem, your content first goes to a temporary file, which then replaces the original. \n
		//! See also: filesystem_transacted. \n
		//! In order to perform transacted operations, you must obtain a transacted filesystem explicitly, or get one passed down from a higher level context (example: in config_io_callback_v3).
		void rewrite_file( const char * path, abort_callback & abort, double opTimeout, std::function<void (file::ptr) > worker );
		void rewrite_file(const char* path, abort_callback& abort, double opTimeout, const void* payload, size_t bytes);
		//! Full directory rewrite helper that automatically does the right thing to ensure atomic update. \n
		//! If this is a transacted filesystem, a simple in-place rewrite is performed. \n
		//! If this is not a transacted filesystem, your content first goes to a temporary folder, which then replaces the original. \n
		//! It is encouraged to perform flushFileBuffers on all files accessed from within. \n
		//! See also: filesystem_transacted. \n
		//! In order to perform transacted operations, you must obtain a transacted filesystem explicitly, or get one passed down from a higher level context (example: in config_io_callback_v3).
		void rewrite_directory(const char * path, abort_callback & abort, double opTimeout, std::function<void(const char *) > worker);

		t_filestats2 get_stats2_(const char* p_path, uint32_t s2flags, abort_callback& p_abort);
		static t_filestats2 g_get_stats2(const char* p_path, uint32_t s2flags, abort_callback& p_abort);

		bool get_display_name_short_(const char* path, pfc::string_base& out);

		fsItemFolder::ptr makeItemFolderStd(const char* pathCanonical, t_filestats2 const& stats = filestats2_invalid );
		fsItemFile::ptr makeItemFileStd(const char* pathCanonical, t_filestats2 const& stats = filestats2_invalid );
		fsItemBase::ptr findItem_(const char* path, abort_callback& p_abort);
		fsItemFile::ptr findItemFile_(const char* path, abort_callback& p_abort);
		fsItemFolder::ptr findItemFolder_(const char* path, abort_callback& p_abort);


		typedef std::function<void(const char*, t_filestats2 const&) > list_callback_t;
		void list_directory_(const char* path, list_callback_t cb, unsigned listMode,abort_callback& a);

		//! Compares two paths determining if one is a subpath of another,
		//! Returns false if the paths are unrelated.
		//! Returns true if the paths are related, and then: result is set 0 if they are equal, 1 if p2 is a subpath of p1, -1 if p1 is a subpath of p2
		static bool g_compare_paths(const char* p1, const char* p2, int& result);

		//! Batch file stats read. Some filesystems provide an optimized implementation of this.
		static void g_readStatsMulti(fb2k::arrayRef items, uint32_t s2flags, t_filestats2* outStats, abort_callback& abort);
        
        //! See filesystem_v3::fileNameSanity(). Throws pfc::exception_not_implemented() if not available.
        fb2k::stringRef fileNameSanity_(const char* fn);
        
        //! See filesystem_v3::getDriveSpace(). Throws pfc::exception_not_implemented() if not available.
        drivespace_t getDriveSpace_(const char* pathAt, abort_callback& abort);

	protected:
		static bool get_parent_helper(const char * path, char separator, pfc::string_base & out);
		void read_whole_file_fallback( const char * path, mem_block_container & out, pfc::string_base & outContentType, size_t maxBytes, abort_callback & abort );
	};

	class filesystem_v2 : public filesystem {
		FB2K_MAKE_SERVICE_INTERFACE( filesystem_v2, filesystem )
	public:
		//! Moves/renames a file, overwriting the destination atomically if exists. \n
		//! Note that this function may throw exception_io_denied instead of exception_io_sharing_violation when the file is momentarily in use, due to bugs in Windows MoveFile() API. There is no legitimate way for us to distinguish between the two scenarios.
		virtual void move_overwrite(const char * src, const char * dst, abort_callback & abort) = 0;
		//! Moves/renames a file, overwriting the destination atomically if exists. \n
		//! Meant to retain destination attributes if feasible. Otherwise identical to move_overwrite(). \n
		//! Note that this function may throw exception_io_denied instead of exception_io_sharing_violation when the file is momentarily in use, due to bugs in Windows MoveFile() API. There is no legitimate way for us to distinguish between the two scenarios.
		virtual void replace_file(const char * src, const char * dst, abort_callback & abort);
		//! Create a directory, without throwing an exception if it already exists.
		//! @param didCreate bool flag indicating whether a new directory was created or not. \n
		//! This should be a retval, but because it's messy to obtain this information with certain APIs, the caller can opt out of receiving this information,.
		virtual void make_directory(const char * path, abort_callback & abort, bool * didCreate = nullptr) = 0;
		virtual bool directory_exists(const char * path, abort_callback & abort) = 0;
		virtual bool file_exists(const char * path, abort_callback & abort) = 0;
		virtual char pathSeparator() = 0;
		virtual void extract_filename_ext(const char * path, pfc::string_base & outFN);
		virtual bool get_parent_path( const char * path, pfc::string_base & out);
		virtual void list_directory_ex(const char * p_path, directory_callback & p_out, unsigned listMode, abort_callback & p_abort) = 0;
		virtual void read_whole_file(const char * path, mem_block_container & out, pfc::string_base & outContentType, size_t maxBytes, abort_callback & abort);

		//! Wrapper to list_directory_ex
		void list_directory( const char * p_path, directory_callback & p_out, abort_callback & p_abort );
		
		bool make_directory_check(const char * path, abort_callback & abort);
	};

	//! \since 2.0
	class filesystem_v3 : public filesystem_v2 {
		FB2K_MAKE_SERVICE_INTERFACE(filesystem_v3, filesystem_v2);
	public:
		//! Retrieves free space on the volume at the specified path.
		//! Optional functionality, throws pfc::exception_not_implemented if n/a.
		virtual drivespace_t getDriveSpace(const char* pathAt, abort_callback& abort);

		//! Retrieves stats of a file at specified path.
		virtual t_filestats2 get_stats2(const char* p_path, uint32_t s2flags, abort_callback& p_abort) = 0;

		virtual bool get_display_name_short(const char* in, pfc::string_base& out);

		virtual void list_directory_v3(const char* path, directory_callback_v3& callback, unsigned listMode, abort_callback& p_abort) = 0;

		virtual fsItemBase::ptr findItem(const char* path, abort_callback& p_abort);
		virtual fsItemFile::ptr findItemFile(const char* path, abort_callback& p_abort);
		virtual fsItemFolder::ptr findItemFolder(const char* path, abort_callback& p_abort);
		virtual fsItemFolder::ptr findParentFolder(const char* path, abort_callback& p_abort);
		virtual fb2k::stringRef fileNameSanity(const char* fn);
		virtual void readStatsMulti(fb2k::arrayRef items, uint32_t s2flags, t_filestats2* outStats, abort_callback& abort);

		//! Optional method to return a native filesystem path to this item, null if N/A.
		//! Aborter provided for corner cases, normally not needed.
		virtual fb2k::stringRef getNativePath(const char* in, abort_callback& a) { return nullptr; }

		// Old method wrapped to get_stats2()
		void get_stats(const char* p_path, t_filestats& p_stats, bool& p_is_writeable, abort_callback& p_abort) override;
		// Old method wrapped to list_directory_v3()
		void list_directory_ex(const char* p_path, directory_callback& p_out, unsigned listMode, abort_callback& p_abort) override;

		// Old method wrapped to get_stats2()
		bool directory_exists(const char* path, abort_callback& abort) override;
		// Old method wrapped to get_stats2()
		bool file_exists(const char* path, abort_callback& abort) override;
	};

	class directory_callback_impl : public directory_callback
	{
		struct t_entry
		{
			pfc::string_simple m_path;
			t_filestats m_stats;
			t_entry(const char * p_path, const t_filestats & p_stats) : m_path(p_path), m_stats(p_stats) {}
		};


		pfc::list_t<pfc::rcptr_t<t_entry> > m_data;
		bool m_recur;

		static int sortfunc(const pfc::rcptr_t<const t_entry> & p1, const pfc::rcptr_t<const t_entry> & p2) {return pfc::io::path::compare(p1->m_path,p2->m_path);}
	public:
		bool on_entry(filesystem * owner,abort_callback & p_abort,const char * url,bool is_subdirectory,const t_filestats & p_stats);

		directory_callback_impl(bool p_recur) : m_recur(p_recur) {}
		t_size get_count() {return m_data.get_count();}
		const char * operator[](t_size n) const {return m_data[n]->m_path;}
		const char * get_item(t_size n) const {return m_data[n]->m_path;}
		const t_filestats & get_item_stats(t_size n) const {return m_data[n]->m_stats;}
		void sort() {m_data.sort_t(sortfunc);}
	};


	t_filetimestamp filetimestamp_from_system_timer();

#ifdef _WIN32
	inline t_filetimestamp import_filetimestamp(FILETIME ft) {
		return *reinterpret_cast<t_filetimestamp*>(&ft);
	}
#endif

	void generate_temp_location_for_file(pfc::string_base & p_out, const char * p_origpath,const char * p_extension,const char * p_magic);


	inline file_ptr fileOpen(const char * p_path,filesystem::t_open_mode p_mode,abort_callback & p_abort,double p_timeout) {
		file_ptr temp; filesystem::g_open_timeout(temp,p_path,p_mode,p_timeout,p_abort); PFC_ASSERT(temp.is_valid()); return temp;
	}

	inline file_ptr fileOpenReadExisting(const char * p_path,abort_callback & p_abort,double p_timeout = 0) {
		return fileOpen(p_path,filesystem::open_mode_read,p_abort,p_timeout);
	}
	inline file_ptr fileOpenWriteExisting(const char * p_path,abort_callback & p_abort,double p_timeout = 0) {
		return fileOpen(p_path,filesystem::open_mode_write_existing,p_abort,p_timeout);
	}
	inline file_ptr fileOpenWriteNew(const char * p_path,abort_callback & p_abort,double p_timeout = 0) {
		return fileOpen(p_path,filesystem::open_mode_write_new,p_abort,p_timeout);
	}
	
	template<typename t_list>
	class directory_callback_retrieveList : public directory_callback {
	public:
		directory_callback_retrieveList(t_list & p_list,bool p_getFiles,bool p_getSubDirectories) : m_list(p_list), m_getFiles(p_getFiles), m_getSubDirectories(p_getSubDirectories) {}
		bool on_entry(filesystem * p_owner,abort_callback & p_abort,const char * p_url,bool p_is_subdirectory,const t_filestats & p_stats) {
			p_abort.check();
			if (p_is_subdirectory ? m_getSubDirectories : m_getFiles) {
				m_list.add_item(p_url);
			}
			return true;
		}
	private:
		const bool m_getSubDirectories;
		const bool m_getFiles;
		t_list & m_list;
	};
	template<typename t_list>
	class directory_callback_retrieveListEx : public directory_callback {
	public:
		directory_callback_retrieveListEx(t_list & p_files, t_list & p_directories) : m_files(p_files), m_directories(p_directories) {}
		bool on_entry(filesystem * p_owner,abort_callback & p_abort,const char * p_url,bool p_is_subdirectory,const t_filestats & p_stats) {
			p_abort.check();
			if (p_is_subdirectory) m_directories += p_url;
			else m_files += p_url;
			return true;
		}
	private:
		t_list & m_files;
		t_list & m_directories;
	};
	template<typename t_list> class directory_callback_retrieveListRecur : public directory_callback {
	public:
		directory_callback_retrieveListRecur(t_list & p_list) : m_list(p_list) {}
		bool on_entry(filesystem * owner,abort_callback & p_abort,const char * path, bool isSubdir, const t_filestats&) {
			if (isSubdir) {
				try { owner->list_directory(path,*this,p_abort); } catch(exception_io) {}
			} else {
				m_list.add_item(path);
			}
			return true;
		}
	private:
		t_list & m_list;
	};

	template<typename t_list>
	static void listFiles(const char * p_path,t_list & p_out,abort_callback & p_abort) {
		directory_callback_retrieveList<t_list> callback(p_out,true,false);
		filesystem::g_list_directory(p_path,callback,p_abort);
	}
	template<typename t_list>
	static void listDirectories(const char * p_path,t_list & p_out,abort_callback & p_abort) {
		directory_callback_retrieveList<t_list> callback(p_out,false,true);
		filesystem::g_list_directory(p_path,callback,p_abort);
	}
	template<typename t_list>
	static void listFilesAndDirectories(const char * p_path,t_list & p_files,t_list & p_directories,abort_callback & p_abort) {
		directory_callback_retrieveListEx<t_list> callback(p_files,p_directories);
		filesystem::g_list_directory(p_path,callback,p_abort);
	}
	template<typename t_list>
	static void listFilesRecur(const char * p_path,t_list & p_out,abort_callback & p_abort) {
		directory_callback_retrieveListRecur<t_list> callback(p_out);
		filesystem::g_list_directory(p_path,callback,p_abort);
	}

	bool extract_native_path(const char * p_fspath,pfc::string_base & p_native);
	bool _extract_native_path_ptr(const char * & p_fspath);
	bool is_native_filesystem( const char * p_fspath );
	bool extract_native_path_ex(const char * p_fspath, pfc::string_base & p_native);//prepends \\?\ where needed

	bool extract_native_path_archive_aware( const char * fspatch, pfc::string_base & out );
	bool extract_native_path_archive_aware_ex( const char * fspatch, pfc::string_base & out, abort_callback & a );

	template<typename T>
	pfc::string getPathDisplay(const T& source) {
		const char * c = pfc::stringToPtr(source);
		if ( *c == 0 ) return c;
		pfc::string_formatter temp;
		filesystem::g_get_display_path(c,temp);
		return temp.toString();
	}
	template<typename T>
	pfc::string getPathCanonical(const T& source) {
		const char * c = pfc::stringToPtr(source);
		if ( *c == 0 ) return c;
		pfc::string_formatter temp;
		filesystem::g_get_canonical_path(c,temp);
		return temp.toString();
	}


	bool matchContentType(const char * fullString, const char * ourType);
	bool matchProtocol(const char * fullString, const char * protocolName);
    bool testIfHasProtocol( const char * fullString );
	const char * afterProtocol( const char * fullString );
	pfc::string8 getProtocol(const char* fullString);
	void substituteProtocol(pfc::string_base & out, const char * fullString, const char * protocolName);
    
    bool matchContentType_MP3( const char * fullString);
    bool matchContentType_MP4audio( const char * fullString);
    bool matchContentType_MP4( const char * fullString);
    bool matchContentType_Ogg( const char * fullString);
    bool matchContentType_Opus( const char * fullString);
    bool matchContentType_FLAC( const char * fullString);
    bool matchContentType_WavPack( const char * fullString);
    bool matchContentType_WAV( const char * fullString);
    bool matchContentType_Musepack( const char * fullString);
    const char * extensionFromContentType( const char * contentType );
	const char * contentTypeFromExtension( const char * ext );

	void purgeOldFiles(const char * directory, t_filetimestamp period, abort_callback & abort);

#ifndef _WIN32
    // Struct stat to fb2k filestats converter
    t_filestats nixMakeFileStats(const struct stat & st);
    t_filestats2 nixMakeFileStats2(const struct stat & st);
    bool nixQueryReadonly( const struct stat & st );
    bool nixQueryDirectory( const struct stat & st );
    bool compactHomeDir(pfc::string_base & str);
    bool expandHomeDir(pfc::string_base & str);


#endif

	//! \since 1.6
	class read_ahead_tools : public service_base {
		FB2K_MAKE_SERVICE_COREAPI(read_ahead_tools);
	public:
		//! Turn any file object into asynchronous read-ahead-buffered file.
		//! @param f File object to wrap. Do not call this object's method after a successful call to add_read_ahead; new file object takes over the ownership of it.
		//! @param size Requested read-ahead bytes. Pass 0 to use user settings for local/remote playback.
		virtual file::ptr add_read_ahead(file::ptr f, size_t size, abort_callback & aborter) = 0;

		//! A helper method to use prior to opening decoders. \n
		//! May open the file if needed or leave it blank for the decoder to open.
		//! @param f File object to open if needed (buffering mandated by user settings). May be valid or null prior to call. May be valid or null (no buffering) after call.
		//! @param path Path to open. May be null if f is not null. At least one of f and path must be valid prior to call.
		virtual void open_file_helper(file::ptr & f, const char * path, abort_callback & aborter) = 0;
	};

}

using namespace foobar2000_io;

#include "filesystem_helper.h"
