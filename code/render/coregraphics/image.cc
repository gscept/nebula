//------------------------------------------------------------------------------
//  image.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "coregraphics/config.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace CoreGraphics
{


//------------------------------------------------------------------------------
/**
*/
ImageChannelPrimitive
ToImagePrimitive(CoreGraphics::PixelFormat::Code code)
{
    switch (code)
    {
        case PixelFormat::R8:
        case PixelFormat::R8G8B8:
        case PixelFormat::R8G8B8X8:
        case PixelFormat::R8G8B8A8:
        case PixelFormat::B8G8R8A8:
            return ImageChannelPrimitive::Bit8UInt;
        case PixelFormat::R16F:
        case PixelFormat::R16G16F:
        case PixelFormat::R16G16B16A16F:
            return ImageChannelPrimitive::Bit16Float;
        case PixelFormat::R16:
        case PixelFormat::R16G16:
        case PixelFormat::R16G16B16A16:
            return ImageChannelPrimitive::Bit16UInt;
        case PixelFormat::R32F:
        case PixelFormat::R32G32F:
        case PixelFormat::R32G32B32F:
        case PixelFormat::R32G32B32A32F:
            return ImageChannelPrimitive::Bit32Float;
        case PixelFormat::R32:
        case PixelFormat::R32G32:
        case PixelFormat::R32G32B32:
        case PixelFormat::R32G32B32A32:
            return ImageChannelPrimitive::Bit32UInt;
        default:
            n_error("Unsupported PixelFormat for Image");
            return ImageChannelPrimitive::Bit8UInt;
    }
}

CmdBufferPoolId cmdPool = CmdBufferPoolId::Invalid();
ImageAllocator imageAllocator;
//------------------------------------------------------------------------------
/**
*/
ImageId
CreateImage(const ImageCreateInfoFile& info)
{
    Ids::Id32 image = imageAllocator.Alloc();
    ImageLoadInfo& loadInfo = imageAllocator.Get<0>(image);
    stbi_uc* data = nullptr;
    if (stbi_is_16_bit(info.path.LocalPath().AsCharPtr()))
    {
        loadInfo.data.stbiData16 = stbi_load_16(info.path.AsString().AsCharPtr(), &loadInfo.width, &loadInfo.height, &loadInfo.channels, STBI_rgb_alpha);
        loadInfo.primitive = Bit16UInt;
    }
    else if (stbi_is_hdr(info.path.LocalPath().AsCharPtr()))
    {
        loadInfo.data.stbiDataFloat = stbi_loadf(info.path.AsString().AsCharPtr(), &loadInfo.width, &loadInfo.height, &loadInfo.channels, STBI_rgb_alpha);
        loadInfo.primitive = Bit32Float;
    }
    else
    {
        loadInfo.data.stbiData8 = stbi_load(info.path.AsString().AsCharPtr(), &loadInfo.width, &loadInfo.height, &loadInfo.channels, STBI_rgb_alpha);
        loadInfo.primitive = Bit8UInt;
    }

    return ImageId(image);
}

//------------------------------------------------------------------------------
/**
*/
ImageId
CreateImage(const ImageCreateInfoData& info)
{
    return ImageId();
}

//------------------------------------------------------------------------------
/**
*/
ImageId
CreateImage(const CoreGraphics::TextureId tex, CoreGraphics::PipelineStage stage)
{
    if (cmdPool == CmdBufferPoolId::Invalid())
    {
        CmdBufferPoolCreateInfo cmdPoolInfo;
        cmdPoolInfo.name = "ImageLoaderCmdPool";
        cmdPoolInfo.queue = CoreGraphics::QueueType::GraphicsQueueType;
        cmdPoolInfo.resetable = false;
        cmdPoolInfo.shortlived = true;
        cmdPool = CoreGraphics::CreateCmdBufferPool(cmdPoolInfo);
    }

    CoreGraphics::TextureDimensions dims = TextureGetDimensions(tex);
    CoreGraphics::PixelFormat::Code format = TextureGetPixelFormat(tex);
    CoreGraphics::CmdBufferCreateInfo cmdBufInfo;
    cmdBufInfo.pool = cmdPool;
    cmdBufInfo.usage = CoreGraphics::QueueType::GraphicsQueueType;
    cmdBufInfo.name = "ImageCreateTextureRead";
    CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::CreateCmdBuffer(cmdBufInfo);
    CoreGraphics::CmdBufferBeginInfo beginInfo;
    beginInfo.resubmittable = false;
    beginInfo.submitDuringPass = false;
    beginInfo.submitOnce = true;
    CoreGraphics::CmdBeginRecord(cmdBuf, beginInfo);

    CoreGraphics::TextureCopy texCopy;
    texCopy.bits = ImageBits::ColorBits;
    texCopy.layer = 0;
    texCopy.mip = 0;
    texCopy.region = Math::rectangle<SizeT>(0, 0, dims.width, dims.height);

    CoreGraphics::BufferCopy bufCopy;
    bufCopy.imageHeight = 0;
    bufCopy.offset = 0;
    bufCopy.rowLength = 0;
    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.byteSize = dims.width * dims.height * CoreGraphics::PixelFormat::ToSize(format);
    bufInfo.usageFlags = CoreGraphics::BufferUsage::TransferDestination;
    bufInfo.mode = CoreGraphics::BufferAccessMode::HostCached;
    CoreGraphics::BufferId buf = CoreGraphics::CreateBuffer(bufInfo);

    // Perform the copy immediately on the GPU
    CoreGraphics::CmdBarrier(cmdBuf, stage, CoreGraphics::PipelineStage::TransferRead, 
        CoreGraphics::BarrierDomain::Global, 
        {
            CoreGraphics::TextureBarrierInfo{ tex, TextureSubresourceInfo() }
        }
    );
    CoreGraphics::CmdCopy(cmdBuf, tex, { texCopy }, buf, { bufCopy });
    CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::TransferRead, stage,
        CoreGraphics::BarrierDomain::Global,
        {
            CoreGraphics::TextureBarrierInfo{ tex, TextureSubresourceInfo() }
        }
    );
    CoreGraphics::FenceId fence = CoreGraphics::CreateFence({false});
    CoreGraphics::CmdEndRecord(cmdBuf);
    CoreGraphics::SubmitCommandBufferImmediate(cmdBuf, CoreGraphics::QueueType::GraphicsQueueType, fence);
    CoreGraphics::FenceWait(fence, FENCE_WAIT_FOREVER);

    CoreGraphics::DestroyCmdBuffer(cmdBuf);

    Ids::Id32 image = imageAllocator.Alloc();
    ImageLoadInfo& loadInfo = imageAllocator.Get<0>(image);
    loadInfo.width = dims.width;
    loadInfo.height = dims.height;
    loadInfo.format = format;
    loadInfo.channels = CoreGraphics::PixelFormat::ToChannels(format);
    loadInfo.primitive = ToImagePrimitive(format);
    void* data = Memory::Alloc(Memory::ResourceHeap, bufInfo.byteSize);
    void* bufData = CoreGraphics::BufferMap(buf);
    memcpy(data, bufData, bufInfo.byteSize);
    loadInfo.data.stbiData8 = (unsigned char*)data;
    CoreGraphics::BufferUnmap(buf);
    CoreGraphics::DestroyBuffer(buf);

    return ImageId(image);
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyImage(const ImageId id)
{
    ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id.id);
    Memory::Free(Memory::ResourceHeap, loadInfo.data.stbiData8);
}

//------------------------------------------------------------------------------
/**
*/
ImageDimensions
ImageGetDimensions(const ImageId id)
{
    ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id.id);
    return ImageDimensions{ loadInfo.width, loadInfo.height };
}

//------------------------------------------------------------------------------
/**
*/
const ubyte* 
ImageGetBuffer(const ImageId id)
{
    ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id.id);
    return loadInfo.data.stbiData8;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT 
ImageGetPixelStride(const ImageId id)
{
    ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id.id);
    return CoreGraphics::PixelFormat::ToSize(loadInfo.format);
}


//------------------------------------------------------------------------------
/**
*/
const SizeT
ImageGetChannelStride(const ImageId id)
{
    ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id.id);
    return CoreGraphics::PixelFormat::ToSize(loadInfo.format) / loadInfo.channels;
}

//------------------------------------------------------------------------------
/**
*/
float
half_to_float(uint16_t h)
{
    uint16_t h_exp = (h & 0x7C00u);
    uint32_t f_sgn = (uint32_t)(h & 0x8000u) << 16;
    uint32_t f_exp, f_sig;

    if (h_exp == 0) {
        // Zero or subnormal
        if ((h & 0x7FFFu) == 0) {
            // ±0
            uint32_t f = f_sgn;
            float out;
            memcpy(&out, &f, sizeof(f));
            return out;
        }

        // Normalize subnormal half
        uint16_t mant = (h & 0x03FFu);
        int shift = 10;
        while ((mant & 0x0400u) == 0) {
            mant <<= 1;
            shift--;
        }
        mant &= 0x03FFu;

        f_exp = (127 - 15 - (10 - shift)) << 23;
        f_sig = ((uint32_t)mant) << 13;
    }
    else if (h_exp == 0x7C00u) {
        // Inf or NaN
        f_exp = 0xFFu << 23;
        f_sig = ((uint32_t)(h & 0x03FFu)) << 13;
    }
    else {
        // Normalized
        f_exp = ((uint32_t)((h_exp >> 10) + (127 - 15))) << 23;
        f_sig = ((uint32_t)(h & 0x03FFu)) << 13;
    }

    uint32_t f = f_sgn | f_exp | f_sig;
    float out;
    memcpy(&out, &f, sizeof(f));
    return out;
}

//------------------------------------------------------------------------------
/**
*/
void
ImageConvertPrimitive(const ImageId id, const ImageChannelPrimitive primitive, bool denormalize)
{
    ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id.id);
    switch (loadInfo.primitive)
    {
        case Bit16Float:
        {
            switch (primitive)
            {
                case Bit16UInt:
                {
                    SizeT pixelSize = CoreGraphics::PixelFormat::ToSize(loadInfo.format);
                    SizeT newSize = loadInfo.width * loadInfo.height * pixelSize;
                    unsigned short* newData = (unsigned short*)Memory::Alloc(Memory::ResourceHeap, newSize);
                    for (SizeT i = 0; i < loadInfo.width * loadInfo.height * loadInfo.channels; i++)
                    {
                        float value = half_to_float(loadInfo.data.stbiData16[i]);
                        if (denormalize)
                            value *= 65535 + 0.5f;
                        uint16_t pixel = (uint16_t)(value);
                        newData[i] = pixel;
                    }
                    Memory::Free(Memory::ResourceHeap, loadInfo.data.stbiDataFloat);
                    loadInfo.data.stbiData16 = newData;
                    break;
                }
                case Bit8UInt:
                {
                    SizeT pixelSize = CoreGraphics::PixelFormat::ToSize(loadInfo.format);
                    SizeT newSize = loadInfo.width * loadInfo.height * pixelSize / 2;
                    unsigned char* newData = (unsigned char*)Memory::Alloc(Memory::ResourceHeap, newSize);
                    for (SizeT i = 0; i < loadInfo.width * loadInfo.height * loadInfo.channels; i++)
                    {
                        float value = half_to_float(loadInfo.data.stbiData16[i]);
                        if (denormalize)
                            value *= 255.0f + 0.5f;
                        uint8_t pixel = (uint8_t)(value);
                        newData[i] = pixel;
                    }
                    Memory::Free(Memory::ResourceHeap, loadInfo.data.stbiDataFloat);
                    loadInfo.data.stbiData8 = newData;
                    break;
                }
                default:
                    n_error("Unsupported image primitive conversion!");
                    break;
            }
            break;
        }
        case Bit32UInt: // When converting from 32 bit uint, the best we can do is to truncate
        {
            switch (primitive)
            {
                case Bit8UInt:
                {
                    SizeT pixelSize = CoreGraphics::PixelFormat::ToSize(loadInfo.format);
                    SizeT newSize = loadInfo.width * loadInfo.height * pixelSize / 4;
                    unsigned char* newData = (unsigned char*)Memory::Alloc(Memory::ResourceHeap, newSize);
                    for (SizeT i = 0; i < loadInfo.width * loadInfo.height * loadInfo.channels; i++)
                    {
                        uint32_t value = loadInfo.data.stbiData32[i];
                        uint8_t pixel = (uint8_t)(value);
                        newData[i] = pixel;
                    }
                    Memory::Free(Memory::ResourceHeap, loadInfo.data.stbiData32);
                    loadInfo.data.stbiData8 = newData;
                    break;
                }
                case Bit16UInt:
                {
                    SizeT pixelSize = CoreGraphics::PixelFormat::ToSize(loadInfo.format);
                    SizeT newSize = loadInfo.width * loadInfo.height * pixelSize / 2;
                    unsigned short* newData = (unsigned short*)Memory::Alloc(Memory::ResourceHeap, newSize);
                    for (SizeT i = 0; i < loadInfo.width * loadInfo.height * loadInfo.channels; i++)
                    {
                        uint32_t value = loadInfo.data.stbiData32[i];
                        uint16_t pixel = (uint16_t)(value);
                        newData[i] = pixel;
                    }
                    Memory::Free(Memory::ResourceHeap, loadInfo.data.stbiData32);
                    loadInfo.data.stbiData16 = newData;
                    break;
                }
                default:
                    n_error("Unsupported image primitive conversion!");
                    break;
            }
            break;
        }
    }
    loadInfo.primitive = primitive;
}

//------------------------------------------------------------------------------
/**
*/
bool
ImageSaveToFile(const ImageId id, const ImageContainer container, const IO::URI& path)
{
    ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id.id);
    SizeT stride = CoreGraphics::PixelFormat::ToSize(loadInfo.format);

    int res = 0;
    switch (container)
    {
        case ImageContainer::PNG:
            n_assert_msg(loadInfo.primitive == Bit8UInt || loadInfo.primitive == Bit16UInt, "PNG only allows 8/16bit integer images");
            if (loadInfo.primitive == Bit16UInt)
                res = stbi_write_png16(path.LocalPath().AsCharPtr(), loadInfo.width, loadInfo.height, loadInfo.channels, loadInfo.data.stbiData16, stride);
            else
                res = stbi_write_png(path.LocalPath().AsCharPtr(), loadInfo.width, loadInfo.height, loadInfo.channels, loadInfo.data.stbiData8, stride);
            break;
        case ImageContainer::TGA:
            n_assert_msg(loadInfo.primitive == Bit8UInt, "TGA only supports 8 bit integer images");
            res = stbi_write_tga(path.LocalPath().AsCharPtr(), loadInfo.width, loadInfo.height, loadInfo.channels, loadInfo.data.stbiData8);
            break;
        case ImageContainer::JPEG:
            n_assert_msg(loadInfo.primitive == Bit8UInt, "JPG only supports 8 bit integer images");
            res = stbi_write_jpg(path.LocalPath().AsCharPtr(), loadInfo.width, loadInfo.height, loadInfo.channels, loadInfo.data.stbiData8, 100);
            break;
        case ImageContainer::HDR:
            n_assert_msg(loadInfo.primitive == Bit32Float, "HDR only supports 32 bit float images");
            res = stbi_write_hdr(path.LocalPath().AsCharPtr(), loadInfo.width, loadInfo.height, loadInfo.channels, loadInfo.data.stbiDataFloat);
            break;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
ImageChannelPrimitive
ImageGetChannelPrimitive(const ImageId id)
{
    ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id.id);
    return loadInfo.primitive;
}


} // namespace CoreGraphics
