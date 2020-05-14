#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan Memory Manager

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/array.h"
#include "coregraphics/memory.h"
#include "threading/criticalsection.h"
namespace Vulkan
{

/// allocate memory for an image
CoreGraphics::Alloc AllocateMemory(const VkDevice dev, const VkImage& img, CoreGraphics::MemoryPoolType type);
/// allocate memory for a buffer
CoreGraphics::Alloc AllocateMemory(const VkDevice dev, const VkBuffer& buf, CoreGraphics::MemoryPoolType type);
/// allocate memory for sparse memory
CoreGraphics::Alloc AllocateMemory(const VkDevice dev, VkDeviceSize alignment, VkDeviceSize allocSize);

/// flush a mapped memory region
void Flush(const VkDevice dev, const CoreGraphics::Alloc& alloc);

/// get vulkan memory type based on resource requirements and wanted memory properties
VkResult GetMemoryType(uint32_t bits, VkMemoryPropertyFlags flags, uint32_t& index);

} // namespace Vulkan
