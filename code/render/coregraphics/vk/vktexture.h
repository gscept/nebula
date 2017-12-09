#pragma once
//------------------------------------------------------------------------------
/**
	Vulkan texture interface.

	Container class for shared Vulkan texture resources. Both the
	streaming and the memory texture loader needs these, which is why this
	is here...
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/pixelformat.h"
#include "resources/resourcepool.h"
#include "ids/idallocator.h"
namespace Vulkan
{
struct TextureLoadInfo
{
	VkImage img;
	VkDeviceMemory mem;
	CoreGraphics::TextureDimensions dims;
	uint32_t mips;
	CoreGraphics::PixelFormat::Code format;
	CoreGraphics::GpuBufferTypes::Usage usage;
	CoreGraphics::GpuBufferTypes::Access access;
	CoreGraphics::GpuBufferTypes::Syncing syncing;
};

struct TextureRuntimeInfo
{
	VkImageView view;
	CoreGraphics::TextureType type;
	uint32_t bind;
};

struct TextureMappingInfo
{
	VkBuffer buf;
	VkDeviceMemory mem;
	VkImageCopy region;
	uint32_t mapCount;
};

/// we need a thread-safe allocator since it will be used by both the memory and stream pool
typedef Ids::IdAllocatorSafe<
	TextureRuntimeInfo,						// 0 runtime info (for binding)
	TextureLoadInfo,						// 1 loading info (mostly used during the load/unload phase)
	TextureMappingInfo						// 2 used when image is mapped to memory
> VkTextureAllocator;
extern VkTextureAllocator textureAllocator;

/// get Vk image
const VkImage TextureGetVk(const CoreGraphics::TextureId id);

} // namespace Vulkan