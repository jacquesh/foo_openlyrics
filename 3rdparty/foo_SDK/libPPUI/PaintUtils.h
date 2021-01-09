#pragma once

#include <Uxtheme.h>

namespace PaintUtils {
	t_uint32 BlendColor(t_uint32 p_color1, t_uint32 p_color2, int p_percentage = 50) throw();
	t_uint32 BlendColorEx(t_uint32 p_color1, t_uint32 p_color2, double mix = 0.5) throw();
	t_uint32 DriftColor(t_uint32 p_color,unsigned p_delta,bool p_direction) throw();
	void FillVertexColor(TRIVERTEX & p_vertex,t_uint32 p_color,t_uint16 p_alpha = 0) throw();
	void FillRectSimple(CDCHandle p_dc,const CRect & p_rect,t_uint32 p_color) throw();
	void GradientFillRect(CDCHandle p_dc,const CRect & p_rect,t_uint32 p_color1, t_uint32 p_color2, bool p_horizontal) throw();
	void GradientSplitRect(CDCHandle p_dc,const CRect & p_rect,t_uint32 p_bkColor, t_uint32 p_gradientColor,int p_splitPercent) throw();
	void GradientBar(CDCHandle p_dc,const CRect & p_rect,t_uint32 p_exterior, t_uint32 p_interior, int p_percentage) throw();
	void RenderItemBackground(CDCHandle p_dc,const CRect & p_itemRect,t_size p_item,t_uint32 p_color) throw();

	t_uint32 DetermineTextColor(t_uint32 background) throw();
	double Luminance(t_uint32 color) throw();
	
	void AddRectToRgn(HRGN rgn, CRect const & rect) throw();

	void FocusRect(CDCHandle dc, CRect const & rect) throw();
	void FocusRect2(CDCHandle dc, CRect const & rect, COLORREF bkColor) throw();

	namespace TrackBar {
		void DrawThumb(HTHEME theme,HDC dc,int state,const RECT * rcThumb, const RECT * rcUpdate);
		void DrawTrack(HTHEME theme,HDC dc,const RECT * rcTrack, const RECT * rcUpdate);
		void DrawTrackVolume(HTHEME theme,HDC dc,const RECT * rcTrack, const RECT * rcUpdate);
	};

	void DrawSmoothedLine(HDC dc, CPoint p1, CPoint p2, COLORREF col, double width);

	enum {
		TextOutColors_Dim = 21,
		TextOutColors_Highlight = 22,
	};
	void TextOutColors(HDC dc,const TCHAR * src,int len,CPoint offset,const CRect & clip,const t_uint32 colors[3], int tabWidthTotal = 0, int tabWidthDiv = 1);
	void TextOutColorsEx(HDC dc,const TCHAR * src,const CRect & target,DWORD flags,const t_uint32 colors[3]);
	void TextOutColors_StripCodesAppend(pfc::string_formatter & out, const char * in);
	void TextOutColors_StripCodes(pfc::string_formatter & out, const char * in);

	t_uint32 TextOutColors_CalcWidth(HDC dc, const TCHAR * src);
	CSize TextOutColors_CalcSize(HDC dc, const TCHAR * src);

	pfc::string TextOutColors_ImportScript(pfc::string script);
	void TextOutColors_ImportScript(pfc::string_base & out, const char * in);

	bool TextContainsCodes(const TCHAR * src);

	t_uint32 DrawText_TranslateHeaderAlignment(t_uint32 val);

	void RenderButton(HWND wnd_, HDC dc_, CRect rcUpdate, bool bPressed);
	void PaintSeparatorControl(HWND wnd_);
}
