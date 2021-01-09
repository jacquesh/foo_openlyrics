#include "stdafx.h"
#include <vsstyle.h>

#include "PaintUtils.h"
#include "gdiplus_helpers.h"
// #include <helpers/win32_misc.h>
// #include <ATLHelpers/GDIUtils.h>

#include "GDIUtils.h"
#include "win32_op.h"
#include "wtl-pp.h"

namespace PaintUtils {
	static t_uint16 extractChannel16(t_uint32 p_color,int p_which) throw() {
		return (t_uint16)( ((p_color >> (p_which * 8)) & 0xFF) << 8 );
	}

	static t_uint8 extractbyte(t_uint32 p_val,t_size p_which) throw() {
		return (t_uint8) ( (p_val >> (p_which * 8)) & 0xFF );
	}

	t_uint32 BlendColorEx(t_uint32 p_color1, t_uint32 p_color2, double mix) throw() {
		PFC_ASSERT(mix >= 0 && mix <= 1);
		t_uint32 ret = 0;
		for(t_size walk = 0; walk < 3; ++walk) {
			int val1 = extractbyte(p_color1,walk), val2 = extractbyte(p_color2,walk);
			int val = val1 + pfc::rint32((val2 - val1) * mix);
			ret |= (t_uint32)val << (walk * 8);
		}
		return ret;
	}
	t_uint32 BlendColor(t_uint32 p_color1, t_uint32 p_color2, int p_percentage) throw() {
		PFC_ASSERT(p_percentage <= 100);
		t_uint32 ret = 0;
		for(t_size walk = 0; walk < 3; ++walk) {
			int val1 = extractbyte(p_color1,walk), val2 = extractbyte(p_color2,walk);
			int val = val1 + MulDiv(val2 - val1,p_percentage,100);
			ret |= (t_uint32)val << (walk * 8);
		}
		return ret;
	}
	t_uint32 DriftColor(t_uint32 p_color,unsigned p_delta,bool p_direction) throw() {
		t_uint32 ret = 0;
		for(t_size walk = 0; walk < 3; ++walk) {
			unsigned val = extractbyte(p_color,walk);
			if (p_direction) val = 0xFF - val;
			if (val < p_delta) val = p_delta;
			val += p_delta;
			if (val > 0xFF) val = 0xFF;
			if (p_direction) val = 0xFF - val;
			ret |= (t_uint32)val << (walk * 8);
		}
		return ret;
	}

	void FillVertexColor(TRIVERTEX & p_vertex,t_uint32 p_color,t_uint16 p_alpha) throw() {
		p_vertex.Red = extractChannel16(p_color,0);
		p_vertex.Green = extractChannel16(p_color,1);
		p_vertex.Blue = extractChannel16(p_color,2);
		p_vertex.Alpha = p_alpha;
	}

	void FillRectSimple(CDCHandle p_dc,const CRect & p_rect,t_uint32 p_color) throw() {
		p_dc.FillSolidRect(p_rect, p_color);
	}

	void GradientFillRect(CDCHandle p_dc,const CRect & p_rect,t_uint32 p_color1, t_uint32 p_color2, bool p_horizontal) throw() {
		TRIVERTEX verticies[2];
		GRADIENT_RECT element = {0,1};
		FillVertexColor(verticies[0],p_color1);
		FillVertexColor(verticies[1],p_color2);
		verticies[0].x = p_rect.left; verticies[0].y = p_rect.top;
		verticies[1].x = p_rect.right; verticies[1].y = p_rect.bottom;
		WIN32_OP_D( p_dc.GradientFill(verticies,tabsize(verticies),&element,1,p_horizontal ? GRADIENT_FILL_RECT_H : GRADIENT_FILL_RECT_V) );
	}

	void GradientSplitRect(CDCHandle p_dc,const CRect & p_rect,t_uint32 p_bkColor, t_uint32 p_gradientColor,int p_splitPercent) throw() {
		const long split = p_rect.top + MulDiv(p_rect.Height(),p_splitPercent,100);
		CRect rcTemp;
		rcTemp = p_rect;
		rcTemp.bottom = split;
		GradientFillRect(p_dc,rcTemp,p_bkColor,p_gradientColor,false);
		rcTemp = p_rect;
		rcTemp.top = split;
		GradientFillRect(p_dc,rcTemp,p_gradientColor,p_bkColor,false);
	}

	void GradientBar(CDCHandle p_dc,const CRect & p_rect,t_uint32 p_exterior, t_uint32 p_interior, int p_percentage) throw() {
		const int gradientPix = MulDiv(p_rect.Height(),p_percentage,100);
		CRect rcTemp;

		rcTemp = p_rect;
		rcTemp.bottom = rcTemp.top + gradientPix;
		GradientFillRect(p_dc,rcTemp,p_exterior,p_interior,false);

		rcTemp = p_rect;
		rcTemp.top += gradientPix; rcTemp.bottom -= gradientPix;
		FillRectSimple(p_dc,rcTemp,p_interior);
		
		rcTemp = p_rect;
		rcTemp.top = rcTemp.bottom - gradientPix;
		GradientFillRect(p_dc,rcTemp,p_interior,p_exterior,false);
	}

	void RenderItemBackground(CDCHandle p_dc,const CRect & p_itemRect,t_size p_item,t_uint32 p_color) throw() {
		const DWORD bkColor_base = p_color;
		const DWORD bkColor = DriftColor(bkColor_base,3, (p_item&1) != 0);

		//GradientSplitRect(p_dc,p_itemRect,bkColor,BlendColor(bkColor,textColor,7),80);
		GradientBar(p_dc,p_itemRect,bkColor_base,bkColor,10);
	}

	double Luminance(t_uint32 color) throw() {
		double r = extractbyte(color,0), g = extractbyte(color,1), b = extractbyte(color,2);
		return (0.2126 * r + 0.7152 * g + 0.0722 * b) / 255.0;
		//return (r * 0.3 + g * 0.59 + b * 0.11) / 255.0;
	}
	t_uint32 DetermineTextColor(t_uint32 bk) throw() {
		double l = Luminance(bk);
		if ( l > 0.6 ) {
			return 0; // black
		} else {
			return 0xFFFFFF; // white
		}
	}

	void AddRectToRgn(HRGN p_rgn,CRect const & p_rect) throw() {
		CRgn temp; 
		WIN32_OP_D( temp.CreateRectRgnIndirect(p_rect) != NULL );
		CRgnHandle(p_rgn).CombineRgn(temp,RGN_OR);
	}

	void FocusRect2(CDCHandle dc, CRect const & rect, COLORREF bkColor) throw() {
		COLORREF txColor = DetermineTextColor( bkColor );
		COLORREF useColor = BlendColor(bkColor, txColor, 50);
		CDCBrush brush(dc, useColor);
		WIN32_OP_D( dc.FrameRect(rect,brush) );
	}
	void FocusRect(CDCHandle dc, CRect const & rect) throw() {
		CDCBrush brush(dc, 0x7F7F7F);
		WIN32_OP_D( dc.FrameRect(rect,brush) );
		//dc.DrawFocusRect(rect);
	}

	namespace TrackBar {
		void DrawThumb(HTHEME theme,HDC dc,int state,const RECT * rcThumb, const RECT * rcUpdate) {
			if (theme == NULL) {
				RECT blah = *rcThumb;
				int flags = DFCS_BUTTONPUSH;
				switch(state) {
					case TUS_NORMAL:
						break;
					case TUS_DISABLED:
						flags |= DFCS_INACTIVE;
						break;
					case TUS_PRESSED:
						flags |= DFCS_PUSHED;
						break;
				}
				DrawFrameControl(dc,&blah,DFC_BUTTON,flags);
			} else {
				DrawThemeBackground(theme,dc,TKP_THUMB,state,rcThumb,rcUpdate);
			}
		}
		void DrawTrack(HTHEME theme,HDC dc,const RECT * rcTrack, const RECT * rcUpdate) {
			if (theme == NULL) {
				RECT blah = *rcTrack;
				DrawFrameControl(dc,&blah,DFC_BUTTON,DFCS_BUTTONPUSH|DFCS_PUSHED);
			} else {
				DrawThemeBackground(theme,dc,TKP_TRACK,TKS_NORMAL,rcTrack,rcUpdate);
			}
		}

		void DrawTrackVolume(HTHEME theme,HDC p_dc,const RECT * rcTrack, const RECT * rcUpdate) {
			CMemoryDC dc(p_dc,CRect(rcUpdate));
			CRect rc(*rcTrack);
			CRect update(*rcUpdate);

			WIN32_OP_D( dc.BitBlt(update.left,update.top,update.Width(),update.Height(),p_dc,update.left,update.top,SRCCOPY) );


			
			/*CDCHandle dc(p_dc);
			CPen pen; pen.CreatePen(PS_SOLID,1, GetSysColor(COLOR_GRAYTEXT));
			SelectObjectScope scope(dc, pen);
			dc.MoveTo(rc.left,rc.bottom);
			dc.LineTo(rc.right,rc.bottom);
			dc.LineTo(rc.right,rc.top);
			dc.LineTo(rc.left,rc.bottom);*/
			
			//DrawTrack(theme,dc,rcTrack,rcUpdate);

			try {
				Gdiplus::Point points[] = { Gdiplus::Point(rc.left, rc.bottom), Gdiplus::Point(rc.right, rc.bottom), Gdiplus::Point(rc.right, rc.top) } ;
				GdiplusErrorHandler eh;
				Gdiplus::Graphics graphics(dc);
				eh << graphics.GetLastStatus();
				Gdiplus::Color c;
				c.SetFromCOLORREF( GetSysColor(COLOR_BTNHIGHLIGHT) );
				Gdiplus::Pen penHL(c);
				c.SetFromCOLORREF( GetSysColor(COLOR_BTNSHADOW) );
				Gdiplus::Pen penSH(c);
				eh << graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
				//graphics.DrawPolygon(&pen,points,tabsize(points));
				eh << graphics.DrawLine(&penSH, points[0], points[0] + Gdiplus::Point(0,-1));
				eh << graphics.DrawLine(&penHL, points[0], points[1]);
				eh << graphics.DrawLine(&penHL, points[1], points[2]);
				eh << graphics.DrawLine(&penSH, points[2], points[0] + Gdiplus::Point(0,-1));
			} catch(std::exception const & e) {
				(void) e;
				// console::print(e.what());
			}
		}
	}

	void DrawSmoothedLine(HDC dc, CPoint pt1, CPoint pt2, COLORREF col, double width) {
		try {
			Gdiplus::Point points[] = { Gdiplus::Point(pt1.x,pt1.y), Gdiplus::Point(pt2.x,pt2.y) };
			GdiplusErrorHandler eh;
			Gdiplus::Graphics graphics(dc);
			eh << graphics.GetLastStatus();
			Gdiplus::Color c;
			c.SetFromCOLORREF( col );
			Gdiplus::Pen pen(c, (Gdiplus::REAL)( width ));
			eh << graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
			//graphics.DrawPolygon(&pen,points,tabsize(points));
			eh << graphics.DrawLine(&pen, points[0], points[1]);
		} catch(std::exception const & e) {
			(void) e;
			// console::print(e.what());
		}
	}



	static int get_text_width(HDC dc,const TCHAR * src,int len) {
		if (len<=0) return 0;
		else {
			SIZE goatse;
			GetTextExtentPoint32(dc,src,len,&goatse);
			return goatse.cx;
		}
	}

	static t_uint32 TextOutColors_TranslateColor(const t_uint32 colors[3], int offset) {
		const double v = (double)offset / 3.0;
		if (v <= -1) return colors[0];
		else if (v < 0) return BlendColorEx(colors[0], colors[1], v + 1);
		else if (v == 0) return colors[1];
		else if (v < 1) return BlendColorEx(colors[1], colors[2], v);
		else return colors[2];
	}

	void TextOutColors_StripCodesAppend(pfc::string_formatter & out, const char * in) {
		t_size done = 0, walk = 0;
		for(;;) {
			if (in[walk] == 0) {
				if (walk > done) out.add_string_nc(in + done, walk - done);
				return;
			}
			if ((unsigned)in[walk] < 32) {
				if (walk > done) {out.add_string_nc(in + done, walk - done);}
				done = walk + 1;
			}
			++walk;
		}
	}
	void TextOutColors_StripCodes(pfc::string_formatter & out, const char * in) {
		out.reset(); TextOutColors_StripCodesAppend(out, in);
	}

	static bool IsControlChar(TCHAR c) {
		return (unsigned)c < 32;
	}
	static int MatchTruncat(HDC dc, int & pixels, const TCHAR * text, int textLen) {
		int min = 0, max = textLen;
		int minWidth = 0;
		while(min + 1 < max) {
			const int probe = (min + max) / 2;
			CSize size;
			WIN32_OP( GetTextExtentPoint32(dc, text, probe, &size) );
			if (size.cx <= pixels) {min = probe; minWidth = size.cx;}
			else max = probe;
		}
		pixels = minWidth;
		return min;
	}
	static int TruncatHeadroom(HDC dc) {
		CSize size;
		WIN32_OP( GetTextExtentPoint32(dc, _T("\x2026"), 1, &size) );
		return size.cx;
	}
	static void ExtTextOut_Truncat(HDC dc, int x, int y, CRect const & clip, const TCHAR * text, int textLen) {
		int width = pfc::max_t<int>(0, clip.right - x - TruncatHeadroom(dc));
		int truncat = MatchTruncat(dc, width, text, textLen);
		WIN32_OP( ExtTextOut(dc, x, y, ETO_CLIPPED, &clip, text, truncat, NULL) );
		WIN32_OP( ExtTextOut(dc, x + width, y, ETO_CLIPPED, &clip, _T("\x2026"), 1, NULL) );
		
		
	}
	bool TextContainsCodes(const TCHAR * src) {
		for(;;) {
			if (*src == 0) return false;
			if ((unsigned)*src < 32) return true;
			++src;
		}
	}
	void TextOutColorsEx(HDC dc,const TCHAR * src,const CRect & target,DWORD flags,const t_uint32 colors[3]) {
		if (!TextContainsCodes(src)) {
			SetTextColorScope cs(dc, colors[1]);
			CRect rc(target);
			CDCHandle(dc).DrawText(src,(int)_tcslen(src),rc,DT_NOPREFIX | DT_END_ELLIPSIS | DT_SINGLELINE | DT_VCENTER | flags);
		} else {
			const CSize textSize = PaintUtils::TextOutColors_CalcSize(dc, src);
			CPoint origin = target.TopLeft();
			origin.y = (target.top + target.bottom - textSize.cy) / 2;
			switch(flags & (DT_LEFT | DT_RIGHT | DT_CENTER)) {
				case DT_LEFT:
					break;
				case DT_RIGHT:
					if (textSize.cx < target.Width()) origin.x = target.right - textSize.cx;
					break;
				case DT_CENTER:
					if (textSize.cx < target.Width()) origin.x = (target.right + target.left - textSize.cx) / 2;
					break;
			}
			TextOutColors(dc, src, (int)_tcslen(src), origin, target, colors);
		}
	}
	void TextOutColors(HDC dc,const TCHAR * src,int len,CPoint offset,const CRect & clip,const t_uint32 colors[3], int tabWidthTotal, int tabWidthDiv) {
		SetTextAlign(dc,TA_LEFT);
		SetBkMode(dc,TRANSPARENT);
		
	
		int walk = 0;
		int position = offset.x;
		int colorOffset = 0;
		int tabs = 0;
		int positionTabDelta = 0;

		for(;;) {
			int base = walk;
			while(walk < len && !IsControlChar(src[walk])) ++walk;
			if (walk>base) {
				SetTextColor(dc,TextOutColors_TranslateColor(colors, colorOffset));
				int width = get_text_width(dc,src+base,walk-base);
				if (position + width > clip.right) {
					ExtTextOut_Truncat(dc, position, offset.y, clip, src + base, walk - base);
					return;
				}
				WIN32_OP( ExtTextOut(dc,position,offset.y,ETO_CLIPPED,&clip,src+base,walk-base,0) );
				position += width;
			}
			if (walk>=len) break;
			
			while(walk < len && IsControlChar(src[walk])) {
				if (src[walk] == TextOutColors_Dim) --colorOffset;
				else if (src[walk] == TextOutColors_Highlight) ++colorOffset;
				else if (src[walk] == '\t') {
					int newDelta = MulDiv(++tabs, tabWidthTotal, tabWidthDiv);
					position += newDelta - positionTabDelta;
					positionTabDelta = newDelta;
				}
				walk++;
			}
		}
	}

	CSize TextOutColors_CalcSize(HDC dc, const TCHAR * src) {
		CSize acc(0,0);
		for(int walk = 0;;) {
			const int done = walk;
			while(!IsControlChar(src[walk])) ++walk;
			if (walk > done) {
				CSize temp;
				WIN32_OP( GetTextExtentPoint32(dc,src + done, walk - done, &temp) );
				acc.cx += temp.cx; pfc::max_acc(acc.cy, temp.cy);
			}
			if (src[walk] == 0) return acc;
			while(src[walk] != 0 && IsControlChar(src[walk])) ++walk;
		}
	}
	t_uint32 TextOutColors_CalcWidth(HDC dc, const TCHAR * src) {
		t_uint32 acc = 0;
		for(int walk = 0;;) {
			const int done = walk;
			while(!IsControlChar(src[walk])) ++walk;
			acc += get_text_width(dc, src + done, walk - done);
			if (src[walk] == 0) return acc;
			while(src[walk] != 0 && IsControlChar(src[walk])) ++walk;
		}
	}
	static const unsigned Marker_Dim = '<', Marker_Highlight = '>';
	
	pfc::string TextOutColors_ImportScript(pfc::string script) {
		pfc::string_formatter temp; TextOutColors_ImportScript(temp, script.ptr()); return temp.get_ptr();
	}
	void TextOutColors_ImportScript(pfc::string_base & out, const char * in) {
		out.reset();
		for(;;) {
			t_size delta; t_uint32 c;
			delta = pfc::utf8_decode_char(in, c);
			if (delta == 0) break;
			switch(c) {
				case '>':
					c = PaintUtils::TextOutColors_Highlight;
					break;
				case '<':
					c = PaintUtils::TextOutColors_Dim;
					break;
			}
			out.add_char(c);
			in += delta;
		}
	}
	t_uint32 DrawText_TranslateHeaderAlignment(t_uint32 val) {
		switch(val & HDF_JUSTIFYMASK) {
			case HDF_LEFT:
			default:
				return DT_LEFT;
			case HDF_RIGHT:
				return DT_RIGHT;
			case HDF_CENTER:
				return DT_CENTER;
		}
	}

	void RenderButton(HWND wnd_, HDC dc_, CRect rcUpdate, bool bPressed) {
		CDCHandle dc(dc_); CWindow wnd(wnd_);
		CTheme theme; theme.OpenThemeData(wnd, L"BUTTON");

		RelayEraseBkgnd(wnd, wnd.GetParent(), dc);

		const int part = BP_PUSHBUTTON;

		enum {
			stNormal = PBS_NORMAL,
			stHot = PBS_HOT,
			stDisabled = PBS_DISABLED,
			stPressed = PBS_PRESSED,
		};

		int state = 0;
		if (!wnd.IsWindowEnabled()) state = stDisabled;
		else if (bPressed) state = stPressed;
		else state = stNormal;

		CRect rcClient; WIN32_OP_D( wnd.GetClientRect(rcClient) );

		if (theme != NULL && IsThemePartDefined(theme, part, 0)) {
			DrawThemeBackground(theme, dc, part, state, rcClient, &rcUpdate);
		} else {
			int stateEx = DFCS_BUTTONPUSH;
			switch(state) {
			case stPressed: stateEx |= DFCS_PUSHED; break;
			case stDisabled: stateEx |= DFCS_INACTIVE; break;			
			}
			DrawFrameControl(dc, rcClient, DFC_BUTTON, stateEx);
		}
	}


	void PaintSeparatorControl(HWND wnd_) {
		CWindow wnd(wnd_);
		CPaintDC dc(wnd);
		TCHAR buffer[512] = {};
		wnd.GetWindowText(buffer, _countof(buffer));
		const int txLen = (int) pfc::strlen_max_t(buffer, _countof(buffer));
		CRect contentRect;
		WIN32_OP_D(wnd.GetClientRect(contentRect));

		dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
		dc.SetBkMode(TRANSPARENT);

		{
			CBrushHandle brush = (HBRUSH)wnd.GetParent().SendMessage(WM_CTLCOLORSTATIC, (WPARAM)(HDC)dc, (LPARAM)wnd.m_hWnd);
			if (brush != NULL) dc.FillRect(contentRect, brush);
		}
		SelectObjectScope scopeFont(dc, wnd.GetFont());

		if (txLen > 0) {
			CRect rcText(contentRect);
			if (!wnd.IsWindowEnabled()) {
				dc.SetTextColor(GetSysColor(COLOR_GRAYTEXT));
			}
			WIN32_OP_D(dc.DrawText(buffer, txLen, rcText, DT_NOPREFIX | DT_END_ELLIPSIS | DT_SINGLELINE | DT_VCENTER | DT_LEFT) > 0);
			//			WIN32_OP_D( dc.GrayString(NULL, NULL, (LPARAM) buffer, txLen, rcText.left, rcText.top, rcText.Width(), rcText.Height() ) );		
		}

		SIZE txSize, probeSize;
		const TCHAR probe[] = _T("#");
		if (dc.GetTextExtent(buffer, txLen, &txSize) && dc.GetTextExtent(probe, _countof(probe), &probeSize)) {
			int spacing = txSize.cx > 0 ? (probeSize.cx / 4) : 0;
			if (txSize.cx + spacing < contentRect.Width()) {
				const CPoint center = contentRect.CenterPoint();
				CRect rcEdge(contentRect.left + txSize.cx + spacing, center.y, contentRect.right, contentRect.bottom);
				WIN32_OP_D(dc.DrawEdge(rcEdge, EDGE_ETCHED, BF_TOP));
			}
		}
	}
}

