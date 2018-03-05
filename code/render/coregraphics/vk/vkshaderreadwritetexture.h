#pragma once
//------------------------------------------------------------------------------
/**
	Implements a read/write image in Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "coregraphics/shaderreadwritetexture.h"
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

/// get vk image view
const VkImageView ShaderRWTextureGetVkImageView(const CoreGraphics::ShaderRWTextureId id);
/// get vk image
const VkImage ShaderRWTextureGetVkImage(const CoreGraphics::ShaderRWTextureId id);

} // namespace Vulkan