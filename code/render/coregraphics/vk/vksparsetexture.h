#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan sparse texture

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/sparsetexture.h"
#include "ids/idallocator.h"
#include "coregraphics/memory.h"
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
	Util::FixedArray<Util::FixedArray<Util::FixedArray<uint32_t>>> bindCounts;
	VkSparseImageMemoryRequirements sparseMemoryRequirements;
	bool bindless : 1;
};

/// get vulkan image
VkImage SparseTextureGetVkImage(const CoreGraphics::SparseTextureId id);
/// get vulkan image view
VkImageView SparseTextureGetVkImageView(const CoreGraphics::SparseTextureId id);

struct TexturePage
{
	VkSparseImageMemoryBind binding;
	VkOffset3D offset;
	VkExtent3D extent;
	uint32_t mip;
	uint32_t layer;
	uint32_t size;
	uint32_t refCount;
	CoreGraphics::Alloc alloc;
};

struct TexturePageTable
{
	Util::FixedArray<Util::FixedArray<Util::Array<TexturePage>>> pages;
};

struct VkSparseTextureRuntimeInfo
{
	VkImageView view;
	uint bind;
};

enum
{
	SparseTexture_OpaqueBinds,
	SparseTexture_PageTable,
	SparseTexture_PendingBinds,
	SparseTexture_Allocs,
	SparseTexture_Load,
	SparseTexture_Runtime

};

typedef Ids::IdAllocator<
	Util::Array<VkSparseMemoryBind>,
	TexturePageTable,
	Util::Array<VkSparseImageMemoryBind>,
	Util::Array<CoreGraphics::Alloc>,
	VkSparseTextureLoadInfo,
	VkSparseTextureRuntimeInfo
> VkSparseTextureAllocator;
extern VkSparseTextureAllocator vkSparseTextureAllocator;

} // namespace Vulkan
