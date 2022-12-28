#pragma once

#include "windowLifetime.h"

namespace PP {
	// CContainedWindow replacement
	// Rationale: 
	// With CContainedWindow, all messages that the window gets are routed through the message map of your class.
	// The window might bounce some of the messages to other windows (say, mouse4/5 handlers, WM_CONTEXTMENU), or do modal loops.
	// If some events triggered by those destroy your CContainedWindow host, crash happens, because your object is still on stack.
	// hookWindowMessages() sends only the messages you ask for to your class, also managing hook lifetime behind the scenes.
	void hookWindowMessages(HWND wnd, CMessageMap* target, DWORD targetID, std::initializer_list<DWORD>&& msgs);

	typedef std::function<bool(HWND, UINT, WPARAM, LPARAM, LRESULT&) > messageHook_t;
	void hookWindowMessages(HWND wnd, messageHook_t h);
}