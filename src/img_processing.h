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
    Image& operator=(Image&& other);
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    ~Image();
    bool valid() const;
};

RGBAColour from_colorref(COLORREF colour);
RGBAColour lerp_colour(RGBAColour lhs, RGBAColour rhs, uint8_t factor);

std::optional<Image> load_image(const char* file_path);
std::optional<Image> decode_image(const void* input_buffer, size_t input_buffer_length);

Image generate_background_colour(int width, int height, RGBAColour colour);
Image generate_background_colour(int width,
                                 int height,
                                 RGBAColour topleft,
                                 RGBAColour topright,
                                 RGBAColour botleft,
                                 RGBAColour botright);
Image lerp_image(const Image& lhs, const Image& rhs, double t);
Image lerp_offset_image(const Image& full_img, const Image& offset_img, CPoint offset, double t);
Image resize_image(const Image& input, int out_width, int out_height);
Image transpose_image(const Image& input);
RGBAColour compute_average_colour(const Image& img);
Image blur_image(const Image& input, int radius);

void toggle_image_rgba_bgra_inplace(Image& img);
