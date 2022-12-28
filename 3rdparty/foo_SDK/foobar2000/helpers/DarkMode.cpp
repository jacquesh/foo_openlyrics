#include "StdAfx.h"
#include "DarkMode.h"

bool fb2k::isDarkMode() {
	auto api = ui_config_manager::tryGet();
	if (api.is_valid()) return api->is_dark_mode();
	else return false;
}
