#pragma once

#include <libPPUI/AutoComplete.h>

#include "dropdown_helper.h"

HRESULT InitializeDropdownAC(HWND comboBox, cfg_dropdown_history_mt & var, const char * initVal);
