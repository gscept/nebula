//------------------------------------------------------------------------------
//  texture.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/config.h"
#include "coregraphics/texture.h"
#include "coregraphics/memorytexturepool.h"
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

MemoryTexturePool* texturePool = nullptr;

//------------------------------------------------------------------------------
/**
*/
const TextureId
CreateTexture(const TextureCreateInfo& info)
{
    TextureId id = texturePool->ReserveResource(info.name, info.tag);
    n_assert(id.resourceType == TextureIdType);
    texturePool->LoadFromMemory(id, &info);
    return id;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyTexture(const TextureId id)
{
    texturePool->DiscardResource(id);
}

//------------------------------------------------------------------------------
/**
*/
TextureDimensions
TextureGetDimensions(const TextureId id)
{
    return texturePool->GetDimensions(id);
}

//------------------------------------------------------------------------------
/**
*/
TextureRelativeDimensions 
TextureGetRelativeDimensions(const TextureId id)
{
    return texturePool->GetRelativeDimensions(id);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::PixelFormat::Code
TextureGetPixelFormat(const TextureId id)
{
    return texturePool->GetPixelFormat(id);
}

//------------------------------------------------------------------------------
/**
*/
TextureType
TextureGetType(const TextureId id)
{
    return texturePool->GetType(id);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
TextureGetNumMips(const TextureId id)
{
    return texturePool->GetNumMips(id);
}

//------------------------------------------------------------------------------
/**
*/
SizeT 
TextureGetNumLayers(const TextureId id)
{
    return texturePool->GetNumLayers(id);
}

//------------------------------------------------------------------------------
/**
*/
SizeT 
TextureGetNumSamples(const TextureId id)
{
    return texturePool->GetNumSamples(id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::TextureId 
TextureGetAlias(const TextureId id)
{
    return texturePool->GetAlias(id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::TextureUsage 
TextureGetUsage(const TextureId id)
{
    return texturePool->GetUsageBits(id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ImageLayout 
TextureGetDefaultLayout(const TextureId id)
{
    return texturePool->GetDefaultLayout(id);
}

//------------------------------------------------------------------------------
/**
*/
uint 
TextureGetBindlessHandle(const TextureId id)
{
    return texturePool->GetBindlessHandle(id);
}

//------------------------------------------------------------------------------
/**
*/
uint 
TextureGetStencilBindlessHandle(const TextureId id)
{
    return texturePool->GetStencilBindlessHandle(id);
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
TextureSwapBuffers(const TextureId id)
{
    return texturePool->SwapBuffers(id);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureWindowResized(const TextureId id)
{
    texturePool->Reload(id);
}

//------------------------------------------------------------------------------
/**
*/
TextureMapInfo 
TextureMap(const TextureId id, IndexT mip, const CoreGraphics::GpuBufferTypes::MapType type)
{
    TextureMapInfo info;
    n_assert(texturePool->Map(id, mip, type, info));
    return info;
}

//------------------------------------------------------------------------------
/**
*/
void
TextureUnmap(const TextureId id, IndexT mip)
{
    texturePool->Unmap(id, mip);
}

//------------------------------------------------------------------------------
/**
*/
TextureMapInfo
TextureMapFace(const TextureId id, IndexT mip, TextureCubeFace face, const CoreGraphics::GpuBufferTypes::MapType type)
{
    TextureMapInfo info;
    n_assert(texturePool->MapCubeFace(id, face, mip, type, info));
    return info;
}

//------------------------------------------------------------------------------
/**
*/
void
TextureUnmapFace(const TextureId id, IndexT mip, TextureCubeFace face)
{
    texturePool->UnmapCubeFace(id, face, mip);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureGenerateMipmaps(const TextureId id)
{
    texturePool->GenerateMipmaps(id);
}

//------------------------------------------------------------------------------
/**
*/
TextureSparsePageSize 
TextureSparseGetPageSize(const CoreGraphics::TextureId id)
{
    return texturePool->SparseGetPageSize(id);
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
TextureSparseGetPageIndex(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT x, IndexT y, IndexT z)
{
    return texturePool->SparseGetPageIndex(id, layer, mip, x, y, z);
}

//------------------------------------------------------------------------------
/**
*/
const TextureSparsePage& 
TextureSparseGetPage(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT pageIndex)
{
    return texturePool->SparseGetPage(id, layer, mip, pageIndex);
}

//------------------------------------------------------------------------------
/**
*/
SizeT 
TextureSparseGetNumPages(const CoreGraphics::TextureId id, IndexT layer, IndexT mip)
{
    return texturePool->SparseGetNumPages(id, layer, mip);
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
TextureSparseGetMaxMip(const CoreGraphics::TextureId id)
{
    return texturePool->SparseGetMaxMip(id);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureSparseEvict(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT pageIndex)
{
    texturePool->SparseEvict(id, layer, mip, pageIndex);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureSparseMakeResident(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT pageIndex)
{
    texturePool->SparseMakeResident(id, layer, mip, pageIndex);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureSparseEvictMip(const CoreGraphics::TextureId id, IndexT layer, IndexT mip)
{
    texturePool->SparseEvictMip(id, layer, mip);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureSparseMakeMipResident(const CoreGraphics::TextureId id, IndexT layer, IndexT mip)
{
    texturePool->SparseMakeMipResident(id, layer, mip);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureSparseCommitChanges(const CoreGraphics::TextureId id)
{
    texturePool->SparseCommitChanges(id);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureUpdate(const CoreGraphics::TextureId id, const Math::rectangle<int>& region, IndexT mip, IndexT layer, char* buf, const CoreGraphics::SubmissionContextId sub)
{
    texturePool->Update(id, region, mip, layer, buf, sub);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureUpdate(const CoreGraphics::TextureId id, IndexT mip, IndexT layer, char* buf, const CoreGraphics::SubmissionContextId sub)
{
    TextureDimensions dims = TextureGetDimensions(id);
    Math::rectangle<int> region;
    region.left = 0;
    region.top = 0;
    region.right = dims.width;
    region.bottom = dims.height;
    texturePool->Update(id, region, mip, layer, buf, sub);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureClearColor(const CoreGraphics::TextureId id, Math::vec4 color, const CoreGraphics::ImageLayout layout, const CoreGraphics::ImageSubresourceInfo& subres, const CoreGraphics::SubmissionContextId sub)
{
    texturePool->ClearColor(id, color, layout, subres, sub);
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureClearDepthStencil(const CoreGraphics::TextureId id, float depth, uint stencil, const CoreGraphics::ImageLayout layout, const CoreGraphics::ImageSubresourceInfo& subres, const CoreGraphics::SubmissionContextId sub)
{
    texturePool->ClearDepthStencil(id, depth, stencil, layout, subres, sub);
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
        n_assert2(info.alias == CoreGraphics::TextureId::Invalid(), "Texture created as window may not be alias");
        n_assert2(info.buffer == nullptr, "Texture created as window may not have any buffer data");
        
        rt.window = CoreGraphics::DisplayDevice::Instance()->GetCurrentWindow();
        const CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(rt.window);
        rt.name = info.name;
        rt.usage = CoreGraphics::TextureUsage::RenderTexture | CoreGraphics::TextureUsage::TransferTextureDestination;
        rt.tag = info.tag;
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
        rt.bindless = info.bindless;
        rt.sparse = info.sparse;
        rt.alias = CoreGraphics::TextureId::Invalid();
        rt.defaultLayout = CoreGraphics::ImageLayout::Present;
    }
    else
    {
        n_assert(info.width > 0 && info.height > 0 && info.depth > 0);


        rt.name = info.name;
        rt.usage = info.usage;
        rt.tag = info.tag;
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
        rt.window = CoreGraphics::WindowId::Invalid();
        rt.alias = info.alias;
        rt.defaultLayout = info.defaultLayout;

        // correct depth-stencil formats if layout is shader read
        if (CoreGraphics::PixelFormat::IsDepthFormat(rt.format) && rt.defaultLayout == CoreGraphics::ImageLayout::ShaderRead)
            rt.defaultLayout = CoreGraphics::ImageLayout::DepthStencilRead;

        if (rt.windowRelative)
        {
            CoreGraphics::WindowId wnd = CoreGraphics::DisplayDevice::Instance()->GetCurrentWindow();
            const CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(wnd);
            rt.width = SizeT(Math::n_ceil(mode.GetWidth() * info.width));
            rt.height = SizeT(Math::n_ceil(mode.GetHeight() * info.height));
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
            rt.mips = 1;
            while (true)
            {
                width = width >> 1;
                height = height >> 1;

                // break if any dimension reaches 0
                if (width == 0 || height == 0)
                    break;
                rt.mips++;
            }
        }
    }
    return rt;
}

} // namespace CoreGraphics
