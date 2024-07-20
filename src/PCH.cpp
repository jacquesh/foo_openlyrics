#include "stdafx.h"

#pragma warning(push, 0)
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STBIR_SSE2 // Force support for at least SSE2 (required by fb2k 2.0)
#include "stb_image_resize2.h"
#pragma warning(pop)

#define MVTF_IMPLEMENTATION
#include "mvtf/mvtf.h"
