#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan implementation of a GPU buffer

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "coregraphics/config.h"
#include "coregraphics/gpubuffertypes.h"
#include "coregraphics/buffer.h"
#include "coregraphics/memory.h"
#include "vkloader.h"
namespace Vulkan
{

struct VkBufferLoadInfo
{
	VkDevice dev;
	CoreGraphics::Alloc mem;
	CoreGraphics::BufferAccessMode mode;
	uint32_t size;
	uint32_t elementSize;
	uint32_t byteSize;
};

struct VkBufferRuntimeInfo
{
	VkBuffer buf;
	CoreGraphics::BufferUsageFlags usageFlags;
};

struct VkBufferMapInfo
{
	void* mappedMemory;
	uint32_t mapCount;
};

enum
{
	Buffer_LoadInfo,
	Buffer_RuntimeInfo,
	Buffer_MapInfo,
};


typedef Ids::IdAllocatorSafe<
	VkBufferLoadInfo,
	VkBufferRuntimeInfo,
	VkBufferMapInfo
> VkBufferAllocator;
extern VkBufferAllocator bufferAllocator;

/// get buffer object
VkBuffer BufferGetVk(const CoreGraphics::BufferId id);
/// get buffer memory
VkDeviceMemory BufferGetVkMemory(const CoreGraphics::BufferId id);
/// get buffer device
VkDevice BufferGetVkDevice(const CoreGraphics::BufferId id);


} // namespace Vulkan
