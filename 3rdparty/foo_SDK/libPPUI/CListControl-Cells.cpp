#include "stdafx.h"
#include "CListControl.h"
#include "CListControlHeaderImpl.h"
#include "CListControl-Cells.h"
#include "PaintUtils.h"
#include "GDIUtils.h"
#include <vsstyle.h>
#include "InPlaceEdit.h"
#include "DarkMode.h"

#define PRETEND_CLASSIC_THEME 0


#if PRETEND_CLASSIC_THEME
#define IsThemePartDefined(A,B,C) false
#endif

LONG CListCell::AccRole() {
	return ROLE_SYSTEM_LISTITEM;
}

void RenderCheckbox( HTHEME theme, CDCHandle dc, CRect rcCheckBox, unsigned stateFlags, bool bRadio ) {

	const int part = bRadio ? BP_RADIOBUTTON : BP_CHECKBOX;

	const bool bDisabled = (stateFlags & CListCell::cellState_disabled) != 0;
	const bool bPressed = (stateFlags & CListCell::cellState_pressed ) != 0;
	const bool bHot = ( stateFlags & CListCell::cellState_hot ) != 0;

	if (theme != NULL && IsThemePartDefined(theme, part, 0)) {
		int state = 0;
		if (bDisabled) {
			state = bPressed ? CBS_CHECKEDDISABLED : CBS_UNCHECKEDDISABLED;
		} else if ( bHot ) {
			state = bPressed ? CBS_CHECKEDHOT : CBS_UNCHECKEDHOT;
		} else {
			state = bPressed ? CBS_CHECKEDNORMAL : CBS_UNCHECKEDNORMAL;
		}

		CSize size;
		if (SUCCEEDED(GetThemePartSize(theme, dc, part, state, rcCheckBox, TS_TRUE, &size))) {
			if (size.cx <= rcCheckBox.Width() && size.cy <= rcCheckBox.Height()) {
				CRect rc = rcCheckBox;
				rc.left += ( rc.Width() - size.cx ) / 2;
				rc.top += ( rc.Height() - size.cy ) / 2;
				rc.right = rc.left + size.cx;
				rc.bottom = rc.top + size.cy;
				DrawThemeBackground(theme, dc, part, state, rc, &rc);
				return;
			}
		}
	}

	auto DPI = QueryContextDPI(dc);
	CSize size(MulDiv(13, DPI.cx, 96), MulDiv(13, DPI.cy, 96));
	CSize sizeBig = rcCheckBox.Size();
	if (sizeBig.cx >= size.cx && sizeBig.cy >= size.cy) {
		CPoint center = rcCheckBox.CenterPoint();
		rcCheckBox.left = center.x - size.cx / 2; rcCheckBox.right = rcCheckBox.left + size.cx;
		rcCheckBox.top = center.y - size.cy / 2; rcCheckBox.bottom = rcCheckBox.top + size.cy;
	}
	
	int stateEx = bRadio ? DFCS_BUTTONRADIO : DFCS_BUTTONCHECK;
	if ( bPressed ) stateEx |= DFCS_CHECKED;
	if ( bDisabled ) stateEx |= DFCS_INACTIVE;
	else if ( bHot ) stateEx |= DFCS_HOT;
	DrawFrameControl(dc, rcCheckBox, DFC_BUTTON, stateEx);
}

void RenderButton( HTHEME theme, CDCHandle dc, CRect rcButton, CRect rcUpdate, uint32_t cellState ) {

	const int part = BP_PUSHBUTTON;

	enum {
		stNormal = PBS_NORMAL,
		stHot = PBS_HOT,
		stDisabled = PBS_DISABLED,
		stPressed = PBS_PRESSED,
	};

	int state = 0;
	if (cellState & CListCell::cellState_disabled) state = stDisabled;
	if ( cellState & CListCell::cellState_pressed ) state = stPressed;
	else if ( cellState & CListCell::cellState_hot ) state = stHot;
	else state = stNormal;

	CRect rcClient  = rcButton;

	if (theme != NULL && IsThemePartDefined(theme, part, 0)) {
		DrawThemeBackground(theme, dc, part, state, rcClient, &rcUpdate);
	} else {
		int stateEx = DFCS_BUTTONPUSH;
		switch (state) {
		case stPressed: stateEx |= DFCS_PUSHED; break;
		case stDisabled: stateEx |= DFCS_INACTIVE; break;
		}
		DrawFrameControl(dc, rcClient, DFC_BUTTON, stateEx);
	}
}

bool CListCell::ApplyTextStyle( LOGFONT & font, double scale, uint32_t ) { 
	if ( scale != 1.0 ) {
		font.lfHeight = pfc::rint32( font.lfHeight * scale );
		return true;
	} else {
		return false;
	}
}

void CListCell_Text::DrawContent( DrawContentArg_t const & arg ) {
	const auto fgWas = arg.dc.GetTextColor();
	CDCHandle dc = arg.dc;
	if ((arg.cellState & cellState_disabled) != 0 && arg.allowColors) {
		dc.SetTextColor(DarkMode::GetSysColor(COLOR_GRAYTEXT, arg.darkMode));
	}
	
	CRect clip = arg.rcText;

	if (arg.imageRenderer && clip.Width() > clip.Height() ) {
		CRect rcImage = clip; rcImage.right = rcImage.left + clip.Height();
		arg.imageRenderer(dc, rcImage);
		clip.left = rcImage.right;
	}

	const t_uint32 format = PaintUtils::DrawText_TranslateHeaderAlignment(arg.hdrFormat);
	dc.DrawText( arg.text, (int)wcslen(arg.text), clip, format | DT_NOPREFIX | DT_END_ELLIPSIS | DT_SINGLELINE | DT_VCENTER  );

	dc.SetTextColor(fgWas);
}

void CListCell_TextColors::DrawContent( DrawContentArg_t const & arg ) {
	CDCHandle dc = arg.dc;
	
	CRect clip = arg.rcText;

	const uint32_t fgWas = dc.GetTextColor();

	const t_uint32 format = PaintUtils::DrawText_TranslateHeaderAlignment(arg.hdrFormat);
	const t_uint32 bk = dc.GetBkColor();
	const t_uint32 fg = fgWas;
	const t_uint32 hl = (arg.allowColors ? arg.colorHighlight : fg);
	const t_uint32 colors[3] = { PaintUtils::BlendColor(bk, fg, 33), fg, hl };

	PaintUtils::TextOutColorsEx(dc, arg.text, clip, format, colors);

	dc.SetTextColor(fgWas);
}

void CListCell_MultiText::DrawContent( DrawContentArg_t const & arg ) {
	CDCHandle dc = arg.dc;

	const int textLen = (int) wcslen( arg.text );

	CRect clip = arg.rcText;

	const t_uint32 format = PaintUtils::DrawText_TranslateHeaderAlignment(arg.hdrFormat) | DT_NOPREFIX | DT_VCENTER ;

	CRect rcDraw = clip;
	dc.DrawText(arg.text, textLen, rcDraw, format | DT_CALCRECT);
	auto txSize = rcDraw.Size();
	rcDraw = clip;
	if ( txSize.cy < rcDraw.Height() ) {
		int sub = rcDraw.Height() - txSize.cy;
		rcDraw.top += sub/2;
		rcDraw.bottom = rcDraw.top + txSize.cy;
	}
	dc.DrawText(arg.text, textLen, rcDraw, format);
}

bool CListCell_Hyperlink::ApplyTextStyle( LOGFONT & font, double scale, uint32_t state ) {
	bool rv = __super::ApplyTextStyle(font, scale, state);

	if ( state & cellState_hot ) {
		font.lfUnderline = TRUE;
		rv = true;
	}

	return rv;
}

HCURSOR CListCell_Hyperlink::HotCursor() {
	return LoadCursor(NULL, IDC_HAND);
}

LONG CListCell_Hyperlink::AccRole() {
	return ROLE_SYSTEM_LINK;
}

void CListCell_Hyperlink::DrawContent( DrawContentArg_t const & arg ) {

	CDCHandle dc = arg.dc;

	const uint32_t fgWas = dc.GetTextColor();

	const t_uint32 format = PaintUtils::DrawText_TranslateHeaderAlignment(arg.hdrFormat);
	if (arg.allowColors) dc.SetTextColor( arg.colorHighlight );
	// const t_uint32 bk = dc.GetBkColor();

	CRect rc = arg.rcText;
	dc.DrawText(arg.text, (int) wcslen(arg.text), rc, format | DT_NOPREFIX | DT_END_ELLIPSIS | DT_SINGLELINE | DT_VCENTER );

	dc.SetTextColor(fgWas);
}

LONG CListCell_Button::AccRole() {
	return ROLE_SYSTEM_PUSHBUTTON;
}

void CListCell_Button::DrawContent( DrawContentArg_t const & arg ) {

	CDCHandle dc = arg.dc;

	const bool bPressed = (arg.cellState & cellState_pressed) != 0;
	const bool bHot = (arg.cellState & cellState_hot) != 0;

	
	if ( !m_lite || bHot || bPressed ) {
		RenderButton( arg.theme, dc, arg.rcHot, arg.rcHot, arg.cellState );
	}

	CRect clip = arg.rcText;

	dc.DrawText(arg.text, (int) wcslen(arg.text), clip, DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER | DT_CENTER);
}

bool CListCell_ButtonGlyph::ApplyTextStyle( LOGFONT & font, double scale, uint32_t state ) {
	return __super::ApplyTextStyle(font, scale * 1.3, state);
}

static CRect CheckBoxRect(CRect rc) {
	if (rc.Width() > rc.Height()) {
		rc.right = rc.left + rc.Height();
	}
	return rc;
}

LONG CListCell_Checkbox::AccRole()  {
	return m_radio ? ROLE_SYSTEM_RADIOBUTTON : ROLE_SYSTEM_CHECKBUTTON;
}

CRect CListCell_Checkbox::HotRect( CRect rc ) {
	return CheckBoxRect( rc );
}

void CListCell_Checkbox::DrawContent( DrawContentArg_t const & arg ) {
	
	CDCHandle dc = arg.dc;
	
	// const bool bPressed = (arg.cellState & cellState_pressed) != 0;
	// const bool bHot = (arg.cellState & cellState_hot) != 0;
	

	// CRect clip = arg.rcText;

	const uint32_t fgWas = dc.GetTextColor();

	if (arg.subItemRect.Width() > arg.subItemRect.Height() ) {
		CRect rcCheckbox = arg.subItemRect;
		rcCheckbox.right = rcCheckbox.left + rcCheckbox.Height();
		RenderCheckbox(arg.theme, dc, rcCheckbox, arg.cellState, m_radio );
		CRect rcText = arg.subItemRect;
		rcText.left = rcCheckbox.right;
		if (arg.cellState & cellState_disabled) {
			dc.SetTextColor(GetSysColor(COLOR_GRAYTEXT));
		}

		if (arg.imageRenderer && rcText.Width() > rcText.Height()) {
			CRect rcImage = rcText; rcImage.right = rcImage.left + rcImage.Height();
			arg.imageRenderer(dc, rcImage);
			rcText.left = rcImage.right;
		}

		dc.DrawText(arg.text, (int) wcslen(arg.text), rcText, DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER | DT_LEFT);
	} else {
		RenderCheckbox(arg.theme, dc, arg.subItemRect, arg.cellState, m_radio );
	}

	dc.SetTextColor(fgWas);
}

void CListCell_Text_FixedColor::DrawContent(DrawContentArg_t const & arg) {
	if (arg.allowColors) {
		SetTextColorScope scope(arg.dc, m_col);
		__super::DrawContent(arg);
	} else {
		__super::DrawContent(arg);
	}
}

uint32_t CListCell_Combo::EditFlags() {
	return InPlaceEdit::KFlagCombo;
}

void CListCell_Combo::DrawContent(DrawContentArg_t const & arg) {
	CDCHandle dc = arg.dc;

	const bool bDisabled = (arg.cellState & CListCell::cellState_disabled) != 0;
	const bool bPressed = (arg.cellState & cellState_pressed) != 0;
	const bool bHot = (arg.cellState & cellState_hot) != 0;

	const int part = CP_DROPDOWNBUTTONRIGHT;

	const HTHEME theme = arg.theme;

	const int w = MulDiv(16, GetDeviceCaps(dc, LOGPIXELSX), 96);
	CRect rcText = arg.rcText;
	if (theme != NULL && IsThemePartDefined(theme, part, 0)) {
		int state = CBXSR_NORMAL;
		if (bDisabled) {
			state = CBXSR_DISABLED;
		} else if (bPressed) {
			state = CBXSR_PRESSED;
		} else if (bHot) {
			state = CBXSR_HOT;
		}

		CSize size;
		CRect rcCombo = arg.subItemRect;
		if (w < rcCombo.Width()) {
			rcCombo.left = rcCombo.right - w;
			DrawThemeBackground(theme, dc, part, state, rcCombo, &rcCombo);
			if (rcCombo.left < rcText.right ) rcText.right = rcCombo.left;
		}
	} else {
		CRect rcCombo = arg.subItemRect;
		if (w < rcCombo.Width()) {
			rcCombo.left = rcCombo.right - w;
			if (rcCombo.left < rcText.right) rcText.right = rcCombo.left;

			if (bHot) {
				DrawFrameControl(dc, rcCombo, DFC_BUTTON, DFCS_BUTTONPUSH | DFCS_HOT);
			}
			dc.DrawText(L"˅", 1, rcCombo, DT_VCENTER | DT_CENTER | DT_SINGLELINE);
		}

	}

	DrawContentArg_t arg2 = arg;
	arg2.rcText = rcText;
	PFC_SINGLETON(CListCell_Text).DrawContent(arg2);
}

LONG CListCell_Combo::AccRole() {
	return ROLE_SYSTEM_DROPLIST;
}