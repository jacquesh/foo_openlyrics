#pragma once
#include <memory>
namespace Gdiplus { class Image; class Bitmap; };

void SaveImage(Gdiplus::Image* bmp, const TCHAR* out, const TCHAR* format, ULONG quality = 100);
void SaveImage( Gdiplus::Bitmap * bmp, const TCHAR * out, const TCHAR * format, ULONG quality = 100);
void ConvertImage(const TCHAR * in, const TCHAR * out, const TCHAR * format, ULONG quality = 100);
std::unique_ptr<Gdiplus::Bitmap> image16bpcto8(Gdiplus::Bitmap* img);