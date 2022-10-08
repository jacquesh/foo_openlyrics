#include "stdafx.h"
#include "CMiddleDragImpl.h"

double CMiddleDragCommon::myPow(double p_val, double p_exp) {
	if (p_val < 0) {
		return -pow(-p_val, p_exp);
	} else {
		return pow(p_val, p_exp);
	}
}

double CMiddleDragCommon::ProcessMiddleDragDeltaInternal(double p_delta) {
	p_delta *= (double)KTimerPeriod / 25; /*originally calculated for 25ms timer interval*/
	return myPow(p_delta * 0.05, 2.0);
}

double CMiddleDragCommon::radiusHelper(double p_x, double p_y) {
	return sqrt(p_x * p_x + p_y * p_y);
}
int CMiddleDragCommon::mySGN(LONG v) {
	if (v > 0) return 1;
	else if (v < 0) return -1;
	else return 0;
}

int32_t CMiddleDragCommon::Round(double val, double & acc) {
	val += acc;
	int32_t ret = (int32_t)floor(val + 0.5);
	acc = val - ret;
	return ret;
}

LONG CMiddleDragCommon::LineToPixelsHelper(LONG & p_overflow, LONG p_pixels, LONG p_dpi, LONG p_lineWidth) {
	const int lineWidth = MulDiv(p_lineWidth, p_dpi, 96);
	if (lineWidth == 0) return 0;
	p_overflow += p_pixels;
	LONG ret = p_overflow / lineWidth;
	p_overflow -= ret * lineWidth;
	return ret;
}

void CMiddleDragOverlay::ShowHere(CPoint pt) {

	auto dpi = QueryScreenDPIEx(*this);
	CSize size(MulDiv(32, dpi.cx, 96), MulDiv(32, dpi.cy, 96));
	// Original path values are for 32x32, don't rescale for sizes close-enough
	if (size.cx < 48) size.cx = 32;
	if (size.cy < 48) size.cy = 32;
	CPoint center(pt);
	CPoint origin = center - CSize(size.cx / 2, size.cy / 2);
	CRect rect(origin, origin + size);
	this->SetWindowPos(HWND_TOPMOST, rect, SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

namespace {
	struct pt_t { uint8_t x, y; };
	static CPoint transform(pt_t pt, CRect const& rc) {
		return CPoint(rc.left + MulDiv(pt.x, rc.Width(), 32), rc.top + MulDiv(pt.y, rc.Height(), 32));
	}
}

void CMiddleDragOverlay::Paint(CDCHandle dc) {
	CRect client;
	WIN32_OP_D(GetClientRect(&client));
	static const pt_t path[] = {
		{15,0}, {9,6}, {9,7}, {14,7}, {14, 14},
		{7, 14}, {7, 9}, {6, 9}, {0, 15},
		{0, 16}, {6, 22}, {7, 22}, {7, 17}, {14, 17},
		{14, 24}, {9, 24}, {9, 25}, {15, 31},
		{16, 31}, {22, 25}, {22, 24}, {17, 24}, {17, 17},
		{24, 17}, {24, 22}, {25, 22}, {31, 16},
		{31, 15}, {25, 9}, {24, 9}, {24, 14}, {17, 14},
		{17, 7}, {22, 7}, {22, 6}, {16, 0},
	};

	POINT points[std::size(path)];
	for (size_t walk = 0; walk < std::size(path); ++walk) {
		points[walk] = transform(path[walk], client);
	}
	CRgn rgn = CreatePolygonRgn(points, (int)std::size(path), WINDING);
	PFC_ASSERT(rgn != NULL);
	const HBRUSH brush = (HBRUSH)GetStockObject(DC_BRUSH);
	dc.SetDCBrushColor(0);
	dc.FillRgn(rgn, brush);
	dc.SetDCBrushColor(0xFFFFFF);
	dc.FrameRgn(rgn, brush, 1, 1);
}