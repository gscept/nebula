#pragma once
//------------------------------------------------------------------------------
/**
    A barrier is a memory barrier between two GPU operations, 
    and thus allows for a guarantee of concurrency.

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "util/array.h"
#include "coregraphics/texture.h"
#include "coregraphics/buffer.h"
#include "coregraphics/config.h"
#include <tuple>
namespace CoreGraphics
{

ID_24_8_TYPE(BarrierId);

struct CmdBufferId;
struct TextureSubresourceInfo
{
    CoreGraphics::ImageAspect aspect;
    uint mip, mipCount, layer, layerCount;

    TextureSubresourceInfo() :
        aspect(CoreGraphics::ImageAspect::ColorBits),
        mip(0),
        mipCount(1),
        layer(0),
        layerCount(1)
    {}

    TextureSubresourceInfo(CoreGraphics::ImageAspect aspect, uint mip, uint mipCount, uint layer, uint layerCount) :
        aspect(aspect),
        mip(mip),
        mipCount(mipCount),
        layer(layer),
        layerCount(layerCount)
    {}

    static TextureSubresourceInfo ColorNoMipNoLayer()
    {
        return TextureSubresourceInfo(CoreGraphics::ImageAspect::ColorBits, 0, 1, 0, 1);
    }

    static TextureSubresourceInfo ColorNoMip(uint layerCount)
    {
        return TextureSubresourceInfo(CoreGraphics::ImageAspect::ColorBits, 0, 1, 0, layerCount);
    }

    static TextureSubresourceInfo ColorNoLayer(uint mipCount)
    {
        return TextureSubresourceInfo(CoreGraphics::ImageAspect::ColorBits, 0, mipCount, 0, 1);
    }

    static TextureSubresourceInfo DepthStencilNoMipNoLayer()
    {
        return TextureSubresourceInfo(CoreGraphics::ImageAspect::DepthBits | CoreGraphics::ImageAspect::StencilBits, 0, 1, 0, 1);
    }

    static TextureSubresourceInfo DepthStencilNoMip(uint layerCount)
    {
        return TextureSubresourceInfo(CoreGraphics::ImageAspect::DepthBits | CoreGraphics::ImageAspect::StencilBits, 0, 1, 0, layerCount);
    }

    static TextureSubresourceInfo DepthStencilNoLayer(uint mipCount)
    {
        return TextureSubresourceInfo(CoreGraphics::ImageAspect::DepthBits | CoreGraphics::ImageAspect::StencilBits, 0, mipCount, 0, 1);
    }

    const bool Overlaps(const TextureSubresourceInfo& rhs) const
    {
        return ((this->aspect & rhs.aspect) != 0) && (this->mip <= rhs.mip && this->mip + this->mipCount >= rhs.mip) && (this->layer <= rhs.layer && this->layer + this->layerCount >= rhs.layer);
    }
};

struct BufferSubresourceInfo
{
    uint offset, size;

    BufferSubresourceInfo() :
        offset(0),
        size(NEBULA_WHOLE_BUFFER_SIZE)
    {}

    BufferSubresourceInfo(uint offset, uint size) :
        offset(offset),
        size(size)
    {}

    const bool Overlaps(const BufferSubresourceInfo& rhs) const
    {
        return (this->offset <= rhs.offset && this->offset + this->size >= rhs.offset);
    }
};

struct TextureBarrier
{
    TextureId tex;
    TextureSubresourceInfo subres;
    CoreGraphics::PipelineStage fromStage;
    CoreGraphics::PipelineStage toStage;
};

struct BufferBarrier
{
    BufferId buf;
    CoreGraphics::PipelineStage fromStage;
    CoreGraphics::PipelineStage toStage;
    IndexT offset;
    SizeT size; // set to -1 to use whole buffer
};

struct TextureBarrierInfo
{
    TextureId tex;
    TextureSubresourceInfo subres;
};

struct BufferBarrierInfo
{
    BufferId buf;
    BufferSubresourceInfo subres;
};

struct BarrierCreateInfo
{
    Util::StringAtom name;
    BarrierDomain domain;
    CoreGraphics::PipelineStage fromStage;
    CoreGraphics::PipelineStage toStage;
    CoreGraphics::QueueType fromQueue;
    CoreGraphics::QueueType toQueue;
    Util::Array<TextureBarrierInfo> textures;
    Util::Array<BufferBarrierInfo> buffers;
};

/// create barrier object
BarrierId CreateBarrier(const BarrierCreateInfo& info);
/// destroy barrier object
void DestroyBarrier(const BarrierId id);

/// reset resources previously set in barrier
void BarrierReset(const BarrierId id);

/// Push barrier to stack
void BarrierPush(
    const CoreGraphics::CmdBufferId buf,
    CoreGraphics::PipelineStage fromStage,
    CoreGraphics::PipelineStage toStage,
    CoreGraphics::BarrierDomain domain,
    const Util::FixedArray<TextureBarrierInfo>& textures,
    const Util::FixedArray<BufferBarrierInfo>& buffers);
/// Push barrier to stack
void BarrierPush(
    const CoreGraphics::CmdBufferId buf,
    CoreGraphics::PipelineStage fromStage,
    CoreGraphics::PipelineStage toStage,
    CoreGraphics::BarrierDomain domain,
    const Util::FixedArray<TextureBarrierInfo>& textures);
/// Push barrier to stack
void BarrierPush(
    const CoreGraphics::CmdBufferId buf,
    CoreGraphics::PipelineStage fromStage,
    CoreGraphics::PipelineStage toStage,
    CoreGraphics::BarrierDomain domain,
    const Util::FixedArray<BufferBarrierInfo>& buffers);
/// pop barrier, reverses the from-to stages and any access flags in the buffers and texture barriers
void BarrierPop(const CoreGraphics::CmdBufferId buf);
/// repeat barrier in queue
void BarrierRepeat(const CoreGraphics::CmdBufferId buf);


//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::ImageAspect
ImageAspectFromString(const Util::String& str)
{
    Util::Array<Util::String> comps = str.Tokenize("|");
    CoreGraphics::ImageAspect aspect = CoreGraphics::ImageAspect(0x0);
    for (IndexT i = 0; i < comps.Size(); i++)
    {
        if (comps[i] == "Color")            aspect |= CoreGraphics::ImageAspect::ColorBits;
        else if (comps[i] == "Depth")       aspect |= CoreGraphics::ImageAspect::DepthBits;
        else if (comps[i] == "Stencil")     aspect |= CoreGraphics::ImageAspect::StencilBits;
        else if (comps[i] == "Metadata")    aspect |= CoreGraphics::ImageAspect::MetaBits;
        else if (comps[i] == "Plane0")      aspect |= CoreGraphics::ImageAspect::Plane0Bits;
        else if (comps[i] == "Plane1")      aspect |= CoreGraphics::ImageAspect::Plane1Bits;
        else if (comps[i] == "Plane2")      aspect |= CoreGraphics::ImageAspect::Plane2Bits;
        else
        {
            n_error("Invalid access string '%s'\n", comps[i].AsCharPtr());
            return CoreGraphics::ImageAspect::ColorBits;
        }
    }
    return aspect;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::ImageLayout
ImageLayoutFromString(const Util::String& str)
{
    if (str == "Undefined")                 return CoreGraphics::ImageLayout::Undefined;
    else if (str == "General")              return CoreGraphics::ImageLayout::General;
    else if (str == "ColorRenderTexture")   return CoreGraphics::ImageLayout::ColorRenderTexture;
    else if (str == "DepthRenderTexture")   return CoreGraphics::ImageLayout::DepthStencilRenderTexture;
    else if (str == "DepthStencilRead")     return CoreGraphics::ImageLayout::DepthStencilRead;
    else if (str == "ShaderRead")           return CoreGraphics::ImageLayout::ShaderRead;
    else if (str == "TransferSource")       return CoreGraphics::ImageLayout::TransferSource;
    else if (str == "TransferDestination")  return CoreGraphics::ImageLayout::TransferDestination;
    else if (str == "Preinitialized")       return CoreGraphics::ImageLayout::Preinitialized;
    else if (str == "Present")              return CoreGraphics::ImageLayout::Present;
    return CoreGraphics::ImageLayout::Undefined;
}
} // namespace CoreGraphics
