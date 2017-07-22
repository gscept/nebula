#pragma once
//------------------------------------------------------------------------------
/**
	Implements a memory texture loader for Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcememorypool.h"
#include "coregraphics/pixelformat.h"

namespace Vulkan
{
class VkMemoryTexturePool : public Resources::ResourceMemoryPool
{
	__DeclareClass(VkMemoryTexturePool);
public:

	/// sets the image buffer
	void SetImageBuffer(const void* buffer, SizeT width, SizeT height, CoreGraphics::PixelFormat::Code format);

	/// load
	LoadStatus Load(const Resources::ResourceId id);
private:
	CoreGraphics::PixelFormat::Code format;
	SizeT width, height;
	VkImage image;
	VkImageView view;
	VkDeviceMemory mem;

};
} // namespace Vulkan