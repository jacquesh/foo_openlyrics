#include "stdafx.h"
#include <helpers/file_list_helper.h>
#include <helpers/filetimetools.h>

void RunIOTest() {
	try {
		abort_callback_dummy noAbort;
		auto request = http_client::get()->create_request("GET");
		request->run("https://www.foobar2000.org", noAbort);
	} catch (std::exception const & e) {
		popup_message::g_show( PFC_string_formatter() << "Network test failure:\n" << e, "Information");
		return;
	}
	popup_message::g_show(PFC_string_formatter() << "Network test OK", "Information");
}

namespace { // anon namespace local classes for good measure
	class tpc_copyFiles : public threaded_process_callback {
	public:
		tpc_copyFiles ( metadb_handle_list_cref items, const char * pathTo ) : m_pathTo(pathTo), m_outFS(filesystem::get(pathTo)) {
			m_lstFiles.init_from_list( items );
		}

		void on_init(HWND p_wnd) override {
			// Main thread, called before run() gets started
		}
		void run(threaded_process_status & p_status, abort_callback & p_abort) override {
			// Worker thread
			for( size_t fileWalk = 0; fileWalk < m_lstFiles.get_size(); ++ fileWalk ) {

				// always do this in every timeconsuming loop
				// will throw exception_aborted if the user pushed 'cancel' on us
				p_abort.check(); 

				const char * inPath = m_lstFiles[fileWalk];
				p_status.set_progress(fileWalk, m_lstFiles.get_size());
				p_status.set_item_path( inPath );

				try {
					workWithFile(inPath, p_abort);
				} catch(exception_aborted) {
					// User abort, just bail
					throw;
				} catch(std::exception const & e) {
					m_errorLog << "Could not copy: " << file_path_display(inPath) << ", reason: " << e << "\n";
				}
				
			}
		}

		void workWithFile( const char * inPath, abort_callback & abort ) {
			FB2K_console_formatter() << "File: " << file_path_display(inPath);

			// Filesystem API for inPath
			const filesystem::ptr inFS = filesystem::get( inPath );
			pfc::string8 inFN;
			// Extract filename+extension according to this filesystem's rules
			// If it's HTTP or so, there might be ?key=value that needs stripping
			inFS->extract_filename_ext(inPath, inFN);

			pfc::string8 outPath = m_pathTo;
			// Suffix with outFS path separator. On Windows local filesystem this is always a backslash.
			outPath.end_with( m_outFS->pathSeparator() );
			outPath += inFN;

			const double openTimeout = 1.0; // wait and keep retrying for up to 1 second in case of a sharing violation error

			// Not strictly needed, but we do it anyway
			// Acquire a read lock on the file, so anyone trying to acquire a write lock will just wait till we have finished
			auto inLock = file_lock_manager::get()->acquire_read( inPath, abort );

			auto inFile = inFS->openRead( inPath, abort, openTimeout );

			// Let's toy around with inFile
			{
				auto stats = inFile->get_stats(abort);

				if ( stats.m_size != filesize_invalid ) {
					FB2K_console_formatter() << "Size: " << pfc::format_file_size_short(stats.m_size);
				}
				if ( stats.m_timestamp != filetimestamp_invalid ) {
					FB2K_console_formatter() << "Last modified: " << format_filetimestamp(stats.m_timestamp);
				}
				pfc::string8 contentType;
				if ( inFile->get_content_type( contentType ) ) {
					FB2K_console_formatter() << "Content type: " << contentType;
				}

				uint8_t buffer[256];
				size_t got = inFile->read(buffer, sizeof(buffer), abort);

				if ( got > 0 ) {
					FB2K_console_formatter() << "Header bytes: " << pfc::format_hexdump( buffer, got );
				}

				if ( inFile->is_remote() ) {
					FB2K_console_formatter() << "File is remote";
				} else {
					FB2K_console_formatter() << "File is local";
				}

				// For seekable files, reopen() seeks to the beginning.
				// For nonseekable stream, reopen() restarts reading the stream.
				inFile->reopen(abort);
			}

			// This is a glorified strcmp() for file paths.
			if ( metadb::path_compare( inPath, outPath ) == 0 ) {
				// Same path, go no further. Specifically don't attempt acquiring a writelock because that will never complete, unless user aborted.
				FB2K_console_formatter() << "Input and output paths are the same - not copying!";
				return;
			}

			// Required to write to files being currently played.
			// See file_lock_manager documentation for details.
			auto outLock = file_lock_manager::get()->acquire_write( outPath, abort );

			// WARNING : if a file exists at outPath prior to this, it will be reset to zero bytes (win32 CREATE_ALWAYS semantics)
			auto outFile = m_outFS->openWriteNew(outPath, abort, openTimeout);

			

			try {
				// Refer to g_transfer_file implementation details in the SDK for lowlevel reading & writing details
				file::g_transfer_file(inFile, outFile, abort);
			} catch(...) {

				if ( inFile->is_remote() ) {
					// Remote file was being downloaded? Suppress deletion of incomplete output
					throw;
				}
				// Failed for some reason
				// Release our destination file hadnle
				outFile.release();

				// .. and delete the incomplete file
				try {
					abort_callback_dummy noAbort; // we might be being aborted, don't let that prevent deletion
					m_outFS->remove( outPath, noAbort );
				} catch(...) {
					// disregard errors - just report original copy error
				}

				throw; // rethrow the original copy error
			}

		}
		
		void on_done(HWND p_wnd, bool p_was_aborted) override {
			// All done, main thread again

			if (! p_was_aborted && m_errorLog.length() > 0 ) {
				popup_message::g_show(m_errorLog, "Information");
			}

		}

		

		// This is a helper class that generates a sorted list of unique file paths in this metadb_handle_list.
		// The metadb_handle_list might contain duplicate tracks or multiple subsongs in the same file. m_lstFiles will list each file only once.
		file_list_helper::file_list_from_metadb_handle_list m_lstFiles;

		// Destination path
		const pfc::string8 m_pathTo;
		// Destination filesystem API. Obtained via filesystem::get() with destination path.
		const filesystem::ptr m_outFS;

		// Error log
		pfc::string_formatter m_errorLog;
	};
}

void RunCopyFiles(metadb_handle_list_cref data) {

	// Detect modal dialog wars.
	// If another modal dialog is active, bump it instead of allowing our modal dialog (uBrowseForFolder) to run.
	// Suppress this if the relevant code is intended to be launched by a modal dialog.
	if (!ModalDialogPrologue()) return;

	const HWND wndParent = core_api::get_main_window();
	pfc::string8 copyTo;
	// shared.dll method
	if (!uBrowseForFolder( wndParent, "Choose destination folder", copyTo )) return;
	
	// shared.dll methods are win32 API wrappers and return plain paths with no protocol prepended
	// Prefix with file:// before passing to fb2k filesystem methods.
	// Actually the standard fb2k filesystem implementation recognizes paths even without the prefix, but we enforce it here as a good practice.
	pfc::string8 copyTo2 = PFC_string_formatter() << "file://" << copyTo;

	// Create worker object, a threaded_process_callback implementation.
	auto worker = fb2k::service_new<tpc_copyFiles>(data, copyTo2);
	const uint32_t flags = threaded_process::flag_show_abort | threaded_process::flag_show_progress | threaded_process::flag_show_item;
	// Start the process asynchronously.
	threaded_process::get()->run_modeless( worker, flags, wndParent, "Sample Component: Copying Files" );
	
	// Our worker is now running.
}


namespace {
	class processLLtags : public threaded_process_callback {
	public:
		processLLtags( metadb_handle_list_cref data ) : m_items(data) {
			m_hints = metadb_io_v2::get()->create_hint_list();
		}
		void on_init(ctx_t p_wnd) override {
			// Main thread, called before run() gets started
		}
		void run(threaded_process_status & p_status, abort_callback & p_abort) override {
			// Worker thread

			// Note:
			// We should look for references to the same file (such as multiple subsongs) in the track list and update each file only once.
			// But for the sake of simplicity we don't do this in a sample component.
			for( size_t itemWalk = 0; itemWalk < m_items.get_size(); ++ itemWalk ) {

				// always do this in every timeconsuming loop
				// will throw exception_aborted if the user pushed 'cancel' on us
				p_abort.check();

				auto item = m_items[ itemWalk ];
				p_status.set_progress( itemWalk, m_items.get_size() );
				p_status.set_item_path( item->get_path() );

				try {
					workWithTrack(item, p_abort);
				} catch(exception_aborted) {
					// User abort, just bail
					throw;
				} catch(std::exception const & e) {
					m_errorLog << "Could not update: " << item << ", reason: " << e << "\n";
				}
			}
		}
		void on_done(ctx_t p_wnd, bool p_was_aborted) override {
			// All done, main thread again
			if ( m_hints.is_valid() ) {
				// This is the proper time to finalize the hint list
				// All playlists showing these files and such will now be refreshed
				m_hints->on_done();
			}

			if (!p_was_aborted && m_errorLog.length() > 0) {
				popup_message::g_show(m_errorLog, "Information");
			}
		}

	private:
		void workWithTrack( metadb_handle_ptr item, abort_callback & abort ) {

			FB2K_console_formatter() << "foo_sample will update tags on: " << item;

			const auto subsong = item->get_subsong_index();
			const auto path = item->get_path();

			const double openTimeout = 1.0; // wait and keep retrying for up to 1 second in case of a sharing violation error

			// Required to write to files being currently played.
			// See file_lock_manager documentation for details.
			auto lock = file_lock_manager::get()->acquire_write( path, abort );

			input_info_writer::ptr writer;
			input_entry::g_open_for_info_write_timeout(writer, nullptr, path, abort, openTimeout );

			{ // let's toy around with info that the writer can hand to us
				auto stats = writer->get_file_stats( abort );
				if (stats.m_timestamp != filetimestamp_invalid) {
					FB2K_console_formatter() << "Last-modified before tag update: " << format_filetimestamp_utc(stats.m_timestamp);
				}
			}
			
			file_info_impl info;
			writer->get_info( subsong, info, abort );

			info.meta_set("comment", "foo_sample lowlevel tags write demo was here");

			// Touchy subject
			// Should we let the user abort an incomplete tag write?
			// Let's better not
			abort_callback_dummy noAbort;

			// This can be called many times for files with multiple subsongs
			writer->set_info( subsong, info, noAbort );
			
			// This is called once - when we're done set_info()'ing
			writer->commit( noAbort );

			{ // let's toy around with info that the writer can hand to us
				auto stats = writer->get_file_stats(abort);
				if (stats.m_timestamp != filetimestamp_invalid) {
					FB2K_console_formatter() << "Last-modified after tag update: " << format_filetimestamp_utc(stats.m_timestamp);
				}
			}

			// Now send new info to metadb
			// If we don't do this, old info may still be shown in playlists etc.
			if ( true ) {
				// Method #1: feed altered info directly to the hintlist.
				// Makes sense here as we updated just one subsong.
				auto stats = writer->get_file_stats(abort);
				m_hints->add_hint(item, info, stats, true);
			} else {
				// Method #2: let metadb talk to our writer object (more commonly used).
				// The writer is a subclass of input_info_reader and can therefore be legitimately fed to add_hint_reader()
				// This will read info from all subsongs in the file and update metadb if appropriate.
				m_hints->add_hint_reader(path, writer, abort);
			}
		}
		metadb_hint_list::ptr m_hints;
		const metadb_handle_list m_items;
		pfc::string_formatter m_errorLog;
	};
}
void RunAlterTagsLL(metadb_handle_list_cref data) {
	
	const HWND wndParent = core_api::get_main_window();

	// Our worker object, a threaded_process_callback subclass.
	auto worker = fb2k::service_new< processLLtags > ( data );

	const uint32_t flags = threaded_process::flag_show_abort | threaded_process::flag_show_progress | threaded_process::flag_show_item;

	// Start the worker asynchronously.
	threaded_process::get()->run_modeless(worker, flags, wndParent, "Sample Component: Updating Tags");

	// The worker is now running.

}
