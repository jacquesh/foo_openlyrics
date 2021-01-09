#pragma once

#include <libPPUI/AutoComplete.h>

class cfg_dropdown_history_mt;

HRESULT InitializeDropdownAC(HWND comboBox, cfg_dropdown_history_mt & var, const char * initVal);
