#include "StdAfx.h"
#include "album_art_helpers.h"
#include <SDK/imageLoaderLite.h>
#include <SDK/album_art_helpers.h>

#ifdef _WIN32
#include <libPPUI/gdiplus_helpers.h>
using namespace Gdiplus;
static GdiplusErrorHandler EH;

static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

static void SaveImage(Gdiplus::Bitmap* image, const TCHAR* out, const TCHAR* format, ULONG quality) {
	CLSID             encoderClsid;
	EncoderParameters encoderParameters;

	if (GetEncoderClsid(format, &encoderClsid) < 0) throw std::runtime_error("Encoder not found");

	if (_tcscmp(format, _T("image/jpeg")) == 0) {
		encoderParameters.Count = 1;
		encoderParameters.Parameter[0].Guid = EncoderQuality;
		encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
		encoderParameters.Parameter[0].NumberOfValues = 1;
		encoderParameters.Parameter[0].Value = &quality;

		EH << image->Save(out, &encoderClsid, &encoderParameters);
	} else {
		EH << image->Save(out, &encoderClsid, NULL);
	}
}

#endif

namespace album_art_helpers {
	bool isJPEG(album_art_data_ptr aa) {
		if (aa->size() < 2) return false;
		const uint8_t* p = (const uint8_t*)aa->data();
		return p[0] == 0xFF && p[1] == 0xD8;
	}

	bool isPNG(album_art_data_ptr aa) {
		if (aa->size() < 2) return false;
		const uint8_t* p = (const uint8_t*)aa->data();
		return p[0] == 0x89 && p[1] == 0x50;
	}
	bool isWebP(album_art_data_ptr aa) {
		if (aa->size() < 12) return false;
		const uint8_t* p = (const uint8_t*)aa->data();
		return memcmp(p, "RIFF", 4) == 0 && memcmp(p + 8, "WEBP", 4) == 0;
	}
#ifdef _WIN32
	album_art_data_ptr encodeJPEG(album_art_data_ptr aa, int quality1to100) {
		GdiplusScope::Once();
		auto api = fb2k::imageLoaderLite::get();

		std::unique_ptr< Gdiplus::Image > img(api->load(aa));

		if (img->GetType() != Gdiplus::ImageTypeBitmap) throw std::runtime_error("Excepted a bitmap");

		{
			pfc::string8 temp_path, temp_file;
			uGetTempPath(temp_path);
			uGetTempFileName(temp_path, "img", 0, temp_file);
			pfc::onLeaving scope([&] {
				uDeleteFile(temp_file);
			});
			SaveImage(static_cast<Gdiplus::Bitmap*>(img.get()), pfc::stringcvt::string_os_from_utf8(temp_file), L"image/jpeg", quality1to100);

			file::ptr f = fileOpenReadExisting(temp_file, fb2k::noAbort);
			uint64_t s = f->get_size_ex(fb2k::noAbort);
			if (s > 1024 * 1024 * 64) throw exception_io_data();
			auto obj = fb2k::service_new<album_art_data_impl>();
			obj->from_stream(f.get_ptr(), (size_t)s, fb2k::noAbort);
			return obj;
		}

	}
#endif
}
