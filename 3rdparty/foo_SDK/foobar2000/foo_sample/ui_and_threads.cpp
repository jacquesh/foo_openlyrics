#include "stdafx.h"
#include "resource.h"

// Modern multi threading with C++
// Or: how I learned to stop worrying and love the lambdas

#include <memory> // shared_ptr
#include <libPPUI/CDialogResizeHelper.h>
#include <helpers/filetimetools.h>
#include <helpers/duration_counter.h>
#include <helpers/atl-misc.h>



namespace { // anon namespace local classes for good measure

	
	class CDemoDialog; // forward declaration

	// This is kept a separate shared_ptr'd struct because it may outlive CDemoDialog instance.
	struct sharedData_t {
		metadb_handle_list items;
		CDemoDialog * owner; // weak reference to the owning dialog; can be only used after checking the validity by other means.
	};

	static const CDialogResizeHelper::Param resizeData[] = {
		// Dialog resize handling matrix, defines how the controls scale with the dialog
		//			 L T R B
		{IDOK,       1,1,1,1 },
		{IDCANCEL,   1,1,1,1 },
		{IDC_HEADER, 0,0,1,0 },
		{IDC_LIST,   0,0,1,1 },

		// current position of a control is determined by initial_position + factor * (current_dialog_size - initial_dialog_size)
		// where factor is the value from the table above
		// applied to all four values - left, top, right, bottom
		// 0,0,0,0 means that a control doesn't react to dialog resizing (aligned to top+left, no resize)
		// 1,1,1,1 means that the control is aligned to bottom+right but doesn't resize
		// 0,0,1,0 means that the control disregards vertical resize (aligned to top) and changes its width with the dialog
	};
	
	// Minimum/maximum size, in dialog box units; see MSDN MapDialogRect for more info about dialog box units.
	// The values can be declared constant here and will be scaled appropriately depending on display DPI.
	static const CRect resizeMinMax(150, 100, 1000, 1000);

	class CDemoDialog : public CDialogImpl<CDemoDialog> {
	public:
		enum { IDD = IDD_THREADS };
		CDemoDialog( metadb_handle_list_cref items ) : m_resizer(resizeData, resizeMinMax) {
			m_sharedData = std::make_shared< sharedData_t > ();
			m_sharedData->items = items;
			m_sharedData->owner = this;
		}

		BEGIN_MSG_MAP_EX(CDemoDialog)
			CHAIN_MSG_MAP_MEMBER(m_resizer)
			MSG_WM_INITDIALOG(OnInitDialog)
			COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnOK)
			COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
			MSG_WM_CLOSE(OnClose)
			MSG_WM_DESTROY(OnDestroy)
			MSG_WM_SIZE(OnSize)
		END_MSG_MAP()
	private:
		BOOL OnInitDialog(CWindow, LPARAM) {
			uSetDlgItemText(*this, IDC_HEADER, PFC_string_formatter() << "Selected: " << m_sharedData->items.get_size() << " tracks." );
			m_listBox = GetDlgItem(IDC_LIST);

			m_statusBar.Create(*this, NULL, TEXT(""), WS_CHILD | WS_VISIBLE);

			m_statusBar.SetWindowText(L"Ready");

			ShowWindow(SW_SHOW);

			return TRUE; // system should set focus
		}
		void OnSize(UINT nType, CSize size) {
			// Tell statusbar that we got resized. CDialogResizeHelper can't do this for us.
			m_statusBar.SendMessage(WM_SIZE);
		}
		void OnDestroy() {
			cancelTask();
		}

		void OnClose() {
			// NOTE if we do not handle WM_CLOSE, WM_COMMAND with IDCANCEL will be invoked, executing our cancel handler.
			// We provide our own WM_CLOSE handler to provide a different response to closing the window.
			DestroyWindow();
		}

		void OnCancel(UINT, int, CWindow) {
			// If a task is active, cancel it
			// otherwise destroy the dialog
			if (! cancelTask() ) {
				DestroyWindow();
			} else {
				// Refresh UI
				taskCompleted();
			}
		}
		void OnOK(UINT, int, CWindow) {
			startTask();
		}
		void startTask() {
			cancelTask(); // cancel any running task
			
			GetDlgItem(IDCANCEL).SetWindowText(L"Cancel");
			m_statusBar.SetWindowText(L"Working...");

			auto shared = m_sharedData;
			auto aborter = std::make_shared<abort_callback_impl>();
			m_aborter = aborter;

			// New in fb2k 1.4.5: async_task_manager & splitTask
			// Use fb2k::splitTask() for starting detached threads.
			// In fb2k < 1.4.5, it will fall back to just starting a detached thread.
			// fb2k 1.4.5+ async_task_manager cleanly deals with user exiting foobar2000 while a detached async task is running.
			// Shutdown of foobar2000 process will be stalled until your task completes.
			// If you use other means to ensure that the thread has finished, such as waiting for the thread to exit in your dialog's destructor, there's no need for this.

			fb2k::splitTask( [aborter, shared] {
				// In worker thread!
				try {
					work( shared, aborter );
				} catch(exception_aborted) {
					return; // user abort?
				} catch(std::exception const & e) {
					// should not really get here
					logLineProc( shared, aborter, PFC_string_formatter() << "Critical error: " << e);
				}
				try {
					mainThreadOp( aborter, [shared] {
						shared->owner->taskCompleted();
					} );
				} catch(...) {} // mainThreadOp may throw exception_aborted
			} );
		}
		void taskCompleted() {
			m_aborter.reset();
			GetDlgItem(IDCANCEL).SetWindowText(L"Close");
			m_statusBar.SetWindowText(L"Finished, ready");
		}

		static void mainThreadOp(std::shared_ptr<abort_callback_impl> aborter, std::function<void ()> op ) {
			aborter->check(); // are we getting aborted?
			fb2k::inMainThread( [=] {
				if ( aborter->is_set() ) return; // final user abort check
				// Past this, we're main thread, the task has not been cancelled by the user and the dialog is still alive
				// and any dialog methods can be safely called
				op();
			} );
		}

		static void logLineProc(std::shared_ptr<sharedData_t> shared, std::shared_ptr<abort_callback_impl> aborter, const char * line_ ) {
			pfc::string8 line( line_ ); // can't hold to the const char* we got passed, we have no guarantees about its lifetime
			mainThreadOp( aborter, [shared, line] {
				shared->owner->logLine(line);
			} );
		}
		static void work( std::shared_ptr<sharedData_t> shared, std::shared_ptr<abort_callback_impl> aborter ) {
			// clear the log
			mainThreadOp(aborter, [shared] {
				shared->owner->clearLog();
			} );

			// A convenience wrapper that calls logLineProc()
			auto log = [shared, aborter] ( const char * line ) {
				logLineProc(shared, aborter, line);
			};
			// Use log(X) instead of logLineProc(shared, aborter, X)

			for( size_t trackWalk = 0; trackWalk < shared->items.get_size(); ++ trackWalk ) {
				aborter->check();
				auto track = shared->items[trackWalk];
				log( PFC_string_formatter() << "Track: " << track );

				try {
					const auto path = track->get_path();
					const auto subsong = track->get_subsong_index();

					// Not strictly needed, but we do it anyway
					// Acquire a read lock on the file, so anyone trying to acquire a write lock will just wait till we have finished
					auto lock = file_lock_manager::get()->acquire_read(path, *aborter);

					{
						input_decoder::ptr dec;
						input_entry::g_open_for_decoding(dec, nullptr, path, *aborter);

						file_info_impl info;
						dec->get_info( subsong, info, *aborter );
						auto title = info.meta_get("title",0);
						if ( title == nullptr ) log("Untitled");
						else log(PFC_string_formatter() << "Title: " << title );
						if ( info.get_length() > 0 ) log(PFC_string_formatter() << "Duration: " << pfc::format_time_ex(info.get_length(),6) );
						auto stats = dec->get_file_stats( *aborter );
						if ( stats.m_size != filesize_invalid ) log( PFC_string_formatter() << "Size: " << pfc::format_file_size_short(stats.m_size) );
						if ( stats.m_timestamp != filetimestamp_invalid ) log( PFC_string_formatter() << "Last modified: " << format_filetimestamp( stats.m_timestamp ) );


						dec->initialize( subsong, input_flag_simpledecode, * aborter );
						audio_chunk_impl chunk;
						uint64_t numChunks = 0, numSamples = 0;
						// duration_counter tool is a strictly accurate audio duration counter retaining all sample counts passed to it, immune to floatingpoint accuracy errors
						duration_counter duration;
						bool firstChunk = true;
						while(dec->run(chunk, *aborter) ) {
							if ( firstChunk ) {
								auto spec = chunk.get_spec();
								log(PFC_string_formatter() << "Audio sample rate: " << spec.sampleRate );
								log(PFC_string_formatter() << "Audio channels: " << audio_chunk::g_formatChannelMaskDesc( spec.chanMask ) );
								firstChunk = false;
							}
							++ numChunks;
							duration += chunk;
							numSamples += chunk.get_sample_count();
						}
						log(PFC_string_formatter() << "Decoded " << numChunks << " chunks");
						log(PFC_string_formatter() << "Exact duration decoded: " << pfc::format_time_ex(duration.query(), 6) << ", " << numSamples << " samples" );
					}

					try {
						auto aa = album_art_extractor::g_open( nullptr, path, *aborter );
						
						if ( aa->have_entry( album_art_ids::cover_front, *aborter ) ) {
							log("Album art: front cover found");
						}
						if ( aa->have_entry( album_art_ids::cover_back, *aborter ) ) {
							log("Album art: back cover found");
						}
						if (aa->have_entry( album_art_ids::artist, *aborter ) ) {
							log("Album art: artist picture found");
						}
					} catch(exception_album_art_not_found) {
					} catch(exception_album_art_unsupported_format) {
					}

				} catch(exception_aborted) {
					throw;
				} catch(std::exception const & e) {
					log( PFC_string_formatter() << "Failure: " << e);
				}				
			}
			log("All done.");
		}

		bool cancelTask() {
			bool ret = false;
			auto aborter = pfc::replace_null_t(m_aborter);
			if (aborter) { 
				ret = true; 
				aborter->set(); 
				logLine("Aborted by user.");
			}
			return ret;
		}

		void logLine( const char * line ) {
			m_listBox.AddString( pfc::stringcvt::string_os_from_utf8(line) );
		}

		void clearLog() {
			m_listBox.ResetContent();
		}


		// Worker thread aborter. It's re-created with the thread. If the task is ran more than once, each time it gets a new one.
		// A commonly used alternative is to have abort_callback_impl m_aborter, and a blocking cancelTask() that waits for the thread to exit, without all the shared_ptrs and recreation of aborters.
		// However that approach will freeze the UI if the worker thread is taking a long time to exit, as well as require some other shared_ptr based means for fb2k::inMainThread() ops to verify that the task is not being aborted / the dialog still exists.
		// Therefore we use a shared_ptr'd aborter, which is used both to abort worker threads, and for main thread callbacks to know if the task that sent them is still valid.
		std::shared_ptr<abort_callback_impl> m_aborter;

		// Data shared with the worker thread. It is created only once per dialog lifetime.
		std::shared_ptr< sharedData_t > m_sharedData;

		CListBox m_listBox;
		
		CStatusBarCtrl m_statusBar;

		CDialogResizeHelper m_resizer;
	};
}

void RunUIAndThreads(metadb_handle_list_cref data) {
	// Equivalent to new CDemoDialog(data), with modeless registration and auto lifetime
	fb2k::newDialog<CDemoDialog>( data );
}
