#pragma once

#ifdef _WIN32

#include "../SDK/ui_element.h"
#include <libPPUI/DarkMode.h>

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
