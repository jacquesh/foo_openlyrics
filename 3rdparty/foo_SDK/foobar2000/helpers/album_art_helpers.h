#pragma once

namespace album_art_helpers {
	bool isJPEG(album_art_data_ptr p);
	bool isPNG(album_art_data_ptr p);
	bool isWebP(album_art_data_ptr p);

#ifdef _WIN32
	album_art_data_ptr encodeJPEG(album_art_data_ptr source, int quality1to100);
#endif
}