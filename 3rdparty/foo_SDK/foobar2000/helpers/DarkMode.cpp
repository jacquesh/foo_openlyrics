#include "StdAfx.h"
#include "DarkMode.h"

bool fb2k::isDarkMode() {
#if FOOBAR2000_TARGET_VERSION >= 81
	return ui_config_manager::get()->is_dark_mode();
#else
	auto api = ui_config_manager::tryGet();
	if (api.is_valid()) return api->is_dark_mode();
	else return false;
#endif
}
