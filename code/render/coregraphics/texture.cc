//------------------------------------------------------------------------------
//  texture.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/config.h"
#include "coregraphics/texture.h"
#include "coregraphics/memorytexturecache.h"
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

MemoryTextureCache* textureCache = nullptr;

//------------------------------------------------------------------------------
/**
*/
const TextureId
CreateTexture(const TextureCreateInfo& info)
{
    TextureId id = textureCache->ReserveResource(info.name, info.tag);
    n_assert(id.resourceType == TextureIdType);
    textureCache->LoadFromMemory(id, &info);
    return id;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyTexture(const TextureId id)
{
    textureCache->DiscardResource(id);
}

//------------------------------------------------------------------------------
/**
*/
TextureDimensions
TextureGetDimensions(const TextureId id)
{
    return textureCache->GetDimensions(id);
}

//------------------------------------------------------------------------------
/**
*/
TextureRelativeDimensions 
TextureGetRelativeDimensions(const TextureId id)
{
    return textureCache->GetRelativeDimensions(id);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::PixelFormat::Code
TextureGetPixelFormat(const TextureId id)
{
    return textureCache->GetPixelFormat(id);
}

//------------------------------------------------------------------------------
/**
*/
TextureType
TextureGetType(const TextureId id)
{
    return textureCache->GetType(id);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
TextureGetNumMips(const TextureId id)
{
    return textureCache->GetNumMips(id);
}

//------------------------------------------------------------------------------
/**
*/
SizeT 
TextureGetNumLayers(const TextureId id)
{
    return textureCache->GetNumLayers(id);
}

//------------------------------------------------------------------------------
/**
*/
SizeT 
TextureGetNumSamples(const TextureId id)
{
    return textureCache->GetNumSamples(id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::TextureId 
TextureGetAlias(const TextureId id)
{
    return textureCache->GetAlias(id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::TextureUsage 
TextureGetUsage(const TextureId id)
{
    return textureCache->GetUsageBits(id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ImageLayout 
TextureGetDefaultLayout(const TextureId id)
{
    return textureCache->GetDefaultLayout(id);
}

//------------------------------------------------------------------------------
/**
*/
uint 
TextureGetBindlessHandle(const TextureId id)
{
    return textureCache->GetBindlessHandle(id);
}

//------------------------------------------------------------------------------
/**
*/
uint 
TextureGetStencilBindlessHandle(const TextureId id)
{
    return textureCache->GetStencilBindlessHandle(id);
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
TextureSwapBuffers(const TextureId id)
{
    return textureCache->SwapBuffers(id);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureWindowResized(const TextureId id)
{
    textureCache->Reload(id);
}

//------------------------------------------------------------------------------
/**
*/
TextureMapInfo 
TextureMap(const TextureId id, IndexT mip, const CoreGraphics::GpuBufferTypes::MapType type)
{
    TextureMapInfo info;
    n_assert(textureCache->Map(id, mip, type, info));
    return info;
}

//------------------------------------------------------------------------------
/**
*/
void
TextureUnmap(const TextureId id, IndexT mip)
{
    textureCache->Unmap(id, mip);
}

//------------------------------------------------------------------------------
/**
*/
TextureMapInfo
TextureMapFace(const TextureId id, IndexT mip, TextureCubeFace face, const CoreGraphics::GpuBufferTypes::MapType type)
{
    TextureMapInfo info;
    n_assert(textureCache->MapCubeFace(id, face, mip, type, info));
    return info;
}

//------------------------------------------------------------------------------
/**
*/
void
TextureUnmapFace(const TextureId id, IndexT mip, TextureCubeFace face)
{
    textureCache->UnmapCubeFace(id, face, mip);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureGenerateMipmaps(const CoreGraphics::CmdBufferId cmdBuf, const TextureId id)
{
    textureCache->GenerateMipmaps(cmdBuf, id);
}

//------------------------------------------------------------------------------
/**
*/
TextureSparsePageSize 
TextureSparseGetPageSize(const CoreGraphics::TextureId id)
{
    return textureCache->SparseGetPageSize(id);
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
TextureSparseGetPageIndex(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT x, IndexT y, IndexT z)
{
    return textureCache->SparseGetPageIndex(id, layer, mip, x, y, z);
}

//------------------------------------------------------------------------------
/**
*/
const TextureSparsePage& 
TextureSparseGetPage(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT pageIndex)
{
    return textureCache->SparseGetPage(id, layer, mip, pageIndex);
}

//------------------------------------------------------------------------------
/**
*/
SizeT 
TextureSparseGetNumPages(const CoreGraphics::TextureId id, IndexT layer, IndexT mip)
{
    return textureCache->SparseGetNumPages(id, layer, mip);
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
TextureSparseGetMaxMip(const CoreGraphics::TextureId id)
{
    return textureCache->SparseGetMaxMip(id);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureSparseEvict(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT pageIndex)
{
    textureCache->SparseEvict(id, layer, mip, pageIndex);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureSparseMakeResident(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT pageIndex)
{
    textureCache->SparseMakeResident(id, layer, mip, pageIndex);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureSparseEvictMip(const CoreGraphics::TextureId id, IndexT layer, IndexT mip)
{
    textureCache->SparseEvictMip(id, layer, mip);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureSparseMakeMipResident(const CoreGraphics::TextureId id, IndexT layer, IndexT mip)
{
    textureCache->SparseMakeMipResident(id, layer, mip);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureSparseCommitChanges(const CoreGraphics::TextureId id)
{
    textureCache->SparseCommitChanges(id);
}

//------------------------------------------------------------------------------
/**
*/
void
TextureUpdate(const CoreGraphics::CmdBufferId cmd, CoreGraphics::QueueType queue, CoreGraphics::TextureId tex, const SizeT width, SizeT height, SizeT mip, SizeT layer, SizeT size, const void* data)
{
    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.size = size;
    bufInfo.mode = CoreGraphics::HostCached;
    bufInfo.usageFlags = CoreGraphics::BufferUsageFlag::TransferBufferSource | CoreGraphics::BufferUsageFlag::TransferBufferDestination;
    bufInfo.queueSupport = queue;
    CoreGraphics::BufferId buf = CoreGraphics::CreateBuffer(bufInfo);

    // Copy over data to buffer
    char* mapped = (char*)CoreGraphics::BufferMap(buf);
    memcpy(mapped, data, size);

    // Then run a copy on the command buffer
    CoreGraphics::BufferCopy bufCopy;
    bufCopy.offset = 0;
    bufCopy.imageHeight = 0;
    bufCopy.rowLength = 0;
    CoreGraphics::TextureCopy texCopy;
    texCopy.layer = layer;
    texCopy.mip = mip;
    texCopy.region.set(0, 0, width, height);
    CoreGraphics::CmdCopy(cmd, buf, { bufCopy }, tex, { texCopy });

    CoreGraphics::BufferUnmap(buf);
    CoreGraphics::DestroyBuffer(buf);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureClearColor(const CoreGraphics::CmdBufferId cmd, const CoreGraphics::TextureId id, Math::vec4 color, const CoreGraphics::ImageLayout layout, const CoreGraphics::ImageSubresourceInfo& subres)
{
    textureCache->ClearColor(cmd, id, color, layout, subres);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureClearDepthStencil(const CoreGraphics::CmdBufferId cmd, const CoreGraphics::TextureId id, float depth, uint stencil, const CoreGraphics::ImageLayout layout, const CoreGraphics::ImageSubresourceInfo& subres)
{
    textureCache->ClearDepthStencil(cmd, id, depth, stencil, layout, subres);
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
        rt.buffer = nullptr;
        rt.type = CoreGraphics::Texture2D;
        rt.format = mode.GetPixelFormat();
        rt.width = mode.GetWidth();
        rt.height = mode.GetHeight();
        rt.depth = 1;
        rt.widthScale = rt.heightScale = rt.depthScale = 1.0f;
        rt.layers = 1;
        rt.mips = 1;
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
        rt.layers = (info.type == CoreGraphics::TextureCubeArray || info.type == CoreGraphics::TextureCube) ? 6 : info.layers;
        rt.clear = info.clear;
        rt.clearColor = info.clearColorF4;
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
