//------------------------------------------------------------------------------
// vktexture.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vktexture.h"
#include "vulkan/vulkan.h"

namespace Vulkan
{

VkTextureAllocator textureAllocator(0x00FFFFFF);

//------------------------------------------------------------------------------
/**
*/
const VkImage
TextureGetVkImage(const CoreGraphics::TextureId id)
{
	return textureAllocator.GetSafe<1>(id.resourceId).img;
}

//------------------------------------------------------------------------------
/**
*/
const VkImageView
TextureGetVkImageView(const CoreGraphics::TextureId id)
{
	return textureAllocator.GetSafe<0>(id.resourceId).view;
}

} // namespace Vulkan
