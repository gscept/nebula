#pragma once
//------------------------------------------------------------------------------
/**
	Implements a read/write buffer used within shaders, in Vulkan.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/stretchybuffer.h"
#include "coregraphics/shaderrwbuffer.h"
#include "ids/idallocator.h"
namespace Vulkan
{

struct VkShaderRWBufferLoadInfo
{
	VkDevice dev;
	VkDeviceMemory mem;
	VkBufferCreateInfo info;

	SizeT size;
	SizeT stride;
	SizeT numBuffers;
	SizeT grow;
};

struct VkShaderRWBufferRuntimeInfo
{
	VkBuffer buf;
};

struct VkShaderRWBufferMapInfo
{
	void* data;
	SizeT baseOffset;
};

struct ShaderRWBufferStretchInterface
{
	CoreGraphics::StretchyBuffer<ShaderRWBufferStretchInterface> resizer;
	CoreGraphics::ShaderRWBufferId obj;
	SizeT Grow(const SizeT capacity, const SizeT numInstances, SizeT& newCapacity);
};

typedef Ids::IdAllocator<
	VkShaderRWBufferLoadInfo,
	VkShaderRWBufferRuntimeInfo,
	VkShaderRWBufferMapInfo,
	ShaderRWBufferStretchInterface
> ShaderRWBufferAllocator;
extern ShaderRWBufferAllocator shaderRWBufferAllocator;

/// get vk image
const VkBuffer ShaderRWBufferGetVkBuffer(const CoreGraphics::ShaderRWBufferId id);
} // namespace Vulkan