#pragma once
#include <GdiPlus.h>

#include "win32_op.h"
#include "win32_utility.h"

class GdiplusErrorHandler {
public:
	void operator<<(Gdiplus::Status p_code) {
		if (p_code != Gdiplus::Ok) {
			throw pfc::exception(PFC_string_formatter() << "Gdiplus error (" << (unsigned) p_code << ")");
		}
	}
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
	static void Once() {
		static bool done = _Once();
	}
private:
	static bool _Once() {
		ULONG_PTR token = 0;
		Gdiplus::GdiplusStartupInput input;
		Gdiplus::GdiplusStartupOutput output;
		GdiplusErrorHandler() << Gdiplus::GdiplusStartup(&token, &input, &output);
		return true;
	}
	GdiplusScope( const GdiplusScope & ) = delete;
	void operator=( const GdiplusScope & ) = delete;

	ULONG_PTR m_token = 0;
};

static HBITMAP GdiplusLoadBitmap(UINT id, const TCHAR * resType, CSize size) {
	using namespace Gdiplus;
	try {
		auto stream = WinLoadResourceAsStream(GetThisModuleHandle(), MAKEINTRESOURCE(id), resType );
				
		GdiplusErrorHandler EH;
		pfc::ptrholder_t<Image> source = new Image(stream);
		
		EH << source->GetLastStatus();
		pfc::ptrholder_t<Bitmap> resized = new Bitmap(size.cx, size.cy, PixelFormat32bppARGB);
		EH << resized->GetLastStatus();
		
		{
			pfc::ptrholder_t<Graphics> target = new Graphics(resized.get_ptr());
			EH << target->GetLastStatus();
			EH << target->SetInterpolationMode(InterpolationModeHighQuality);
			EH << target->Clear(Color(0,0,0,0));
			EH << target->DrawImage(source.get_ptr(), Rect(0,0,size.cx,size.cy));
		}

		HBITMAP bmp = NULL;
		EH << resized->GetHBITMAP(Gdiplus::Color::White, & bmp );
		return bmp;
	} catch(...) {
		PFC_ASSERT( !"Should not get here");
		return NULL;
	}
}

static Gdiplus::Image * GdiplusImageFromMem(const void * ptr, size_t bytes) {
	using namespace Gdiplus;
	GdiplusErrorHandler EH;
	pfc::ptrholder_t<Image> source;

	{
		pfc::com_ptr_t<IStream> stream;
		stream.attach( SHCreateMemStream((const BYTE*)ptr, pfc::downcast_guarded<UINT>(bytes)) );
		if (stream.is_empty()) throw std::bad_alloc();
		source = new Image(stream.get_ptr());
	}

	EH << source->GetLastStatus();
	return source.detach();
}

static Gdiplus::Bitmap * GdiplusResizeImage( Gdiplus::Image * source, CSize size ) {
	using namespace Gdiplus;
	GdiplusErrorHandler EH;
	pfc::ptrholder_t<Bitmap> resized = new Bitmap(size.cx, size.cy, PixelFormat32bppARGB);
	EH << resized->GetLastStatus();
	pfc::ptrholder_t<Graphics> target = new Graphics(resized.get_ptr());
	EH << target->GetLastStatus();
	EH << target->SetInterpolationMode(InterpolationModeHighQuality);
	EH << target->Clear(Color(0, 0, 0, 0));
	EH << target->DrawImage(source, Rect(0, 0, size.cx, size.cy));
	return resized.detach();
}

static HICON GdiplusLoadIconFromMem( const void * ptr, size_t bytes, CSize size ) {
	using namespace Gdiplus;
	try {

		GdiplusErrorHandler EH;
		pfc::ptrholder_t<Image> source = GdiplusImageFromMem(ptr, bytes);
		pfc::ptrholder_t<Bitmap> resized = GdiplusResizeImage( source.get_ptr(), size );
		HICON icon = NULL;
		EH << resized->GetHICON(&icon);
		return icon;
	} catch (...) {
		PFC_ASSERT(!"Should not get here");
		return NULL;
	}
}

static HICON GdiplusLoadIcon(UINT id, const TCHAR * resType, CSize size) {
	using namespace Gdiplus;
	try {
		auto stream = WinLoadResourceAsStream(GetThisModuleHandle(), MAKEINTRESOURCE(id), resType);

		GdiplusErrorHandler EH;
		pfc::ptrholder_t<Image> source = new Image(stream);
		
		EH << source->GetLastStatus();
		pfc::ptrholder_t<Bitmap> resized = new Bitmap(size.cx, size.cy, PixelFormat32bppARGB);
		EH << resized->GetLastStatus();
		
		{
			pfc::ptrholder_t<Graphics> target = new Graphics(resized.get_ptr());
			EH << target->GetLastStatus();
			EH << target->SetInterpolationMode(InterpolationModeHighQuality);
			EH << target->Clear(Color(0,0,0,0));
			EH << target->DrawImage(source.get_ptr(), Rect(0,0,size.cx,size.cy));
		}

		HICON icon = NULL;
		EH << resized->GetHICON(&icon);
		return icon;
	} catch(...) {
		PFC_ASSERT( !"Should not get here");
		return NULL;
	}
}
static HICON GdiplusLoadPNGIcon(UINT id, CSize size) {return GdiplusLoadIcon(id, _T("PNG"), size);}

static HICON LoadPNGIcon(UINT id, CSize size) {
	GdiplusScope scope;
	return GdiplusLoadPNGIcon(id, size);
}

static void GdiplusLoadButtonPNG(CIcon & icon, HWND btn_, UINT image) {
	CButton btn(btn_);
	if (icon == NULL) {
		CRect client; WIN32_OP_D( btn.GetClientRect(client) );
		CSize size = client.Size();
		int v = MulDiv(pfc::min_t<int>(size.cx, size.cy), 3, 4);
		if (v < 8) v = 8;
		icon = GdiplusLoadPNGIcon(image, CSize(v,v));
	}
	btn.SetIcon(icon);
}

static Gdiplus::Bitmap * GdiplusLoadResource(UINT id, const TCHAR * resType) {
	using namespace Gdiplus;
	GdiplusErrorHandler EH;

	auto stream = WinLoadResourceAsStream(GetThisModuleHandle(), MAKEINTRESOURCE(id), resType);
	pfc::ptrholder_t<Bitmap> img = new Bitmap(stream);
	EH << img->GetLastStatus();
	return img.detach();
}

static Gdiplus::Bitmap * GdiplusLoadResourceAsSize(UINT id, const TCHAR * resType, CSize size) {
	using namespace Gdiplus;
	GdiplusErrorHandler EH;
	pfc::ptrholder_t<Bitmap> source = GdiplusLoadResource(id, resType);

	pfc::ptrholder_t<Bitmap> resized = new Bitmap(size.cx, size.cy, PixelFormat32bppARGB);
	EH << resized->GetLastStatus();
	{
		pfc::ptrholder_t<Graphics> target = new Graphics(resized.get_ptr());
		EH << target->GetLastStatus();
		EH << target->SetInterpolationMode(InterpolationModeHighQuality);
		EH << target->DrawImage(source.get_ptr(), Rect(0, 0, size.cx, size.cy));
	}
	return resized.detach();
}

static void GdiplusDimImage(Gdiplus::Bitmap * bmp) {
	using namespace Gdiplus;
	GdiplusErrorHandler EH;
	Gdiplus::Rect r(0, 0, bmp->GetWidth(), bmp->GetHeight());
	BitmapData data = {};
	EH << bmp->LockBits(&r, ImageLockModeRead | ImageLockModeWrite, PixelFormat32bppARGB, &data);

	BYTE * scan = (BYTE*)data.Scan0;
	for (UINT y = 0; y < data.Height; ++y) {
		BYTE * px = scan;
		for (UINT x = 0; x < data.Width; ++x) {
			//px[0] = _dimPixel(px[0]);
			//px[1] = _dimPixel(px[1]);
			//px[2] = _dimPixel(px[2]);
			px[3] /= 3;
			px += 4;
		}
		scan += data.Stride;
	}

	EH << bmp->UnlockBits(&data);
}
static void GdiplusInvertImage(Gdiplus::Bitmap * bmp) {
	using namespace Gdiplus;
	GdiplusErrorHandler EH;
	Gdiplus::Rect r(0, 0, bmp->GetWidth(), bmp->GetHeight());
	BitmapData data = {};
	EH << bmp->LockBits(&r, ImageLockModeRead | ImageLockModeWrite, PixelFormat32bppARGB, &data);

	BYTE * scan = (BYTE*)data.Scan0;
	for (UINT y = 0; y < data.Height; ++y) {
		BYTE * px = scan;
		for (UINT x = 0; x < data.Width; ++x) {
			unsigned v = (unsigned)px[0] + (unsigned)px[1] + (unsigned)px[2];
			v /= 3;
			px[0] = px[1] = px[2] = 255 - v;
			px += 4;
		}
		scan += data.Stride;
	}

	EH << bmp->UnlockBits(&data);
}
#pragma comment(lib, "gdiplus.lib")
