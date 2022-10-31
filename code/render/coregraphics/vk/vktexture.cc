//------------------------------------------------------------------------------
// vktexture.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vktexture.h"
#include "vktextureview.h"
#include "vkloader.h"
#include "io/stream.h"
#include "vkgraphicsdevice.h"
#include "vktypes.h"
#include "graphics/bindlessregistry.h"
#include "vkcommandbuffer.h"
#include "coregraphics/glfw/glfwwindow.h"

N_DECLARE_COUNTER(N_SPARSE_PAGE_MEMORY_COUNTER, Sparse Texture Allocated Memory);

namespace Vulkan
{

VkTextureAllocator textureAllocator(0x00FFFFFF);
VkTextureStencilExtensionAllocator textureStencilExtensionAllocator(0x00FFFFFF);
VkTextureSwapExtensionAllocator textureSwapExtensionAllocator(0x00FFFFFF);
VkTextureSparseExtensionAllocator textureSparseExtensionAllocator(0x00FFFFFF);

//------------------------------------------------------------------------------
/**
*/
const VkImage
TextureGetVkImage(const CoreGraphics::TextureId id)
{
    return textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).img;
}

//------------------------------------------------------------------------------
/**
*/
const VkImageView
TextureGetVkImageView(const CoreGraphics::TextureId id)
{
    return textureAllocator.GetUnsafe<Texture_RuntimeInfo>(id.resourceId).view;
}

//------------------------------------------------------------------------------
/**
*/
const VkImageView 
TextureGetVkStencilImageView(const CoreGraphics::TextureId id)
{
    Ids::Id32 stencil = textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).stencilExtension;
    n_assert(stencil != Ids::InvalidId32);
    return TextureViewGetVk(textureStencilExtensionAllocator.GetUnsafe<TextureExtension_StencilInfo>(stencil));
}

//------------------------------------------------------------------------------
/**
*/
const VkDevice 
TextureGetVkDevice(const CoreGraphics::TextureId id)
{
    return textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).dev;
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

    __Lock(textureSparseExtensionAllocator, Util::ArrayAllocatorAccess::Write);
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
                        CoreGraphics::TextureSparsePage page;
                        page.offset = CoreGraphics::TextureSparsePageOffset{ (uint32_t)offset.x, (uint32_t)offset.y, (uint32_t)offset.z };
                        page.extent = CoreGraphics::TextureSparsePageSize{ extent.width, extent.height, extent.depth };
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

namespace CoreGraphics
{
using namespace Vulkan;

//------------------------------------------------------------------------------
/**
*/
void
SetupTexture(const TextureId id)
{
    __Lock(textureAllocator, Util::ArrayAllocatorAccess::Write);
    Vulkan::VkTextureRuntimeInfo& runtimeInfo = textureAllocator.Get<Vulkan::Texture_RuntimeInfo>(id.resourceId);
    Vulkan::VkTextureLoadInfo& loadInfo = textureAllocator.Get<Vulkan::Texture_LoadInfo>(id.resourceId);
    Vulkan::VkTextureWindowInfo& windowInfo = textureAllocator.Get<Vulkan::Texture_WindowInfo>(id.resourceId);

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
    if (loadInfo.usage & TextureUsage::SampleTexture)
        usage |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (loadInfo.usage & TextureUsage::RenderTexture)
        usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | (isDepthFormat ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (loadInfo.usage & TextureUsage::ReadWriteTexture)
        usage |= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (loadInfo.usage & TextureUsage::TransferTextureSource)
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (loadInfo.usage & TextureUsage::TransferTextureDestination)
        usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (!loadInfo.windowTexture)
    {
        VkSampleCountFlagBits samples = VkTypes::AsVkSampleFlags(loadInfo.samples);
        VkImageViewType viewType = VkTypes::AsVkImageViewType(runtimeInfo.type);
        VkImageType type = VkTypes::AsVkImageType(runtimeInfo.type);

        // if read-write, we will almost definitely use this texture on multiple queues
        VkSharingMode sharingMode = (loadInfo.usage & TextureUsage::ReadWriteTexture) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        const Util::Set<uint32_t>& queues = CoreGraphics::GetQueueIndices();

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

        // Setup subresource range
        VkImageSubresourceRange viewRange;
        viewRange.aspectMask = isDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT; // view only supports reading depth in shader
        viewRange.baseMipLevel = loadInfo.minMip;
        viewRange.levelCount = loadInfo.mips - loadInfo.minMip;
        viewRange.baseArrayLayer = 0;
        viewRange.layerCount = loadInfo.layers;


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
                CoreGraphics::Alloc mem = textureAllocator.Get<Texture_LoadInfo>(loadInfo.alias.resourceId).mem;
                loadInfo.mem = mem;
                VkResult res = vkBindImageMemory(loadInfo.dev, loadInfo.img, loadInfo.mem.mem, loadInfo.mem.offset);
                n_assert(res == VK_SUCCESS);
            }

            // if we have initial data to setup, perform a data transfer
            if (loadInfo.buffer != nullptr)
            {
                // use resource submission
                CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::LockGraphicsSetupCommandBuffer();

                // Initial state will be transfer
                CoreGraphics::CmdBarrier(cmdBuf,
                    CoreGraphics::PipelineStage::ImageInitial,
                    CoreGraphics::PipelineStage::TransferWrite,
                    CoreGraphics::BarrierDomain::Global,
                    {
                        TextureBarrierInfo
                        {
                            id,
                            CoreGraphics::TextureSubresourceInfo(CoreGraphics::ImageBits::ColorBits, viewRange.baseMipLevel, viewRange.levelCount, 0, viewRange.layerCount)
                        }
                    },
                    nullptr);

                uint32_t size = PixelFormat::ToSize(loadInfo.format);
                CoreGraphics::TextureUpdate(cmdBuf, TransferQueueType, id, extents.width, extents.height, 0, 0, loadInfo.dims.width * loadInfo.dims.height * loadInfo.dims.depth * size, loadInfo.buffer);

                CoreGraphics::CmdBarrier(cmdBuf,
                    CoreGraphics::PipelineStage::TransferWrite,
                    CoreGraphics::PipelineStage::GraphicsShadersRead,
                    CoreGraphics::BarrierDomain::Global,
                    {
                        TextureBarrierInfo
                        {
                            id,
                            CoreGraphics::TextureSubresourceInfo(CoreGraphics::ImageBits::ColorBits, viewRange.baseMipLevel, viewRange.levelCount, 0, viewRange.layerCount)
                        }
                    },
                    nullptr);
                CoreGraphics::UnlockGraphicsSetupCommandBuffer();

                // this should always be set to nullptr after it has been transfered. TextureLoadInfo should never own the pointer!
                loadInfo.buffer = nullptr;
            }
        }

        // if used for render, find appropriate renderable format
        if (loadInfo.usage & TextureUsage::RenderTexture)
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

        VkComponentMapping mapping;
        mapping.r = VkSwizzle[(uint)loadInfo.swizzle.red];
        mapping.g = VkSwizzle[(uint)loadInfo.swizzle.green];
        mapping.b = VkSwizzle[(uint)loadInfo.swizzle.blue];
        mapping.a = VkSwizzle[(uint)loadInfo.swizzle.alpha];

        VkImageViewCreateInfo viewCreate =
        {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            nullptr,
            0,
            loadInfo.img,
            viewType,
            vkformat,
            mapping,
            viewRange
        };
        stat = vkCreateImageView(loadInfo.dev, &viewCreate, nullptr, &runtimeInfo.view);
        n_assert(stat == VK_SUCCESS);

        // setup stencil image
        if (isDepthFormat)
        {
            // setup stencil extension
            TextureViewCreateInfo viewCreate;
            viewCreate.format = loadInfo.format;
            viewCreate.numLayers = loadInfo.layers;
            viewCreate.numMips = viewRange.levelCount;
            viewCreate.startLayer = 0;
            viewCreate.startMip = viewRange.baseMipLevel;
            viewCreate.tex = id;
            viewCreate.bits = ImageBits::StencilBits;
            TextureViewId stencilView = CreateTextureView(viewCreate);

            loadInfo.stencilExtension = textureStencilExtensionAllocator.Alloc();
            textureStencilExtensionAllocator.Set<TextureExtension_StencilInfo>(loadInfo.stencilExtension, stencilView);
            textureStencilExtensionAllocator.Set<TextureExtension_StencilBind>(loadInfo.stencilExtension, 0xFFFFFFFF);
        }

        // use setup submission
        CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::LockGraphicsSetupCommandBuffer();

        CoreGraphics::TextureSubresourceInfo subres(
            isDepthFormat ? CoreGraphics::ImageBits::DepthBits | CoreGraphics::ImageBits::StencilBits : CoreGraphics::ImageBits::ColorBits
            , viewRange.baseMipLevel
            , viewRange.levelCount
            , viewRange.baseArrayLayer
            , viewRange.layerCount);

        if (loadInfo.clear || loadInfo.usage & TextureUsage::RenderTexture)
        {
            // The first barrier is to transition from the initial layout to transfer for clear
            CoreGraphics::CmdBarrier(cmdBuf,
                CoreGraphics::PipelineStage::ImageInitial,
                CoreGraphics::PipelineStage::TransferWrite,
                CoreGraphics::BarrierDomain::Global,
                {
                    TextureBarrierInfo
                    {
                        id,
                        subres
                    }
                });

            if (!isDepthFormat)
            {
                // Clear color
                TextureClearColor(cmdBuf
                                  , id
                                  , { loadInfo.clearColorF4.x, loadInfo.clearColorF4.y, loadInfo.clearColorF4.z, loadInfo.clearColorF4.w }
                                  , CoreGraphics::ImageLayout::TransferDestination, subres
                );

                // Transition back to color read if color render target
                CoreGraphics::CmdBarrier(cmdBuf,
                    CoreGraphics::PipelineStage::TransferWrite,
                    CoreGraphics::PipelineStage::AllShadersRead,
                    CoreGraphics::BarrierDomain::Global,
                    {
                        TextureBarrierInfo
                        {
                            id,
                            subres
                        }
                    });
            }
            else
            {
                // Clear depth and stencil
                TextureClearDepthStencil(cmdBuf, id, loadInfo.clearDepthStencil.depth, loadInfo.clearDepthStencil.stencil, CoreGraphics::ImageLayout::TransferDestination, subres);

                // Transition to depth read
                CoreGraphics::CmdBarrier(cmdBuf,
                    CoreGraphics::PipelineStage::TransferWrite,
                    CoreGraphics::PipelineStage::DepthStencilRead,
                    CoreGraphics::BarrierDomain::Global,
                    {
                        TextureBarrierInfo
                        {
                            id,
                            subres
                        }
                    });
            }
        }
        else if (loadInfo.usage & TextureUsage::ReadWriteTexture)
        {
            // Transition read-write texture 
            CoreGraphics::CmdBarrier(cmdBuf,
                CoreGraphics::PipelineStage::ImageInitial,
                CoreGraphics::PipelineStage::AllShadersRead,
                CoreGraphics::BarrierDomain::Global,
                {
                    TextureBarrierInfo
                    {
                        id,
                        subres
                    }
                });
        }

        CoreGraphics::UnlockGraphicsSetupCommandBuffer();

        // register image with shader server
        if (loadInfo.bindless)
        {
            if (runtimeInfo.bind == 0xFFFFFFFF)
                runtimeInfo.bind = Graphics::RegisterTexture(id, runtimeInfo.type, isDepthFormat);
            else
                Graphics::ReregisterTexture(id, runtimeInfo.type, runtimeInfo.bind, isDepthFormat);

            // if this is a depth-stencil texture, also register the stencil
            if (isDepthFormat)
            {
                __Lock(textureStencilExtensionAllocator, Util::ArrayAllocatorAccess::Write);
                IndexT& bind = textureStencilExtensionAllocator.Get<TextureExtension_StencilBind>(loadInfo.stencilExtension);
                bind = Graphics::RegisterTexture(id, runtimeInfo.type, false, true);
            }
        }
        else
            runtimeInfo.bind = 0xFFFFFFFF;

    }
    else // setup as window texture
    {
        // get submission context
        n_assert(windowInfo.window != CoreGraphics::InvalidWindowId);
        const CmdBufferId cmdBuf = CoreGraphics::LockGraphicsSetupCommandBuffer();
        const VkCommandBuffer vkCmdBuf = CmdBufferGetVk(cmdBuf);

        // setup swap extension
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

        // We need to transition all the backbuffers to transfer, clear them, and transition them to renderable.
        // But because we're accessing the swapimages, we can't use the CoreGraphics barriers and clear...
        Util::FixedArray<VkImageMemoryBarrier> swapbufferBarriers(swapInfo.swapimages.Size());
        for (IndexT i = 0; i < swapbufferBarriers.Size(); i++)
        {
            swapbufferBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            swapbufferBarriers[i].pNext = nullptr;
            swapbufferBarriers[i].srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            swapbufferBarriers[i].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            swapbufferBarriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            swapbufferBarriers[i].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            swapbufferBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            swapbufferBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            swapbufferBarriers[i].image = swapInfo.swapimages[i];
            swapbufferBarriers[i].subresourceRange = subres;
        }
        vkCmdPipelineBarrier(vkCmdBuf,
            VkTypes::AsVkPipelineStage(CoreGraphics::PipelineStage::HostWrite), VkTypes::AsVkPipelineStage(CoreGraphics::PipelineStage::TransferWrite),
            0,
            0, nullptr,
            0, nullptr,
            swapbufferBarriers.Size(), swapbufferBarriers.Begin());

        // clear textures
        IndexT i;
        for (i = 0; i < swapInfo.swapimages.Size(); i++)
        {
            vkCmdClearColorImage(vkCmdBuf, swapInfo.swapimages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &subres);

            swapbufferBarriers[i].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            swapbufferBarriers[i].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            swapbufferBarriers[i].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            swapbufferBarriers[i].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }

        vkCmdPipelineBarrier(vkCmdBuf,
            VkTypes::AsVkPipelineStage(CoreGraphics::PipelineStage::TransferWrite), VkTypes::AsVkPipelineStage(CoreGraphics::PipelineStage::Present),
            0,
            0, nullptr,
            0, nullptr,
            swapbufferBarriers.Size(), swapbufferBarriers.Begin());
        CoreGraphics::UnlockGraphicsSetupCommandBuffer();

        n_assert(runtimeInfo.type == Texture2D);
        n_assert(loadInfo.mips == 1);
        n_assert(loadInfo.samples == 1);

        loadInfo.img = swapInfo.swapimages[0];
        loadInfo.mem.mem = VK_NULL_HANDLE;

        runtimeInfo.view = backbufferInfo.backbufferViews[0];
    }
}

//------------------------------------------------------------------------------
/**
*/
const TextureId
CreateTexture(const TextureCreateInfo& info)
{
     /// during the load-phase, we can safetly get the structs
    uint32_t id = textureAllocator.Alloc();

    TextureId ret;
    ret.resourceId = id;
    ret.resourceType = CoreGraphics::TextureIdType;

    Vulkan::VkTextureRuntimeInfo& runtimeInfo = textureAllocator.Get<Vulkan::Texture_RuntimeInfo>(id);
    Vulkan::VkTextureLoadInfo& loadInfo = textureAllocator.Get<Vulkan::Texture_LoadInfo>(id);
    Vulkan::VkTextureWindowInfo& windowInfo = textureAllocator.Get<Vulkan::Texture_WindowInfo>(id);

    // create adjusted info
    TextureCreateInfoAdjusted adjustedInfo = TextureGetAdjustedInfo(info);

    VkDevice dev = Vulkan::GetCurrentDevice();

    loadInfo.dev = dev;
    loadInfo.name = adjustedInfo.name;
    loadInfo.dims.width = adjustedInfo.width;
    loadInfo.dims.height = adjustedInfo.height;
    loadInfo.dims.depth = adjustedInfo.depth;
    loadInfo.relativeDims.width = adjustedInfo.widthScale;
    loadInfo.relativeDims.height = adjustedInfo.heightScale;
    loadInfo.relativeDims.depth = adjustedInfo.depthScale;
    loadInfo.mips = adjustedInfo.mips;
    loadInfo.minMip = adjustedInfo.minMip;
    loadInfo.layers = adjustedInfo.layers;
    loadInfo.format = adjustedInfo.format;
    loadInfo.usage = adjustedInfo.usage;
    loadInfo.alias = adjustedInfo.alias;
    loadInfo.samples = adjustedInfo.samples;
    loadInfo.clear = adjustedInfo.clear;
    loadInfo.clearColorF4 = adjustedInfo.clearColorF4;
    loadInfo.defaultLayout = adjustedInfo.defaultLayout;
    loadInfo.windowTexture = adjustedInfo.windowTexture;
    loadInfo.windowRelative = adjustedInfo.windowRelative;
    loadInfo.bindless = adjustedInfo.bindless;
    loadInfo.sparse = adjustedInfo.sparse;
    loadInfo.swizzle = adjustedInfo.swizzle;
    runtimeInfo.bind = 0xFFFFFFFF;

    // borrow buffer pointer
    loadInfo.buffer = adjustedInfo.buffer;
    windowInfo.window = adjustedInfo.window;

    if (loadInfo.windowTexture)
    {
        runtimeInfo.type = Texture2D;
    }
    else
    {
        runtimeInfo.type = adjustedInfo.type;
    }

    loadInfo.stencilExtension = Ids::InvalidId32;
    loadInfo.swapExtension = Ids::InvalidId32;

    SetupTexture(ret);

#if NEBULA_GRAPHICS_DEBUG
    ObjectSetName(ret, loadInfo.name.Value());
#endif

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DeleteTexture(const TextureId id)
{
    __Lock(textureAllocator, Util::ArrayAllocatorAccess::Write);
    VkTextureLoadInfo& loadInfo = textureAllocator.Get<Texture_LoadInfo>(id.resourceId);
    VkTextureRuntimeInfo& runtimeInfo = textureAllocator.Get<Texture_RuntimeInfo>(id.resourceId);
    VkTextureWindowInfo& windowInfo = textureAllocator.Get<Texture_WindowInfo>(id.resourceId);

    if (loadInfo.stencilExtension != Ids::InvalidId32)
    {
        textureStencilExtensionAllocator.Lock(Util::ArrayAllocatorAccess::Read);
        TextureViewId stencil = textureStencilExtensionAllocator.Get<TextureExtension_StencilInfo>(loadInfo.stencilExtension);
        IndexT bind = textureStencilExtensionAllocator.Get<TextureExtension_StencilBind>(loadInfo.stencilExtension);
        CoreGraphics::DelayedDeleteTextureView(stencil);
        if (runtimeInfo.type != 0xFFFFFFFF)
            Graphics::UnregisterTexture(bind, runtimeInfo.type);
        textureStencilExtensionAllocator.Unlock(Util::ArrayAllocatorAccess::Read);
        textureStencilExtensionAllocator.Dealloc(loadInfo.stencilExtension);
    }

    if (loadInfo.swapExtension != Ids::InvalidId32)
    {
        textureSwapExtensionAllocator.Dealloc(loadInfo.swapExtension);
    }

    // if sparse, run through and dealloc pages
    if (loadInfo.sparse)
    {
        textureSparseExtensionAllocator.Lock(Util::ArrayAllocatorAccess::Write);
        // dealloc all opaque bindings
        Util::Array<CoreGraphics::Alloc>& allocs = textureSparseExtensionAllocator.Get<TextureExtension_SparseOpaqueAllocs>(loadInfo.sparseExtension);
        for (IndexT i = 0; i < allocs.Size(); i++)
            CoreGraphics::DelayedFreeMemory(allocs[i]);
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
                        CoreGraphics::DelayedFreeMemory(pages[pageIdx].alloc);
                        pages[pageIdx].alloc.mem = VK_NULL_HANDLE;
                        pages[pageIdx].alloc.offset = 0;
                    }
                }
            }
        }
        table.pages.Clear();
        table.bindCounts.Clear();

        textureSparseExtensionAllocator.Unlock(Util::ArrayAllocatorAccess::Write);
        textureSparseExtensionAllocator.Dealloc(loadInfo.sparseExtension);
    }
    else if (loadInfo.alias == CoreGraphics::InvalidTextureId && loadInfo.mem.mem != VK_NULL_HANDLE)
    {
        CoreGraphics::DelayedFreeMemory(loadInfo.mem);
        loadInfo.mem = CoreGraphics::Alloc{};
    }

    // only unload a texture which isn't a window texture, since their textures come from the swap chain
    if (!loadInfo.windowTexture)
    {
        if (runtimeInfo.bind != 0xFFFFFFFF)
            Graphics::UnregisterTexture(runtimeInfo.bind, runtimeInfo.type);
        CoreGraphics::DelayedDeleteTexture(id);
        runtimeInfo.view = VK_NULL_HANDLE;
        loadInfo.img = VK_NULL_HANDLE;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyTexture(const TextureId id)
{
    DeleteTexture(id);
    textureAllocator.Dealloc(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::TextureDimensions
TextureGetDimensions(const CoreGraphics::TextureId id)
{
    return textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).dims;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::TextureRelativeDimensions
TextureGetRelativeDimensions(const CoreGraphics::TextureId id)
{
    return textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).relativeDims;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::PixelFormat::Code
TextureGetPixelFormat(const CoreGraphics::TextureId id)
{
    return textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).format;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::TextureType
TextureGetType(const CoreGraphics::TextureId id)
{
    return textureAllocator.GetUnsafe<Texture_RuntimeInfo>(id.resourceId).type;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
TextureGetNumMips(const CoreGraphics::TextureId id)
{
    return textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).mips;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
TextureGetNumLayers(const CoreGraphics::TextureId id)
{
    return textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).layers;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
TextureGetNumSamples(const CoreGraphics::TextureId id)
{
    return textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).samples;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::TextureId
TextureGetAlias(const CoreGraphics::TextureId id)
{
    return textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).alias;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::TextureUsage
TextureGetUsage(const TextureId id)
{
    return textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).usage;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ImageLayout
TextureGetDefaultLayout(const CoreGraphics::TextureId id)
{
    return textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).defaultLayout;
}

//------------------------------------------------------------------------------
/**
*/
uint
TextureGetBindlessHandle(const CoreGraphics::TextureId id)
{
    return textureAllocator.GetUnsafe<Texture_RuntimeInfo>(id.resourceId).bind;
}

//------------------------------------------------------------------------------
/**
*/
uint
TextureGetStencilBindlessHandle(const CoreGraphics::TextureId id)
{
    Ids::Id32 stencil = textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).stencilExtension;
    n_assert(stencil != Ids::InvalidId32);
    return textureStencilExtensionAllocator.GetUnsafe<TextureExtension_StencilBind>(stencil);
}

//------------------------------------------------------------------------------
/**
*/
IndexT
TextureSwapBuffers(const CoreGraphics::TextureId id)
{
    __Lock(textureAllocator, Util::ArrayAllocatorAccess::Write);
    __Lock(textureSwapExtensionAllocator, Util::ArrayAllocatorAccess::Read);
    VkTextureLoadInfo& loadInfo = textureAllocator.Get<Texture_LoadInfo>(id.resourceId);
    VkTextureRuntimeInfo& runtimeInfo = textureAllocator.Get<Texture_RuntimeInfo>(id.resourceId);
    VkTextureWindowInfo& wnd = textureAllocator.Get<Texture_WindowInfo>(id.resourceId);
    VkTextureSwapInfo& swap = textureSwapExtensionAllocator.Get<TextureExtension_SwapInfo>(loadInfo.swapExtension);
    n_assert(wnd.window != CoreGraphics::InvalidWindowId);
    VkWindowSwapInfo& swapInfo = CoreGraphics::glfwWindowAllocator.Get<GLFW_WindowSwapInfo>(wnd.window.id24);

    // get present fence and be sure it is finished before getting the next image
    VkDevice dev = Vulkan::GetCurrentDevice();
    VkFence fence = Vulkan::GetPresentFence();
    VkResult res = vkWaitForFences(dev, 1, &fence, true, UINT64_MAX);
    n_assert(res == VK_SUCCESS);
    res = vkResetFences(dev, 1, &fence);
    n_assert(res == VK_SUCCESS);

    // get the next image
    res = vkAcquireNextImageKHR(dev, swapInfo.swapchain, UINT64_MAX, VK_NULL_HANDLE, fence, &swapInfo.currentBackbuffer);
    switch (res)
    {
        case VK_SUCCESS:
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_SUBOPTIMAL_KHR:
            break;
        default:
            n_error("Present failed");
    }

    // set image and update texture
    loadInfo.img = swap.swapimages[swapInfo.currentBackbuffer];
    runtimeInfo.view = swap.swapviews[swapInfo.currentBackbuffer];
    return swapInfo.currentBackbuffer;
}

//------------------------------------------------------------------------------
/**
*/
void
TextureWindowResized(const TextureId id)
{
    __Lock(textureAllocator, Util::ArrayAllocatorAccess::Write);
    VkTextureLoadInfo& loadInfo = textureAllocator.Get<Texture_LoadInfo>(id.resourceId);
    VkTextureRuntimeInfo& runtimeInfo = textureAllocator.Get<Texture_RuntimeInfo>(id.resourceId);
    VkTextureWindowInfo& windowInfo = textureAllocator.Get<Texture_WindowInfo>(id.resourceId);

    if (loadInfo.windowTexture || loadInfo.windowRelative)
    {
        uint tmp = runtimeInfo.bind;
        runtimeInfo.bind = 0xFFFFFFFF;
        DeleteTexture(id);

        // if the window has been resized, we need to update our dimensions based on relative size
        const CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(windowInfo.window);
        loadInfo.dims.width = SizeT(mode.GetWidth() * loadInfo.relativeDims.width);
        loadInfo.dims.height = SizeT(mode.GetHeight() * loadInfo.relativeDims.height);
        loadInfo.dims.depth = 1;

        runtimeInfo.bind = tmp;
        SetupTexture(id);
    }
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::TextureSparsePageSize
TextureSparseGetPageSize(const CoreGraphics::TextureId id)
{
    Ids::Id32 sparseExtension = textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).sparseExtension;
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
TextureSparseGetPageIndex(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT x, IndexT y, IndexT z)
{
    __Lock(textureAllocator, Util::ArrayAllocatorAccess::Read);
    Ids::Id32 sparseExtension = textureAllocator.Get<Texture_LoadInfo>(id.resourceId).sparseExtension;
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
TextureSparseGetPage(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT pageIndex)
{
    Ids::Id32 sparseExtension = textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).sparseExtension;
    n_assert(sparseExtension != Ids::InvalidId32);

    const TextureSparsePageTable& table = textureSparseExtensionAllocator.GetUnsafe<TextureExtension_SparsePageTable>(sparseExtension);
    return table.pages[layer][mip][pageIndex];
}

//------------------------------------------------------------------------------
/**
*/
SizeT
TextureSparseGetNumPages(const CoreGraphics::TextureId id, IndexT layer, IndexT mip)
{
    Ids::Id32 sparseExtension = textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).sparseExtension;
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
TextureSparseGetMaxMip(const CoreGraphics::TextureId id)
{
    Ids::Id32 sparseExtension = textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).sparseExtension;
    n_assert(sparseExtension != Ids::InvalidId32);
    const VkSparseImageMemoryRequirements& reqs = textureSparseExtensionAllocator.GetUnsafe<TextureExtension_SparseMemoryRequirements>(sparseExtension);
    return reqs.imageMipTailFirstLod;
}

//------------------------------------------------------------------------------
/**
*/
void
TextureSparseEvict(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT pageIndex)
{
    __Lock(textureAllocator, Util::ArrayAllocatorAccess::Write);
    Ids::Id32 sparseExtension = textureAllocator.Get<Texture_LoadInfo>(id.resourceId).sparseExtension;
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
TextureSparseMakeResident(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT pageIndex)
{
    __Lock(textureAllocator, Util::ArrayAllocatorAccess::Write);
    Ids::Id32 sparseExtension = textureAllocator.Get<Texture_LoadInfo>(id.resourceId).sparseExtension;
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
TextureSparseEvictMip(const CoreGraphics::TextureId id, IndexT layer, IndexT mip)
{
    __Lock(textureAllocator, Util::ArrayAllocatorAccess::Write);
    Ids::Id32 sparseExtension = textureAllocator.Get<Texture_LoadInfo>(id.resourceId).sparseExtension;
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
TextureSparseMakeMipResident(const CoreGraphics::TextureId id, IndexT layer, IndexT mip)
{
    __Lock(textureAllocator, Util::ArrayAllocatorAccess::Write);
    Ids::Id32 sparseExtension = textureAllocator.Get<Texture_LoadInfo>(id.resourceId).sparseExtension;
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
TextureSparseCommitChanges(const CoreGraphics::TextureId id)
{
    __Lock(textureAllocator, Util::ArrayAllocatorAccess::Write);
    Ids::Id32 sparseExtension = textureAllocator.Get<Texture_LoadInfo>(id.resourceId).sparseExtension;
    VkImage img = textureAllocator.Get<Texture_LoadInfo>(id.resourceId).img;
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
TextureSparseUpdate(const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::TextureId id, const Math::rectangle<uint>& region, IndexT mip, IndexT layer, const CoreGraphics::TextureId source)
{
    __Lock(textureAllocator, Util::ArrayAllocatorAccess::Read);
    TextureDimensions dims = textureAllocator.Get<Texture_LoadInfo>(source.resourceId).dims;

    VkImageBlit blit;
    blit.srcOffsets[0] = { 0, 0, 0 };
    blit.srcOffsets[1] = { dims.width, dims.height, dims.depth };
    blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    blit.dstOffsets[0] = { (int32_t)region.left, (int32_t)region.top, 0 };
    blit.dstOffsets[1] = { (int32_t)region.right, (int32_t)region.bottom, 1 };
    blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mip, (uint32_t)layer, 1 };

    // perform blit
    vkCmdBlitImage(CmdBufferGetVk(cmdBuf),
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
TextureSparseUpdate(const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::TextureId id, const Math::rectangle<uint>& region, IndexT mip, IndexT layer, char* buf)
{
    __Lock(textureAllocator, Util::ArrayAllocatorAccess::Read);
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
    const uint32_t qfamily = CoreGraphics::GetQueueIndex(CoreGraphics::TransferQueueType);
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
        CmdBufferGetVk(cmdBuf),
        vkbuf,
        TextureGetVkImage(id),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copy);

    CoreGraphics::DelayedFreeMemory(alloc);
    Vulkan::DelayedDeleteVkBuffer(dev, vkbuf);
}

//------------------------------------------------------------------------------
/**
*/
void
TextureSparseUpdate(const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::TextureId id, IndexT mip, IndexT layer, char* buf)
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
    const uint32_t qfamily = CoreGraphics::GetQueueIndex(CoreGraphics::TransferQueueType);
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
        CmdBufferGetVk(cmdBuf),
        vkbuf,
        TextureGetVkImage(id),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copy);

    CoreGraphics::DelayedFreeMemory(alloc);
    Vulkan::DelayedDeleteVkBuffer(dev, vkbuf);
}

//------------------------------------------------------------------------------
/**
*/
void
TextureClearColor(const CoreGraphics::CmdBufferId cmd, const CoreGraphics::TextureId id, Math::vec4 color, const CoreGraphics::ImageLayout layout, const CoreGraphics::TextureSubresourceInfo& subres)
{
    VkClearColorValue clear;
    VkImageSubresourceRange vksubres;
    vksubres.aspectMask = VkTypes::AsVkImageAspectFlags(subres.bits);
    vksubres.baseArrayLayer = subres.layer;
    vksubres.layerCount = subres.layerCount;
    vksubres.baseMipLevel = subres.mip;
    vksubres.levelCount = subres.mipCount;

    color.storeu(clear.float32);
    vkCmdClearColorImage(
        CmdBufferGetVk(cmd),
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
TextureClearDepthStencil(const CoreGraphics::CmdBufferId cmd, const CoreGraphics::TextureId id, float depth, uint stencil, const CoreGraphics::ImageLayout layout, const CoreGraphics::TextureSubresourceInfo& subres)
{
    VkClearDepthStencilValue clear;
    VkImageSubresourceRange vksubres;
    vksubres.aspectMask = VkTypes::AsVkImageAspectFlags(subres.bits);
    vksubres.baseArrayLayer = subres.layer;
    vksubres.layerCount = subres.layerCount;
    vksubres.baseMipLevel = subres.mip;
    vksubres.levelCount = subres.mipCount;

    clear.depth = depth;
    clear.stencil = stencil;
    vkCmdClearDepthStencilImage(
        CmdBufferGetVk(cmd),
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
TextureSetHighestLod(const CoreGraphics::TextureId id, uint lod)
{
    __LockName(&textureAllocator, lock, Util::ArrayAllocatorAccess::Write);
    const VkTextureLoadInfo& loadInfo = textureAllocator.Get<Texture_LoadInfo>(id.resourceId);
    VkTextureRuntimeInfo& runtimeInfo = textureAllocator.Get<Texture_RuntimeInfo>(id.resourceId);

    // create image view
    VkImageSubresourceRange viewSubres;
    viewSubres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewSubres.baseArrayLayer = 0;
    viewSubres.baseMipLevel = lod;
    viewSubres.layerCount = loadInfo.layers;
    viewSubres.levelCount = loadInfo.mips - viewSubres.baseMipLevel;

    VkComponentMapping mapping;
    mapping.r = VkSwizzle[(uint)loadInfo.swizzle.red];
    mapping.g = VkSwizzle[(uint)loadInfo.swizzle.green];
    mapping.b = VkSwizzle[(uint)loadInfo.swizzle.blue];
    mapping.a = VkSwizzle[(uint)loadInfo.swizzle.alpha];

    VkImageViewCreateInfo viewCreate =
    {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        loadInfo.img,
        VkTypes::AsVkImageViewType(runtimeInfo.type),
        VkTypes::AsVkFormat(loadInfo.format),
        mapping,
        viewSubres
    };

    VkShaderServer::Instance()->AddPendingImageView(TextureId(id), viewCreate, runtimeInfo.bind);
}

} // namespace CoreGraphics
