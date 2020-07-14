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
#include "coregraphics/vertexbuffer.h"
#include "vkloader.h"
namespace Vulkan
{

struct VkVertexBufferLoadInfo
{
	VkDevice dev;
	CoreGraphics::Alloc mem;
	CoreGraphics::GpuBufferTypes::SetupFlags gpuResInfo;
	CoreGraphics::BufferUpdateMode mode;
	uint32_t vertexCount;
	uint32_t vertexByteSize;
};

struct VkVertexBufferRuntimeInfo
{
	VkBuffer buf;
	CoreGraphics::VertexLayoutId layout;
};

struct VkVertexBufferMapInfo
{
	void* mappedMemory;
	uint32_t mapCount;
};

enum
{
	VertexBuffer_LoadInfo,
	VertexBuffer_RuntimeInfo,
	VertexBuffer_MapCount,
};

typedef Ids::IdAllocatorSafe<
	VkVertexBufferLoadInfo,			//0 loading stage info
	VkVertexBufferRuntimeInfo,		//1 runtime stage info
	VkVertexBufferMapInfo			//2 mapping stage info
> VkVertexBufferAllocator;
extern VkVertexBufferAllocator vboAllocator;


/// get vertex buffer object
VkBuffer VertexBufferGetVk(const CoreGraphics::VertexBufferId id);
/// get vertex buffer object memory
VkDeviceMemory VertexBufferGetVkMemory(const CoreGraphics::VertexBufferId id);

} // namespace Vulkan