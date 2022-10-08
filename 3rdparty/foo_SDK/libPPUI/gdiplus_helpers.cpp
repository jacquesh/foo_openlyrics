#include "stdafx.h"
#include "gdiplus_helpers.h"

static bool _Once() {
	ULONG_PTR token = 0;
	Gdiplus::GdiplusStartupInput input;
	Gdiplus::GdiplusStartupOutput output;
	GdiplusErrorHandler() << Gdiplus::GdiplusStartup(&token, &input, &output);
	return true;
}

void GdiplusScope::Once() {
	static bool done = _Once();
	(void)done; // suppress warning
}

void GdiplusErrorHandler::Handle(Gdiplus::Status p_code) {
	if (p_code != Gdiplus::Ok) {
		switch (p_code) {
		case Gdiplus::InvalidParameter:
			throw pfc::exception_invalid_params();
		default:
			throw std::runtime_error(pfc::format("Gdiplus error (", (unsigned)p_code, ")"));
		}
		
	}
}

HBITMAP GdiplusLoadBitmap(UINT id, const TCHAR* resType, CSize size) {
	using namespace Gdiplus;
	try {
		auto stream = WinLoadResourceAsStream(GetThisModuleHandle(), MAKEINTRESOURCE(id), resType);

		GdiplusErrorHandler EH;
		std::unique_ptr<Image> source ( new Image(stream) );
		EH << source->GetLastStatus();

		std::unique_ptr<Bitmap> resized ( new Bitmap(size.cx, size.cy, PixelFormat32bppARGB) );
		EH << resized->GetLastStatus();

		{
			std::unique_ptr<Graphics> target ( new Graphics(resized.get()) );
			EH << target->GetLastStatus();
			EH << target->SetInterpolationMode(InterpolationModeHighQuality);
			EH << target->Clear(Color(0, 0, 0, 0));
			EH << target->DrawImage(source.get(), Rect(0, 0, size.cx, size.cy));
		}

		HBITMAP bmp = NULL;
		EH << resized->GetHBITMAP(Gdiplus::Color::White, &bmp);
		return bmp;
	} catch (...) {
		PFC_ASSERT(!"Should not get here");
		return NULL;
	}
}

std::unique_ptr<Gdiplus::Image> GdiplusImageFromMem(const void* ptr, size_t bytes) {
	using namespace Gdiplus;
	GdiplusErrorHandler EH;

	CComPtr<IStream> stream;
	stream.p = SHCreateMemStream((const BYTE*)ptr, pfc::downcast_guarded<UINT>(bytes));
	if (!stream) throw std::bad_alloc();
	std::unique_ptr<Image> source ( new Image(stream.p) );
	EH << source->GetLastStatus();
	return source;
}

std::unique_ptr< Gdiplus::Bitmap > GdiplusResizeImage(Gdiplus::Image* source, CSize size, Gdiplus::PixelFormat pf) {
	PFC_ASSERT(size.cx > 0 && size.cy > 0);
	using namespace Gdiplus;
	GdiplusErrorHandler EH;
	std::unique_ptr<Bitmap> resized ( new Bitmap(size.cx, size.cy, pf) );
	EH << resized->GetLastStatus();
	std::unique_ptr<Graphics> target ( new Graphics(resized.get()) );
	EH << target->GetLastStatus();
	EH << target->SetInterpolationMode(InterpolationModeHighQuality);
	EH << target->Clear(Color(0, 0, 0, 0));
	EH << target->DrawImage(source, Rect(0, 0, size.cx, size.cy));
	return resized;
}

HICON GdiplusLoadIconFromMem(const void* ptr, size_t bytes, CSize size) {
	using namespace Gdiplus;
	try {

		GdiplusErrorHandler EH;
		auto source = GdiplusImageFromMem(ptr, bytes);
		auto resized = GdiplusResizeImage(source.get(), size);
		HICON icon = NULL;
		EH << resized->GetHICON(&icon);
		return icon;
	} catch (...) {
		PFC_ASSERT(!"Should not get here");
		return NULL;
	}
}

HICON GdiplusLoadIcon(UINT id, const TCHAR* resType, CSize size) {
	GdiplusIconArg_t arg = { size };
	return GdiplusLoadIconEx(id, resType, arg);
}

HICON GdiplusLoadIconEx(UINT id, const TCHAR* resType, GdiplusIconArg_t const& arg) {
	using namespace Gdiplus;
	try {
		auto stream = WinLoadResourceAsStream(GetThisModuleHandle(), MAKEINTRESOURCE(id), resType);

		GdiplusErrorHandler EH;
		pfc::ptrholder_t<Image> source = new Image(stream);

		EH << source->GetLastStatus();
		pfc::ptrholder_t<Bitmap> resized = new Bitmap(arg.size.cx, arg.size.cy, PixelFormat32bppARGB);
		EH << resized->GetLastStatus();
		
		{
			pfc::ptrholder_t<Graphics> target = new Graphics(resized.get_ptr());
			EH << target->GetLastStatus();
			EH << target->SetInterpolationMode(InterpolationModeHighQuality);
			EH << target->Clear(Color(0, 0, 0, 0));
			EH << target->DrawImage(source.get_ptr(), Rect(0, 0, arg.size.cx, arg.size.cy));
		}

		source = nullptr;

		if (arg.transform) arg.transform(resized.get_ptr());

		HICON icon = NULL;
		EH << resized->GetHICON(&icon);
		return icon;
	} catch (...) {
		PFC_ASSERT(!"Should not get here");
		return NULL;
	}
}

HICON GdiplusLoadPNGIcon(UINT id, CSize size) { 
	return GdiplusLoadIcon(id, _T("PNG"), size); 
}

HICON LoadPNGIcon(UINT id, CSize size) {
	GdiplusScope scope;
	return GdiplusLoadPNGIcon(id, size);
}

void GdiplusLoadButtonPNG(CIcon& icon, HWND btn_, UINT image, GdiplusBitmapTransform_t transform) {
	CButton btn(btn_);
	if (icon == NULL) {
		CRect client; WIN32_OP_D(btn.GetClientRect(client));
		CSize size = client.Size();
		int v = MulDiv(pfc::min_t<int>(size.cx, size.cy), 3, 4);
		if (v < 8) v = 8;

		GdiplusIconArg_t arg;
		arg.size = CSize(v, v);
		arg.transform = transform;
		icon = GdiplusLoadIconEx(image, L"PNG", arg);
	}
	btn.SetIcon(icon);
}

std::unique_ptr<Gdiplus::Bitmap> GdiplusLoadResource(UINT id, const TCHAR* resType) {
	using namespace Gdiplus;
	GdiplusErrorHandler EH;

	auto stream = WinLoadResourceAsStream(GetThisModuleHandle(), MAKEINTRESOURCE(id), resType);

	std::unique_ptr<Bitmap> img ( new Bitmap(stream) );
	EH << img->GetLastStatus();
	return img;
}

std::unique_ptr<Gdiplus::Bitmap> GdiplusLoadResourceAsSize(UINT id, const TCHAR* resType, CSize size) {
	auto source = GdiplusLoadResource(id, resType);
	return GdiplusResizeImage(source.get(), size);
}

void GdiplusDimImage(Gdiplus::Bitmap* bmp) {
	using namespace Gdiplus;
	GdiplusErrorHandler EH;
	Gdiplus::Rect r(0, 0, bmp->GetWidth(), bmp->GetHeight());
	BitmapData data = {};
	EH << bmp->LockBits(&r, ImageLockModeRead | ImageLockModeWrite, PixelFormat32bppARGB, &data);

	BYTE* scan = (BYTE*)data.Scan0;
	for (UINT y = 0; y < data.Height; ++y) {
		BYTE* px = scan;
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

void GdiplusInvertImage(Gdiplus::Bitmap* bmp) {
	using namespace Gdiplus;
	GdiplusErrorHandler EH;
	Gdiplus::Rect r(0, 0, bmp->GetWidth(), bmp->GetHeight());
	BitmapData data = {};
	EH << bmp->LockBits(&r, ImageLockModeRead | ImageLockModeWrite, PixelFormat32bppARGB, &data);

	BYTE* scan = (BYTE*)data.Scan0;
	for (UINT y = 0; y < data.Height; ++y) {
		BYTE* px = scan;
		for (UINT x = 0; x < data.Width; ++x) {
			unsigned v = (unsigned)px[0] + (unsigned)px[1] + (unsigned)px[2];
			v /= 3;
			px[0] = px[1] = px[2] = (BYTE)(255 - v);
			px += 4;
		}
		scan += data.Stride;
	}

	EH << bmp->UnlockBits(&data);
}

#pragma comment(lib, "gdiplus.lib")
