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
