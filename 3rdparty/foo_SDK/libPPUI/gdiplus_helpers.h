#pragma once
#include <GdiPlus.h>
#include <functional>
#include <memory>

#include "win32_op.h"
#include "win32_utility.h"

class GdiplusErrorHandler {
public:
	static void Handle(Gdiplus::Status code);
	void operator<<(Gdiplus::Status p_code) {Handle(p_code);}
};

class GdiplusScope {
public:
	GdiplusScope() {
		Gdiplus::GdiplusStartupInput input;
		Gdiplus::GdiplusStartupOutput output;
		GdiplusErrorHandler() << Gdiplus::GdiplusStartup(&m_token,&input,&output);
	}
	~GdiplusScope() {
		Gdiplus::GdiplusShutdown(m_token);
	}
	static void Once();
private:
	GdiplusScope( const GdiplusScope & ) = delete;
	void operator=( const GdiplusScope & ) = delete;

	ULONG_PTR m_token = 0;
};

typedef std::function<void(Gdiplus::Bitmap*) > GdiplusBitmapTransform_t;

struct GdiplusIconArg_t {
	CSize size = CSize(0,0);
	GdiplusBitmapTransform_t transform = nullptr;
};

HBITMAP GdiplusLoadBitmap(UINT id, const TCHAR* resType, CSize size);
std::unique_ptr<Gdiplus::Image> GdiplusImageFromMem(const void* ptr, size_t bytes);
std::unique_ptr<Gdiplus::Bitmap> GdiplusResizeImage(Gdiplus::Image* source, CSize size, Gdiplus::PixelFormat pf = PixelFormat32bppARGB);
HICON GdiplusLoadIconFromMem(const void* ptr, size_t bytes, CSize size);
HICON GdiplusLoadIcon(UINT id, const TCHAR* resType, CSize size);
HICON GdiplusLoadIconEx(UINT id, const TCHAR* resType, GdiplusIconArg_t const & arg);
HICON GdiplusLoadPNGIcon(UINT id, CSize size);
HICON LoadPNGIcon(UINT id, CSize size);

void GdiplusLoadButtonPNG(CIcon& icon, HWND btn_, UINT image, GdiplusBitmapTransform_t transform = nullptr);
std::unique_ptr<Gdiplus::Bitmap> GdiplusLoadResource(UINT id, const TCHAR* resType);
std::unique_ptr<Gdiplus::Bitmap> GdiplusLoadResourceAsSize(UINT id, const TCHAR* resType, CSize size);
void GdiplusDimImage(Gdiplus::Bitmap* bmp);
void GdiplusInvertImage(Gdiplus::Bitmap* bmp);
