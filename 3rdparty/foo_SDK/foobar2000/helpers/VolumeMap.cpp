#include "stdafx.h"
#include "VolumeMap.h"

static const double powval = 2.0;
static const double silence = -100.0;

double VolumeMap::SliderToDB2(double slider) {
	double v = SliderToDB(slider);
	v = floor(v * 2.0 + 0.5) * 0.5;
	return v;
}

double VolumeMap::SliderToDB(double slider) {
	if (slider > 0) {
		return pfc::max_t<double>(silence,10.0 * log(slider) / log(powval));
	} else {
		return silence;
	}
}
double VolumeMap::DBToSlider(double volumeDB) {
	return pfc::clip_t<double>(pow(powval,volumeDB / 10.0), 0, 1);
}
