#include "stdafx.h"
#include <helpers/writer_wav.h>

namespace {
	// private classes in anon namespace

	typedef CWavWriter wav_writer;
	typedef wavWriterSetup_t wav_writer_setup;

	static pfc::string8 g_outputPath;
	static wav_writer g_wav_writer;

	class playback_stream_capture_callback_impl : public playback_stream_capture_callback {
	public:
		void on_chunk(const audio_chunk & chunk) override {
			PFC_ASSERT(core_api::is_main_thread());

			try {
				// writing files in main thread is not pretty, but good enough for our demo
			
				auto & abort = fb2k::noAbort;

				if (g_wav_writer.is_open() && g_wav_writer.get_spec() != chunk.get_spec()) {
					g_wav_writer.finalize(abort);
				}
				if (!g_wav_writer.is_open() && ! core_api::is_shutting_down() ) {
					wav_writer_setup setup; setup.initialize(chunk, 16, false, false);
					
					GUID g; CoCreateGuid(&g);
					pfc::string_formatter fn;
					fn << "capture-" << pfc::print_guid(g) << ".wav";
					pfc::string_formatter path = g_outputPath;
					path.add_filename( fn ); // pretty method to add file path components with auto inserted delimiter
					g_wav_writer.open(path, setup, abort);
				}
				g_wav_writer.write(chunk, abort);
			} catch(std::exception const & e) {
				FB2K_console_formatter() << "Playback stream capture error: " << e;
				// FIX ME handle this in a pretty manner, likely inaccessible output folder or out of disk space
			}
		}
	};
	static playback_stream_capture_callback_impl g_callback;
	static bool g_active = false;

	static void FlushCapture() {
		if (g_active) {
			g_wav_writer.finalize(fb2k::noAbort);
		}
	}
	static void StopCapture() {
		if ( g_active ) {
			g_active = false;
			playback_stream_capture::get()->remove_callback(&g_callback);
			g_wav_writer.finalize(fb2k::noAbort);
		}
	}
	static void StartCapture() {
		PFC_ASSERT( g_outputPath.length() > 0 );
		if (!g_active && !core_api::is_shutting_down()) {
			g_active = true;
			playback_stream_capture::get()->add_callback(&g_callback); 
		}
	}

	// Forcibly stop capture when fb2k is shutting down
	class initquit_psc : public initquit {
	public:
		void on_quit() override {
			PFC_ASSERT( core_api::is_shutting_down() );
			StopCapture();
		}
	};

	// Handle playback stop events to split output WAV files
	class play_callback_psc : public play_callback_static {
	public:
		unsigned get_flags() override {
			return flag_on_playback_stop;
		}
		void on_playback_stop(play_control::t_stop_reason p_reason) override {
			// Terminate the current stream
			FlushCapture();
		}
		void on_playback_starting(play_control::t_track_command p_command,bool p_paused) override {}
		void on_playback_new_track(metadb_handle_ptr p_track) override {}
		void on_playback_seek(double p_time) override {}
		void on_playback_pause(bool p_state) override {}
		void on_playback_edited(metadb_handle_ptr p_track) override {}
		void on_playback_dynamic_info(const file_info & p_info) override {}
		void on_playback_dynamic_info_track(const file_info & p_info) override {}
		void on_playback_time(double p_time) override {}
		void on_volume_change(float p_new_val) override {}
	};

	// pretty modern macro for service_factory_single_t<>
	FB2K_SERVICE_FACTORY( initquit_psc );
	FB2K_SERVICE_FACTORY( play_callback_psc );
}

void ToggleCapture() {
	// Block modal dialog recursions.
	// Folder picker below is a modal dialog, don't ever call it if there's another modal dialog in progress.
	// Also prevents this function from recursing into itself if someone manages to hit the menu item while already picking folder.
	// This will bump whatever modal dialog already exists, so the user has some idea why this was refused.
	if ( !ModalDialogPrologue() ) return;

	if (g_active) {
		StopCapture();
	} else {
		const HWND wndParent = core_api::get_main_window();
		modal_dialog_scope scope(wndParent); // we can't have a handle to the modal dialog, but parent handle is good enough
		if (uBrowseForFolder(wndParent, "Choose output folder", g_outputPath)) {
			StartCapture();
		}
	}
}

bool IsCaptureRunning() {
	return g_active;
}
