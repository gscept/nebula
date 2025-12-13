#ifndef INCLUDE_STB_IMAGE_WRITE_16_H
#define INCLUDE_STB_IMAGE_WRITE_16_H

#include <stdlib.h>

// if STB_IMAGE_WRITE_STATIC causes problems, try defining STBIWDEF to 'inline' or 'static inline'
#ifndef STBIW16DEF
#ifdef STB_IMAGE_WRITE_16_STATIC
#define STBIW16DEF  static
#else
#ifdef __cplusplus
#define STBIW16DEF  extern "C"
#else
#define STBIW16DEF  extern
#endif
#endif
#endif

#ifndef STB_IMAGE_WRITE_16_STATIC  // C++ forbids static forward declarations
extern int stbi_write_png_16_compression_level;
extern int stbi_write_force_png_16_filter;
#endif

#ifndef STBI_WRITE_16_NO_STDIO
STBIW16DEF int stbi_write_png16(char const* filename, int w, int h, int comp, const void* data, int stride_in_bytes);
STBIW16DEF int stbi_write_png16_to_func(stbi_write_func* func, void* context, int w, int h, int comp, const void* data, int stride_in_bytes);

#endif // INCLUDE_STB_IMAGE_WRITE_16_H

#ifdef STB_IMAGE_WRITE_16_IMPLEMENTATION

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif
#endif


#ifndef STBI_WRITE_16_NO_STDIO
#include <stdio.h>
#endif // STBI_WRITE_NO_STDIO

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(STBIW_MALLOC) && defined(STBIW_FREE) && (defined(STBIW_REALLOC) || defined(STBIW_REALLOC_SIZED))
// ok
#elif !defined(STBIW_MALLOC) && !defined(STBIW_FREE) && !defined(STBIW_REALLOC) && !defined(STBIW_REALLOC_SIZED)
// ok
#else
#error "Must define all or none of STBIW_MALLOC, STBIW_FREE, and STBIW_REALLOC (or STBIW_REALLOC_SIZED)."
#endif

#ifndef STBIW_MALLOC
#define STBIW_MALLOC(sz)        malloc(sz)
#define STBIW_REALLOC(p,newsz)  realloc(p,newsz)
#define STBIW_FREE(p)           free(p)
#endif

#ifndef STBIW_REALLOC_SIZED
#define STBIW_REALLOC_SIZED(p,oldsz,newsz) STBIW_REALLOC(p,newsz)
#endif


#ifndef STBIW_MEMMOVE
#define STBIW_MEMMOVE(a,b,sz) memmove(a,b,sz)
#endif


#ifndef STBIW_ASSERT
#include <assert.h>
#define STBIW_ASSERT(x) assert(x)
#endif

#define STBIW_UCHAR(x) (unsigned char) ((x) & 0xff)

#ifdef STB_IMAGE_WRITE_STATIC
static int stbi_write_png_16_compression_level = 8;
static int stbi_write_force_png_16_filter = -1;
#else
int stbi_write_png_16_compression_level = 8;
int stbi_write_force_png_16_filter = -1;
#endif

STBIW16DEF unsigned char* stbi_write_png16_to_mem(const unsigned short* pixels, int stride_bytes, int x, int y, int comp, int* out_len)
{
    int force_filter = stbi_write_force_png_16_filter;
    int ctype[5] = { -1, 0, 4, 2, 6 };
    unsigned char sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    unsigned char* out, * o, * filt, * zlib, * pixels8;
    signed char* line_buffer;
    int j, zlen;
    int bytes_per_pixel = comp * 2;

    /* Normalize stride_bytes:
       API expects stride = bytes from row start to next row start.
       Many callers pass per-pixel bytes (pixel_stride) instead of row stride;
       that causes src indexing to step incorrectly and overwrite memory.
       Accept pixel_stride as a convenience: if stride_bytes equals bytes_per_pixel,
       treat it as tightly-packed rows (x * bytes_per_pixel).
    */
    if (stride_bytes == 0)
        stride_bytes = x * bytes_per_pixel;

    if (force_filter >= 5) {
        force_filter = -1;
    }

    /* convert 16-bit samples to network-order bytes (big-endian) */
    pixels8 = (unsigned char*)STBIW_MALLOC((size_t)y * stride_bytes);
    if (!pixels8) return 0;
    {
        int src_row_shorts = stride_bytes / 2;
        int r;
        for (r = 0; r < y; ++r) {
            const unsigned short* src = pixels + (size_t)r * src_row_shorts;
            unsigned char* dst = pixels8 + (size_t)r * stride_bytes;
            int i, c;
            for (i = 0; i < x; ++i) {
                for (c = 0; c < comp; ++c) {
                    unsigned short v = src[i * comp + c];
                    /* PNG requires big-endian sample bytes */
                    dst[(i * bytes_per_pixel) + c * 2 + 0] = STBIW_UCHAR(v >> 8);
                    dst[(i * bytes_per_pixel) + c * 2 + 1] = STBIW_UCHAR(v);
                }
            }
        }
    }

    /* Build filtered image buffer (reuses existing filter logic, operating on bytes) */
    filt = (unsigned char*)STBIW_MALLOC((x * bytes_per_pixel + 1) * y); if (!filt) { STBIW_FREE(pixels8); return 0; }
    line_buffer = (signed char*)STBIW_MALLOC(x * bytes_per_pixel); if (!line_buffer) { STBIW_FREE(filt); STBIW_FREE(pixels8); return 0; }
    for (j = 0; j < y; ++j) {
        int filter_type;
        if (force_filter > -1) {
            filter_type = force_filter;
            stbiw__encode_png_line(pixels8, stride_bytes, x, y, j, bytes_per_pixel, force_filter, line_buffer);
        }
        else {
            int best_filter = 0, best_filter_val = 0x7fffffff, est, i;
            for (filter_type = 0; filter_type < 5; filter_type++) {
                stbiw__encode_png_line(pixels8, stride_bytes, x, y, j, bytes_per_pixel, filter_type, line_buffer);
                est = 0;
                for (i = 0; i < x * bytes_per_pixel; ++i) {
                    est += abs((signed char)line_buffer[i]);
                }
                if (est < best_filter_val) {
                    best_filter_val = est;
                    best_filter = filter_type;
                }
            }
            if (filter_type != best_filter) {
                stbiw__encode_png_line(pixels8, stride_bytes, x, y, j, bytes_per_pixel, best_filter, line_buffer);
                filter_type = best_filter;
            }
        }
        filt[j * (x * bytes_per_pixel + 1)] = (unsigned char)filter_type;
        STBIW_MEMMOVE(filt + j * (x * bytes_per_pixel + 1) + 1, line_buffer, x * bytes_per_pixel);
    }
    STBIW_FREE(line_buffer);
    STBIW_FREE(pixels8);

    zlib = stbi_zlib_compress(filt, y * (x * bytes_per_pixel + 1), &zlen, stbi_write_png_16_compression_level);
    STBIW_FREE(filt);
    if (!zlib) return 0;

    /* allocate output and write PNG chunks; IHDR bit depth = 16 */
    out = (unsigned char*)STBIW_MALLOC(8 + 12 + 13 + 12 + zlen + 12);
    if (!out) { STBIW_FREE(zlib); return 0; }
    *out_len = 8 + 12 + 13 + 12 + zlen + 12;

    o = out;
    STBIW_MEMMOVE(o, sig, 8); o += 8;
    stbiw__wp32(o, 13); // header length
    stbiw__wptag(o, "IHDR");
    stbiw__wp32(o, x);
    stbiw__wp32(o, y);
    *o++ = 16;                                        /* 16-bit depth */
    *o++ = STBIW_UCHAR(ctype[comp]);                  /* color type */
    *o++ = 0;
    *o++ = 0;
    *o++ = 0;
    stbiw__wpcrc(&o, 13);

    stbiw__wp32(o, zlen);
    stbiw__wptag(o, "IDAT");
    STBIW_MEMMOVE(o, zlib, zlen);
    o += zlen;
    STBIW_FREE(zlib);
    stbiw__wpcrc(&o, zlen);

    stbiw__wp32(o, 0);
    stbiw__wptag(o, "IEND");
    stbiw__wpcrc(&o, 0);

    STBIW_ASSERT(o == out + *out_len);

    return out;
}

STBIW16DEF int stbi_write_png16_to_func(stbi_write_func* func, void* context, int x, int y, int comp, const void* data, int stride_bytes)
{
    int len;
    unsigned char* png = stbi_write_png16_to_mem((const unsigned short*)data, stride_bytes, x, y, comp, &len);
    if (png == NULL) return 0;
    func(context, png, len);
    STBIW_FREE(png);
    return 1;
}

#ifndef STBI_WRITE_NO_STDIO
STBIW16DEF int stbi_write_png16(char const* filename, int x, int y, int comp, const void* data, int stride_bytes)
{
    FILE* f;
    int len;
    unsigned char* png = stbi_write_png16_to_mem((const unsigned short*)data, stride_bytes, x, y, comp, &len);
    if (png == NULL) return 0;

    f = stbiw__fopen(filename, "wb");
    if (!f) { STBIW_FREE(png); return 0; }
    fwrite(png, 1, len, f);
    fclose(f);
    STBIW_FREE(png);
    return 1;
}
#endif

#endif // STB_IMAGE_WRITE_16_IMPLEMENTATION

#endif // INCLUDE_STB_IMAGE_WRITE_16_H