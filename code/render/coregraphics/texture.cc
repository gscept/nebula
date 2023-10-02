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

    // insert initial barrier for texture
    CoreGraphics::CmdBarrier(
        cmdBuf,
        CoreGraphics::PipelineStage::GraphicsShadersRead,
        CoreGraphics::PipelineStage::TransferRead,
        CoreGraphics::BarrierDomain::Global,
        {
            {
                id,
                CoreGraphics::TextureSubresourceInfo::Texture(ImageBits::ColorBits, id),
            },
        });

    // calculate number of mips
    TextureDimensions dims = TextureGetDimensions(id);

    CoreGraphics::ImageLayout prevLayout = CoreGraphics::ImageLayout::TransferSource;
    CoreGraphics::PipelineStage prevStageSrc = CoreGraphics::PipelineStage::TransferRead;
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
            prevStageSrc,
            CoreGraphics::PipelineStage::TransferRead,
            CoreGraphics::BarrierDomain::Global,
            {
                {
                    id,
                    CoreGraphics::TextureSubresourceInfo{ CoreGraphics::ImageBits::ColorBits, (uint)mip, 1, 0, 1 },
                }
            },
            nullptr);

        CoreGraphics::CmdBarrier(
            cmdBuf,
            CoreGraphics::PipelineStage::TransferRead,
            CoreGraphics::PipelineStage::TransferWrite,
            CoreGraphics::BarrierDomain::Global,
            {
                {
                    id,
                    CoreGraphics::TextureSubresourceInfo{ CoreGraphics::ImageBits::ColorBits, (uint)mip + 1, 1, 0, 1 },
                }
            },
            nullptr);
        CoreGraphics::CmdBlit(cmdBuf, id, fromRegion, CoreGraphics::ImageBits::ColorBits, mip, 0, id, toRegion, CoreGraphics::ImageBits::ColorBits, mip + 1, 0);

        // The previous textuer will be in write, so it needs to pingpong from transfer write/read, with the first being shader read
        prevStageSrc = CoreGraphics::PipelineStage::TransferWrite;
    }

    // At the end, only the last mip will be in transfer write, so lets just transition that one
    CoreGraphics::CmdBarrier(
        cmdBuf,
        CoreGraphics::PipelineStage::TransferWrite,
        CoreGraphics::PipelineStage::AllShadersRead,
        CoreGraphics::BarrierDomain::Global,
        {
            CoreGraphics::TextureBarrierInfo
            {
                id,
                CoreGraphics::TextureSubresourceInfo::Texture(ImageBits::ColorBits, id)
            }
        },
        nullptr);
}

//------------------------------------------------------------------------------
/**
*/
void
TextureUpdate(const CoreGraphics::CmdBufferId cmd, CoreGraphics::QueueType queue, CoreGraphics::TextureId tex, const SizeT width, SizeT height, SizeT mip, SizeT layer, SizeT size, const void* data)
{
    SizeT alignment = CoreGraphics::PixelFormat::ToTexelSize(TextureGetPixelFormat(tex));
    CoreGraphics::BufferId buf = CoreGraphics::GetUploadBuffer();
    BufferIdAcquire(buf);

    uint offset = CoreGraphics::Upload(data, size, alignment);

    // Then run a copy on the command buffer
    CoreGraphics::BufferCopy bufCopy;
    bufCopy.offset = offset;
    bufCopy.imageHeight = 0;
    bufCopy.rowLength = 0;
    CoreGraphics::TextureCopy texCopy;
    texCopy.layer = layer;
    texCopy.mip = mip;
    texCopy.region.set(0, 0, width, height);
    CoreGraphics::CmdCopy(cmd, buf, { bufCopy }, tex, { texCopy });

    BufferIdRelease(buf);
}

//------------------------------------------------------------------------------
/**
*/
TextureCreateInfoAdjusted 
TextureGetAdjustedInfo(const TextureCreateInfo& info)
{
    TextureCreateInfoAdjusted rt;
    if (info.windowTexture)
    {
        n_assert2(info.samples == 1, "Texture created as window may not have any multisampling enabled");
        n_assert2(info.alias == CoreGraphics::InvalidTextureId, "Texture created as window may not be alias");
        n_assert2(info.buffer == nullptr, "Texture created as window may not have any buffer data");
        
        rt.window = CoreGraphics::DisplayDevice::Instance()->GetCurrentWindow();
        const CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(rt.window);
        rt.usage = CoreGraphics::TextureUsage::RenderTexture | CoreGraphics::TextureUsage::TransferTextureDestination;
        rt.name = "__WINDOW__";
        rt.buffer = nullptr;
        rt.type = CoreGraphics::Texture2D;
        rt.format = mode.GetPixelFormat();
        rt.width = mode.GetWidth();
        rt.height = mode.GetHeight();
        rt.depth = 1;
        rt.widthScale = rt.heightScale = rt.depthScale = 1.0f;
        rt.layers = 1;
        rt.mips = 1;
        rt.minMip = 1;
        rt.clear = false;
        rt.samples = 1;
        rt.windowTexture = true;
        rt.windowRelative = true;
        rt.bindless = false;
        rt.sparse = false;
        rt.alias = CoreGraphics::InvalidTextureId;
        rt.defaultLayout = CoreGraphics::ImageLayout::Present;
    }
    else
    {
        n_assert(info.width > 0 && info.height > 0 && info.depth > 0);
        if (info.type == CoreGraphics::TextureCubeArray || info.type == CoreGraphics::TextureCube)
            n_assert(info.layers == 6);
        rt.name = info.name;
        rt.usage = info.usage;
        rt.buffer = info.buffer;
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
        rt.windowTexture = false;
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
            CoreGraphics::WindowId wnd = CoreGraphics::DisplayDevice::Instance()->GetCurrentWindow();
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
            SizeT width = rt.width;
            SizeT height = rt.height;

            // calculate the second logarithm of height and width and pick the smallest value to guarantee no 0xN or Nx0 sizes
            // add 1 because we always have one mip
            rt.mips = Math::min(Math::log2(rt.width), Math::log2(rt.height)) + 1;
        }
    }
    return rt;
}

} // namespace CoreGraphics
