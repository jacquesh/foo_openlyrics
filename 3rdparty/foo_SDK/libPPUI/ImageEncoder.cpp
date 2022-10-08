#include "stdafx.h"
#include "ImageEncoder.h"
#include "gdiplus_helpers.h"
#include <memory>
#include <vector>

using namespace Gdiplus;
static GdiplusErrorHandler EH;

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes

   ImageCodecInfo* pImageCodecInfo = NULL;

   GetImageEncodersSize(&num, &size);
   if(size == 0)
      return -1;  // Failure

   pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
   if(pImageCodecInfo == NULL)
      return -1;  // Failure

   GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j)
   {
      if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         free(pImageCodecInfo);
         return j;  // Success
      }    
   }

   free(pImageCodecInfo);
   return -1;  // Failure
}

void ConvertImage(const TCHAR * in, const TCHAR * out, const TCHAR * format, ULONG quality) {
	GdiplusScope scope;
	std::unique_ptr< Bitmap > image ( new Bitmap(in) );
	EH << image->GetLastStatus();
	SaveImage(&*image, out, format, quality);
}

static void add16clip(uint16_t& v, int d) {
	int v2 = (int) v + d;
	if ( v2 < 0 ) v2 = 0;
	if ( v2 > (int) UINT16_MAX ) v2 = UINT16_MAX;
	v = (uint16_t) v2;
}

std::unique_ptr<Gdiplus::Bitmap> image16bpcto8(Gdiplus::Bitmap* img) {

	const unsigned channels = [img] {
		switch (img->GetPixelFormat()) {
		case PixelFormat48bppRGB:
			return 3;
		case PixelFormat16bppGrayScale:
			return 1;
		default:
			throw std::runtime_error("Invalid pixel format");
		}
	} ();

	
	const unsigned Width = img->GetWidth();
	const unsigned Height = img->GetHeight();

	if ( Width == 0 || Height == 0 ) throw std::runtime_error("Invalid dimensions");

	std::unique_ptr< Gdiplus::Bitmap > ret ( new Gdiplus::Bitmap( Width, Height, channels == 3 ? PixelFormat24bppRGB : PixelFormat8bppIndexed ) );

	
	Gdiplus::Rect rcWhole = {};
	rcWhole.Width = Width; rcWhole.Height = Height;
	Gdiplus::BitmapData dataIn, dataOut;
	EH << ret->LockBits( &rcWhole, ImageLockModeWrite, PixelFormat24bppRGB, &dataOut);
	EH << img->LockBits(&rcWhole, ImageLockModeRead, PixelFormat48bppRGB, &dataIn);

	
	

	const uint8_t * inLine = (const uint8_t*)dataIn.Scan0;
	uint8_t * outLine = (uint8_t*)dataOut.Scan0;

	std::vector<uint16_t> curLineBuf, nextLineBuf;
	curLineBuf.resize( (Width + 2) * channels );
	nextLineBuf.resize( (Width + 2) * channels );

	memcpy(&nextLineBuf[channels], inLine, Width * channels * 2 );

	for (unsigned y = 0; y < Height; ++y) {
		std::swap( curLineBuf, nextLineBuf );
		if (y + 1 < Height) {
			inLine += dataIn.Stride;
			memcpy(&nextLineBuf[channels], inLine, Width * channels * 2 );
		}

		auto & inPix = curLineBuf;
		auto & inPixNext = nextLineBuf;
		size_t inOffset = channels;
		uint8_t * outPix = outLine;
		for (unsigned x = 0; x < Width; ++x) {
			for (int c = 0; c < (int)channels; ++c) {
				uint16_t orig = inPix[inOffset];
				uint16_t v8 = orig >> 8;
				uint16_t v16 = v8 | (v8 << 8);

				outPix[c] = (uint8_t) v8;

				int d = (int) orig - (int) v16;

				add16clip(inPix    [inOffset + channels], d * 7 / 16 ); // x+1
				add16clip(inPixNext[inOffset - channels], d * 3 / 16 ); // x-1, y+1
				add16clip(inPixNext[inOffset           ], d * 5 / 16 ); // y+1
				add16clip(inPixNext[inOffset + channels], d * 1 / 16 ); // x+1, y+1

				++ inOffset;
			}

			outPix += channels;
		}
		
		outLine += dataOut.Stride;
	}

	EH << img->UnlockBits(&dataIn);
	EH << ret->UnlockBits(&dataOut);

	return ret;
}

void SaveImage(Gdiplus::Image* bmp, const TCHAR* out, const TCHAR* format, ULONG quality) {
	if (bmp->GetType() != Gdiplus::ImageTypeBitmap) throw std::runtime_error("Bitmap expected");
	SaveImage(static_cast<Gdiplus::Bitmap*>(bmp), out, format, quality);
}
void SaveImage(Gdiplus::Bitmap* image, const TCHAR* out, const TCHAR* format, ULONG quality) {
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
