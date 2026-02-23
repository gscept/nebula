#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan implementation of Swapchain

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/swapchain.h"
#include "ids/idallocator.h"
namespace Vulkan
{

enum
{
    Swapchain_Device,
    Swapchain_Surface,
    Swapchain_Swapchain,
    Swapchain_CurrentBackbuffer,
    Swapchain_DisplaySemaphores,
    Swapchain_RenderingSemaphores,
    Swapchain_DisplayMode,
    Swapchain_Images,
    Swapchain_ImageViews,
    Swapchain_Queue,
    Swapchain_QueueType,
    Swapchain_CommandPool,
};

typedef Ids::IdAllocator<
    VkDevice,
    VkSurfaceKHR,
    VkSwapchainKHR,
    uint,
    Util::FixedArray<CoreGraphics::SemaphoreId>,
    Util::FixedArray<CoreGraphics::SemaphoreId>,
    CoreGraphics::DisplayMode,
    Util::Array<VkImage>,
    Util::Array<VkImageView>,
    VkQueue,
    CoreGraphics::QueueType,
    CoreGraphics::CmdBufferPoolId
> SwapchainAllocator;
extern SwapchainAllocator swapchainAllocator;

/// Get vulkan device
VkDevice SwapchainGetVkDevice(const CoreGraphics::SwapchainId id);
/// Get vulkan swapchain
VkSwapchainKHR SwapchainGetVkSwapchain(const CoreGraphics::SwapchainId id);
/// Get vulkan image views
const Util::Array<VkImageView>& SwapchainGetVkImageViews(const CoreGraphics::SwapchainId id);

} // namespace Vulkan
