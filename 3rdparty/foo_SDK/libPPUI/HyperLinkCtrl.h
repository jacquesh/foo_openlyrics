#pragma once
#include <functional>
namespace PP {
	// One-line method to turn static control to hyperlink firing WM_NOTIFY
	void createHyperLink(HWND wndReplaceMe);
	void createHyperLink(HWND wndReplaceMe, std::function<void ()> handler);
	void createHyperLink(HWND wndReplaceMe, const wchar_t * openURL);
}