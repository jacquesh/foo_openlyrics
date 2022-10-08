#include "stdafx.h"

#include "GDIUtils.h"

HBITMAP CreateDIB24(CSize size) {
	struct {
		BITMAPINFOHEADER bmi;
	} bi = {};
	bi.bmi.biSize = sizeof(bi.bmi);
	bi.bmi.biWidth = size.cx;
	bi.bmi.biHeight = size.cy;
	bi.bmi.biPlanes = 1;
	bi.bmi.biBitCount = 24;
	bi.bmi.biCompression = BI_RGB;
	void* bitsPtr;
	return CreateDIBSection(NULL, reinterpret_cast<const BITMAPINFO*>(&bi), DIB_RGB_COLORS, &bitsPtr, 0, 0);
}

HBITMAP CreateDIB16(CSize size) {
	struct {
		BITMAPINFOHEADER bmi;
	} bi = {};
	bi.bmi.biSize = sizeof(bi.bmi);
	bi.bmi.biWidth = size.cx;
	bi.bmi.biHeight = size.cy;
	bi.bmi.biPlanes = 1;
	bi.bmi.biBitCount = 16;
	bi.bmi.biCompression = BI_RGB;
	void* bitsPtr;
	return CreateDIBSection(NULL, reinterpret_cast<const BITMAPINFO*>(&bi), DIB_RGB_COLORS, &bitsPtr, 0, 0);
}

HBITMAP CreateDIB8(CSize size, const COLORREF palette[256]) {
	struct {
		BITMAPINFOHEADER bmi;
		COLORREF colors[256];
	} bi = { };
	for (int c = 0; c < 256; ++c) bi.colors[c] = palette[c];
	bi.bmi.biSize = sizeof(bi.bmi);
	bi.bmi.biWidth = size.cx;
	bi.bmi.biHeight = size.cy;
	bi.bmi.biPlanes = 1;
	bi.bmi.biBitCount = 8;
	bi.bmi.biCompression = BI_RGB;
	bi.bmi.biClrUsed = 256;
	void* bitsPtr;
	return CreateDIBSection(NULL, reinterpret_cast<const BITMAPINFO*>(&bi), DIB_RGB_COLORS, &bitsPtr, 0, 0);
}

void CreateScaledFont(CFont& out, CFontHandle in, double scale) {
	LOGFONT lf;
	WIN32_OP_D(in.GetLogFont(lf));
	int temp = pfc::rint32(scale * lf.lfHeight);
	if (temp == 0) temp = pfc::sgn_t(lf.lfHeight);
	lf.lfHeight = temp;
	WIN32_OP_D(out.CreateFontIndirect(&lf) != NULL);
}

void CreateScaledFontEx(CFont& out, CFontHandle in, double scale, int weight) {
	LOGFONT lf;
	WIN32_OP_D(in.GetLogFont(lf));
	int temp = pfc::rint32(scale * lf.lfHeight);
	if (temp == 0) temp = pfc::sgn_t(lf.lfHeight);
	lf.lfHeight = temp;
	lf.lfWeight = weight;
	WIN32_OP_D(out.CreateFontIndirect(&lf) != NULL);
}

void CreatePreferencesHeaderFont(CFont& out, CWindow source) {
	CreateScaledFontEx(out, source.GetFont(), 1.3, FW_BOLD);
}

void CreatePreferencesHeaderFont2(CFont& out, CWindow source) {
	CreateScaledFontEx(out, source.GetFont(), 1.1, FW_BOLD);
}

CSize GetBitmapSize(HBITMAP bmp) {
	PFC_ASSERT(bmp != NULL);
	CBitmapHandle h(bmp);
	BITMAP bm = {};
	WIN32_OP_D(h.GetBitmap(bm));
	return CSize(bm.bmWidth, bm.bmHeight);
}

CSize GetIconSize(HICON icon) {
	PFC_ASSERT(icon != NULL);
	CIconHandle h(icon);
	ICONINFO info = {};
	WIN32_OP_D( h.GetIconInfo(&info) );
	CSize ret;
	if (info.hbmColor != NULL) ret = GetBitmapSize(info.hbmColor);
	else if (info.hbmMask != NULL) ret = GetBitmapSize(info.hbmMask);
	else { PFC_ASSERT(!"???"); }
	if (info.hbmColor != NULL) DeleteObject(info.hbmColor);
	if (info.hbmMask != NULL) DeleteObject(info.hbmMask);
	return ret;
}

HBRUSH MakeTempBrush(HDC dc, COLORREF color) noexcept {
	SetDCBrushColor(dc, color); return (HBRUSH)GetStockObject(DC_BRUSH);
}
