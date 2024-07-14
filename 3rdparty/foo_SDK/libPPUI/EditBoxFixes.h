#pragma once

namespace PP {
	// One-line methods to inject our edit box shims: Ctrl+A, Ctrl+Backspace, etc
	void editBoxFix(HWND wndEdit);
	void comboBoxFix(HWND wndCombo);
}