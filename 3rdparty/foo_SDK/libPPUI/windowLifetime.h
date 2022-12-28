#pragma once
#include "ImplementOnFinalMessage.h"
#include <functional>

namespace PP {
	//! Create a new window object - obj_t derived from CWindowImpl<> etc, with automatic lifetime management, \n
	//! OnFinalMessage() is automatically overridden to delete the window object. \n
	//! Does NOT create the window itself, it's up to you to call Create() or SubclassWindow() with the returned object.
	template<typename obj_t, typename ... arg_t>
	obj_t* newWindowObj(arg_t && ... arg) {
		return new ImplementOnFinalMessage<obj_t>(std::forward<arg_t>(arg) ...);
	}

	//! Subclass HWND with window class object obj_t deriving from CWindowImpl<> etc, with automatic lifetime management of the object, \n
	//! OnFinalMessage() is automatically overridden to delete the window subclass object.
	template<typename obj_t, typename ... arg_t>
	obj_t* subclassThisWindow(HWND wnd, arg_t && ... arg) {
		auto ret = newWindowObj<obj_t>(std::forward<arg_t>(arg) ...);
		WIN32_OP_D(ret->SubclassWindow(wnd));
		return ret;
	}
}