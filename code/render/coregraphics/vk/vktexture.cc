//------------------------------------------------------------------------------
// vktexture.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vktexture.h"
#include "vkrenderdevice.h"
#include "coregraphics/pixelformat.h"
#include "vktypes.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/config.h"
#include "vkshaderserver.h"
#include "vkutilities.h"

namespace Vulkan
{

VkTextureAllocator textureAllocator(0x00FFFFFF);

//------------------------------------------------------------------------------
/**
*/
const VkImage
TextureGetVk(const CoreGraphics::TextureId id)
{
	return textureAllocator.GetSafe<1>(id.id24).img;
}

} // namespace Vulkan
