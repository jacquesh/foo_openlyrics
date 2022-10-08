#pragma once

#include "DarkMode.h"

namespace DarkMode {
	void PaintTabsErase(CTabCtrl, CDCHandle);
	void PaintTabs(CTabCtrl, CDCHandle, const RECT* rcPaint = nullptr);
}