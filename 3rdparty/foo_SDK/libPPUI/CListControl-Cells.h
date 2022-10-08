#pragma once

#include "CListControl-Cell.h"


class CListCell_Interactive : public CListCell {
public:
	bool IsInteractive() override { return true; }
};

class CListCell_Text : public CListCell {
public:
	void DrawContent( DrawContentArg_t const & ) override;
	bool AllowDrawThemeText() override { return true; }
};

class CListCell_Edit: public CListCell_Text {
public:
	bool ClickToEdit() { return true; }
};

class CListCell_TextColors : public CListCell_Text {
public:
	void DrawContent( DrawContentArg_t const & ) override;
};

class CListCell_MultiText : public CListCell {
public:
	void DrawContent( DrawContentArg_t const & ) override;
};

class CListCell_Hyperlink : public CListCell_Interactive {
public:
	void DrawContent( DrawContentArg_t const & ) override;
	bool ApplyTextStyle( LOGFONT & font, double scale, uint32_t state ) override;
	HCURSOR HotCursor() override;
	LONG AccRole() override;
	bool SuppressRowSelect() override { return true; }
};

class CListCell_Button : public CListCell_Interactive {
public:
	void DrawContent( DrawContentArg_t const & ) override;
	const char * Theme() override { return "BUTTON"; }
	bool AllowTypeFind() override { return false; }
	LONG AccRole() override;
	bool SuppressRowSelect() override { return true; }

protected:
	bool m_lite = false;
};

class CListCell_ButtonLite : public CListCell_Button {
public:
	CListCell_ButtonLite() { m_lite = true; }
};

class CListCell_ButtonGlyph : public CListCell_ButtonLite {
public:
	bool ApplyTextStyle( LOGFONT & font, double scale, uint32_t state ) override;
};

class CListCell_Checkbox : public CListCell_Interactive {
public:
	void DrawContent( DrawContentArg_t const & ) override;
	const char * Theme() override { return "BUTTON"; }
	bool IsToggle() override { return true; }
	CRect HotRect( CRect rc ) override;
	bool IsRadioToggle() override { return m_radio; }
	LONG AccRole() override;

protected:
	bool m_radio = false;
};

class CListCell_RadioCheckbox : public CListCell_Checkbox {
public:
	CListCell_RadioCheckbox() { m_radio = true; }

	static CListCell_RadioCheckbox instance;
};

class CListCell_Combo : public CListCell_Interactive {
public:
	void DrawContent(DrawContentArg_t const &) override;
	const char * Theme() override { return "COMBOBOX"; }
	LONG AccRole() override;
	uint32_t EditFlags() override;
	bool ClickToEdit() { return true; }
};

void RenderButton( HTHEME theme, CDCHandle dc, CRect rcButton, CRect rcUpdate, uint32_t cellState );
void RenderCheckbox( HTHEME theme, CDCHandle dc, CRect rcCheckBox, unsigned stateFlags, bool bRadio );


class CListCell_Text_FixedColor : public CListCell_Text {
	const COLORREF m_col;
public:
	CListCell_Text_FixedColor(COLORREF col) : m_col(col) {}
	void DrawContent(DrawContentArg_t const & arg) override;
};
