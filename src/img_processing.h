#pragma once

#include "stdafx.h"

struct RGBAColour
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

// A 4-channel RGBA image
struct Image
{
    uint8_t* pixels = nullptr;
    int width = 0;
    int height = 0;

    Image() = default;
    Image(Image&& other);
    Image& operator =(Image&& other);
    Image(const Image&) = delete;
    Image& operator =(const Image&) = delete;
    ~Image();
    bool valid();
};

RGBAColour from_colorref(COLORREF colour);

std::optional<Image> load_image(const char* file_path);
std::optional<Image> decode_image(const void* input_buffer, size_t input_buffer_length);

Image generate_background_image(int width, int height, RGBAColour topleft, RGBAColour topright, RGBAColour botleft, RGBAColour botright);
Image lerp_image(const Image& lhs, const Image& rhs, double t);
Image resize_image(const Image& input, int out_width, int out_height);
Image transpose_image(const Image& input);
Image blur_image(const Image& input, int radius);

void toggle_image_rgba_bgra_inplace(Image& img);
