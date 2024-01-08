#pragma once
//------------------------------------------------------------------------------
/**
    @file image.h
    
    Implements a CPU side image

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idallocator.h"
#include "pixelformat.h"
#include "io/uri.h"
namespace CoreGraphics
{

ID_24_8_TYPE(ImageId);


struct ImageCreateInfoFile
{
    IO::URI path;
    bool convertTo32Bit;
};

struct ImageCreateInfoData
{
    CoreGraphics::PixelFormat format;
    SizeT width, height, depth;
    void* data;
};

/// create image from file path
ImageId CreateImage(const ImageCreateInfoFile& info);
/// create image from data buffer
ImageId CreateImage(const ImageCreateInfoData& info);
/// destroy image
void DestroyImage(const ImageId id);

struct ImageDimensions
{
    SizeT width, height, depth;
};
/// get image dimensions
ImageDimensions ImageGetDimensions(const ImageId id);

/// get pointer to buffer
const byte* ImageGetBuffer(const ImageId id);
/// get pointer to first element of red channel
const byte* ImageGetRedPtr(const ImageId id);
/// get pointer to first element of green channel
const byte* ImageGetGreenPtr(const ImageId id);
/// get pointer to first element of blue channel
const byte* ImageGetBluePtr(const ImageId id);
/// get pointer to first element if alpha channel
const byte* ImageGetAlphaPtr(const ImageId id);
/// get pixel stride, using the above pointers makes it possible to get all reds, blues, greens, etc.
const SizeT ImageGetPixelStride(const ImageId id);

enum ImageChannelPrimitive
{
    Bit8Uint,
    Bit16Uint,
    Bit16Float,
    Bit32Uint,
    Bit32Float
};
/// get channel primitive
ImageChannelPrimitive ImageGetChannelPrimitive(const ImageId id);

enum ImageContainer
{
    PNG,
    JPEG,
    DDS
};


struct ImageLoadInfo
{
    SizeT width, height, depth;
    ImageChannelPrimitive primitive;
    PixelFormat::Code format;
    ImageContainer container;
    uint8_t redOffset, greenOffset, blueOffset, alphaOffset;
    byte* buffer;
};

typedef Ids::IdAllocator<
    ImageLoadInfo
> ImageAllocator;
extern ImageAllocator imageAllocator;

} // namespace CoreGraphics
