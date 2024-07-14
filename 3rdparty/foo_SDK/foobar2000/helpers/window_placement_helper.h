#pragma once

// DEPRECATED, NOT DPI SAFE
// use cfgDialogPosition & cfgWindowSize2 instead
#ifdef FOOBAR2000_DESKTOP_WINDOWS

#include "../SDK/cfg_var.h"

//! Window position management helpers
//! Usage: create a static instance, like with any cfg_var; access it on creation/reposition/destruction of your window.
class cfg_window_placement : public cfg_struct_t<WINDOWPLACEMENT> {
public:
	cfg_window_placement(const GUID& guid) : cfg_struct_t(guid) {}
	//! Read and save position data from HWND.
	bool read_from_window(HWND window);
	//! Apply saved position data to HWND.
	bool apply_to_window(HWND window, bool allowHidden);

	// OLD methods tracking only destroy/create.
	// Use of read_from_window() / apply_to_window() instead is preferred, so changes can be saved immediately.
	bool on_window_creation(HWND window, bool allowHidden = false);//returns true if window position has been changed, false if not
	void on_window_creation_silent(HWND window);
	void on_window_destruction(HWND window);
};

// At one point there was a separate cfg_window_placement_v2 without legacy methods
typedef cfg_window_placement cfg_window_placement_v2;

//! Window size tracker \n
//! Usage: create a static instance, like with any cfg_var; access it on creation/reposition/destruction of your window.
class cfg_window_size : public cfg_struct_t<SIZE> {
public:
	cfg_window_size(const GUID& id) : cfg_struct_t(id) {}
	//! Read and save size data from HWND.
	bool read_from_window(HWND window);
	//! Apply saved size data to HWND.
	bool apply_to_window(HWND);

	// OLD methods tracking only destroy/create.
	// Use of read_from_window() / apply_to_window() instead is preferred, so changes can be saved immediately.
	bool on_window_creation(HWND window);//returns true if window position has been changed, false if not
	void on_window_destruction(HWND window);
};

#endif // FOOBAR2000_DESKTOP_WINDOWS
