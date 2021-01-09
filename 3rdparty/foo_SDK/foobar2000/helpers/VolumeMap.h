#pragma once

namespace VolumeMap {
	double SliderToDB(double slider /*0..1 range*/);
	double SliderToDB2(double slider); // rounds to 0.5dB
	double DBToSlider(double volumeDB);
}
