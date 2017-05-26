#pragma once
//------------------------------------------------------------------------------
/**
	Implements a memory texture loader for Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourceloader.h"
#include "coregraphics/pixelformat.h"

namespace Vulkan
{
class VkMemoryTextureLoader : public Resources::ResourceLoader
{
	__DeclareClass(VkMemoryTextureLoader);
public:

	/// sets the image buffer
	void SetImageBuffer(const void* buffer, SizeT width, SizeT height, CoreGraphics::PixelFormat::Code format);

	/// on load callback
	virtual bool OnLoadRequested();
private:
	CoreGraphics::PixelFormat::Code format;
	SizeT width, height;
	VkImage image;
	VkImageView view;
	VkDeviceMemory mem;

};
} // namespace Vulkan