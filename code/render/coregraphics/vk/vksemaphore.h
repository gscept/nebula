#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan semaphore (cross-queue synchronization primitive)

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/semaphore.h"
#include "ids/idallocator.h"

namespace Vulkan
{

enum
{
    Semaphore_Device,
    Semaphore_VkHandle,
    Semaphore_Type,
    Semaphore_LastIndex
};
typedef Ids::IdAllocator<
    VkDevice,
    VkSemaphore,
    CoreGraphics::SemaphoreType,
    uint64
> VkSemaphoreAllocator;
extern VkSemaphoreAllocator semaphoreAllocator;

/// get vulkan sampler
VkSemaphore SemaphoreGetVk(const CoreGraphics::SemaphoreId& id);

} // namespace Vulkan
