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
	VkDeviceMemory mem;
	CoreGraphics::GpuBufferTypes::SetupFlags gpuResInfo;
	uint32_t vertexCount;
	uint32_t vertexByteSize;
};

struct VkVertexBufferRuntimeInfo
{
	VkBuffer buf;
	CoreGraphics::VertexLayoutId layout;
};

enum
{
	VertexBuffer_LoadInfo,
	VertexBuffer_RuntimeInfo,
	VertexBuffer_MapCount,
};

typedef Ids::IdAllocator<
	VkVertexBufferLoadInfo,			//0 loading stage info
	VkVertexBufferRuntimeInfo,		//1 runtime stage info
	uint32_t						//2 mapping stage info
> VkVertexBufferAllocator;
extern VkVertexBufferAllocator vboAllocator;


/// get vertex buffer object
VkBuffer VertexBufferGetVk(const CoreGraphics::VertexBufferId id);
/// get vertex buffer object memory
VkDeviceMemory VertexBufferGetVkMemory(const CoreGraphics::VertexBufferId id);

} // namespace Vulkan