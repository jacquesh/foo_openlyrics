#include "foobar2000-sdk-pch.h"
#include "playlist_loader.h"
#include "link_resolver.h"
#include "archive.h"
#include "file_info_impl.h"
#include "input.h"
#include "advconfig.h"
#include <string>
#include <unordered_set>
#include <list>

static void process_path_internal(const char * p_path,const service_ptr_t<file> & p_reader,playlist_loader_callback::ptr callback, abort_callback & abort,playlist_loader_callback::t_entry_type type,const t_filestats & p_stats);

bool playlist_loader::g_try_load_playlist(file::ptr fileHint,const char * p_path,playlist_loader_callback::ptr p_callback, abort_callback & p_abort) {
	// Determine if this file is a playlist or not (which usually means that it's a media file)
	pfc::string8 filepath;

	filesystem::g_get_canonical_path(p_path,filepath);
	
	pfc::string8 extension = filesystem::g_get_extension(filepath);

	service_ptr_t<file> l_file = fileHint;

	if (l_file.is_empty()) {
		filesystem::ptr fs;
		if (filesystem::g_get_interface(fs,filepath)) {
			if (fs->supports_content_types()) {
				try {
					fs->open(l_file,filepath,filesystem::open_mode_read,p_abort);
				} catch(exception_io) { return false; } // fall thru
			}
		}
	}

	service_enum_t<playlist_loader> e;

	if (l_file.is_valid()) {

		// Important: in case of remote HTTP files, use actual connected path for matching file extensions, following any redirects.
		// At least one internet radio station has been known to present .pls links that are 302 redirects to real streams, so they don't parse as playlists.
		{
			file_metadata_http::ptr meta;
			if (meta &= l_file->get_metadata_(p_abort)) {
				pfc::string8 realPath;
				meta->get_connected_path(realPath);
				extension = filesystem::g_get_extension(realPath);
			}
		}

		pfc::string8 content_type;
		if (l_file->get_content_type(content_type)) {
			for (auto l : e) {
				if (l->is_our_content_type(content_type)) {
					try {
						TRACK_CODE("playlist_loader::open",l->open(filepath,l_file,p_callback, p_abort));
						return true;
					} catch(exception_io_unsupported_format) {
						l_file->reopen(p_abort);
					}
				}
			}
		}
	}

	if (extension.length()>0) {
		for (auto l : e) {
			if (stricmp_utf8(l->get_extension(),extension) == 0) {
				if (l_file.is_empty()) filesystem::g_open_read(l_file,filepath,p_abort);
				try {
					TRACK_CODE("playlist_loader::open",l->open(filepath,l_file,p_callback,p_abort));
					return true;
				} catch(exception_io_unsupported_format) {
					l_file->reopen(p_abort);
				}
			}
		}
	}

	return false;
}

void playlist_loader::g_load_playlist_filehint(file::ptr fileHint,const char * p_path,playlist_loader_callback::ptr p_callback, abort_callback & p_abort) {
	if (!g_try_load_playlist(fileHint, p_path, p_callback, p_abort)) throw exception_io_unsupported_format();
}

void playlist_loader::g_load_playlist(const char * p_path,playlist_loader_callback::ptr callback, abort_callback & abort) {
	g_load_playlist_filehint(NULL,p_path,callback,abort);
}
namespace {
	class MIC_impl : public metadb_info_container_v2 {
	public:
		t_filestats2 const& stats2() override { return m_stats; }
		file_info const& info() override { return m_info; }
		t_filestats const& stats() override { return m_stats.as_legacy(); }
		bool isInfoPartial() override { return false; }

		file_info_impl m_info;
		t_filestats2 m_stats;
	};
}
static void index_tracks_helper(const char * p_path,const service_ptr_t<file> & p_reader,const t_filestats & p_stats,playlist_loader_callback::t_entry_type p_type,playlist_loader_callback::ptr p_callback, abort_callback & p_abort,bool & p_got_input)
{
	TRACK_CALL_TEXT("index_tracks_helper");
	if (p_reader.is_empty() && filesystem::g_is_remote_safe(p_path))
	{
		TRACK_CALL_TEXT("remote");
		metadb_handle_ptr handle;
		p_callback->handle_create(handle,make_playable_location(p_path,0));
		p_got_input = true;
		p_callback->on_entry(handle,p_type,p_stats,true);
	} else {
		TRACK_CALL_TEXT("hintable");
		service_ptr_t<input_info_reader> instance;
		try {
			input_entry::g_open_for_info_read(instance,p_reader,p_path,p_abort);
		} catch(exception_io_unsupported_format) {
			// specifically bail
			throw;
		} catch(exception_io) {
			// broken file or some other error, open() failed - show it anyway
			metadb_handle_ptr handle;
			p_callback->handle_create(handle, make_playable_location(p_path, 0));
			p_callback->on_entry(handle, p_type, p_stats, true);
			return;
		}

		const auto stats = instance->get_stats2_(p_path, stats2_all, p_abort);
	
		t_uint32 subsong,subsong_count = instance->get_subsong_count();
		bool bInfoGetError = false;
		for(subsong=0;subsong<subsong_count;subsong++)
		{
			TRACK_CALL_TEXT("subsong-loop");
			p_abort.check();
			metadb_handle_ptr handle;
			t_uint32 index = instance->get_subsong(subsong);
			p_callback->handle_create(handle,make_playable_location(p_path,index));

			p_got_input = true;
			if (! bInfoGetError && p_callback->want_info(handle,p_type,stats.as_legacy(),true) )
			{
				auto mic = fb2k::service_new<MIC_impl>();
				mic->m_stats = stats;
				try {
					TRACK_CODE("get_info",instance->get_info(index,mic->m_info,p_abort));
				} catch(...) {
					bInfoGetError = true;
				}
				if (! bInfoGetError ) {
					playlist_loader_callback_v2::ptr v2;
					if (v2 &= p_callback) {
						v2->on_entry_info_v2(handle, p_type, mic, true);
					} else {
						p_callback->on_entry_info(handle, p_type, stats.as_legacy(), mic->m_info, true);
					}
					
				}
			}
			else
			{
				p_callback->on_entry(handle,p_type,stats.as_legacy(),true);
			}
		}
	}
}

static void track_indexer__g_get_tracks_wrap(const char * p_path,const service_ptr_t<file> & p_reader,const t_filestats & p_stats,playlist_loader_callback::t_entry_type p_type,playlist_loader_callback::ptr p_callback, abort_callback & p_abort) {
	bool got_input = false;
	bool fail = false;
	try {
		index_tracks_helper(p_path,p_reader,p_stats,p_type,p_callback,p_abort, got_input);
	} catch(exception_aborted) {
		throw;
	} catch(exception_io_unsupported_format) {
		fail = true;
	} catch(std::exception const & e) {
		fail = true;
		FB2K_console_formatter() << "could not enumerate tracks (" << e << ") on:\n" << file_path_display(p_path);
	}
	if (fail) {
		if (!got_input && !p_abort.is_aborting()) {
			if (p_type == playlist_loader_callback::entry_user_requested)
			{
				metadb_handle_ptr handle;
				p_callback->handle_create(handle,make_playable_location(p_path,0));
				p_callback->on_entry(handle,p_type,p_stats,true);
			}
		}
	}
}

namespace {

	static bool queryAddHidden() {
		// {2F9F4956-363F-4045-9531-603B1BF39BA8}
		static const GUID guid_cfg_addhidden =
		{ 0x2f9f4956, 0x363f, 0x4045,{ 0x95, 0x31, 0x60, 0x3b, 0x1b, 0xf3, 0x9b, 0xa8 } };

		advconfig_entry_checkbox::ptr ptr;
		if (advconfig_entry::g_find_t(ptr, guid_cfg_addhidden)) {
			return ptr->get_state();
		}
		return false;
	}

	class directory_callback_myimpl : public directory_callback
	{
	public:
		void main(const char* folder, abort_callback& abort) {
			visit(folder);
            
            abort.check();
            const uint32_t flags = listMode::filesAndFolders | (queryAddHidden() ? listMode::hidden : 0);
            
            auto workHere = [&] (folder_t const & f) {
                filesystem_v2::ptr v2;
                if (v2 &= f.m_fs) {
                    v2->list_directory_ex(f.m_folder.c_str(), *this, flags, abort);
                } else {
                    f.m_fs->list_directory(f.m_folder.c_str(), *this, abort);
                }
            };

            workHere( folder_t { folder, filesystem::get(folder) } );
            
			for (;; ) {
				abort.check();
				auto iter = m_foldersPending.begin();
				if ( iter == m_foldersPending.end() ) break;
				auto f = std::move(*iter); m_foldersPending.erase(iter);

				try {
                    workHere( f );
				} catch (exception_io const & e) {
					FB2K_console_formatter() << "Error walking directory (" << e << "): " << f.m_folder.c_str();
				}
			}
		}

		bool on_entry(filesystem * owner,abort_callback & p_abort,const char * url,bool is_subdirectory,const t_filestats & p_stats) {
			p_abort.check();
			if (!visit(url)) return true;

			filesystem_v2::ptr v2;
			v2 &= owner;

			if ( is_subdirectory ) {
				m_foldersPending.emplace_back( folder_t { url, owner } );
			} else {
				m_entries.emplace_back( entry_t { url, p_stats } );
			}
			return true;
		}

		struct entry_t {
			std::string m_path;
			t_filestats m_stats;
		};
		std::list<entry_t> m_entries;

		bool visit(const char* path) {
			return m_visited.insert( path ).second;
		}
		std::unordered_set<std::string> m_visited;
		
		struct folder_t {
			std::string m_folder;
			filesystem::ptr m_fs;
		};
		std::list<folder_t> m_foldersPending;
	};
}


static void process_path_internal(const char * p_path,const service_ptr_t<file> & p_reader,playlist_loader_callback::ptr callback, abort_callback & abort,playlist_loader_callback::t_entry_type type,const t_filestats & p_stats)
{
	//p_path must be canonical

	abort.check();

	callback->on_progress(p_path);

	
	{
		if (p_reader.is_empty() && type != playlist_loader_callback::entry_directory_enumerated) {
			try {
				directory_callback_myimpl results;
				results.main( p_path, abort );
				for( auto & i : results.m_entries ) {
					try {
						process_path_internal(i.m_path.c_str(), 0, callback, abort, playlist_loader_callback::entry_directory_enumerated, i.m_stats);
					} catch (exception_aborted) {
						throw;
					} catch (std::exception const& e) {
						FB2K_console_formatter() << "Error walking path (" << e << "): " << file_path_display(i.m_path.c_str());
					} catch (...) {
						FB2K_console_formatter() << "Error walking path (bad exception): " << file_path_display(i.m_path.c_str());
					}
				}
				return; // successfully enumerated directory - go no further
			} catch(exception_aborted) {
				throw;
			} catch (exception_io_not_directory) {
				// disregard
			} catch(exception_io_not_found) {
				// disregard
			} catch (std::exception const& e) {
				FB2K_console_formatter() << "Error walking directory (" << e << "): " << p_path;
			} catch (...) {
				FB2K_console_formatter() << "Error walking directory (bad exception): " << p_path;
			}
		}

		{
			for (auto f : filesystem::enumerate()) {
				abort.check();
				service_ptr_t<archive> arch;
				if ((arch &= f) && arch->is_our_archive(p_path)) {
					if (p_reader.is_valid()) p_reader->reopen(abort);

					try {
						archive::list_func_t archive_results = [callback, &abort](const char* p_path, const t_filestats& p_stats, file::ptr p_reader) {
							process_path_internal(p_path,p_reader,callback,abort,playlist_loader_callback::entry_directory_enumerated,p_stats);
						};
						TRACK_CODE("archive::archive_list",arch->archive_list(p_path,p_reader,archive_results,/*want readers*/true, abort));
						return;
					} catch(exception_aborted) {throw;} 
					catch(...) {
						// Something failed hard
						// Is is_our_archive() meaningful?
						archive_v2::ptr arch2;
						if (arch2 &= arch) {
							// If yes, show errors
							throw;
						}
						// Outdated archive implementation, preserve legacy behavior (try to open as non-archive)
					}
				}
			} 
		}
	}

	

	{
		service_ptr_t<link_resolver> ptr;
		if (link_resolver::g_find(ptr,p_path))
		{
			if (p_reader.is_valid()) p_reader->reopen(abort);

			pfc::string8 temp;
			try {
				TRACK_CODE("link_resolver::resolve",ptr->resolve(p_reader,p_path,temp,abort));

				track_indexer__g_get_tracks_wrap(temp,0,filestats_invalid,playlist_loader_callback::entry_from_playlist,callback, abort);
				return;//success
			} catch(exception_aborted) {throw;}
			catch(...) {}
		}
	}

	if (callback->is_path_wanted(p_path,type)) {
		track_indexer__g_get_tracks_wrap(p_path,p_reader,p_stats,type,callback, abort);
	}
}

namespace {
	class plcallback_simple : public playlist_loader_callback {
	public:
		void on_progress(const char* p_path) override {}

		void on_entry(const metadb_handle_ptr& p_item, t_entry_type p_type, const t_filestats& p_stats, bool p_fresh) override {
			m_items += p_item;
		}
		bool want_info(const metadb_handle_ptr& p_item, t_entry_type p_type, const t_filestats& p_stats, bool p_fresh) override {
			return p_item->should_reload(p_stats, p_fresh);
		}

		void on_entry_info(const metadb_handle_ptr& p_item, t_entry_type p_type, const t_filestats& p_stats, const file_info& p_info, bool p_fresh) override {
			m_items += p_item;
			m_hints->add_hint(p_item, p_info, p_stats, p_fresh);
		}

		void handle_create(metadb_handle_ptr& p_out, const playable_location& p_location) override {
			m_metadb->handle_create(p_out, p_location);
		}

		bool is_path_wanted(const char* path, t_entry_type type) override { return true; }

		bool want_browse_info(const metadb_handle_ptr& p_item, t_entry_type p_type, t_filetimestamp ts) override {
			return true;
		}
		void on_browse_info(const metadb_handle_ptr& p_item, t_entry_type p_type, const file_info& info, t_filetimestamp ts) override {
			metadb_hint_list_v2::ptr v2;
			if (v2 &= m_hints) v2->add_hint_browse(p_item, info, ts);
		}

		~plcallback_simple() {
			m_hints->on_done();
		}
		metadb_handle_list m_items;
	private:
		const metadb_hint_list::ptr m_hints = metadb_hint_list::create();
		const metadb::ptr m_metadb = metadb::get();
	};
}
void playlist_loader::g_path_to_handles_simple(const char* p_path, pfc::list_base_t<metadb_handle_ptr>& p_out, abort_callback& p_abort) {
	auto cb = fb2k::service_new<plcallback_simple>();
	g_process_path(p_path, cb, p_abort);
	p_out = cb->m_items;
}
void playlist_loader::g_process_path(const char * p_filename,playlist_loader_callback::ptr callback, abort_callback & abort,playlist_loader_callback::t_entry_type type)
{
	TRACK_CALL_TEXT("playlist_loader::g_process_path");

	auto filename = file_path_canonical(p_filename);

	process_path_internal(filename,0,callback,abort, type,filestats_invalid);
}

void playlist_loader::g_save_playlist(const char * p_filename,const pfc::list_base_const_t<metadb_handle_ptr> & data,abort_callback & p_abort)
{
	TRACK_CALL_TEXT("playlist_loader::g_save_playlist");
	pfc::string8 filename;
	filesystem::g_get_canonical_path(p_filename,filename);
	try {
		service_ptr_t<file> r;
		filesystem::g_open(r,filename,filesystem::open_mode_write_new,p_abort);

		auto ext = pfc::string_extension(filename);
		
		service_enum_t<playlist_loader> e;
		service_ptr_t<playlist_loader> l;
		if (e.first(l)) do {
			if (l->can_write() && !stricmp_utf8(ext,l->get_extension())) {
				try {
					TRACK_CODE("playlist_loader::write",l->write(filename,r,data,p_abort));
					return;
				} catch(exception_io_data) {}
			}
		} while(e.next(l));
		throw exception_io_data();
	} catch(...) {
		try {filesystem::g_remove(filename,p_abort);} catch(...) {}
		throw;
	}
}


bool playlist_loader::g_process_path_ex(const char * filename,playlist_loader_callback::ptr callback, abort_callback & abort,playlist_loader_callback::t_entry_type type)
{
	if (g_try_load_playlist(NULL, filename, callback, abort)) return true;
	//not a playlist format
	g_process_path(filename,callback,abort,type);
	return false;
}
