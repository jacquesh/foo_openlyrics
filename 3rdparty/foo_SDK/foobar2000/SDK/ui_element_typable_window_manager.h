#pragma once

// API obsoleted in 1.5 and should no longer be used by anything
// Core-implemented to keep components that specifically rely on it working

class NOVTABLE ui_element_typable_window_manager : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(ui_element_typable_window_manager);
public:
	virtual void add(HWND wnd) = 0;
	virtual void remove(HWND wnd) = 0;
	virtual bool is_registered(HWND wnd) = 0;
};

FOOGUIDDECL const GUID ui_element_typable_window_manager::class_guid = { 0xbaa99ee2, 0xf770, 0x4981,{ 0x9e, 0x50, 0xf3, 0x4c, 0x5c, 0x6d, 0x98, 0x81 } };
