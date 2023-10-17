#include "stdafx.h"

#pragma warning(push, 0)
#include "libdivide.h"
#include "stb_image.h"
#include "stb_image_resize2.h"
#pragma warning(pop)

#include "img_processing.h"
#include "logging.h"

RGBAColour from_colorref(COLORREF colour)
{
    const RGBAColour result = {GetRValue(colour), GetGValue(colour), GetBValue(colour), 255};
    return result;
}

// Normalized byte multiplication:
// Range [0, 255] maps to [0.0, 1.0]
// Taken from https://gist.github.com/d7samurai/f98cb2aa30a6d73e62a65a376d24c6da
// examples:
// 0xff * 0xff = 0xff   (255 * 255 = 255)
// 0xff * 0x7f = 0x7f   (255 * 127 = 127)
// 0x7f * 0x7f = 0x3f   (127 * 127 =  63)
// 0x01 * 0xff = 0x01   (  1 * 255 =   1)
// 0x01 * 0x7f = 0x00   (  1 * 127 =   0)
static uint8_t nmul(uint8_t a, uint8_t b)
{
    return (((uint16_t)a + 1) * b) >> 8;
}

// Linear interpolation between `lhs` and `rhs` by `factor.
// lerp_colour(lhs, rhs, 0) = lhs
// lerp_colour(lhs, rhs, 255) = rhs
RGBAColour lerp_colour(RGBAColour lhs, RGBAColour rhs, uint8_t factor)
{
    uint8_t r = nmul(lhs.r, 255 - factor) + nmul(rhs.r, factor);
    uint8_t g = nmul(lhs.g, 255 - factor) + nmul(rhs.g, factor);
    uint8_t b = nmul(lhs.b, 255 - factor) + nmul(rhs.b, factor);
    uint8_t a = nmul(lhs.a, 255 - factor) + nmul(rhs.a, factor);
    return RGBAColour{r,g,b,a};
}

// Bilinear interpolation between `topleft`, `topright`, `botleft` & `botright`
static RGBAColour blerp_colour(RGBAColour topleft, RGBAColour topright, RGBAColour botleft, RGBAColour botright, uint8_t factor_x, uint8_t factor_y)
{
    RGBAColour top = lerp_colour(topleft, topright, factor_x);
    RGBAColour bot = lerp_colour(botleft, botright, factor_x);
    RGBAColour result = lerp_colour(top, bot, factor_y);
    return result;
}

Image::Image(Image&& other)
    : pixels(other.pixels)
    , width(other.width)
    , height(other.height)
{
    other.pixels = nullptr;
}

Image& Image::operator =(Image&& other)
{
    free(pixels);

    pixels = other.pixels;
    width = other.width;
    height = other.height;

    other.pixels = nullptr;
    return *this;
}

Image::~Image()
{
    free(pixels);
}

bool Image::valid() const
{
    return pixels != nullptr;
}

std::optional<Image> load_image(const char* file_path)
{
    int width = 0;
    int height = 0;
    int available_channels = 0;
    uint8_t* pixels = stbi_load(file_path,
                                &width,
                                &height,
                                &available_channels,
                                4);
    if(pixels == nullptr)
    {
        LOG_INFO("FAILED TO LOAD ALBUM ART DATA!");
        return {};
    }

    Image result = {};
    result.pixels = pixels;
    result.width = width;
    result.height = height;
    return result;
}

std::optional<Image> decode_image(const void* input_buffer, size_t input_buffer_length)
{
    // TODO: For some unknown reason (I still need to check the docs),
    //       Bad Things happen if you try to use 24-bit/3-byte RGB
    //       (not RGBA) images with StretchDIBits.
    int width = 0;
    int height = 0;
    int available_channels = 0;
    uint8_t* pixels = stbi_load_from_memory((const uint8_t*)input_buffer,
                                            input_buffer_length,
                                            &width,
                                            &height,
                                            &available_channels,
                                            4);
    if(pixels == nullptr)
    {
        LOG_INFO("FAILED TO LOAD ALBUM ART DATA!");
        return {};
    }

    Image result = {};
    result.pixels = pixels;
    result.width = width;
    result.height = height;
    return result;
}

Image generate_background_colour(int width, int height, RGBAColour colour)
{
    assert(width >= 0);
    assert(height >= 0);
    uint8_t* pixels = (uint8_t*)malloc(width * height * 4);

    for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++)
        {
            uint8_t* px = pixels + 4*(y*width + x);
            px[0] = colour.r;
            px[1] = colour.g;
            px[2] = colour.b;
            px[3] = 255; // It's the background, we don't care about transparency
        }
    }

    Image result = {};
    result.pixels = pixels;
    result.width = width;
    result.height = height;
    return result;
}

Image generate_background_colour(int width, int height, RGBAColour topleft, RGBAColour topright, RGBAColour botleft, RGBAColour botright)
{
    assert(width >= 0);
    assert(height >= 0);
    uint8_t* pixels = (uint8_t*)malloc(width * height * 4);

    for(int y=0; y<height; y++)
    {
        const uint8_t factor_y = ((255 * y)/(height-1)) & 0xFF;
        for(int x=0; x<width; x++)
        {
            const uint8_t factor_x = ((255 * x)/(width-1)) & 0xFF;
            const RGBAColour colour = blerp_colour(topleft, topright, botleft, botright, factor_x, factor_y);

            uint8_t* px = pixels + 4*(y*width + x);
            px[0] = colour.r;
            px[1] = colour.g;
            px[2] = colour.b;
            px[3] = 255; // It's the background, we don't care about transparency
        }
    }

    Image result = {};
    result.pixels = pixels;
    result.width = width;
    result.height = height;
    return result;
}

Image lerp_image(const Image& lhs, const Image& rhs, double t)
{
    assert(lhs.width == rhs.width);
    assert(lhs.height == rhs.height);

    const uint8_t factor = uint8_t(255.0 * t);
    uint8_t* pixels = (uint8_t*)malloc(lhs.width * lhs.height * 4);
    for(int y=0; y<lhs.height; y++)
    {
        for(int x=0; x<lhs.width; x++)
        {
            int offset = 4*(y*lhs.width + x);
            uint8_t* lhs_px = lhs.pixels + offset;
            uint8_t* rhs_px = rhs.pixels + offset;
            uint8_t* out_px = pixels + offset;

            RGBAColour lhs_colour = *((RGBAColour*)lhs_px);
            RGBAColour rhs_colour = *((RGBAColour*)rhs_px);
            RGBAColour out_colour = lerp_colour(lhs_colour, rhs_colour, factor);

            out_px[0] = out_colour.r;
            out_px[1] = out_colour.g;
            out_px[2] = out_colour.b;
            out_px[3] = 255;
        }
    }

    Image result = {};
    result.pixels = pixels;
    result.width = lhs.width;
    result.height = lhs.height;
    return result;
}

Image lerp_offset_image(const Image& full_img, const Image& offset_img, CPoint offset, double t)
{
    assert(offset.x >= 0);
    assert(offset.y >= 0);
    assert(full_img.width >= offset.x + offset_img.width);
    assert(full_img.height >= offset.y + offset_img.height);

    const uint8_t factor = uint8_t(255.0 * t);
    uint8_t* pixels = (uint8_t*)malloc(full_img.width * full_img.height * 4);

    // Just memcopy whatever rows at the top contain only the full image
    memcpy(pixels, full_img.pixels, full_img.width * offset.y * 4);

    for(int y=0; y<offset_img.height; y++)
    {
        const int full_y = offset.y + y;
        uint8_t* out_row = pixels + 4*full_img.width*full_y;
        uint8_t* in_full_row = full_img.pixels + 4*full_img.width*full_y;
        uint8_t* in_offset_row = offset_img.pixels + 4*offset_img.width*y;

        // memcopy pixels to the left of the offset image
        memcpy(out_row, in_full_row, offset.x * 4);

        for(int x=0; x<offset_img.width; x++)
        {
            const int full_x = offset.x + x;
            uint8_t* full_px = in_full_row + 4*full_x;
            uint8_t* offset_px = in_offset_row + 4*x;
            uint8_t* out_px = out_row + 4*full_x;

            RGBAColour full_colour = *((RGBAColour*)full_px);
            RGBAColour offset_colour = *((RGBAColour*)offset_px);
            RGBAColour out_colour = lerp_colour(full_colour, offset_colour, factor);

            out_px[0] = out_colour.r;
            out_px[1] = out_colour.g;
            out_px[2] = out_colour.b;
            out_px[3] = 255;
        }

        const int columns_on_or_before_offset_img = offset.x + offset_img.width;
        const int columns_after_offset_img = full_img.width - columns_on_or_before_offset_img;
        const int post_offsetimg_row_offset = 4*columns_on_or_before_offset_img;
        assert(columns_after_offset_img >= 0);
        memcpy(out_row + post_offsetimg_row_offset, in_full_row + post_offsetimg_row_offset, 4*columns_after_offset_img);
    }

    // Just memcopy whatever rows at the bottom contain only the full image
    const int rows_on_or_above_offset_img = offset.y + offset_img.height;
    const int rows_below_offset_img = full_img.height - rows_on_or_above_offset_img;
    uint8_t* first_full_pixel_below_offset_img = full_img.pixels + 4*full_img.width*rows_on_or_above_offset_img;
    uint8_t* first_out_pixels_below_offset_img = pixels + 4*full_img.width*rows_on_or_above_offset_img;
    assert(rows_below_offset_img >= 0);
    memcpy(first_out_pixels_below_offset_img, first_full_pixel_below_offset_img, full_img.width * rows_below_offset_img * 4);

    Image result = {};
    result.pixels = pixels;
    result.width = full_img.width;
    result.height = full_img.height;
    return result;
}

Image resize_image(const Image& input, int out_width, int out_height)
{
    if(!input.valid())
    {
        return {};
    }

    uint8_t* resized_pixels = stbir_resize_uint8_linear(
            input.pixels, input.width, input.height,  0/*stride*/,
            nullptr/*output_pixels*/, out_width, out_height, 0/*stride*/,
            STBIR_4CHANNEL);

    Image result = {};
    result.pixels = std::move(resized_pixels);
    result.width = out_width;
    result.height = out_height;
    return result;
}

void transpose_image_noalloc(int width, int height, const uint8_t* in_pixels, uint8_t* out_pixels)
{
    // Instead of doing the entire image at once, we transpose in "blocks" for better cache efficiency
    const int block_size = 256;
    for(int block_y=0; block_y<height; block_y+=block_size)
    {
        for(int x=0; x < width; x++)
        {
            const uint8_t* in_block_column = in_pixels + 4*(block_y*width + x);
            uint8_t* out_foo = out_pixels + 4*(x*height + block_y);
            for(int y=0; (y<block_size) && (block_y+y<height); y++)
            {
                const uint8_t* in_px = in_block_column + 4*y*width;
                uint8_t* out_px = out_foo + 4*y;
                memcpy(out_px, in_px, 4);
            }
        }
    }
}
Image transpose_image(const Image& img)
{
    TIME_FUNCTION();

    uint8_t* pixels = (uint8_t*)malloc(img.width * img.height * 4);
    transpose_image_noalloc(img.width, img.height, img.pixels, pixels);

    Image result = {};
    result.width = img.height;
    result.height = img.width;
    result.pixels = pixels;
    return result;
}

static void boxblur_horizontal_noalloc(int width, int height, const uint8_t* in_pixels, uint8_t* out_pixels, int radius)
{
    if(width <= 2*radius+1)
    {
        // We assume the image is wide enough to contain at least one full blur window.
        LOG_INFO("Skipping blur as image of width %d is too small to have a blur window of size %d",
                width, 2*radius+1);
        return;
    }
    assert(width > 2*radius+1);

    // A box blur requires that we total up the pixel values across the blur window and
    // divide them by the window width. This is an integer division and therefore is
    // incredibly slow. Using libdivide speeds it up significantly on its own, but also
    // allows us to express integer division in terms of integer additions, multiplies
    // and shifts, which in turn allows us to apply SIMD to compute several divisions
    // in parallel.
    libdivide::libdivide_u16_branchfree_t divisor = libdivide::libdivide_u16_branchfree_gen(2*uint16_t(radius)+1);
    for(int y=0; y<height; y++)
    {
        const uint8_t* in_row = &in_pixels[4*width*y];
        uint8_t* out_row = &out_pixels[4*width*y];

        // Prime the accumulation buffer
        uint16_t accum[4] = {};
        for(int accum_x=-radius; accum_x<radius+1; accum_x++)
        {
            int x = max(0, min(width-1, accum_x));
            for(int c=0; c<4; c++)
            {
                accum[c] += in_pixels[4*(width*y + x) + c];
            }
        }

        // A naive implementation would need to clamp the old & new pixel X values to
        // within the range [0,width]. This clamp is a branch and proved to be a CPU
        // bottleneck. We eliminate that by splitting the loop across a row into 3
        // separate loops:
        // * One where the X is "hard coded" as "clamped to zero"
        // * One where we assume the entire blur window falls within the valid horizontal
        //   range. This loop covers almost all pixels.
        // * One where X is "hard coded" as "clamped to `width-1`"
        for(int x=0; x<radius; x++)
        {
            for(int c=0; c<4; c++)
            {
                uint16_t val = libdivide::libdivide_u16_branchfree_do(accum[c], &divisor);
                out_pixels[4*(width*y + x) + c] = val & 0xFF;
            }

            const int old_x = 0;
            const int new_x = x+radius+1;

            for(int c=0; c<4; c++)
            {
                const uint8_t oldval = in_pixels[(4*(width*y + old_x) + c)];
                const uint8_t newval = in_pixels[(4*(width*y + new_x) + c)];
                accum[c] += newval;
                accum[c] -= oldval;
            }
        }

        const __m128i zero = {};
        __m128i accum128 = _mm_loadu_si64(accum);
        for(int x=radius; x<width-radius-1; x++)
        {
            // This is the main loop. Effectively all it needs to do is this:
            //     for(int c=0; c<4; c++)
            //     {
            //         uint16_t val = libdivide::libdivide_u16_branchfree_do(accum[c], &divisor);
            //         out_row[4*x + c] = val & 0xFF;
            //     }
            // however if we inline the calls to libdivide:
            //     uint16_t libdivide_u16_branchfree_do(
            //         uint16_t numer, const struct libdivide_u16_branchfree_t *denom) {
            //         uint16_t q = libdivide_mullhi_u16(denom->magic, numer);
            //         uint16_t t = ((numer - q) >> 1) + q;
            //         return t >> denom->more;
            //     }
            //     static LIBDIVIDE_INLINE uint16_t libdivide_mullhi_u16(uint16_t x, uint16_t y) {
            //         uint32_t xl = x, yl = y;
            //         uint32_t rl = xl * yl;
            //         return (uint16_t)(rl >> 16);
            //     }
            // then we can express that calculation using SIMD intrinsics and compute 4 of
            // them in parallel, allowing us to do all 4 channels of the pixel at once.
            __m128i magic = _mm_set1_epi16(divisor.magic);
            __m128i numer = accum128;
            __m128i q = _mm_mulhi_epu16(magic, numer);
            __m128i t1 = _mm_sub_epi16(numer, q);
            __m128i t2 = _mm_srli_epi16(t1, 1);
            __m128i t3 = _mm_add_epi16(t2, q);
            __m128i val = _mm_srli_epi16(t3, divisor.more);
            __m128i packed_val = _mm_packus_epi16(val, val);
            _mm_storeu_si64(&out_row[4*x], packed_val);

            const int old_x = x - radius;
            const int new_x = x + radius + 1;
            const uint8_t* old_pixel_ptr = &in_row[4*old_x];
            const uint8_t* new_pixel_ptr = &in_row[4*new_x];

            // Now that we've computed the output pixel, we need to update the accumulator
            // to remove the 'old' pixel on the left of the blur window, and add in the
            // 'new' pixel on the right of the blur window.
            // A naive implementation would look like this:
            //     for(int c=0; c<4; c++)
            //     {
            //         accum[c] += new_pixel[c];
            //         accum[c] -= old_pixel[c];
            //     }
            // as above, since this is just simple addition, we can SIMD it to do the
            // entire accumulator at once.
            __m128i old_pixel_packed = _mm_loadu_si32(old_pixel_ptr);
            __m128i new_pixel_packed = _mm_loadu_si32(new_pixel_ptr);
            __m128i old_pixel = _mm_unpacklo_epi8(old_pixel_packed, zero);
            __m128i new_pixel = _mm_unpacklo_epi8(new_pixel_packed, zero);
            __m128i accum_tmp = _mm_add_epi16(accum128, new_pixel);
            accum128 = _mm_sub_epi16(accum_tmp, old_pixel);
        }
        _mm_storeu_si64(accum, accum128);

        for(int x=width-radius-1; x<width; x++)
        {
            for(int c=0; c<4; c++)
            {
                uint16_t val = libdivide::libdivide_u16_branchfree_do(accum[c], &divisor);
                out_pixels[4*(width*y + x) + c] = val & 0xFF;
            }

            const int old_x = x-radius;
            const int new_x = width-1;

            for(int c=0; c<4; c++)
            {
                const uint8_t oldval = in_pixels[(4*(width*y + old_x) + c)];
                const uint8_t newval = in_pixels[(4*(width*y + new_x) + c)];
                accum[c] += newval;
                accum[c] -= oldval;
            }
        }
    }
}
static Image image_boxblur_linear_horizontal(const Image& img, int radius)
{
    uint8_t* pixels = (uint8_t*)malloc(img.width * img.height * 4);

    boxblur_horizontal_noalloc(img.width, img.height, img.pixels, pixels, radius);

    Image result = {};
    result.width = img.width;
    result.height = img.height;
    result.pixels = pixels;
    return result;
}

Image blur_image(const Image& img, int radius)
{
    if(radius > img.width/3) radius = img.width/3;
    if(radius > img.height/3) radius = img.height/3;

    // This is a repeated box blur that approximates a gaussian blur.
    // Technically for correctness we should be doing several blurs with different
    // radii to better approximate a gaussian curve, but for our use-case we don't
    // actually care about it being a "correct" *Gaussian* blur, it just needs to blur.
    // For more info see:
    // * https://blog.ivank.net/fastest-gaussian-blur.html
    // * https://www.peterkovesi.com/papers/FastGaussianSmoothing.pdf
    // * https://github.com/bfraboni/FastGaussianBlur
    Image transposed = transpose_image(img);

    Image vblurred1 = image_boxblur_linear_horizontal(transposed, radius);
    Image vblurred2 = image_boxblur_linear_horizontal(vblurred1, radius);
    Image vblurred3 = image_boxblur_linear_horizontal(vblurred2, radius);

    Image untransed = transpose_image(vblurred3);

    Image result1 = image_boxblur_linear_horizontal(untransed, radius);
    Image result2 = image_boxblur_linear_horizontal(result1, radius);
    Image result3 = image_boxblur_linear_horizontal(result2, radius);

    return result3;
}

// Convert an RGBA image into a BGRA image (or vice-versa)
void toggle_image_rgba_bgra_inplace(Image& img)
{
    for(int y=0; y<img.height; y++)
    {
        for(int x=0; x<img.width; x++)
        {
            uint8_t* px = img.pixels + (y*img.width + x)*4;
            uint8_t r = px[0];
            uint8_t b = px[2];
            px[0] = b;
            px[2] = r;
        }
    }
}
