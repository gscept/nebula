#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan implementation of Swapchain

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
namespace Vulkan
{

enum
{
    Swapchain_Device,
    Swapchain_Surface,
    Swapchain_Swapchain,
    Swapchain_CurrentBackbuffer,
    Swapchain_DisplayMode,
    Swapchain_Images,
    Swapchain_ImageViews,
    Swapchain_Queue,
    Swapchain_QueueType,
    Swapchain_CommandPool
};

typedef Ids::IdAllocator<
    VkDevice,
    VkSurfaceKHR,
    VkSwapchainKHR,
    uint,
    CoreGraphics::DisplayMode,
    Util::Array<VkImage>,
    Util::Array<VkImageView>,
    VkQueue,
    CoreGraphics::QueueType,
    CoreGraphics::CmdBufferPoolId
> SwapchainAllocator;
extern SwapchainAllocator swapchainAllocator;

} // namespace Vulkan
