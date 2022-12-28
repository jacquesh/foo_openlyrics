#pragma once

namespace PP {
	//! Single-function-call API to add middle click drag capability to any standard Windows control. \n
	//! Works with listviews, listboxes, treeviews, etc. \n
	//! Window will be subclassed, lifetime of subclass managed automatically. No further interaction needed from your code. \n
	//! If you also subclass it to handle middle click events, pay attention to order in which you install the subclasses to receive your events.
	void addMiddleDragToCtrl(HWND wndCtrl);
}
