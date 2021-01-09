#pragma once
#include "CListControl-Cells.h"

// Wrapper for old code using cell type enum constants

#define cell_text			&PFC_SINGLETON(CListCell_Text)
#define cell_multitext		&PFC_SINGLETON(CListCell_MultiText)
#define cell_hyperlink		&PFC_SINGLETON(CListCell_Hyperlink)
#define cell_button			&PFC_SINGLETON(CListCell_Button)
#define cell_button_lite	&PFC_SINGLETON(CListCell_ButtonLite)
#define cell_button_glyph	&PFC_SINGLETON(CListCell_ButtonGlyph)
#define cell_checkbox		&PFC_SINGLETON(CListCell_Checkbox)
#define cell_radiocheckbox	&PFC_SINGLETON(CListCell_RadioCheckbox)
