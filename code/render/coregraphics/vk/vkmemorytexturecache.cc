//------------------------------------------------------------------------------
// vkmemorytextureloader.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkmemorytexturecache.h"
#include "coregraphics/texture.h"
#include "coregraphics/barrier.h"
#include "vkbuffer.h"
#include "vkgraphicsdevice.h"
#include "vktypes.h"
#include "vkutilities.h"
#include "resources/resourceserver.h"
#include "vkshaderserver.h"
#include "vkcommandbuffer.h"
#include "coregraphics/submissioncontext.h"
#include "vksubmissioncontext.h"
#include "coregraphics/glfw/glfwwindow.h"

N_DECLARE_COUNTER(N_SPARSE_PAGE_MEMORY_COUNTER, Sparse Texture Allocated Memory);

using namespace CoreGraphics;
using namespace Resources;
namespace Vulkan
{

__ImplementClass(Vulkan::VkMemoryTextureCache, 'VKTO', Resources::ResourceMemoryCache);

void SetupSparse(VkDevice dev, VkImage img, Ids::Id32 sparseExtension, const VkTextureLoadInfo& info);

//------------------------------------------------------------------------------
/**
*/
ResourceCache::LoadStatus
VkMemoryTextureCache::LoadFromMemory(const Resources::ResourceId id, const void* info)
{
    const TextureCreateInfo* data = (const TextureCreateInfo*)info;

    /// during the load-phase, we can safetly get the structs
    __Lock(textureAllocator);
    VkTextureRuntimeInfo& runtimeInfo = this->Get<Texture_RuntimeInfo>(id.resourceId);
    VkTextureLoadInfo& loadInfo = this->Get<Texture_LoadInfo>(id.resourceId);
    VkTextureWindowInfo& windowInfo = this->Get<Texture_WindowInfo>(id.resourceId);

    // create adjusted info
    TextureCreateInfoAdjusted adjustedInfo = TextureGetAdjustedInfo(*data);

    VkPhysicalDevice physicalDev = Vulkan::GetCurrentPhysicalDevice();
    VkDevice dev = Vulkan::GetCurrentDevice();

    loadInfo.dev = dev;
    loadInfo.dims.width = adjustedInfo.width;
    loadInfo.dims.height = adjustedInfo.height;
    loadInfo.dims.depth = adjustedInfo.depth;
    loadInfo.relativeDims.width = adjustedInfo.widthScale;
    loadInfo.relativeDims.height = adjustedInfo.heightScale;
    loadInfo.relativeDims.depth = adjustedInfo.depthScale;
    loadInfo.mips = adjustedInfo.mips;
    loadInfo.layers = adjustedInfo.layers;
    loadInfo.format = adjustedInfo.format;
    loadInfo.texUsage = adjustedInfo.usage;
    loadInfo.alias = adjustedInfo.alias;
    loadInfo.samples = adjustedInfo.samples;
    loadInfo.clear = adjustedInfo.clear;
    loadInfo.clearColor = adjustedInfo.clearColor;
    loadInfo.defaultLayout = adjustedInfo.defaultLayout;
    loadInfo.windowTexture = adjustedInfo.windowTexture;
    loadInfo.windowRelative = adjustedInfo.windowRelative;
    loadInfo.bindless = adjustedInfo.bindless;
    loadInfo.sparse = adjustedInfo.sparse;
    runtimeInfo.bind = 0xFFFFFFFF;

    // borrow buffer pointer
    loadInfo.texBuffer = adjustedInfo.buffer;
    windowInfo.window = adjustedInfo.window;
    
    if (loadInfo.windowTexture)
    {
        runtimeInfo.type = Texture2D;
    }
    else
    {
        runtimeInfo.type = adjustedInfo.type;
    }

    if (this->Setup(id))
    {
        n_assert(this->GetState(id) == Resource::Pending);
        n_assert(loadInfo.img != VK_NULL_HANDLE);

        // set loaded flag
        this->states[id.poolId] = Resources::Resource::Loaded;

#if NEBULA_GRAPHICS_DEBUG
        ObjectSetName((TextureId)id, adjustedInfo.name.Value());
#endif

        return ResourceCache::Success;
    }

    return ResourceCache::Failed;   
}


//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTextureCache::Unload(const Resources::ResourceId id)
{
    __Lock(textureAllocator);
    VkTextureLoadInfo& loadInfo = this->Get<Texture_LoadInfo>(id);
    VkTextureRuntimeInfo& runtimeInfo = this->Get<Texture_RuntimeInfo>(id);
    VkTextureWindowInfo& windowInfo = this->Get<Texture_WindowInfo>(id);

    if (loadInfo.stencilExtension != Ids::InvalidId32)
    {
        __Lock(textureStencilExtensionAllocator);
        VkTextureRuntimeInfo& stencil = textureStencilExtensionAllocator.Get<TextureExtension_StencilInfo>(loadInfo.stencilExtension);
        Vulkan::DelayedDeleteImageView(stencil.view);
        VkShaderServer::Instance()->UnregisterTexture(stencil.bind, stencil.type);
        textureStencilExtensionAllocator.Dealloc(loadInfo.stencilExtension);
    }

    if (loadInfo.swapExtension)
    {
        textureSwapExtensionAllocator.Dealloc(loadInfo.swapExtension);
    }

    // if sparse, run through and dealloc pages
    if (loadInfo.sparse)
    {
        __Lock(textureSparseExtensionAllocator);

        // dealloc all opaque bindings
        Util::Array<CoreGraphics::Alloc>& allocs = textureSparseExtensionAllocator.Get<TextureExtension_SparseOpaqueAllocs>(loadInfo.sparseExtension);
        for (IndexT i = 0; i < allocs.Size(); i++)
            Vulkan::DelayedFreeMemory(allocs[i]);
        allocs.Clear();

        // clear all pages
        TextureSparsePageTable& table = textureSparseExtensionAllocator.Get<TextureExtension_SparsePageTable>(loadInfo.sparseExtension);
        for (IndexT layer = 0; layer < table.pages.Size(); layer++)
        {
            for (IndexT mip = 0; mip < table.pages[layer].Size(); mip++)
            {
                Util::Array<TextureSparsePage>& pages = table.pages[layer][mip];
                for (IndexT pageIdx = 0; pageIdx < pages.Size(); pageIdx++)
                {
                    if (pages[pageIdx].alloc.mem != VK_NULL_HANDLE)
                    {
                        Vulkan::DelayedFreeMemory(pages[pageIdx].alloc);
                        pages[pageIdx].alloc.mem = VK_NULL_HANDLE;
                        pages[pageIdx].alloc.offset = 0;
                    }
                }
            }
        }
        table.pages.Clear();
        table.bindCounts.Clear();

        textureSparseExtensionAllocator.Dealloc(loadInfo.sparseExtension);
    }
    else if (loadInfo.alias == CoreGraphics::InvalidTextureId && loadInfo.mem.mem != VK_NULL_HANDLE)
    {
        Vulkan::DelayedFreeMemory(loadInfo.mem);
        loadInfo.mem = CoreGraphics::Alloc{};
    }

    // only unload a texture which isn't a window texture, since their textures come from the swap chain
    if (!loadInfo.windowTexture)
    {
        if (runtimeInfo.bind != 0xFFFFFFFF)
            VkShaderServer::Instance()->UnregisterTexture(runtimeInfo.bind, runtimeInfo.type);
        Vulkan::DelayedDeleteImageView(runtimeInfo.view);
        Vulkan::DelayedDeleteImage(loadInfo.img);
        runtimeInfo.view = VK_NULL_HANDLE;
        loadInfo.img = VK_NULL_HANDLE;
    }

    this->states[id.poolId] = Resources::Resource::State::Unloaded;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTextureCache::Reload(const Resources::ResourceId id)
{
    __Lock(textureAllocator);
    VkTextureLoadInfo& loadInfo = this->Get<Texture_LoadInfo>(id.resourceId);
    VkTextureRuntimeInfo& runtimeInfo = this->Get<Texture_RuntimeInfo>(id.resourceId);
    VkTextureWindowInfo& windowInfo = this->Get<Texture_WindowInfo>(id.resourceId);

    if (loadInfo.windowTexture || loadInfo.windowRelative)
    {
        uint tmp = runtimeInfo.bind;
        runtimeInfo.bind = 0xFFFFFFFF;
        this->Unload(id);

        // if the window has been resized, we need to update our dimensions based on relative size
        const CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(windowInfo.window);
        loadInfo.dims.width = SizeT(mode.GetWidth() * loadInfo.relativeDims.width);
        loadInfo.dims.height = SizeT(mode.GetHeight() * loadInfo.relativeDims.height);
        loadInfo.dims.depth = 1;

        runtimeInfo.bind = tmp;
        this->Setup(id);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTextureCache::GenerateMipmaps(const CoreGraphics::TextureId id)
{
    CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_TRANSFER, "Mipmap");
    SizeT numMips = CoreGraphics::TextureGetNumMips(id);

    // insert initial barrier for texture
    CoreGraphics::BarrierInsert(
        CoreGraphics::GraphicsQueueType,
        CoreGraphics::BarrierStage::AllGraphicsShaders,
        CoreGraphics::BarrierStage::Transfer,
        CoreGraphics::BarrierDomain::Global,
        {
            {
                id,
                CoreGraphics::ImageSubresourceInfo{ImageAspect::ColorBits, 0, (uint)numMips, 0, 1},
                CoreGraphics::ImageLayout::ShaderRead,
                CoreGraphics::ImageLayout::TransferSource,
                CoreGraphics::BarrierAccess::ShaderRead,
                CoreGraphics::BarrierAccess::TransferRead,
            },
        },
        nullptr,
        "Mipmap Generation Initial Barrier");

    // calculate number of mips
    TextureDimensions dims = GetDimensions(id);

    CoreGraphics::ImageLayout prevLayout = CoreGraphics::ImageLayout::TransferSource;
    CoreGraphics::BarrierAccess prevAccess = CoreGraphics::BarrierAccess::TransferRead;
    for (int mip = 0; mip < numMips - 1; mip++)
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
        CoreGraphics::BarrierInsert(
            CoreGraphics::GraphicsQueueType,
            CoreGraphics::BarrierStage::Transfer,
            CoreGraphics::BarrierStage::Transfer,
            CoreGraphics::BarrierDomain::Global,
            {
                {
                    id,
                    CoreGraphics::ImageSubresourceInfo{ CoreGraphics::ImageAspect::ColorBits, (uint)mip, 1, 0, 1 },
                    prevLayout,
                    CoreGraphics::ImageLayout::TransferSource,
                    prevAccess,
                    CoreGraphics::BarrierAccess::TransferRead,
                },
                {
                    id,
                    CoreGraphics::ImageSubresourceInfo{ CoreGraphics::ImageAspect::ColorBits, (uint)mip + 1, 1, 0, 1 },
                    CoreGraphics::ImageLayout::TransferSource,
                    CoreGraphics::ImageLayout::TransferDestination,
                    CoreGraphics::BarrierAccess::TransferRead,
                    CoreGraphics::BarrierAccess::TransferWrite,
                }
            },
            nullptr,
            "Mipmap Generation Barrier");
        CoreGraphics::Blit(id, fromRegion, mip, 0, id, toRegion, mip + 1, 0);

        prevLayout = CoreGraphics::ImageLayout::TransferDestination;
        prevAccess = CoreGraphics::BarrierAccess::TransferWrite;
    }

    // insert initial barrier for texture
    CoreGraphics::BarrierInsert(
        CoreGraphics::GraphicsQueueType,
        CoreGraphics::BarrierStage::Transfer,
        CoreGraphics::BarrierStage::AllGraphicsShaders,
        CoreGraphics::BarrierDomain::Global,
        {
            {
                id,
                CoreGraphics::ImageSubresourceInfo{ImageAspect::ColorBits, 0, (uint)numMips - 1, 0, 1},
                CoreGraphics::ImageLayout::TransferSource,
                CoreGraphics::ImageLayout::ShaderRead,
                CoreGraphics::BarrierAccess::TransferRead,
                CoreGraphics::BarrierAccess::ShaderRead,
            },
            {
                id,
                CoreGraphics::ImageSubresourceInfo{ImageAspect::ColorBits, (uint)numMips - 1, 1, 0, 1},
                prevLayout,
                CoreGraphics::ImageLayout::ShaderRead,
                prevAccess,
                CoreGraphics::BarrierAccess::ShaderRead,
            },
        },
        nullptr,
        "Mipmap Generation Finish Barrier");

    CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
}

//------------------------------------------------------------------------------
/**
*/
bool
VkMemoryTextureCache::Map(const CoreGraphics::TextureId id, IndexT mipLevel, CoreGraphics::GpuBufferTypes::MapType mapType, CoreGraphics::TextureMapInfo & outMapInfo)
{
    __Lock(textureAllocator);
    VkTextureRuntimeInfo& runtime = this->Get<0>(id.resourceId);
    VkTextureLoadInfo& load = this->Get<1>(id.resourceId);
    VkTextureMappingInfo& map = this->Get<2>(id.resourceId);

    bool retval = false;
    if (Texture2D == runtime.type)
    {
        VkFormat vkformat = VkTypes::AsVkFormat(load.format);
        VkTypes::VkBlockDimensions blockSize = VkTypes::AsVkBlockSize(vkformat);
        uint32_t size = CoreGraphics::PixelFormat::ToSize(load.format);

        uint32_t mipWidth = (uint32_t)Math::max(1.0f, Math::ceil(load.dims.width / Math::pow(2, (float)mipLevel)));
        uint32_t mipHeight = (uint32_t)Math::max(1.0f, Math::ceil(load.dims.height / Math::pow(2, (float)mipLevel)));

        map.region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        map.region.dstOffset = { 0, 0, 0 };
        map.region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mipLevel, 0, 1 };
        map.region.srcOffset = { 0, 0, 0 };
        map.region.extent = { mipWidth, mipHeight, 1 };
        CoreGraphics::Alloc alloc;
        VkUtilities::ReadImage(load.img, load.format, load.dims, runtime.type, map.region, alloc, map.buf);
        map.mem = alloc.mem;

        // the row pitch must be the size of one pixel times the number of pixels in width
        outMapInfo.mipWidth = mipWidth;
        outMapInfo.mipHeight = mipHeight;
        outMapInfo.rowPitch = (int32_t)alloc.size / mipHeight;
        outMapInfo.depthPitch = (int32_t)alloc.size;
        outMapInfo.data = (char*)GetMappedMemory(alloc);
        map.mapCount++;
    }
    else if (Texture3D == runtime.type)
    {
        VkFormat vkformat = VkTypes::AsVkFormat(load.format);
        VkTypes::VkBlockDimensions blockSize = VkTypes::AsVkBlockSize(vkformat);
        uint32_t size = CoreGraphics::PixelFormat::ToSize(load.format);

        uint32_t mipWidth = (uint32_t)Math::max(1.0f, Math::ceil(load.dims.width / Math::pow(2, (float)mipLevel)));
        uint32_t mipHeight = (uint32_t)Math::max(1.0f, Math::ceil(load.dims.height / Math::pow(2, (float)mipLevel)));
        uint32_t mipDepth = (uint32_t)Math::max(1.0f, Math::ceil(load.dims.depth / Math::pow(2, (float)mipLevel)));

        map.region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        map.region.dstOffset = { 0, 0, 0 };
        map.region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mipLevel, 1, 1 };
        map.region.srcOffset = { 0, 0, 0 };
        map.region.extent = { mipWidth, mipHeight, mipDepth };
        CoreGraphics::Alloc alloc;
        VkUtilities::ReadImage(load.img, load.format, load.dims, runtime.type, map.region, alloc, map.buf);

        // the row pitch must be the size of one pixel times the number of pixels in width
        outMapInfo.mipWidth = mipWidth;
        outMapInfo.mipHeight = mipHeight;
        outMapInfo.rowPitch = (int32_t)alloc.size / mipWidth;
        outMapInfo.depthPitch = (int32_t)alloc.size;
        outMapInfo.data = (char*)GetMappedMemory(alloc);
        map.mapCount++;
    }
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTextureCache::Unmap(const CoreGraphics::TextureId id, IndexT mipLevel)
{
    __Lock(textureAllocator);
    VkTextureRuntimeInfo& runtime = this->Get<0>(id.resourceId);
    VkTextureLoadInfo& load = this->Get<1>(id.resourceId);
    VkTextureMappingInfo& map = this->Get<2>(id.resourceId);

    // unmap and dealloc
    vkUnmapMemory(load.dev, load.mem.mem);
    VkUtilities::WriteImage(map.buf, load.img, map.region);
    map.mapCount--;
    if (map.mapCount == 0)
    {
        vkFreeMemory(load.dev, map.mem, nullptr);
        vkDestroyBuffer(load.dev, map.buf, nullptr);
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
VkMemoryTextureCache::MapCubeFace(const CoreGraphics::TextureId id, CoreGraphics::TextureCubeFace face, IndexT mipLevel, CoreGraphics::GpuBufferTypes::MapType mapType, CoreGraphics::TextureMapInfo & outMapInfo)
{
    __Lock(textureAllocator);
    VkTextureRuntimeInfo& runtime = this->Get<0>(id.resourceId);
    VkTextureLoadInfo& load = this->Get<1>(id.resourceId);
    VkTextureMappingInfo& map = this->Get<2>(id.resourceId);

    bool retval = false;

    VkFormat vkformat = VkTypes::AsVkFormat(load.format);
    VkTypes::VkBlockDimensions blockSize = VkTypes::AsVkBlockSize(vkformat);
    uint32_t size = CoreGraphics::PixelFormat::ToSize(load.format);

    uint32_t mipWidth = (uint32_t)Math::max(1.0f, Math::ceil(load.dims.width / Math::pow(2, (float)mipLevel)));
    uint32_t mipHeight = (uint32_t)Math::max(1.0f, Math::ceil(load.dims.height / Math::pow(2, (float)mipLevel)));

    map.region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    map.region.dstOffset = { 0, 0, 0 };
    map.region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mipLevel, (uint32_t)face, 1 };
    map.region.srcOffset = { 0, 0, 0 };
    map.region.extent = { mipWidth, mipHeight, 1 };
    CoreGraphics::Alloc alloc;
    VkUtilities::ReadImage(load.img, load.format, load.dims, runtime.type, map.region, alloc, map.buf);

    // the row pitch must be the size of one pixel times the number of pixels in width
    outMapInfo.mipWidth = mipWidth;
    outMapInfo.mipHeight = mipHeight;
    outMapInfo.rowPitch = (int32_t)alloc.size / mipWidth;
    outMapInfo.depthPitch = (int32_t)alloc.size;
    outMapInfo.data = (char*)GetMappedMemory(alloc);
    map.mapCount++;

    return retval;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTextureCache::UnmapCubeFace(const CoreGraphics::TextureId id, CoreGraphics::TextureCubeFace face, IndexT mipLevel)
{
    __Lock(textureAllocator);
    VkTextureRuntimeInfo& runtime = this->Get<0>(id.resourceId);
    VkTextureLoadInfo& load = this->Get<1>(id.resourceId);
    VkTextureMappingInfo& map = this->Get<2>(id.resourceId);

    // unmap and dealloc
    vkUnmapMemory(load.dev, load.mem.mem);
    VkUtilities::WriteImage(map.buf, load.img, map.region);
    map.mapCount--;
    if (map.mapCount == 0)
    {
        vkFreeMemory(load.dev, map.mem, nullptr);
        vkDestroyBuffer(load.dev, map.buf, nullptr);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTextureCache::Update(const CoreGraphics::TextureId id, void* data, SizeT dataSize, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip)
{
    VkBufferImageCopy copy;
    copy.imageExtent.width = width;
    copy.imageExtent.height = height;
    copy.imageExtent.depth = 1;         // hmm, might want this for cube maps and volume textures too
    copy.imageOffset.x = left;
    copy.imageOffset.y = top;
    copy.imageOffset.z = 0;
    copy.imageSubresource.mipLevel = mip;
    copy.imageSubresource.layerCount = 1;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.bufferOffset = 0;
    copy.bufferRowLength = dataSize / width;
    copy.bufferImageHeight = height;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTextureCache::Update(const CoreGraphics::TextureId id, CoreGraphics::TextureDimensions dims, void* data, SizeT dataSize, IndexT mip)
{
    VkBufferImageCopy copy;
    copy.imageExtent.width = dims.width;
    copy.imageExtent.height = dims.height;
    copy.imageExtent.depth = 1;         // hmm, might want this for cube maps and volume textures too
    copy.imageOffset.x = 0;
    copy.imageOffset.y = 0;
    copy.imageOffset.z = 0;
    copy.imageSubresource.mipLevel = mip;
    copy.imageSubresource.layerCount = 1;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.bufferOffset = 0;
    copy.bufferRowLength = dataSize / dims.width;
    copy.bufferImageHeight = dims.height;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTextureCache::UpdateArray(const CoreGraphics::TextureId id, void* data, SizeT dataSize, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip, IndexT layer)
{
    VkBufferImageCopy copy;
    copy.imageExtent.width = width;
    copy.imageExtent.height = height;
    copy.imageExtent.depth = 1;         // hmm, might want this for cube maps and volume textures too
    copy.imageOffset.x = left;
    copy.imageOffset.y = top;
    copy.imageOffset.z = 0;
    copy.imageSubresource.mipLevel = mip;
    copy.imageSubresource.layerCount = 1;
    copy.imageSubresource.baseArrayLayer = layer;
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.bufferOffset = 0;
    copy.bufferRowLength = dataSize / width;
    copy.bufferImageHeight = height;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTextureCache::UpdateArray(const CoreGraphics::TextureId id, CoreGraphics::TextureDimensions dims, void* data, SizeT dataSize, IndexT mip, IndexT layer)
{
    VkBufferImageCopy copy;
    copy.imageExtent.width = dims.width;
    copy.imageExtent.height = dims.height;
    copy.imageExtent.depth = 1;         // hmm, might want this for cube maps and volume textures too
    copy.imageOffset.x = 0;
    copy.imageOffset.y = 0;
    copy.imageOffset.z = 0;
    copy.imageSubresource.mipLevel = mip;
    copy.imageSubresource.layerCount = 1;
    copy.imageSubresource.baseArrayLayer = layer;
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.bufferOffset = 0;
    copy.bufferRowLength = dataSize / dims.width;
    copy.bufferImageHeight = dims.height;
}

//------------------------------------------------------------------------------
/**
*/
void 
VkMemoryTextureCache::Update(const CoreGraphics::TextureId id, const Math::rectangle<int>& region, IndexT mip, IndexT layer, char* buf, const CoreGraphics::SubmissionContextId sub)
{
    VkDevice dev = Vulkan::GetCurrentDevice();
    CoreGraphics::PixelFormat::Code fmt = TextureGetPixelFormat(id);
    TextureDimensions dims = TextureGetDimensions(id);

    // calculate buffer size and mipped dimensions
    uint width = Math::max(1, dims.width >> mip);
    uint height = Math::max(1, dims.height >> mip);
    uint bpp = CoreGraphics::PixelFormat::ToSize(fmt);
    bool compressed = CoreGraphics::PixelFormat::ToCompressed(fmt);
    SizeT bufSize;
    if (compressed)
        bufSize = ((width + 3) / 4) * ((height + 3) / 4) * bpp;
    else
        bufSize = width * height * bpp;

    // create transfer buffer
    const uint32_t qfamily = Vulkan::GetQueueFamily(CoreGraphics::TransferQueueType);
    VkBufferCreateInfo bufInfo =
    {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0,
        (uint32_t)bufSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        1,
        &qfamily
    };
    VkBuffer vkbuf;
    vkCreateBuffer(dev, &bufInfo, NULL, &vkbuf);

    // allocate temporary buffer
    CoreGraphics::Alloc alloc = AllocateMemory(dev, vkbuf, CoreGraphics::MemoryPool_HostLocal);
    vkBindBufferMemory(dev, vkbuf, alloc.mem, alloc.offset);
    char* mapped = (char*)GetMappedMemory(alloc);
    memcpy(mapped, buf, bufSize);

    // perform update of buffer, and stage a copy of buffer data to image
    VkBufferImageCopy copy;
    copy.bufferOffset = 0;
    copy.bufferImageHeight = 0;
    copy.bufferRowLength = 0;
    copy.imageExtent = { (uint32_t)region.width(), (uint32_t)region.height(), 1 };
    copy.imageOffset = { (int32_t)region.left, (int32_t)region.top, 0 };
    copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mip, (uint32_t)layer, 1 };
    vkCmdCopyBufferToImage(
        CommandBufferGetVk(CoreGraphics::SubmissionContextGetCmdBuffer(sub)),
        vkbuf,
        TextureGetVkImage(id),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copy);

    SubmissionContextFreeMemory(sub, alloc);
    SubmissionContextFreeVkBuffer(sub, dev, vkbuf);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkMemoryTextureCache::ClearColor(const CoreGraphics::TextureId id, Math::vec4 color, const CoreGraphics::ImageLayout layout, const CoreGraphics::ImageSubresourceInfo& subres, const CoreGraphics::SubmissionContextId sub)
{
    VkClearColorValue clear;
    VkImageSubresourceRange vksubres;
    vksubres.aspectMask = VkTypes::AsVkImageAspectFlags(subres.aspect);
    vksubres.baseArrayLayer = subres.layer;
    vksubres.layerCount = subres.layerCount;
    vksubres.baseMipLevel = subres.mip;
    vksubres.levelCount = subres.mipCount;

    VkCommandBuffer buffer = sub == InvalidSubmissionContextId ? GetMainBuffer(GraphicsQueueType) : CommandBufferGetVk(SubmissionContextGetCmdBuffer(sub));

    color.storeu(clear.float32);
    vkCmdClearColorImage(
        buffer,
        TextureGetVkImage(id),
        VkTypes::AsVkImageLayout(layout),
        &clear,
        1,
        &vksubres);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkMemoryTextureCache::ClearDepthStencil(const CoreGraphics::TextureId id, float depth, uint stencil, const CoreGraphics::ImageLayout layout, const CoreGraphics::ImageSubresourceInfo& subres, const CoreGraphics::SubmissionContextId sub)
{
    VkClearDepthStencilValue clear;
    VkImageSubresourceRange vksubres;
    vksubres.aspectMask = VkTypes::AsVkImageAspectFlags(subres.aspect);
    vksubres.baseArrayLayer = subres.layer;
    vksubres.layerCount = subres.layerCount;
    vksubres.baseMipLevel = subres.mip;
    vksubres.levelCount = subres.mipCount;

    VkCommandBuffer buffer = sub == InvalidSubmissionContextId ? GetMainBuffer(GraphicsQueueType) : CommandBufferGetVk(SubmissionContextGetCmdBuffer(sub));

    clear.depth = depth;
    clear.stencil = stencil;
    vkCmdClearDepthStencilImage(
        buffer,
        TextureGetVkImage(id),
        VkTypes::AsVkImageLayout(layout),
        &clear,
        1,
        &vksubres);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::TextureDimensions
VkMemoryTextureCache::GetDimensions(const CoreGraphics::TextureId id)
{
    return this->GetUnsafe<Texture_LoadInfo>(id.resourceId).dims;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::TextureRelativeDimensions 
VkMemoryTextureCache::GetRelativeDimensions(const CoreGraphics::TextureId id)
{
    return this->GetUnsafe<Texture_LoadInfo>(id.resourceId).relativeDims;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::PixelFormat::Code
VkMemoryTextureCache::GetPixelFormat(const CoreGraphics::TextureId id)
{
    return this->GetUnsafe<Texture_LoadInfo>(id.resourceId).format;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::TextureType
VkMemoryTextureCache::GetType(const CoreGraphics::TextureId id)
{
    return this->GetUnsafe<Texture_RuntimeInfo>(id.resourceId).type;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::TextureId 
VkMemoryTextureCache::GetAlias(const CoreGraphics::TextureId id)
{
    return this->GetUnsafe<Texture_LoadInfo>(id.resourceId).alias;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::TextureUsage 
VkMemoryTextureCache::GetUsageBits(const CoreGraphics::TextureId id)
{
    return this->GetUnsafe<Texture_LoadInfo>(id.resourceId).texUsage;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
VkMemoryTextureCache::GetNumMips(const CoreGraphics::TextureId id)
{
    return this->GetUnsafe<Texture_LoadInfo>(id.resourceId).mips;
}

//------------------------------------------------------------------------------
/**
*/
SizeT 
VkMemoryTextureCache::GetNumLayers(const CoreGraphics::TextureId id)
{
    return this->GetUnsafe<Texture_LoadInfo>(id.resourceId).layers;
}

//------------------------------------------------------------------------------
/**
*/
SizeT 
VkMemoryTextureCache::GetNumSamples(const CoreGraphics::TextureId id)
{
    return this->GetUnsafe<Texture_LoadInfo>(id.resourceId).samples;
}

//------------------------------------------------------------------------------
/**
*/
uint 
VkMemoryTextureCache::GetBindlessHandle(const CoreGraphics::TextureId id)
{
    return this->GetUnsafe<Texture_RuntimeInfo>(id.resourceId).bind;
}

//------------------------------------------------------------------------------
/**
*/
uint
VkMemoryTextureCache::GetStencilBindlessHandle(const CoreGraphics::TextureId id)
{
    Ids::Id32 stencil = this->GetUnsafe<Texture_LoadInfo>(id.resourceId).stencilExtension;
    n_assert(stencil != Ids::InvalidId32);
    return textureStencilExtensionAllocator.GetUnsafe<TextureExtension_StencilInfo>(stencil).bind;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ImageLayout 
VkMemoryTextureCache::GetDefaultLayout(const CoreGraphics::TextureId id)
{
    return this->GetUnsafe<Texture_LoadInfo>(id.resourceId).defaultLayout;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::TextureSparsePageSize 
VkMemoryTextureCache::SparseGetPageSize(const CoreGraphics::TextureId id)
{
    Ids::Id32 sparseExtension = this->GetUnsafe<Texture_LoadInfo>(id.resourceId).sparseExtension;
    n_assert(sparseExtension != Ids::InvalidId32);

    const VkSparseImageMemoryRequirements& reqs = textureSparseExtensionAllocator.GetUnsafe<TextureExtension_SparseMemoryRequirements>(sparseExtension);
    return TextureSparsePageSize
    {
        reqs.formatProperties.imageGranularity.width,
        reqs.formatProperties.imageGranularity.height,
        reqs.formatProperties.imageGranularity.depth
    };
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
VkMemoryTextureCache::SparseGetPageIndex(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT x, IndexT y, IndexT z)
{
    __Lock(textureAllocator);
    Ids::Id32 sparseExtension = this->Get<Texture_LoadInfo>(id.resourceId).sparseExtension;
    n_assert(sparseExtension != Ids::InvalidId32);

    const VkSparseImageMemoryRequirements& reqs = textureSparseExtensionAllocator.Get<TextureExtension_SparseMemoryRequirements>(sparseExtension);
    if (reqs.imageMipTailFirstLod > (uint32_t)mip)
    {
        const TextureSparsePageTable& table = textureSparseExtensionAllocator.Get<TextureExtension_SparsePageTable>(sparseExtension);
        uint32_t strideX = reqs.formatProperties.imageGranularity.width;
        uint32_t strideY = reqs.formatProperties.imageGranularity.height;
        uint32_t strideZ = reqs.formatProperties.imageGranularity.depth;
        return x / strideX + (table.bindCounts[layer][mip][0] * (y / strideY + table.bindCounts[layer][mip][1] * z / strideZ));
    }
    else
        return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::TextureSparsePage& 
VkMemoryTextureCache::SparseGetPage(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT pageIndex)
{
    Ids::Id32 sparseExtension = this->GetUnsafe<Texture_LoadInfo>(id.resourceId).sparseExtension;
    n_assert(sparseExtension != Ids::InvalidId32);

    const TextureSparsePageTable& table = textureSparseExtensionAllocator.GetUnsafe<TextureExtension_SparsePageTable>(sparseExtension);
    return table.pages[layer][mip][pageIndex];
}

//------------------------------------------------------------------------------
/**
*/
SizeT 
VkMemoryTextureCache::SparseGetNumPages(const CoreGraphics::TextureId id, IndexT layer, IndexT mip)
{
    Ids::Id32 sparseExtension = this->GetUnsafe<Texture_LoadInfo>(id.resourceId).sparseExtension;
    n_assert(sparseExtension != Ids::InvalidId32);

    const TextureSparsePageTable& table = textureSparseExtensionAllocator.GetUnsafe<TextureExtension_SparsePageTable>(sparseExtension);
    const VkSparseImageMemoryRequirements& reqs = textureSparseExtensionAllocator.GetUnsafe<TextureExtension_SparseMemoryRequirements>(sparseExtension);
    if (reqs.imageMipTailFirstLod > (uint32_t)mip)
        return table.pages[layer][mip].Size();
    else
        return 0;
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
VkMemoryTextureCache::SparseGetMaxMip(const CoreGraphics::TextureId id)
{
    Ids::Id32 sparseExtension = this->GetUnsafe<Texture_LoadInfo>(id.resourceId).sparseExtension;
    n_assert(sparseExtension != Ids::InvalidId32);
    const VkSparseImageMemoryRequirements& reqs = textureSparseExtensionAllocator.GetUnsafe<TextureExtension_SparseMemoryRequirements>(sparseExtension);
    return reqs.imageMipTailFirstLod;
}

//------------------------------------------------------------------------------
/**
*/
void 
VkMemoryTextureCache::SparseEvict(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT pageIndex)
{
    __Lock(textureAllocator);
    Ids::Id32 sparseExtension = this->Get<Texture_LoadInfo>(id.resourceId).sparseExtension;
    n_assert(sparseExtension != Ids::InvalidId32);

    const TextureSparsePageTable& table = textureSparseExtensionAllocator.Get<TextureExtension_SparsePageTable>(sparseExtension);
    Util::Array<VkSparseImageMemoryBind>& pageBinds = textureSparseExtensionAllocator.Get<TextureExtension_SparsePendingBinds>(sparseExtension);
    VkDevice dev = GetCurrentDevice();

    // get page and allocate memory
    CoreGraphics::TextureSparsePage& page = table.pages[layer][mip][pageIndex];
    n_assert(page.alloc.mem != VK_NULL_HANDLE);
    VkSparseImageMemoryBind binding;

    // deallocate memory
    CoreGraphics::FreeMemory(page.alloc);
    page.alloc.mem = VK_NULL_HANDLE;
    page.alloc.offset = 0;
    binding.subresource =
    {
        VK_IMAGE_ASPECT_COLOR_BIT,
        (uint32_t)mip,
        (uint32_t)layer
    };
    binding.offset = { (int32_t)page.offset.x, (int32_t)page.offset.y, (int32_t)page.offset.z };
    binding.extent = { page.extent.width, page.extent.height, page.extent.depth };
    binding.memory = VK_NULL_HANDLE;
    binding.memoryOffset = 0;
    binding.flags = 0;

    N_COUNTER_DECR(N_SPARSE_PAGE_MEMORY_COUNTER, page.alloc.size);

    // append pending page update
    pageBinds.Append(binding);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkMemoryTextureCache::SparseMakeResident(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT pageIndex)
{
    __Lock(textureAllocator);
    Ids::Id32 sparseExtension = this->Get<Texture_LoadInfo>(id.resourceId).sparseExtension;
    n_assert(sparseExtension != Ids::InvalidId32);

    const TextureSparsePageTable& table = textureSparseExtensionAllocator.Get<TextureExtension_SparsePageTable>(sparseExtension);
    Util::Array<VkSparseImageMemoryBind>& pageBinds = textureSparseExtensionAllocator.Get<TextureExtension_SparsePendingBinds>(sparseExtension);
    VkDevice dev = GetCurrentDevice();

    // get page and allocate memory
    CoreGraphics::TextureSparsePage& page = table.pages[layer][mip][pageIndex];
    n_assert(page.alloc.mem == VK_NULL_HANDLE);

    // allocate memory and append page update
    page.alloc = Vulkan::AllocateMemory(dev, table.memoryReqs, table.memoryReqs.alignment);
    VkSparseImageMemoryBind binding;
    binding.subresource =
    {
        VK_IMAGE_ASPECT_COLOR_BIT,
        (uint32_t)mip, 
        (uint32_t)layer
    };
    binding.offset = { (int32_t)page.offset.x, (int32_t)page.offset.y, (int32_t)page.offset.z };
    binding.extent = { page.extent.width, page.extent.height, page.extent.depth };
    binding.memory = page.alloc.mem;
    binding.memoryOffset = page.alloc.offset;
    binding.flags = 0;

    N_COUNTER_INCR(N_SPARSE_PAGE_MEMORY_COUNTER, page.alloc.size);

    // add a pending bind
    pageBinds.Append(binding);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkMemoryTextureCache::SparseEvictMip(const CoreGraphics::TextureId id, IndexT layer, IndexT mip)
{
    __Lock(textureAllocator);
    Ids::Id32 sparseExtension = this->Get<Texture_LoadInfo>(id.resourceId).sparseExtension;
    n_assert(sparseExtension != Ids::InvalidId32);

    const TextureSparsePageTable& table = textureSparseExtensionAllocator.Get<TextureExtension_SparsePageTable>(sparseExtension);
    Util::Array<VkSparseImageMemoryBind>& pageBinds = textureSparseExtensionAllocator.Get<TextureExtension_SparsePendingBinds>(sparseExtension);
    VkDevice dev = GetCurrentDevice();

    const Util::Array<CoreGraphics::TextureSparsePage>& pages = table.pages[layer][mip];

    // evict all pages
    for (int i = 0; i < pages.Size(); i++)
    {
        // release page
        CoreGraphics::FreeMemory(pages[i].alloc);
        VkSparseImageMemoryBind binding;
        binding.subresource =
        {
            VK_IMAGE_ASPECT_COLOR_BIT,
            (uint32_t)mip,
            (uint32_t)layer
        };
        binding.offset = { (int32_t)pages[i].offset.x, (int32_t)pages[i].offset.y, (int32_t)pages[i].offset.z };
        binding.extent = { pages[i].extent.width, pages[i].extent.height, pages[i].extent.depth };
        binding.memory = VK_NULL_HANDLE;
        binding.memoryOffset = 0;
        binding.flags = 0;

        // add a pending bind
        pageBinds.Append(binding);
    }

    N_COUNTER_DECR(N_SPARSE_PAGE_MEMORY_COUNTER, pages[0].alloc.size * pages.Size());

}

//------------------------------------------------------------------------------
/**
*/
void 
VkMemoryTextureCache::SparseMakeMipResident(const CoreGraphics::TextureId id, IndexT layer, IndexT mip)
{
    __Lock(textureAllocator);
    Ids::Id32 sparseExtension = this->Get<Texture_LoadInfo>(id.resourceId).sparseExtension;
    n_assert(sparseExtension != Ids::InvalidId32);

    const TextureSparsePageTable& table = textureSparseExtensionAllocator.Get<TextureExtension_SparsePageTable>(sparseExtension);
    Util::Array<VkSparseImageMemoryBind>& pageBinds = textureSparseExtensionAllocator.Get<TextureExtension_SparsePendingBinds>(sparseExtension);
    VkDevice dev = GetCurrentDevice();

    const Util::Array<CoreGraphics::TextureSparsePage>& pages = table.pages[layer][mip];

    // allocate all pages
    for (int i = 0; i < pages.Size(); i++)
    {
        // allocate memory and append page update
        pages[i].alloc = Vulkan::AllocateMemory(dev, table.memoryReqs, table.memoryReqs.alignment);
        VkSparseImageMemoryBind binding;
        binding.subresource =
        {
            VK_IMAGE_ASPECT_COLOR_BIT,
            (uint32_t)mip,
            (uint32_t)layer
        };
        binding.offset = { (int32_t)pages[i].offset.x, (int32_t)pages[i].offset.y, (int32_t)pages[i].offset.z };
        binding.extent = { pages[i].extent.width, pages[i].extent.height, pages[i].extent.depth };
        binding.memory = pages[i].alloc.mem;
        binding.memoryOffset = pages[i].alloc.offset;
        binding.flags = 0;

        // add a pending bind
        pageBinds.Append(binding);
    }

    N_COUNTER_INCR(N_SPARSE_PAGE_MEMORY_COUNTER, pages[0].alloc.size * pages.Size());
}

//------------------------------------------------------------------------------
/**
*/
void 
VkMemoryTextureCache::SparseCommitChanges(const CoreGraphics::TextureId id)
{
    __Lock(textureAllocator);
    Ids::Id32 sparseExtension = this->Get<Texture_LoadInfo>(id.resourceId).sparseExtension;
    VkImage img = this->Get<Texture_LoadInfo>(id.resourceId).img;
    n_assert(sparseExtension != Ids::InvalidId32);

    Util::Array<VkSparseMemoryBind>& opaqueBinds = textureSparseExtensionAllocator.Get<TextureExtension_SparseOpaqueBinds>(sparseExtension);
    Util::Array<VkSparseImageMemoryBind>& pageBinds = textureSparseExtensionAllocator.Get<TextureExtension_SparsePendingBinds>(sparseExtension);

    // abort early if we have no updates
    if (opaqueBinds.IsEmpty() && pageBinds.IsEmpty())
        return;

    // setup bind structs
    VkSparseImageMemoryBindInfo imageMemoryBindInfo =
    {
        img,
        (uint32_t)(uint32_t)pageBinds.Size(),
        pageBinds.Size() > 0 ? pageBinds.Begin() : nullptr
    };
    VkSparseImageOpaqueMemoryBindInfo opaqueMemoryBindInfo =
    {
        img,
        (uint32_t)opaqueBinds.Size(),
        opaqueBinds.Size() > 0 ? opaqueBinds.Begin() : nullptr
    };
    VkBindSparseInfo bindInfo =
    {
        VK_STRUCTURE_TYPE_BIND_SPARSE_INFO,
        nullptr,
        0, nullptr,
        0, nullptr,
        opaqueBinds.IsEmpty() ? 0u : 1u, &opaqueMemoryBindInfo,
        pageBinds.IsEmpty() ? 0u : 1u, &imageMemoryBindInfo,
        0, nullptr
    };

    // execute sparse bind, the bind call
    Vulkan::SparseTextureBind(img, opaqueBinds, pageBinds);

    // clear all pending binds
    pageBinds.Clear();
    opaqueBinds.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void 
VkMemoryTextureCache::SparseUpdate(const CoreGraphics::TextureId id, const Math::rectangle<uint>& region, IndexT mip, IndexT layer, const CoreGraphics::TextureId source, const CoreGraphics::SubmissionContextId sub)
{
    __Lock(textureAllocator);
    TextureDimensions dims = this->Get<Texture_LoadInfo>(source.resourceId).dims;

    VkImageBlit blit;
    blit.srcOffsets[0] = { 0, 0, 0 };
    blit.srcOffsets[1] = { dims.width, dims.height, dims.depth };
    blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    blit.dstOffsets[0] = { (int32_t)region.left, (int32_t)region.top, 0 };
    blit.dstOffsets[1] = { (int32_t)region.right, (int32_t)region.bottom, 1 };
    blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mip, (uint32_t)layer, 1 };

    // perform blit
    vkCmdBlitImage(CommandBufferGetVk(CoreGraphics::SubmissionContextGetCmdBuffer(sub)),
        TextureGetVkImage(source),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        TextureGetVkImage(id),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &blit, VK_FILTER_LINEAR);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkMemoryTextureCache::SparseUpdate(const CoreGraphics::TextureId id, const Math::rectangle<uint>& region, IndexT mip, IndexT layer, char* buf, const CoreGraphics::SubmissionContextId sub)
{
    __Lock(textureAllocator);
    // allocate intermediate buffer and copy row-wise
    CoreGraphics::PixelFormat::Code fmt = TextureGetPixelFormat(id);
    TextureDimensions dims = TextureGetDimensions(id);

    uint mippedWidth = Math::max(dims.width >> mip, 1);
    uint width = region.width();
    uint height = region.height();
    uint top = region.top;
    uint left = region.left;
    uint bpp = CoreGraphics::PixelFormat::ToSize(fmt);
    bool compressed = CoreGraphics::PixelFormat::ToCompressed(fmt);
    if (compressed)
    {
        // fix up size to be block size
        width = (width + 3) / 4;
        height = (height + 3) / 4;
        top = (top + 3) / 4;
        left = (left + 3) / 4;
        mippedWidth = (mippedWidth + 3) / 4;
    }

    SizeT bufSize = width * height * bpp;
    char* intermediate = n_new_array(char, bufSize);
    uint j = 0;
    for (uint i = top; i < top + height; i++, j++)
    {
        uint fromIndex = j * bpp * width;
        uint toIndex = left * bpp + i * mippedWidth * bpp;
        memcpy(
            intermediate + fromIndex, 
            buf + toIndex, 
            bpp * width);
    }

    VkDevice dev = Vulkan::GetCurrentDevice();

    // create transfer buffer
    const uint32_t qfamily = Vulkan::GetQueueFamily(CoreGraphics::TransferQueueType);
    VkBufferCreateInfo bufInfo =
    {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0,
        (uint32_t)bufSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        1,
        &qfamily
    };
    VkBuffer vkbuf;
    vkCreateBuffer(dev, &bufInfo, NULL, &vkbuf);

    // allocate temporary buffer
    CoreGraphics::Alloc alloc = AllocateMemory(dev, vkbuf, CoreGraphics::MemoryPool_HostLocal);
    vkBindBufferMemory(dev, vkbuf, alloc.mem, alloc.offset);
    char* mapped = (char*)GetMappedMemory(alloc);
    memcpy(mapped, intermediate, alloc.size);

    n_delete_array(intermediate);

    // perform update of buffer, and stage a copy of buffer data to image
    VkBufferImageCopy copy;
    copy.bufferOffset = 0;
    copy.bufferImageHeight = 0;
    copy.bufferRowLength = 0;
    copy.imageExtent = { region.width(), region.height(), 1 };
    copy.imageOffset = { (int32_t)region.left, (int32_t)region.top, 0 };
    copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mip, (uint32_t)layer, 1 };
    vkCmdCopyBufferToImage(
        CommandBufferGetVk(CoreGraphics::SubmissionContextGetCmdBuffer(sub)), 
        vkbuf, 
        TextureGetVkImage(id), 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        1, 
        &copy);

    SubmissionContextFreeMemory(sub, alloc);
    SubmissionContextFreeVkBuffer(sub, dev, vkbuf);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkMemoryTextureCache::SparseUpdate(const CoreGraphics::TextureId id, IndexT mip, IndexT layer, char* buf, const CoreGraphics::SubmissionContextId sub)
{
    VkDevice dev = Vulkan::GetCurrentDevice();
    CoreGraphics::PixelFormat::Code fmt = TextureGetPixelFormat(id);
    TextureDimensions dims = TextureGetDimensions(id);

    // calculate buffer size and mipped dimensions
    uint width = Math::max(1, dims.width >> mip);
    uint height = Math::max(1, dims.height >> mip);
    uint bpp = CoreGraphics::PixelFormat::ToSize(fmt);
    bool compressed = CoreGraphics::PixelFormat::ToCompressed(fmt);
    SizeT bufSize;
    if (compressed)
        bufSize = ((width + 3) / 4) * ((height + 3) / 4) * bpp;
    else
        bufSize = width * height * bpp;

    // create transfer buffer
    const uint32_t qfamily = Vulkan::GetQueueFamily(CoreGraphics::TransferQueueType);
    VkBufferCreateInfo bufInfo =
    {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0,
        (uint32_t)bufSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        1,
        &qfamily
    };
    VkBuffer vkbuf;
    vkCreateBuffer(dev, &bufInfo, NULL, &vkbuf);

    // allocate temporary buffer
    CoreGraphics::Alloc alloc = AllocateMemory(dev, vkbuf, CoreGraphics::MemoryPool_HostLocal);
    vkBindBufferMemory(dev, vkbuf, alloc.mem, alloc.offset);
    char* mapped = (char*)GetMappedMemory(alloc);
    memcpy(mapped, buf, alloc.size);

    // perform update of buffer, and stage a copy of buffer data to image
    VkBufferImageCopy copy;
    copy.bufferOffset = 0;
    copy.bufferImageHeight = 0;
    copy.bufferRowLength = 0;
    copy.imageExtent = { width, height, 1 };
    copy.imageOffset = { 0, 0, 0 };
    copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mip, (uint32_t)layer, 1 };
    vkCmdCopyBufferToImage(
        CommandBufferGetVk(CoreGraphics::SubmissionContextGetCmdBuffer(sub)),
        vkbuf,
        TextureGetVkImage(id),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copy);

    SubmissionContextFreeMemory(sub, alloc);
    SubmissionContextFreeVkBuffer(sub, dev, vkbuf);
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
VkMemoryTextureCache::SwapBuffers(const CoreGraphics::TextureId id)
{
    __Lock(textureAllocator);
    __Lock(textureSwapExtensionAllocator);
    VkTextureLoadInfo& loadInfo = this->Get<Texture_LoadInfo>(id.resourceId);
    VkTextureRuntimeInfo& runtimeInfo = this->Get<Texture_RuntimeInfo>(id.resourceId);
    VkTextureWindowInfo& wnd = this->Get<Texture_WindowInfo>(id.resourceId);
    VkTextureSwapInfo& swap = textureSwapExtensionAllocator.Get<TextureExtension_SwapInfo>(loadInfo.swapExtension);
    n_assert(wnd.window != CoreGraphics::InvalidWindowId);
    VkWindowSwapInfo& swapInfo = CoreGraphics::glfwWindowAllocator.Get<5>(wnd.window.id24);

    // get present fence and be sure it is finished before getting the next image
    VkDevice dev = Vulkan::GetCurrentDevice();
    VkFence fence = Vulkan::GetPresentFence();
    VkResult res = vkWaitForFences(dev, 1, &fence, true, UINT64_MAX);
    n_assert(res == VK_SUCCESS);
    res = vkResetFences(dev, 1, &fence);
    n_assert(res == VK_SUCCESS);

    // get the next image
    res = vkAcquireNextImageKHR(dev, swapInfo.swapchain, UINT64_MAX, VK_NULL_HANDLE, fence, &swapInfo.currentBackbuffer);

    //Vulkan::WaitForPresent(sem);
    if (res == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // this means our swapchain needs a resize!
    }
    else
    {
        n_assert(res == VK_SUCCESS);
    }

    // set image and update texture
    loadInfo.img = swap.swapimages[swapInfo.currentBackbuffer];
    runtimeInfo.view = swap.swapviews[swapInfo.currentBackbuffer];
    return swapInfo.currentBackbuffer;
}

//------------------------------------------------------------------------------
/**
*/
bool
VkMemoryTextureCache::Setup(const Resources::ResourceId id)
{
    __Lock(textureAllocator);
    VkTextureRuntimeInfo& runtimeInfo = this->Get<Texture_RuntimeInfo>(id.resourceId);
    VkTextureLoadInfo& loadInfo = this->Get<Texture_LoadInfo>(id.resourceId);
    VkTextureWindowInfo& windowInfo = this->Get<Texture_WindowInfo>(id.resourceId);

    loadInfo.stencilExtension = Ids::InvalidId32;
    loadInfo.swapExtension = Ids::InvalidId32;

    VkFormat vkformat = VkTypes::AsVkFormat(loadInfo.format);

    VkFormatProperties formatProps;
    VkPhysicalDevice physicalDev = Vulkan::GetCurrentPhysicalDevice();
    vkGetPhysicalDeviceFormatProperties(physicalDev, vkformat, &formatProps);
    VkExtent3D extents;

    extents.width = loadInfo.dims.width;
    extents.height = loadInfo.dims.height;
    extents.depth = loadInfo.dims.depth;

    bool isDepthFormat = VkTypes::IsDepthFormat(loadInfo.format);

    // setup usage flags, by default, all textures can be sampled from
    // we automatically assign VK_IMAGE_USAGE_SAMPLED_BIT to sampled images, render textures and readwrite textures, but not for transfer textures
    VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    if (loadInfo.texUsage & TextureUsage::SampleTexture)
        usage |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (loadInfo.texUsage & TextureUsage::RenderTexture)
        usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | (isDepthFormat ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (loadInfo.texUsage & TextureUsage::ReadWriteTexture)
        usage |= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (loadInfo.texUsage & TextureUsage::TransferTextureSource)
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (loadInfo.texUsage & TextureUsage::TransferTextureDestination)
        usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (!loadInfo.windowTexture)
    {
        VkSampleCountFlagBits samples = VkTypes::AsVkSampleFlags(loadInfo.samples);
        VkImageViewType viewType = VkTypes::AsVkImageViewType(runtimeInfo.type);
        VkImageType type = VkTypes::AsVkImageType(runtimeInfo.type);

        // if read-write, we will almost definitely use this texture on multiple queues
        VkSharingMode sharingMode = (loadInfo.texUsage & TextureUsage::ReadWriteTexture) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        const Util::Set<uint32_t>& queues = Vulkan::GetQueueFamilies();

        if (queues.Size() <= 1)
            sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;

        VkImageCreateFlags createFlags = 0;

        if (loadInfo.alias != CoreGraphics::InvalidTextureId)
            createFlags |= VK_IMAGE_CREATE_ALIAS_BIT;
        if (viewType == VK_IMAGE_VIEW_TYPE_CUBE || viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
            createFlags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        if (loadInfo.sparse)
            createFlags |= VK_IMAGE_CREATE_SPARSE_BINDING_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;

        VkImageCreateInfo imgInfo =
        {
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            nullptr,
            createFlags,
            type,
            vkformat,
            extents,
            loadInfo.mips,
            loadInfo.layers,
            samples,
            VK_IMAGE_TILING_OPTIMAL,
            usage,
            sharingMode,
            sharingMode == VK_SHARING_MODE_CONCURRENT ? queues.Size() : 0u,
            sharingMode == VK_SHARING_MODE_CONCURRENT ? queues.KeysAsArray().Begin() : nullptr,
            VK_IMAGE_LAYOUT_UNDEFINED
        };

        VkResult stat = vkCreateImage(loadInfo.dev, &imgInfo, nullptr, &loadInfo.img);
        n_assert(stat == VK_SUCCESS);

        // if we have a sparse texture, don't allocate any memory or load any pixels
        if (loadInfo.sparse)
        {
            loadInfo.sparseExtension = textureSparseExtensionAllocator.Alloc();

            // setup sparse and commit the initial page updates
            SetupSparse(loadInfo.dev, loadInfo.img, loadInfo.sparseExtension, loadInfo);
            TextureSparseCommitChanges(id);
        }
        else
        {
            // if we don't use aliasing, create new memory
            if (loadInfo.alias == CoreGraphics::InvalidTextureId)
            {
                // allocate memory backing
                CoreGraphics::Alloc alloc = AllocateMemory(loadInfo.dev, loadInfo.img, CoreGraphics::MemoryPool_DeviceLocal);
                VkResult res = vkBindImageMemory(loadInfo.dev, loadInfo.img, alloc.mem, alloc.offset);
                n_assert(res == VK_SUCCESS);
                loadInfo.mem = alloc;
            }
            else
            {
                // otherwise use other image memory to create alias
                CoreGraphics::Alloc mem = this->Get<Texture_LoadInfo>(loadInfo.alias.resourceId).mem;
                loadInfo.mem = mem;
                VkResult res = vkBindImageMemory(loadInfo.dev, loadInfo.img, loadInfo.mem.mem, loadInfo.mem.offset);
                n_assert(res == VK_SUCCESS);
            }

            // if we have initial data to setup, perform a data transfer
            if (loadInfo.texBuffer != nullptr)
            {
                // use resource submission
                CoreGraphics::LockResourceSubmission();
                CoreGraphics::SubmissionContextId sub = CoreGraphics::GetSetupSubmissionContext();

                // transition into transfer mode
                VkImageSubresourceRange subres;
                subres.aspectMask = isDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
                subres.baseArrayLayer = 0;
                subres.baseMipLevel = 0;
                subres.layerCount = loadInfo.layers;
                subres.levelCount = loadInfo.mips;

                // insert barrier
                VkUtilities::ImageBarrier(CoreGraphics::SubmissionContextGetCmdBuffer(sub),
                    CoreGraphics::BarrierStage::Top,
                    CoreGraphics::BarrierStage::Transfer,
                    VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));

                // add image update, take the output buffer and memory and add to delayed delete
                VkBuffer outBuf;
                CoreGraphics::Alloc outAlloc;
                uint32_t size = PixelFormat::ToSize(loadInfo.format);
                VkUtilities::ImageUpdate(loadInfo.dev, CoreGraphics::SubmissionContextGetCmdBuffer(sub), TransferQueueType, loadInfo.img, extents, 0, 0, VkDeviceSize(loadInfo.dims.width * loadInfo.dims.height * loadInfo.dims.depth * size), (uint32_t*)loadInfo.texBuffer, outBuf, outAlloc);

                // add host memory buffer, intermediate device memory, and intermediate device buffer to delete queue
                SubmissionContextFreeMemory(sub, outAlloc);
                SubmissionContextFreeVkBuffer(sub, loadInfo.dev, outBuf);

                // transition image to be used for rendering
                VkUtilities::ImageBarrier(CoreGraphics::SubmissionContextGetCmdBuffer(sub),
                    CoreGraphics::BarrierStage::Transfer,
                    CoreGraphics::BarrierStage::AllGraphicsShaders,
                    VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, TransferQueueType, GraphicsQueueType, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

                // this should always be set to nullptr after it has been transfered. TextureLoadInfo should never own the pointer!
                loadInfo.texBuffer = nullptr;
                CoreGraphics::UnlockResourceSubmission();
            }
        }

        // if used for render, find appropriate renderable format
        if (loadInfo.texUsage & TextureUsage::RenderTexture)
        {
            if (!isDepthFormat)
            {
                VkFormatProperties formatProps;
                vkGetPhysicalDeviceFormatProperties(Vulkan::GetCurrentPhysicalDevice(), vkformat, &formatProps);
                n_assert(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT &&
                    formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT &&
                    formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
            }
        }

        // create view
        VkImageSubresourceRange viewRange;
        viewRange.aspectMask = isDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT; // view only supports reading depth in shader
        viewRange.baseMipLevel = 0;
        viewRange.levelCount = loadInfo.mips;
        viewRange.baseArrayLayer = 0;
        viewRange.layerCount = loadInfo.layers;
        VkImageViewCreateInfo viewCreate =
        {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            nullptr,
            0,
            loadInfo.img,
            viewType,
            vkformat,
            VkTypes::AsVkMapping(loadInfo.format),
            viewRange
        };
        stat = vkCreateImageView(loadInfo.dev, &viewCreate, nullptr, &runtimeInfo.view);
        n_assert(stat == VK_SUCCESS);

        // setup stencil image
        if (isDepthFormat)
        {
            __Lock(textureStencilExtensionAllocator);

            // setup stencil extension
            loadInfo.stencilExtension = textureStencilExtensionAllocator.Alloc();
            VkTextureRuntimeInfo& stencilRuntimeInfo = textureStencilExtensionAllocator.Get<TextureExtension_StencilInfo>(loadInfo.stencilExtension);
            stencilRuntimeInfo.type = runtimeInfo.type;
            viewRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
            viewCreate.subresourceRange = viewRange;
            stat = vkCreateImageView(loadInfo.dev, &viewCreate, nullptr, &stencilRuntimeInfo.view);
        }

        // use setup submission
        CoreGraphics::SubmissionContextId sub = CoreGraphics::GetSetupSubmissionContext();
        CommandBufferId cmdBuf = SubmissionContextGetCmdBuffer(sub);

        if (loadInfo.clear || loadInfo.texUsage & TextureUsage::RenderTexture)
        {
            // perform initial clear if render target
            if (!isDepthFormat)
            {
                VkClearColorValue clear = { loadInfo.clearColor.x, loadInfo.clearColor.y, loadInfo.clearColor.z, loadInfo.clearColor.w };
                VkUtilities::ImageBarrier(cmdBuf, CoreGraphics::BarrierStage::Host, CoreGraphics::BarrierStage::Transfer, VkUtilities::ImageMemoryBarrier(loadInfo.img, viewRange, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
                VkUtilities::ImageColorClear(cmdBuf, loadInfo.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clear, viewRange);
                VkUtilities::ImageBarrier(cmdBuf, CoreGraphics::BarrierStage::Transfer, CoreGraphics::BarrierStage::PassOutput, VkUtilities::ImageMemoryBarrier(loadInfo.img, viewRange, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
            }
            else
            {
                // clear image and transition layout
                VkClearDepthStencilValue clear = { loadInfo.clearDepthStencil.depth, loadInfo.clearDepthStencil.stencil };
                VkImageSubresourceRange clearRange = viewRange;
                clearRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                VkUtilities::ImageBarrier(cmdBuf, CoreGraphics::BarrierStage::Host, CoreGraphics::BarrierStage::Transfer, VkUtilities::ImageMemoryBarrier(loadInfo.img, clearRange, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
                VkUtilities::ImageDepthStencilClear(cmdBuf, loadInfo.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clear, clearRange);
                VkUtilities::ImageBarrier(cmdBuf, CoreGraphics::BarrierStage::Transfer, CoreGraphics::BarrierStage::LateDepth, VkUtilities::ImageMemoryBarrier(loadInfo.img, clearRange, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL));
            }
        }
        else if (loadInfo.texUsage & TextureUsage::ReadWriteTexture)
        {
            // insert barrier to transition into a useable state
            VkUtilities::ImageBarrier(cmdBuf,
                CoreGraphics::BarrierStage::Host,
                CoreGraphics::BarrierStage::AllGraphicsShaders,
                VkUtilities::ImageMemoryBarrier(loadInfo.img, viewRange, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        }

        // register image with shader server
        if (loadInfo.bindless)
        {
            if (runtimeInfo.bind == 0xFFFFFFFF)
                runtimeInfo.bind = VkShaderServer::Instance()->RegisterTexture(TextureId(id), runtimeInfo.type, isDepthFormat);
            else
                VkShaderServer::Instance()->ReregisterTexture(TextureId(id), runtimeInfo.type, runtimeInfo.bind, isDepthFormat);

            // if this is a depth-stencil texture, also register the stencil
            if (isDepthFormat)
            {
                __Lock(textureStencilExtensionAllocator);
                VkTextureRuntimeInfo& stencilRuntimeInfo = textureStencilExtensionAllocator.Get<TextureExtension_StencilInfo>(loadInfo.stencilExtension);
                stencilRuntimeInfo.bind = VkShaderServer::Instance()->RegisterTexture(TextureId(id), runtimeInfo.type, true, true);
            }
        }
        else
            runtimeInfo.bind = 0xFFFFFFFF;

    }
    else // setup as window texture
    {
        // get submission context
        n_assert(windowInfo.window != CoreGraphics::InvalidWindowId);
        CoreGraphics::SubmissionContextId sub = CoreGraphics::GetSetupSubmissionContext();
        CommandBufferId cmdBuf = SubmissionContextGetCmdBuffer(sub);

        // setup swap extension
        __Lock(textureSwapExtensionAllocator);
        loadInfo.swapExtension = textureSwapExtensionAllocator.Alloc();
        VkTextureSwapInfo& swapInfo = textureSwapExtensionAllocator.Get<TextureExtension_SwapInfo>(loadInfo.swapExtension);

        VkBackbufferInfo& backbufferInfo = CoreGraphics::glfwWindowAllocator.Get<GLFW_Backbuffer>(windowInfo.window.id24);
        swapInfo.swapimages = backbufferInfo.backbuffers;
        swapInfo.swapviews = backbufferInfo.backbufferViews;
        VkClearColorValue clear = { 0, 0, 0, 0 };

        VkImageSubresourceRange subres;
        subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subres.baseArrayLayer = 0;
        subres.baseMipLevel = 0;
        subres.layerCount = 1;
        subres.levelCount = 1;

        // clear textures
        IndexT i;
        for (i = 0; i < swapInfo.swapimages.Size(); i++)
        {
            VkUtilities::ImageBarrier(cmdBuf, CoreGraphics::BarrierStage::Host, CoreGraphics::BarrierStage::Transfer, VkUtilities::ImageMemoryBarrier(swapInfo.swapimages[i], subres, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
            VkUtilities::ImageColorClear(cmdBuf, swapInfo.swapimages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clear, subres);
            VkUtilities::ImageBarrier(cmdBuf, CoreGraphics::BarrierStage::Transfer, CoreGraphics::BarrierStage::PassOutput, VkUtilities::ImageMemoryBarrier(swapInfo.swapimages[i], subres, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
        }

        n_assert(runtimeInfo.type == Texture2D);
        n_assert(loadInfo.mips == 1);
        n_assert(loadInfo.samples == 1);

        loadInfo.img = swapInfo.swapimages[0];
        loadInfo.mem.mem = VK_NULL_HANDLE;

        runtimeInfo.view = backbufferInfo.backbufferViews[0];
    }

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
SetupSparse(VkDevice dev, VkImage img, Ids::Id32 sparseExtension, const VkTextureLoadInfo& info)
{
    VkMemoryRequirements memoryReqs;
    vkGetImageMemoryRequirements(dev, img, &memoryReqs);

    VkPhysicalDeviceProperties devProps = GetCurrentProperties();
    n_assert(memoryReqs.size < devProps.limits.sparseAddressSpaceSize);

    // get sparse memory requirements
    uint32_t sparseMemoryRequirementsCount;
    vkGetImageSparseMemoryRequirements(dev, img, &sparseMemoryRequirementsCount, nullptr);
    n_assert(sparseMemoryRequirementsCount > 0);

    Util::FixedArray<VkSparseImageMemoryRequirements> sparseMemoryRequirements(sparseMemoryRequirementsCount);
    vkGetImageSparseMemoryRequirements(dev, img, &sparseMemoryRequirementsCount, sparseMemoryRequirements.Begin());

    uint32_t usedMemoryRequirements = UINT32_MAX;
    for (uint32_t i = 0; i < sparseMemoryRequirementsCount; i++)
    {
        if (sparseMemoryRequirements[i].formatProperties.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
        {
            usedMemoryRequirements = i;
            break;
        }
    }
    n_assert2(usedMemoryRequirements != UINT32_MAX, "No sparse image support for color textures");

    uint32_t memtype;
    VkResult res = GetMemoryType(memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memtype);
    n_assert(res == VK_SUCCESS);

    VkSparseImageMemoryRequirements sparseMemoryRequirement = sparseMemoryRequirements[usedMemoryRequirements];
    bool singleMipTail = sparseMemoryRequirement.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT;

    __Lock(textureSparseExtensionAllocator);
    TextureSparsePageTable& table = textureSparseExtensionAllocator.Get<TextureExtension_SparsePageTable>(sparseExtension);    
    Util::Array<VkSparseMemoryBind>& opaqueBinds = textureSparseExtensionAllocator.Get<TextureExtension_SparseOpaqueBinds>(sparseExtension);
    Util::Array<VkSparseImageMemoryBind>& pendingBinds = textureSparseExtensionAllocator.Get<TextureExtension_SparsePendingBinds>(sparseExtension);
    Util::Array<CoreGraphics::Alloc>& allocs = textureSparseExtensionAllocator.Get<TextureExtension_SparseOpaqueAllocs>(sparseExtension);
    VkSparseImageMemoryRequirements& reqs = textureSparseExtensionAllocator.Get<TextureExtension_SparseMemoryRequirements>(sparseExtension);
    table.memoryReqs = memoryReqs;
    reqs = sparseMemoryRequirement;

    // setup pages and bind counts
    table.pages.Resize(info.layers);
    table.bindCounts.Resize(info.layers);
    for (uint32_t i = 0; i < info.layers; i++)
    {
        table.pages[i].Resize(sparseMemoryRequirement.imageMipTailFirstLod);
        table.bindCounts[i].Resize(sparseMemoryRequirement.imageMipTailFirstLod);
    }

    // create sparse bindings, 
    for (uint32_t layer = 0; layer < info.layers; layer++)
    {
        for (SizeT mip = 0; mip < (SizeT)sparseMemoryRequirement.imageMipTailFirstLod; mip++)
        {
            VkExtent3D extent;
            extent.width = Math::max(info.dims.width >> mip, 1);
            extent.height = Math::max(info.dims.height >> mip, 1);
            extent.depth = Math::max(info.dims.depth >> mip, 1);

            VkImageSubresource subres;
            subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subres.mipLevel = mip;
            subres.arrayLayer = layer;

            table.bindCounts[layer][mip].Resize(3);

            VkExtent3D granularity = sparseMemoryRequirement.formatProperties.imageGranularity;
            table.bindCounts[layer][mip][0] = extent.width / granularity.width + ((extent.width % granularity.width) ? 1u : 0u);
            table.bindCounts[layer][mip][1] = extent.height / granularity.height + ((extent.height % granularity.height) ? 1u : 0u);
            table.bindCounts[layer][mip][2] = extent.depth / granularity.depth + ((extent.depth % granularity.depth) ? 1u : 0u);
            uint32_t lastBlockExtent[3];
            lastBlockExtent[0] = (extent.width % granularity.width) ? extent.width % granularity.width : granularity.width;
            lastBlockExtent[1] = (extent.height % granularity.height) ? extent.height % granularity.height : granularity.height;
            lastBlockExtent[2] = (extent.depth % granularity.depth) ? extent.depth % granularity.depth : granularity.depth;

            // setup memory pages
            for (uint32_t z = 0; z < table.bindCounts[layer][mip][2]; z++)
            {
                for (uint32_t y = 0; y < table.bindCounts[layer][mip][1]; y++)
                {
                    for (uint32_t x = 0; x < table.bindCounts[layer][mip][0]; x++)
                    {
                        VkOffset3D offset;
                        offset.x = x * granularity.width;
                        offset.y = y * granularity.height;
                        offset.z = z * granularity.depth;

                        VkExtent3D extent;
                        extent.width = (x == table.bindCounts[layer][mip][0] - 1) ? lastBlockExtent[0] : granularity.width;
                        extent.height = (y == table.bindCounts[layer][mip][1] - 1) ? lastBlockExtent[1] : granularity.height;
                        extent.depth = (z == table.bindCounts[layer][mip][2] - 1) ? lastBlockExtent[2] : granularity.depth;

                        VkSparseImageMemoryBind sparseBind;
                        sparseBind.extent = extent;
                        sparseBind.offset = offset;
                        sparseBind.subresource = subres;
                        sparseBind.memory = VK_NULL_HANDLE;
                        sparseBind.memoryOffset = 0;
                        sparseBind.flags = 0;

                        // create new virtual page
                        TextureSparsePage page;
                        page.offset = TextureSparsePageOffset{ (uint32_t)offset.x, (uint32_t)offset.y, (uint32_t)offset.z };
                        page.extent = TextureSparsePageSize{ extent.width, extent.height, extent.depth };
                        page.alloc = CoreGraphics::Alloc{ VK_NULL_HANDLE, 0, 0, CoreGraphics::MemoryPool_DeviceLocal };

                        pendingBinds.Append(sparseBind);
                        table.pages[layer][mip].Append(page);

                    }
                }
            }
        }

        // allocate memory if texture only has one mip tail per layer, this is due to the mip tail being smaller than the page granularity,
        // so we can just update all mips with a single copy/blit
        if ((!singleMipTail) && sparseMemoryRequirement.imageMipTailFirstLod < (uint32_t)info.mips)
        {
            CoreGraphics::Alloc alloc = Vulkan::AllocateMemory(dev, memoryReqs, sparseMemoryRequirement.imageMipTailSize);
            allocs.Append(alloc);

            VkSparseMemoryBind sparseBind;
            sparseBind.resourceOffset = sparseMemoryRequirement.imageMipTailOffset + layer * sparseMemoryRequirement.imageMipTailStride;
            sparseBind.size = sparseMemoryRequirement.imageMipTailSize;
            sparseBind.memory = alloc.mem;
            sparseBind.memoryOffset = alloc.offset;
            sparseBind.flags = 0;

            // add to opaque bindings
            opaqueBinds.Append(sparseBind);
        }
    }

    // allocate memory if texture only has one mip tail per layer, this is due to the mip tail being smaller than the page granularity,
    // so we can just update all mips with a single copy/blit
    if ((singleMipTail) && sparseMemoryRequirement.imageMipTailFirstLod < (uint32_t)info.mips)
    {
        CoreGraphics::Alloc alloc = Vulkan::AllocateMemory(dev, memoryReqs, sparseMemoryRequirement.imageMipTailSize);
        allocs.Append(alloc);

        VkSparseMemoryBind sparseBind;
        sparseBind.resourceOffset = sparseMemoryRequirement.imageMipTailOffset;
        sparseBind.size = sparseMemoryRequirement.imageMipTailSize;
        sparseBind.memory = alloc.mem;
        sparseBind.memoryOffset = alloc.offset;
        sparseBind.flags = 0;

        // add memory bind to update queue
        opaqueBinds.Append(sparseBind);
    }
}

} // namespace Vulkan
