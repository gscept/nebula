#pragma once
//------------------------------------------------------------------------------
/**
	Types used for Vulkan vertex buffers,
	see the MemoryVertexBufferPool for the loader code.

	(C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "coregraphics/config.h"
#include "coregraphics/gpubuffertypes.h"
#include "coregraphics/indexbuffer.h"
#include "vkloader.h"
namespace Vulkan
{

struct VkIndexBufferLoadInfo
{
	VkDevice dev;
	CoreGraphics::Alloc mem;
	CoreGraphics::GpuBufferTypes::SetupFlags gpuResInfo;
	CoreGraphics::BufferUpdateMode mode;
	uint32_t indexCount;
};
struct VkIndexBufferRuntimeInfo
{
	VkBuffer buf;
	CoreGraphics::IndexType::Code type;
};

struct VkIndexBufferMapInfo
{
	void* mappedMemory;
	uint32_t mapCount;
};

enum
{
	IndexBuffer_LoadInfo,
	IndexBuffer_RuntimeInfo,
	IndexBuffer_MapCount,
};

typedef Ids::IdAllocatorSafe<
	VkIndexBufferLoadInfo,			//0 loading stage info
	VkIndexBufferRuntimeInfo,		//1 runtime stage info
	VkIndexBufferMapInfo			//2 mapping stage info
> VkIndexBufferAllocator;
extern VkIndexBufferAllocator iboAllocator;

/// get vertex buffer object
VkBuffer IndexBufferGetVk(const CoreGraphics::IndexBufferId id);
/// get index buffer index type
VkIndexType IndexBufferGetVkType(const CoreGraphics::IndexBufferId id);
/// get vertex buffer object memory
VkDeviceMemory IndexBufferGetVkMemory(const CoreGraphics::IndexBufferId id);

} // namespace Vulkan