#include "stdafx.h"

#pragma warning(push, 0)
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STBIR_SSE2 // Force support for at least SSE2 (required by fb2k 2.0)
#include "stb_image_resize2.h"
#pragma warning(pop)
