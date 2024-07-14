#pragma once

#ifdef _WIN32

#include <SDK/ui_element.h>
#include <libPPUI/DarkMode.h>

// fb2k::CDarkModeHooks
// foobar2000 specific helper on top of libPPUI DarkMode::CHooks
// Automatically determines whether dark mode should be on or off
// Keeps track of dark mode preference changes at runtime
// Does nothing if used in foobar2000 older than 2.0

// IMPORTANT
// See also: SDK/coreDarkMode.h
// Using CCoreDarkMode lets you invoke foobar2000's instance of this code instead of static linking it, resulting in much smaller component binary.
// Using CDarkModeHooks directly is good mainly for debugging or troubleshooting.

namespace fb2k {
	bool isDarkMode();

#ifndef CDarkModeHooks
	class CDarkModeHooks : public DarkMode::CHooks, private ui_config_callback_impl {
	public:
		CDarkModeHooks() : CHooks(isDarkMode()) {}

	private:
		void ui_fonts_changed() override {}
		void ui_colors_changed() override { this->SetDark(isDarkMode()); }
	};
#endif
}

#endif
