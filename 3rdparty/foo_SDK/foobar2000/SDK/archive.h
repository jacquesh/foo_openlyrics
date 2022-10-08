#pragma once

namespace foobar2000_io {
	class archive;

	class NOVTABLE archive_callback : public abort_callback {
	public:
		virtual bool on_entry(archive * owner,const char * url,const t_filestats & p_stats,const service_ptr_t<file> & p_reader) = 0;
	};

	//! Interface for archive reader services. When implementing, derive from archive_impl rather than from deriving from archive directly.
	class NOVTABLE archive : public filesystem {
		FB2K_MAKE_SERVICE_INTERFACE(archive,filesystem);
	public:
		//! Lists archive contents. \n
        //! May be called with any path, not only path accepted by is_our_archive.
		virtual void archive_list(const char * p_path,const service_ptr_t<file> & p_reader,archive_callback & p_callback,bool p_want_readers) = 0;

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

	//! Root class for archive implementations. Derive from this instead of from archive directly.
	class NOVTABLE archive_impl : public archive_v3 {
	protected:
		//do not override these
		bool get_canonical_path(const char * path,pfc::string_base & out);
		bool is_our_path(const char * path);
		bool get_display_path(const char * path,pfc::string_base & out);
		void remove(const char * path,abort_callback & p_abort);
		void move(const char * src,const char * dst,abort_callback & p_abort);
		bool is_remote(const char * src);
		bool relative_path_create(const char * file_path,const char * playlist_path,pfc::string_base & out);
		bool relative_path_parse(const char * relative_path,const char * playlist_path,pfc::string_base & out);
		void open(service_ptr_t<file> & p_out,const char * path, t_open_mode mode,abort_callback & p_abort);
		void create_directory(const char * path,abort_callback &);
		void list_directory(const char * p_path,directory_callback & p_out,abort_callback & p_abort);
		void get_stats(const char * p_path,t_filestats & p_stats,bool & p_is_writeable,abort_callback & p_abort);
		void list_extensions(pfc::string_base & out) override { out = get_archive_type(); }
		bool supports_content_types() override { return false; }
	protected:
		//override these
		virtual const char * get_archive_type()=0;//eg. "zip", must be lowercase
		virtual t_filestats get_stats_in_archive(const char * p_archive,const char * p_file,abort_callback & p_abort) = 0;
		virtual void open_archive(service_ptr_t<file> & p_out,const char * archive,const char * file, abort_callback & p_abort) = 0;//opens for reading
	public:
		//override these
		virtual void archive_list(const char * path,const service_ptr_t<file> & p_reader,archive_callback & p_out,bool p_want_readers)= 0 ;
		virtual bool is_our_archive( const char * path ) = 0;
		
		static bool g_is_unpack_path(const char * path);
		static bool g_parse_unpack_path(const char * path,pfc::string_base & archive,pfc::string_base & file);
		static bool g_parse_unpack_path_ex(const char * path,pfc::string_base & archive,pfc::string_base & file, pfc::string_base & type);
		static void g_make_unpack_path(pfc::string_base & path,const char * archive,const char * file,const char * type);
		void make_unpack_path(pfc::string_base & path,const char * archive,const char * file);

		
	};

	template<typename T>
	class archive_factory_t : public service_factory_single_t<T> {};
}
