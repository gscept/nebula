#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan abstraction of a fence.

    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "coregraphics/fence.h"
#include "vkloader.h"
namespace Vulkan
{

struct VkFenceInfo
{
    VkFence fence;
};
typedef Ids::IdAllocator<VkDevice, VkFenceInfo> VkFenceAllocator;
extern VkFenceAllocator fenceAllocator;

//------------------------------------------------------------------------------
/**
*/
VkFence FenceGetVk(const CoreGraphics::FenceId id);

} // namespace Vulkan
