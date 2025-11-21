#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan implementation of a GPU buffer

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "coregraphics/config.h"
#include "coregraphics/buffer.h"
#include "coregraphics/memory.h"

namespace Vulkan
{

struct VkBufferLoadInfo
{
    VkDevice dev;
    CoreGraphics::Alloc mem;
    CoreGraphics::BufferAccessMode mode;
    uint64_t size;
    uint64_t elementSize;
    uint64_t byteSize;
    Ids::Id32 sparseExtension;
};

struct VkBufferRuntimeInfo
{
    VkBuffer buf;
    CoreGraphics::BufferUsage usageFlags;
};

struct VkBufferMapInfo
{
    void* mappedMemory;
};

enum
{
    Buffer_LoadInfo,
    Buffer_RuntimeInfo,
    Buffer_MapInfo,
};


typedef Ids::IdAllocatorSafe<
    0xFFFF
    , VkBufferLoadInfo
    , VkBufferRuntimeInfo
    , VkBufferMapInfo
> VkBufferAllocator;
extern VkBufferAllocator bufferAllocator;


struct BufferSparsePageTable
{
    Util::Array<CoreGraphics::BufferSparsePage> pages;
    Util::Array<VkSparseMemoryBind> pageBindings;
    uint32_t bindCounts;
    VkMemoryRequirements memoryReqs;
};

enum
{
    BufferExtension_SparsePageTable,
    BufferExtension_SparsePendingBinds,
};
typedef Ids::IdAllocatorSafe<
    0xFF
    , BufferSparsePageTable
    , Util::Array<VkSparseMemoryBind>
> VkBufferSparseExtensionAllocator;
extern VkBufferSparseExtensionAllocator bufferSparseExtensionAllocator;

/// get buffer object
VkBuffer BufferGetVk(const CoreGraphics::BufferId id);
/// get buffer memory
VkDeviceMemory BufferGetVkMemory(const CoreGraphics::BufferId id);
/// get buffer device
VkDevice BufferGetVkDevice(const CoreGraphics::BufferId id);


} // namespace Vulkan
