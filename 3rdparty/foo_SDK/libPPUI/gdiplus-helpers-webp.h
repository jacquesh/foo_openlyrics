#pragma once

#include "gdiplus_helpers.h"

// Presumes prior #include of webp/decode.h

static bool IsImageWebP(const void * ptr, size_t bytes) {
	if (bytes < 12) return false;
	return memcmp(ptr, "RIFF", 4) == 0 && memcmp((const char*)ptr + 8, "WEBP", 4) == 0;
}


// WebP-aware GdiplusImageFromMem
static Gdiplus::Image * GdiplusImageFromMem2(const void * ptr, size_t bytes) {
	GdiplusErrorHandler EH;
	using namespace Gdiplus;
	if (IsImageWebP(ptr, bytes)) {
		WebPBitstreamFeatures bs;
		if (WebPGetFeatures((const uint8_t*)ptr, bytes, &bs) != VP8_STATUS_OK) {
			throw std::runtime_error("WebP decoding failure");
		}
		const Gdiplus::PixelFormat pf = bs.has_alpha ? PixelFormat32bppARGB : PixelFormat32bppRGB;
		const int pfBytes = 4; // Gdiplus RGB is 4 bytes
		int w = 0, h = 0;
		// ALWAYS decode BGRA, Gdiplus will disregard alpha if was not originally present
		uint8_t * decodedData = WebPDecodeBGRA((const uint8_t*)ptr, bytes, &w, &h);
		pfc::onLeaving scope([decodedData] {WebPFree(decodedData); });
		if (decodedData == nullptr || w <= 0 || h <= 0) throw std::runtime_error("WebP decoding failure");

		pfc::ptrholder_t<Bitmap> ret = new Gdiplus::Bitmap(w, h, pf);
		EH << ret->GetLastStatus();
		Rect rc(0, 0, w, h);
		Gdiplus::BitmapData bitmapData;
		EH << ret->LockBits(&rc, 0, pf, &bitmapData);
		uint8_t * target = (uint8_t*)bitmapData.Scan0;
		const uint8_t * source = decodedData;
		for (int y = 0; y < h; ++y) {
			memcpy(target, source, w * pfBytes);
			target += bitmapData.Stride;
			source += pfBytes * w;
		}
		EH << ret->UnlockBits(&bitmapData);
		return ret.detach();
	}
	return GdiplusImageFromMem(ptr, bytes);
}
