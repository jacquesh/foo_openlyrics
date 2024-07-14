#pragma  once

namespace core_api {

#ifdef _WIN32
	//! Retrieves HINSTANCE of calling DLL.
	HINSTANCE get_my_instance();
#endif
	//! Retrieves filename of calling dll, excluding extension, e.g. "foo_asdf"
	const char * get_my_file_name();
    //! Retrieves full path of calling dll, e.g. c:\blah\foobar2000\foo_asdf.dll . No file:// prefix, this path can interop with win32 API calls.
    const char * get_my_full_path();
    //! Retrieves main app window. WARNING: this is provided for parent of dialog windows and such only; using it for anything else (such as hooking windowproc to alter app behaviors) is absolutely illegal. \n
	//! Becomes valid when main window has been fully initialized. Returns NULL during creation of main window's embedded elements.
    fb2k::hwnd_t get_main_window();
	//! Tests whether services are available at this time. They are not available only during DLL startup or shutdown (e.g. inside static object constructors or destructors).
	bool are_services_available();
	//! Tests whether calling thread is main app thread, and shows diagnostic message in debugger output if it's not.
	bool assert_main_thread();
	//! Triggers a bug check if the calling thread is not the main app thread.
	void ensure_main_thread();
	//! Returns true if calling thread is main app thread, false otherwise.
	bool is_main_thread();
	//! Returns whether the app is currently shutting down.
	bool is_shutting_down();
	//! Returns whether the app is currently initializing.
	bool is_initializing();
	//! Returns filesystem path to directory with user settings, e.g. file://c:\documents_and_settings\username\blah\foobar2000
	const char * get_profile_path();
	
	//! Returns a path to <file name> in fb2k profile folder.
	inline pfc::string8 pathInProfile(const char * fileName) { pfc::string8 p( core_api::get_profile_path() ); p.add_filename( fileName ); return p; }

	//! Returns whether foobar2000 has been installed in "portable" mode.
	bool is_portable_mode_enabled();

	//! Returns whether foobar2000 is currently running in quiet mode. \n
	//! Quiet mode bypasses all GUI features, disables Media Library and does not save any changes to app configuration. \n
	//! Your component should not display any forms of user interface when running in quiet mode, as well as avoid saving configuration on its own (no need to worry if you only rely on cfg_vars or config_io_callback, they will simply be ignored on shutdown).
	bool is_quiet_mode_enabled();
};

#define FB2K_SUPPORT_LOW_MEM_MODE (SIZE_MAX <= UINT32_MAX)

namespace fb2k {
    bool isDebugModeActive();
#if FB2K_SUPPORT_LOW_MEM_MODE
	bool isLowMemModeActive();
#else
	inline constexpr bool isLowMemModeActive() { return false; }
#endif
}
