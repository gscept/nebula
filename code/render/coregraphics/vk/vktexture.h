#pragma once
//------------------------------------------------------------------------------
/**
	Vulkan texture abstraction types.

	For the actual loader code, see VkStreamTextureLoader and VkMemoryTextureLoader.
	
	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/texture.h"
#include "coregraphics/pixelformat.h"
#include "resources/resourcepool.h"
#include "ids/idallocator.h"
namespace Vulkan
{
struct VkTextureLoadInfo
{
	VkDevice dev;
	VkImage img;
	VkDeviceMemory mem;
	CoreGraphics::TextureDimensions dims;
	uint32_t mips;
	uint32_t layers;
	uint8_t samples;
	CoreGraphics::PixelFormat::Code format;
	CoreGraphics::TextureUsage texUsage;
	CoreGraphics::TextureId alias;
	CoreGraphics::ImageLayout defaultLayout;
};

struct VkTextureRuntimeInfo
{
	VkImageView view;
	CoreGraphics::TextureType type;
	uint32_t bind;
};

struct VkTextureMappingInfo
{
	VkBuffer buf;
	VkDeviceMemory mem;
	VkImageCopy region;
	uint32_t mapCount;
};

struct VkTextureWindowInfo
{
	CoreGraphics::WindowId window;
	Util::FixedArray<VkImage> swapimages;
	Util::FixedArray<VkImageView> swapviews;
};

enum
{
	Texture_RuntimeInfo,
	Texture_LoadInfo,
	Texture_MappingInfo,
	Texture_WindowInfo
};

/// we need a thread-safe allocator since it will be used by both the memory and stream pool
typedef Ids::IdAllocatorSafe<
	VkTextureRuntimeInfo,					// runtime info (for binding)
	VkTextureLoadInfo,						// loading info (mostly used during the load/unload phase)
	VkTextureMappingInfo,					// used when image is mapped to memory
	VkTextureWindowInfo
> VkTextureAllocator;
extern VkTextureAllocator textureAllocator;

/// get Vk image
const VkImage TextureGetVkImage(const CoreGraphics::TextureId id);
/// get vk image view
const VkImageView TextureGetVkImageView(const CoreGraphics::TextureId id);

} // namespace Vulkan