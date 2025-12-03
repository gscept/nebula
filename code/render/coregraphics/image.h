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

enum ImageContainer
{
    PNG,
    JPEG,
    TGA,
    HDR
};

enum ImageChannelPrimitive
{
    Bit8UInt,
    Bit16UInt,
    Bit16Float,
    Bit32UInt,
    Bit32Float
};

/// create image from file path
ImageId CreateImage(const ImageCreateInfoFile& info);
/// create image from data buffer
ImageId CreateImage(const ImageCreateInfoData& info);
/// Create image from a texture and the pipeline stage it was last used with
ImageId CreateImage(const CoreGraphics::TextureId tex, CoreGraphics::PipelineStage stage);
/// destroy image
void DestroyImage(const ImageId id);

struct ImageDimensions
{
    SizeT width, height;
};
/// get image dimensions
ImageDimensions ImageGetDimensions(const ImageId id);

/// Get pointer to buffer
const ubyte* ImageGetBuffer(const ImageId id);
/// Get pixel stride in bytes
const SizeT ImageGetPixelStride(const ImageId id);
/// Get channel stride in bytes
const SizeT ImageGetChannelStride(const ImageId id);
/// Convert image primitive
void ImageConvertPrimitive(const ImageId id, const ImageChannelPrimitive primitive, bool denormalize);

/// Save image to file
bool ImageSaveToFile(const ImageId id, const ImageContainer container, const IO::URI& path);

/// get channel primitive
ImageChannelPrimitive ImageGetChannelPrimitive(const ImageId id);

struct ImageLoadInfo
{
    SizeT width, height, channels;
    ImageChannelPrimitive primitive;
    PixelFormat::Code format;
    uint8_t redOffset, greenOffset, blueOffset, alphaOffset;

    union ImageData
    {
        unsigned char* stbiData8;
        unsigned short* stbiData16;
        unsigned int* stbiData32;
        float* stbiDataFloat;
    } data;
};

typedef Ids::IdAllocator<
    ImageLoadInfo
> ImageAllocator;
extern ImageAllocator imageAllocator;

} // namespace CoreGraphics
