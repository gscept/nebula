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
	return textureAllocator.GetSafe<Texture_LoadInfo>(id.resourceId).img;
}

//------------------------------------------------------------------------------
/**
*/
const VkImageView
TextureGetVkImageView(const CoreGraphics::TextureId id)
{
	return textureAllocator.GetSafe<Texture_RuntimeInfo>(id.resourceId).view;
}

} // namespace Vulkan
