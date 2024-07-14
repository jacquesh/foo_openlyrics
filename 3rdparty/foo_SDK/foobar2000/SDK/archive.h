#pragma once
#include <functional>
#include "filesystem.h"

namespace foobar2000_io {
	class archive;

	//! Callback passed to archive listing methods. \n
	//! For backwards compatibility, this inherits with abort_callback as well. \n
	//! When implementiong, you must override abort_callback methods redirecting them to your abort_callback. \n
	//! It is recommended to use lambda-based archive_list helper instead of implementing this interface.
	class NOVTABLE archive_callback : public abort_callback {
	public:
		virtual bool on_entry(archive * owner,const char * url,const t_filestats & p_stats,const service_ptr_t<file> & p_reader) = 0;
	};

	//! Interface for archive reader services. When implementing, derive from archive_impl rather than from deriving from archive directly.
	class NOVTABLE archive : public filesystem {
		FB2K_MAKE_SERVICE_INTERFACE(archive,filesystem);
	public:
		typedef std::function<void(const char* url, const t_filestats& stats, file::ptr reader) > list_func_t;

		//! Lists archive contents. \n
        //! May be called with any path, not only path accepted by is_our_archive. \n
		//! It is strongly recommended to use the lambda_based archive_list() helper instead of calling this directly.
		virtual void archive_list(const char * p_path,const service_ptr_t<file> & p_reader,archive_callback & p_callback,bool p_want_readers) = 0;
		
		//! Helper implemented on top of the other archive_list, uses lambda instead of callback, avoids having to implement archive_callback.
		void archive_list(const char * path, file::ptr, list_func_t, bool wantReaders, abort_callback&);

		//! Optional method to weed out unsupported formats prior to calling archive_list. \n
        //! Use this to suppress calls to archive_list() to avoid spurious exceptions being thrown. \n
		//! Implemented via archive_v2.
		bool is_our_archive( const char * path );
	};

	//! \since 1.5
	//! New 1.5 series API, though allowed to implement/call in earlier versions. \n
	//! Suppresses spurious C++ exceptions on all files not recognized as archives by this instance.
	class NOVTABLE archive_v2 : public archive {
		FB2K_MAKE_SERVICE_INTERFACE(archive_v2, archive)
	public:

		//! Optional method to weed out unsupported formats prior to calling archive_list. \n
        //! Use this to suppress calls to archive_list() to avoid spurious exceptions being thrown.
        virtual bool is_our_archive( const char * path ) = 0;
	};

	//! \since 1.6
	//! New 1.6 series API, though allowed to implement/call in earlier versions.
	class NOVTABLE archive_v3 : public archive_v2 {
		FB2K_MAKE_SERVICE_INTERFACE(archive_v3, archive_v2)
	public:
		//! Determine supported archive file types. \n
		//! Returns a list of extensions, colon delimited, e.g.: "zip,rar,7z"
		virtual void list_extensions(pfc::string_base & out) = 0;
	};
    //! \since 2.1
    class NOVTABLE archive_v4 : public archive_v3 {
        FB2K_MAKE_SERVICE_INTERFACE(archive_v4, archive_v3)
    public:
        virtual fb2k::arrayRef archive_list_v4( fsItemFilePtr item, file::ptr readerOptional, abort_callback & a) = 0;
    };

	//! Root class for archive implementations. Derive from this instead of from archive directly.
	class NOVTABLE archive_impl : public service_multi_inherit<archive_v4, filesystem_v3> {
	protected:
		//do not override these
		bool get_canonical_path(const char * path,pfc::string_base & out) override;
		bool is_our_path(const char * path) override;
		bool get_display_path(const char * path,pfc::string_base & out) override;
		void remove(const char * path,abort_callback & p_abort) override;
		void move(const char * src,const char * dst,abort_callback & p_abort) override;
		void move_overwrite(const char* src, const char* dst, abort_callback& abort) override;
		bool is_remote(const char * src) override;
		bool relative_path_create(const char * file_path,const char * playlist_path,pfc::string_base & out) override;
		bool relative_path_parse(const char * relative_path,const char * playlist_path,pfc::string_base & out) override;
		void open(service_ptr_t<file> & p_out,const char * path, t_open_mode mode,abort_callback & p_abort) override;
		void create_directory(const char * path,abort_callback &) override;
		void make_directory(const char* path, abort_callback& abort, bool* didCreate = nullptr) override;
		void list_directory(const char* p_path, directory_callback& p_out, abort_callback& p_abort) override;
		void list_directory_ex(const char* p_path, directory_callback& p_out, unsigned listMode, abort_callback& p_abort) override;
		void list_directory_v3(const char* path, directory_callback_v3& callback, unsigned listMode, abort_callback& p_abort) override;
		t_filestats2 get_stats2(const char* p_path, uint32_t s2flags, abort_callback& p_abort) override;
		virtual void get_stats(const char* p_path, t_filestats& p_stats, bool& p_is_writeable, abort_callback& p_abort) override;
		void list_extensions(pfc::string_base & out) override { out = get_archive_type(); }
		bool supports_content_types() override { return false; }
		char pathSeparator() override { return '/'; }
		void extract_filename_ext(const char * path, pfc::string_base & outFN) override;
		bool get_display_name_short(const char* in, pfc::string_base& out) override;
        fb2k::arrayRef archive_list_v4( fsItemFilePtr item, file::ptr readerOptional, abort_callback & a ) override;
	protected:
		//override these
		virtual const char * get_archive_type()=0;//eg. "zip", must be lowercase
		virtual t_filestats2 get_stats2_in_archive(const char * p_archive,const char * p_file,unsigned s2flags,abort_callback & p_abort) = 0;
		virtual void open_archive(service_ptr_t<file> & p_out,const char * archive,const char * file, abort_callback & p_abort) = 0;//opens for reading
	public:
		//override these
		// virtual void archive_list(const char * path,const service_ptr_t<file> & p_reader,archive_callback & p_out,bool p_want_readers) = 0;
		// virtual bool is_our_archive( const char * path ) = 0;
		
		static bool g_is_unpack_path(const char * path);
		static bool g_parse_unpack_path(const char * path,pfc::string_base & archive,pfc::string_base & file);
		static bool g_parse_unpack_path_ex(const char * path,pfc::string_base & archive,pfc::string_base & file, pfc::string_base & type);
		static void g_make_unpack_path(pfc::string_base & path,const char * archive,const char * file,const char * type);
		void make_unpack_path(pfc::string_base & path,const char * archive,const char * file);

		
	};

	template<typename T>
	class archive_factory_t : public service_factory_single_t<T> {};
}
