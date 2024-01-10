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
#include "coregraphics/accelerationstructure.h"
#include "coregraphics/config.h"
#include <tuple>
namespace CoreGraphics
{

ID_24_8_TYPE(BarrierId);

struct CmdBufferId;
struct TextureSubresourceInfo
{
    ImageBits bits;
    uint mip, mipCount, layer, layerCount;

    TextureSubresourceInfo() :
        bits(CoreGraphics::ImageBits::ColorBits),
        mip(0),
        mipCount(1),
        layer(0),
        layerCount(1)
    {}

    TextureSubresourceInfo(CoreGraphics::ImageBits bits, uint mip, uint mipCount, uint layer, uint layerCount) :
        bits(bits),
        mip(mip),
        mipCount(mipCount),
        layer(layer),
        layerCount(layerCount)
    {}

    static TextureSubresourceInfo Texture(CoreGraphics::ImageBits bits, TextureId tex)
    {
        return TextureSubresourceInfo(bits, 0, TextureGetNumMips(tex), 0, TextureGetNumLayers(tex));
    }

    static TextureSubresourceInfo Color(TextureId tex)
    {
        return TextureSubresourceInfo(ImageBits::ColorBits, 0, TextureGetNumMips(tex), 0, TextureGetNumLayers(tex));
    }

    static TextureSubresourceInfo DepthStencil(TextureId tex)
    {
        return TextureSubresourceInfo(ImageBits::DepthBits | ImageBits::StencilBits, 0, TextureGetNumMips(tex), 0, TextureGetNumLayers(tex));
    }

    static TextureSubresourceInfo ColorNoMipNoLayer()
    {
        return TextureSubresourceInfo(ImageBits::ColorBits, 0, 1, 0, 1);
    }

    static TextureSubresourceInfo ColorNoMip(uint layerCount)
    {
        return TextureSubresourceInfo(ImageBits::ColorBits, 0, 1, 0, layerCount);
    }

    static TextureSubresourceInfo ColorNoLayer(uint mipCount)
    {
        return TextureSubresourceInfo(ImageBits::ColorBits, 0, mipCount, 0, 1);
    }

    static TextureSubresourceInfo DepthStencilNoMipNoLayer()
    {
        return TextureSubresourceInfo(ImageBits::DepthBits | ImageBits::StencilBits, 0, 1, 0, 1);
    }

    static TextureSubresourceInfo DepthStencilNoMip(uint layerCount)
    {
        return TextureSubresourceInfo(ImageBits::DepthBits | ImageBits::StencilBits, 0, 1, 0, layerCount);
    }

    static TextureSubresourceInfo DepthStencilNoLayer(uint mipCount)
    {
        return TextureSubresourceInfo(CoreGraphics::ImageBits::DepthBits | CoreGraphics::ImageBits::StencilBits, 0, mipCount, 0, 1);
    }

    const bool Overlaps(const TextureSubresourceInfo& rhs) const
    {
        return ((this->bits & rhs.bits) != 0) && (this->mip <= rhs.mip && this->mip + this->mipCount >= rhs.mip) && (this->layer <= rhs.layer && this->layer + this->layerCount >= rhs.layer);
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

struct AccelerationStructureBarrierInfo
{
    BlasId blas;
    TlasId tlas;
    enum
    {
        BlasBarrier,
        TlasBarrier,
        None
    } type;

    ~AccelerationStructureBarrierInfo() {};
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

struct BarrierScope
{
    BarrierScope(const CmdBufferId buf, CoreGraphics::PipelineStage fromStage, CoreGraphics::PipelineStage toStage, const Util::FixedArray<TextureBarrierInfo>& textures) 
        : buf(buf)
        , fromStage(fromStage)
        , toStage(toStage)
        , textures(textures)
        , buffers(nullptr)
        , accelerationStructures(nullptr)
    {
        this->Push();
    };

    BarrierScope(const CmdBufferId buf, CoreGraphics::PipelineStage fromStage, CoreGraphics::PipelineStage toStage, const Util::FixedArray<BufferBarrierInfo>& buffers) 
        : buf(buf)
        , fromStage(fromStage)
        , toStage(toStage)
        , textures(nullptr)
        , buffers(buffers)
        , accelerationStructures(nullptr)
    {
        this->Push();
    };

    BarrierScope(const CmdBufferId buf, CoreGraphics::PipelineStage fromStage, CoreGraphics::PipelineStage toStage, const Util::FixedArray<AccelerationStructureBarrierInfo>& accelerationStructures) 
        : buf(buf)
        , fromStage(fromStage)
        , toStage(toStage)
        , textures(nullptr)
        , buffers(nullptr)
        , accelerationStructures(accelerationStructures)
    {
        this->Push();
    };

    CmdBufferId buf;
    CoreGraphics::PipelineStage fromStage;
    CoreGraphics::PipelineStage toStage;
    Util::FixedArray<TextureBarrierInfo> textures;
    Util::FixedArray<BufferBarrierInfo> buffers;
    Util::FixedArray<AccelerationStructureBarrierInfo> accelerationStructures;

    /// Inverse barrier
    void Pop()
    {
        CmdBarrier(this->buf, this->toStage, this->fromStage, CoreGraphics::BarrierDomain::Global, this->textures, this->buffers, this->accelerationStructures);
    }

private:
    /// Apply 
    void Push()
    {
        CmdBarrier(this->buf, this->fromStage, this->toStage, CoreGraphics::BarrierDomain::Global, this->textures, this->buffers, this->accelerationStructures);
    }
};


//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::ImageBits
ImageBitsFromString(const Util::String& str)
{
    Util::Array<Util::String> comps = str.Tokenize("|");
    CoreGraphics::ImageBits bits = CoreGraphics::ImageBits(0x0);
    for (IndexT i = 0; i < comps.Size(); i++)
    {
        if (comps[i] == "Auto")             { bits = CoreGraphics::ImageBits::Auto; break; }
        else if (comps[i] == "Color")       bits |= CoreGraphics::ImageBits::ColorBits;
        else if (comps[i] == "Depth")       bits |= CoreGraphics::ImageBits::DepthBits;
        else if (comps[i] == "Stencil")     bits |= CoreGraphics::ImageBits::StencilBits;
        else if (comps[i] == "Metadata")    bits |= CoreGraphics::ImageBits::MetaBits;
        else if (comps[i] == "Plane0")      bits |= CoreGraphics::ImageBits::Plane0Bits;
        else if (comps[i] == "Plane1")      bits |= CoreGraphics::ImageBits::Plane1Bits;
        else if (comps[i] == "Plane2")      bits |= CoreGraphics::ImageBits::Plane2Bits;
        
        else
        {
            n_error("Invalid access string '%s'\n", comps[i].AsCharPtr());
            return CoreGraphics::ImageBits::ColorBits;
        }
    }
    return bits;
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
