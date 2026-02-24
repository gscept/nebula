//------------------------------------------------------------------------------
//  @file vkswapchain.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "coregraphics/swapchain.h"
#include "foundation/stdneb.h"
#include "coregraphics/vk/vkgraphicsdevice.h"
#include "coregraphics/vk/vkcommandbuffer.h"
#include "coregraphics/vk/vktexture.h"
#include "vkswapchain.h"
#include "vksemaphore.h"
#include "vkfence.h"

namespace Vulkan
{
SwapchainAllocator swapchainAllocator;

//------------------------------------------------------------------------------
/**
*/
VkDevice
SwapchainGetVkDevice(const CoreGraphics::SwapchainId id)
{
    return swapchainAllocator.Get<Swapchain_Device>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
VkSwapchainKHR
SwapchainGetVkSwapchain(const CoreGraphics::SwapchainId id)
{
    return swapchainAllocator.Get<Swapchain_Swapchain>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<VkImageView>&
SwapchainGetVkImageViews(const CoreGraphics::SwapchainId id)
{
    return swapchainAllocator.Get<Swapchain_ImageViews>(id.id);
}

}
namespace CoreGraphics
{
using namespace Vulkan;

//------------------------------------------------------------------------------
/**
*/
SwapchainId
CreateSwapchain(const SwapchainCreateInfo& info)
{
    Ids::Id32 id = swapchainAllocator.Alloc();
    swapchainAllocator.Set<Swapchain_Device>(id, Vulkan::GetCurrentDevice());
    VkSurfaceKHR& surface = swapchainAllocator.Get<Swapchain_Surface>(id);
    VkSwapchainKHR& swapchain = swapchainAllocator.Get<Swapchain_Swapchain>(id);
    uint& currentBackbuffer = swapchainAllocator.Get<Swapchain_CurrentBackbuffer>(id);
    VkQueue& queue = swapchainAllocator.Get<Swapchain_Queue>(id);
    Util::Array<VkImage>& images = swapchainAllocator.Get<Swapchain_Images>(id);
    Util::Array<VkImageView>& views = swapchainAllocator.Get<Swapchain_ImageViews>(id);
    Util::FixedArray<SemaphoreId>& displaySemaphores = swapchainAllocator.Get<Swapchain_DisplaySemaphores>(id);
    Util::FixedArray<SemaphoreId>& renderingSemaphores = swapchainAllocator.Get<Swapchain_RenderingSemaphores>(id);
    CoreGraphics::QueueType& queueType = swapchainAllocator.Get<Swapchain_QueueType>(id);
    swapchainAllocator.Set<Swapchain_DisplayMode>(id, info.displayMode);
    VkResult res = glfwCreateWindowSurface(Vulkan::GetInstance(), info.window, nullptr, &surface);
    n_assert(res == VK_SUCCESS);

    VkPhysicalDevice physicalDev = Vulkan::GetCurrentPhysicalDevice();
    VkDevice dev = Vulkan::GetCurrentDevice();

    // find available surface formats
    uint32_t numFormats;
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDev, surface, &numFormats, nullptr);
    n_assert(res == VK_SUCCESS);

    Util::FixedArray<VkSurfaceFormatKHR> formats(numFormats);
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDev, surface, &numFormats, formats.Begin());
    n_assert(res == VK_SUCCESS);
    VkFormat format = formats[0].format;
    VkColorSpaceKHR colorSpace = formats[0].colorSpace;
    VkComponentMapping mapping = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
    uint32_t i;
    for (i = 0; i < numFormats; i++)
    {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
        {
            format = formats[i].format;
            colorSpace = formats[i].colorSpace;
            //mapping.b = VK_COMPONENT_SWIZZLE_R;
            //mapping.r = VK_COMPONENT_SWIZZLE_B;
            break;
        }
    }

    // get surface capabilities
    VkSurfaceCapabilitiesKHR surfCaps;

    res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDev, surface, &surfCaps);
    n_assert(res == VK_SUCCESS);

    uint32_t numPresentModes;
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDev, surface, &numPresentModes, nullptr);
    n_assert(res == VK_SUCCESS);

    // get present modes
    Util::FixedArray<VkPresentModeKHR> presentModes(numPresentModes);
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDev, surface, &numPresentModes, presentModes.Begin());
    n_assert(res == VK_SUCCESS);

    VkExtent2D swapchainExtent;
    if (surfCaps.currentExtent.width == -1)
    {
        swapchainExtent.width = info.displayMode.GetWidth();
        swapchainExtent.height = info.displayMode.GetHeight();
    }
    else
    {
        swapchainExtent = surfCaps.currentExtent;
    }

    // figure out the best present mode, mailo
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (i = 0; i < numPresentModes; i++)
    {
        switch (presentModes[i])
        {
            case VK_PRESENT_MODE_MAILBOX_KHR:
                swapchainPresentMode = presentModes[i];
                numPresentModes = 0;
                break;
            case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
                if (info.vsync)
                {
                    swapchainPresentMode = presentModes[i];
                    numPresentModes = 0;
                }
                break;
            case VK_PRESENT_MODE_IMMEDIATE_KHR:
                if (!info.vsync)
                {
                    swapchainPresentMode = presentModes[i];
                    numPresentModes = 0;
                }
                break;
            case VK_PRESENT_MODE_FIFO_KHR:
                swapchainPresentMode = presentModes[i];
                break;
        }
    }

    // use at least as many swap images as we have buffered frames, if we don't have enough, the swap chain creation will fail
    uint32_t numSwapchainImages = Math::max(surfCaps.minImageCount, Math::min((uint32_t)CoreGraphics::GetNumBufferedFrames(), surfCaps.maxImageCount));

#ifdef CreateSemaphore
#pragma push_macro("CreateSemaphore")
#undef CreateSemaphore
#endif

    displaySemaphores.Resize(CoreGraphics::GetNumBufferedFrames());
    renderingSemaphores.Resize(CoreGraphics::GetNumBufferedFrames());
    for (uint i = 0; i < displaySemaphores.Size(); i++)
    {
        renderingSemaphores[i] = CreateSemaphore({
#if NEBULA_GRAPHICS_DEBUG
            .name = "WaitForRendering",
#endif
            .type = SemaphoreType::Binary });

        displaySemaphores[i] = CreateSemaphore({
#if NEBULA_GRAPHICS_DEBUG
            .name = "Present",
#endif
            .type = SemaphoreType::Binary });
    }

#pragma pop_macro("CreateSemaphore")

    // create a transform
    VkSurfaceTransformFlagBitsKHR transform;
    if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    else                                                                      transform = surfCaps.currentTransform;

    VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    n_assert((usageFlags & surfCaps.supportedUsageFlags) != 0);
    VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE;
    if (info.oldSwapchain != CoreGraphics::InvalidSwapchainId)
        oldSwapchain = CoreGraphics::SwapchainGetVkSwapchain(info.oldSwapchain);
    VkSwapchainCreateInfoKHR swapchainInfo =
    {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        nullptr,
        0,
        surface,
        numSwapchainImages,
        format,
        colorSpace,
        swapchainExtent,
        1,
        usageFlags,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        transform,
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        swapchainPresentMode,
        VK_TRUE,
        oldSwapchain
    };

    VkBool32 canPresent;
    res = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDev, CoreGraphics::GetQueueIndex(info.preferredQueue), surface, &canPresent);
    n_assert(res == VK_SUCCESS);
    if (canPresent)
    {
        queue = Vulkan::GetQueue(info.preferredQueue, 0);
        queueType = info.preferredQueue;
    }
    else
    {
        // get present queue
        for (IndexT i = 0; i < NumQueueTypes; i++)
        {
            VkBool32 canPresent;
            res = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDev, CoreGraphics::GetQueueIndex((CoreGraphics::QueueType)i), surface, &canPresent);
            n_assert(res == VK_SUCCESS);
            if (canPresent)
            {
                queue = Vulkan::GetQueue((CoreGraphics::QueueType)i, 0);
                queueType = (CoreGraphics::QueueType)i;
                break;
            }
        }
    }

    n_assert(queue != VK_NULL_HANDLE);

    // create swapchain
    res = vkCreateSwapchainKHR(dev, &swapchainInfo, nullptr, &swapchain);
    n_assert(res == VK_SUCCESS);

    uint numBuffers;
    res = vkGetSwapchainImagesKHR(dev, swapchain, &numBuffers, nullptr);
    n_assert(res == VK_SUCCESS);

    // get number of buffered frames from the graphics device, and limit the amount of backbuffers
    images.Resize(numBuffers);
    views.Resize(numBuffers);

    res = vkGetSwapchainImagesKHR(dev, swapchain, &numBuffers, images.Begin());
    n_assert(res == VK_SUCCESS);

    CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::LockGraphicsSetupCommandBuffer("Swap chain setup");
    for (i = 0; i < numBuffers; i++)
    {
        // Transition image to present source
        VkCommandBuffer vkBuf = CmdBufferGetVk(cmdBuf);
        VkImageMemoryBarrier imageBarrier;
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.pNext = nullptr;
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.image = images[i];
        imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        imageBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        vkCmdPipelineBarrier(vkBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0x0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

        // setup view
        VkImageViewCreateInfo backbufferViewInfo =
        {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            nullptr,
            0,
            images[i],
            VK_IMAGE_VIEW_TYPE_2D,
            format,
            mapping,
            { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
        };
        res = vkCreateImageView(dev, &backbufferViewInfo, nullptr, &views[i]);
        n_assert(res == VK_SUCCESS);

        VkDebugUtilsObjectNameInfoEXT info =
        {
            VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            nullptr,
            VK_OBJECT_TYPE_IMAGE,
            (uint64_t)images[i],
            "Swap Chain Image"
        };
        VkDevice dev = GetCurrentDevice();
        VkResult res = VkDebugObjectName(dev, &info);
        n_assert(res == VK_SUCCESS);

        info.objectHandle = (uint64_t)views[i];
        info.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        Util::String str = Util::String::Sprintf("%s - View", "Swap Chain Image");
        info.pObjectName = str.AsCharPtr();
        res = VkDebugObjectName(dev, &info);
        n_assert(res == VK_SUCCESS);
    }
    currentBackbuffer = 0;
    CoreGraphics::UnlockGraphicsSetupCommandBuffer(cmdBuf);

    CoreGraphics::CmdBufferPoolCreateInfo poolInfo;
    poolInfo.name = "Swap Commandbuffer Pool";
    poolInfo.shortlived = true;
    poolInfo.queue = queueType;
    poolInfo.resetable = false;
    CoreGraphics::CmdBufferPoolId pool = CoreGraphics::CreateCmdBufferPool(poolInfo);
    swapchainAllocator.Set<Swapchain_CommandPool>(id, pool);

    SwapchainId ret = id;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroySwapchain(const SwapchainId id)
{
    Util::FixedArray<SemaphoreId>& displaySemaphores = swapchainAllocator.Get<Swapchain_DisplaySemaphores>(id.id);
    for (SizeT i = 0; i < displaySemaphores.Size(); i++)
    {
        CoreGraphics::DelayedDeleteSemaphore(displaySemaphores[i]);
    }
    displaySemaphores.Clear();

    Util::FixedArray<SemaphoreId>& renderingSemaphores = swapchainAllocator.Get<Swapchain_RenderingSemaphores>(id.id);
    for (SizeT i = 0; i < renderingSemaphores.Size(); i++)
    {
        CoreGraphics::DelayedDeleteSemaphore(renderingSemaphores[i]);
    }
    renderingSemaphores.Clear();

    CoreGraphics::DelayedDeleteSwapchain(id);

    swapchainAllocator.Get<Swapchain_Images>(id.id).Clear();
    swapchainAllocator.Get<Swapchain_ImageViews>(id.id).Clear();

    // destroy swapchain last
    swapchainAllocator.Dealloc(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
SwapchainSwap(const SwapchainId id)
{
    N_SCOPE(Swap, CoreGraphics)
    VkDevice dev = swapchainAllocator.Get<Swapchain_Device>(id.id);
    const VkSwapchainKHR& swapchain = swapchainAllocator.Get<Swapchain_Swapchain>(id.id);
    uint& currentBackbuffer = swapchainAllocator.Get<Swapchain_CurrentBackbuffer>(id.id);
    Util::FixedArray<CoreGraphics::SemaphoreId>& semaphores = swapchainAllocator.Get<Swapchain_DisplaySemaphores>(id.id);

    // get present fence and be sure it is finished before getting the next image
    VkSemaphore sem = Vulkan::SemaphoreGetVk(semaphores[CoreGraphics::GetBufferedFrameIndex()]);

    // get the next image
    VkResult res = vkAcquireNextImageKHR(dev, swapchain, UINT64_MAX, sem, VK_NULL_HANDLE, &currentBackbuffer);
    switch (res)
    {
        case VK_SUCCESS:
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_SUBOPTIMAL_KHR:
            break;
        default:
            n_error("Present failed");
    }
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::QueueType
SwapchainGetQueueType(const SwapchainId id)
{
    return swapchainAllocator.Get<Swapchain_QueueType>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::CmdBufferId
SwapchainAllocateCmds(const SwapchainId id)
{
    const CoreGraphics::CmdBufferPoolId pool = swapchainAllocator.Get<Swapchain_CommandPool>(id.id);
    CoreGraphics::CmdBufferCreateInfo bufInfo;
    bufInfo.pool = pool;
    bufInfo.name = "Swap";
    bufInfo.queryTypes = CoreGraphics::CmdBufferQueryBits::NoQueries;
    return CoreGraphics::CreateCmdBuffer(bufInfo);
}

//------------------------------------------------------------------------------
/**
*/
void
SwapchainCopy(const SwapchainId id, const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::TextureId source)
{
    const uint currentBackbuffer = swapchainAllocator.Get<Swapchain_CurrentBackbuffer>(id.id);
    const Util::Array<VkImage>& images = swapchainAllocator.Get<Swapchain_Images>(id.id);
    const DisplayMode& displayMode = swapchainAllocator.Get<Swapchain_DisplayMode>(id.id);
    CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(source);
    dims.width = Math::min(dims.width, displayMode.GetWidth());
    dims.height = Math::min(dims.height, displayMode.GetHeight());
    VkImageBlit blit;
    blit.srcSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 };
    blit.dstSubresource = blit.srcSubresource;
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {(int)dims.width, (int)dims.height, 1};
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = blit.srcOffsets[1];

    //VkImageCopy copy;
    //copy.srcOffset = { 0, 0, 0 };
    //copy.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    //copy.dstOffset = { 0, 0, 0 };
    //copy.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    //copy.extent = { (uint)displayMode.GetWidth(), (uint)displayMode.GetHeight(), 1 };

    VkImage sourceImage = TextureGetVkImage(source);
    VkCommandBuffer vkBuf = CmdBufferGetVk(cmdBuf.id);

    VkImageMemoryBarrier imageBarrier;

    // Transition backbuffer
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrier.pNext = nullptr;
    imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.image = images[currentBackbuffer];
    imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdPipelineBarrier(vkBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0x0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

    vkCmdBlitImage(
        vkBuf
        , sourceImage
        , VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        , images[currentBackbuffer]
        , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        , 1
        , &blit
        , VK_FILTER_NEAREST
    );
    //vkCmdCopyImage(
    //    vkBuf
    //    , sourceImage
    //    , VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    //    , images[currentBackbuffer]
    //    , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    //    , 1
    //    , &copy
    //);

    // Transition backbuffer
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrier.pNext = nullptr;
    imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.image = images[currentBackbuffer];
    imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    imageBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdPipelineBarrier(vkBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0x0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
}

//------------------------------------------------------------------------------
/**
*/
void
SwapchainPresent(const SwapchainId id)
{
    const uint currentBackbuffer = swapchainAllocator.Get<Swapchain_CurrentBackbuffer>(id.id);
    const VkSwapchainKHR& swapchain = swapchainAllocator.Get<Swapchain_Swapchain>(id.id);
    const VkQueue& queue = swapchainAllocator.Get<Swapchain_Queue>(id.id);
    const CoreGraphics::QueueType queueType = swapchainAllocator.Get<Swapchain_QueueType>(id.id);
    const Util::Array<VkImage>& images = swapchainAllocator.Get<Swapchain_Images>(id.id);
    const Util::FixedArray<CoreGraphics::SemaphoreId>& renderingSemaphores = swapchainAllocator.Get<Swapchain_RenderingSemaphores>(id.id);

    CoreGraphics::SemaphoreId sem = renderingSemaphores[CoreGraphics::GetBufferedFrameIndex()];
    VkSemaphore semaphores[] =
    {
        SemaphoreGetVk(sem) // this will be the final semaphore of the graphics command buffer that finishes the frame
    };

#if NEBULA_GRAPHICS_DEBUG
    CoreGraphics::QueueBeginMarker(queueType, NEBULA_MARKER_BLACK, "Presentation");
#endif

    const VkPresentInfoKHR info =
    {
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        nullptr,
        1,
        semaphores,
        1,
        &swapchain,
        &currentBackbuffer,
        nullptr
    };

    // present
    VkResult res = vkQueuePresentKHR(queue, &info);
    switch (res)
    {
        case VK_SUCCESS:
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_SUBOPTIMAL_KHR:
            break;
        default:
            n_error("Present failed");
    }


#if NEBULA_GRAPHICS_DEBUG
    CoreGraphics::QueueEndMarker(queueType);
#endif
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::SemaphoreId
SwapchainGetCurrentDisplaySemaphore(const SwapchainId id)
{
    return swapchainAllocator.Get<Swapchain_DisplaySemaphores>(id.id)[CoreGraphics::GetBufferedFrameIndex()];
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::SemaphoreId
SwapchainGetCurrentPresentSemaphore(const SwapchainId id)
{
    return swapchainAllocator.Get<Swapchain_RenderingSemaphores>(id.id)[CoreGraphics::GetBufferedFrameIndex()];
}

} // namespace CoreGraphics
