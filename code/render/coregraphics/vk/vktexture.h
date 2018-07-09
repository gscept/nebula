#pragma once
//------------------------------------------------------------------------------
/**
	Vulkan texture abstraction types.

	For the actual loader code, see VkStreamTextureLoader and VkMemoryTextureLoader.
	
	(C) 2017 Individual contributors, see AUTHORS file
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
	CoreGraphics::PixelFormat::Code format;
	CoreGraphics::GpuBufferTypes::Usage usage;
	CoreGraphics::GpuBufferTypes::Access access;
	CoreGraphics::GpuBufferTypes::Syncing syncing;
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

/// we need a thread-safe allocator since it will be used by both the memory and stream pool
typedef Ids::IdAllocatorSafe<
	VkTextureRuntimeInfo,					// 0 runtime info (for binding)
	VkTextureLoadInfo,						// 1 loading info (mostly used during the load/unload phase)
	VkTextureMappingInfo,					// 2 used when image is mapped to memory
	ImageLayout								// 3 used to keep track of image layout (use only when necessary)
> VkTextureAllocator;
extern VkTextureAllocator textureAllocator;

/// get Vk image
const VkImage TextureGetVkImage(const CoreGraphics::TextureId id);
/// get vk image view
const VkImageView TextureGetVkImageView(const CoreGraphics::TextureId id);

} // namespace Vulkan