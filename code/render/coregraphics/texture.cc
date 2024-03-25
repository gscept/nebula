//------------------------------------------------------------------------------
//  texture.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "coregraphics/config.h"
#include "coregraphics/texture.h"
#include "coregraphics/displaydevice.h"

namespace CoreGraphics
{

TextureId White1D;
TextureId Black2D;
TextureId White2D;
TextureId WhiteCube;
TextureId White3D;
TextureId White1DArray;
TextureId White2DArray;
TextureId WhiteCubeArray;
TextureId Red2D;
TextureId Green2D;
TextureId Blue2D;

//------------------------------------------------------------------------------
/**
*/
void 
TextureGenerateMipmaps(const CoreGraphics::CmdBufferId cmdBuf, const TextureId id)
{
    N_CMD_SCOPE(cmdBuf, NEBULA_MARKER_TRANSFER, "Mipmap");
    SizeT numMips = CoreGraphics::TextureGetNumMips(id);

    // calculate number of mips
    TextureDimensions dims = TextureGetDimensions(id);

    IndexT mip;
    for (mip = 0; mip < numMips - 1; mip++)
    {
        TextureDimensions biggerDims = dims;
        dims.width = dims.width >> 1;
        dims.height = dims.height >> 1;

        Math::rectangle<SizeT> fromRegion;
        fromRegion.left = 0;
        fromRegion.top = 0;
        fromRegion.right = biggerDims.width;
        fromRegion.bottom = biggerDims.height;

        Math::rectangle<SizeT> toRegion;
        toRegion.left = 0;
        toRegion.top = 0;
        toRegion.right = dims.width;
        toRegion.bottom = dims.height;

        // Transition source to source
        CoreGraphics::CmdBarrier(
            cmdBuf,
            CoreGraphics::PipelineStage::AllShadersRead,
            CoreGraphics::PipelineStage::TransferRead,
            CoreGraphics::BarrierDomain::Global,
            {
                {
                    id,
                    CoreGraphics::TextureSubresourceInfo{ CoreGraphics::ImageBits::ColorBits, (uint)mip, 1, 0, 1 },
                }
            });

        CoreGraphics::CmdBarrier(
            cmdBuf,
            CoreGraphics::PipelineStage::AllShadersRead,
            CoreGraphics::PipelineStage::TransferWrite,
            CoreGraphics::BarrierDomain::Global,
            {
                {
                    id,
                    CoreGraphics::TextureSubresourceInfo{ CoreGraphics::ImageBits::ColorBits, (uint)mip + 1, 1, 0, 1 },
                }
            });
        CoreGraphics::CmdBlit(cmdBuf, id, fromRegion, CoreGraphics::ImageBits::ColorBits, mip, 0, id, toRegion, CoreGraphics::ImageBits::ColorBits, mip + 1, 0);

        CoreGraphics::CmdBarrier(
            cmdBuf,
            CoreGraphics::PipelineStage::TransferWrite,
            CoreGraphics::PipelineStage::AllShadersRead,
            CoreGraphics::BarrierDomain::Global,
            {
                {
                    id,
                    CoreGraphics::TextureSubresourceInfo{ CoreGraphics::ImageBits::ColorBits, (uint)mip + 1, 1, 0, 1 },
                }
            });

        // Transition source to source
        CoreGraphics::CmdBarrier(
            cmdBuf,
            CoreGraphics::PipelineStage::TransferRead,
            CoreGraphics::PipelineStage::AllShadersRead,
            CoreGraphics::BarrierDomain::Global,
            {
                {
                    id,
                    CoreGraphics::TextureSubresourceInfo{ CoreGraphics::ImageBits::ColorBits, (uint)mip, 1, 0, 1 },
                }
            });
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
TextureUpdate(const CoreGraphics::CmdBufferId cmd, CoreGraphics::TextureId tex, const SizeT width, SizeT height, SizeT mip, SizeT layer, const void* data, SizeT dataSize)
{
    CoreGraphics::PixelFormat::Code fmt = TextureGetPixelFormat(tex);
    uint blockSize = CoreGraphics::PixelFormat::ToBlockSize(fmt);
    SizeT alignment = CoreGraphics::PixelFormat::ToTexelSize(fmt) / blockSize;
    auto [offset, buffer] = CoreGraphics::UploadArray(data, dataSize, alignment);
    if (buffer == CoreGraphics::InvalidBufferId)
        return false;

    // Then run a copy on the command buffer
    CoreGraphics::BufferCopy bufCopy;
    bufCopy.offset = offset;
    bufCopy.imageHeight = 0;
    bufCopy.rowLength = 0;
    CoreGraphics::TextureCopy texCopy;
    texCopy.layer = layer;
    texCopy.mip = mip;
    texCopy.region.set(0, 0, width, height);
    CoreGraphics::CmdCopy(cmd, buffer, {bufCopy}, tex, {texCopy});
    return true;
}

//------------------------------------------------------------------------------
/**
*/
TextureCreateInfoAdjusted 
TextureGetAdjustedInfo(const TextureCreateInfo& info)
{
    TextureCreateInfoAdjusted rt;

    n_assert(info.width > 0 && info.height > 0 && info.depth > 0);
    if (info.type == CoreGraphics::TextureCubeArray || info.type == CoreGraphics::TextureCube)
        n_assert(info.layers == 6);
    rt.name = info.name;
    rt.usage = info.usage;
    rt.data = info.data;
	rt.dataSize = info.dataSize;
    rt.type = info.type;
    rt.format = info.format;
    rt.width = (SizeT)info.width;
    rt.height = (SizeT)info.height;
    rt.depth = (SizeT)info.depth;
    rt.widthScale = 0;
    rt.heightScale = 0;
    rt.depthScale = 0;
    rt.mips = info.mips;
    rt.minMip = info.minMip;
    rt.layers = info.layers;
    rt.clear = info.clear;
    rt.clearColorF4 = info.clearColorF4;
    rt.samples = info.samples;
    rt.windowRelative = info.windowRelative;
    rt.bindless = info.bindless;
    rt.sparse = info.sparse;
    rt.window = CoreGraphics::InvalidWindowId;
    rt.alias = info.alias;
    rt.defaultLayout = info.defaultLayout;

    // correct depth-stencil formats if layout is shader read
    if (CoreGraphics::PixelFormat::IsDepthFormat(rt.format) && rt.defaultLayout == CoreGraphics::ImageLayout::ShaderRead)
        rt.defaultLayout = CoreGraphics::ImageLayout::DepthStencilRead;

    if (rt.windowRelative)
    {
        CoreGraphics::WindowId wnd = CoreGraphics::CurrentWindow;
        const CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(wnd);
        rt.width = SizeT(Math::ceil(mode.GetWidth() * info.width));
        rt.height = SizeT(Math::ceil(mode.GetHeight() * info.height));
        rt.depth = 1;
        rt.window = wnd;

        rt.widthScale = info.width;
        rt.heightScale = info.height;
        rt.depthScale = info.depth;
    }

    // if the mip value is set to auto generate mips, generate mip chain
    if (info.mips == TextureAutoMips)
    {
        // calculate the second logarithm of height and width and pick the smallest value to guarantee no 0xN or Nx0 sizes
        // add 1 because we always have one mip
        rt.mips = Math::min(Math::log2(rt.width), Math::log2(rt.height)) + 1;
    }
    return rt;
}

} // namespace CoreGraphics
