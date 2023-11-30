#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan implementation of GPU acceleration structure

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
namespace Vulkan
{

enum
{
    AS_Handle,
    AS_View
};

typedef Ids::IdAllocatorSafe<
    0xFFF
    , VkAccelerationStructureKHR
    , VkDeviceAddress
> VkBLASAllocator;

extern VkBLASAllocator vkBlasAllocator;

} // namespace Vulkan
