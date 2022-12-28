#pragma once

#ifdef FOOBAR2000_DESKTOP_WINDOWS

#include "../SDK/cfg_var.h"

class cfg_window_placement_common : public cfg_struct_t<WINDOWPLACEMENT> {
public:
	cfg_window_placement_common(const GUID& guid) : cfg_struct_t(guid) {}
	bool read_from_window(HWND window);
	bool apply_to_window(HWND window, bool allowHidden);
};

class cfg_window_placement : public cfg_window_placement_common
{
public:
	bool on_window_creation(HWND window, bool allowHidden = false);//returns true if window position has been changed, false if not
	void on_window_creation_silent(HWND window);
	void on_window_destruction(HWND window);
	cfg_window_placement(const GUID& guid) : cfg_window_placement_common(guid) {}
};

class cfg_window_placement_v2 : public cfg_window_placement_common {
public:
	cfg_window_placement_v2(const GUID& guid) : cfg_window_placement_common(guid) {}
	// All already in cfg_window_placement_common
};



class cfg_window_size : public cfg_struct_t<SIZE> {
public:
	bool on_window_creation(HWND window);//returns true if window position has been changed, false if not
	void on_window_destruction(HWND window);
	bool read_from_window(HWND window);
	cfg_window_size(const GUID& id) : cfg_struct_t(id) {}
};

#endif // FOOBAR2000_DESKTOP_WINDOWS
