#pragma once
//------------------------------------------------------------------------------
/**
	Implements a read/write image in Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
namespace Vulkan
{

struct VkShaderRWTextureLoadInfo
{
	VkDevice dev;
	VkImage img;
	VkDeviceMemory mem;
};

struct VkShaderRWTextureRuntimeInfo
{
	VkImageView view;
};

typedef Ids::IdAllocator<
	VkShaderRWTextureLoadInfo,
	VkShaderRWTextureRuntimeInfo
> ShaderRWTextureAllocator;
extern ShaderRWTextureAllocator shaderRWTextureAllocator;

} // namespace Vulkan