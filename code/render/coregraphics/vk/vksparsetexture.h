#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan sparse texture

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/sparsetexture.h"
#include "ids/idallocator.h"
namespace Vulkan
{

struct VkSparseTextureLoadInfo
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
	bool bindless : 1;
};

/// get vulkan image
VkImage SparseTextureGetVkImage(const CoreGraphics::SparseTextureId id);
/// get vulkan image view
VkImageView SparseTextureGetVkImageView(const CoreGraphics::SparseTextureId id);

struct TexturePage
{
	VkSparseImageMemoryBind binding;
	uint32_t alignment;
};

enum
{
	SparseTexture_BindInfos,
	SparseTexture_OpaqueBinds,
	SparseTexture_PageBinds
};

typedef Ids::IdAllocator<
	VkBindSparseInfo,
	Util::Array<VkSparseMemoryBind>,
	Util::Array<TexturePage>
> VkSparseTextureAllocator;
extern VkSparseTextureAllocator vkSparseTextureAllocator;

} // namespace Vulkan
