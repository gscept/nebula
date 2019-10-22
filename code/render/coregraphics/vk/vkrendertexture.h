#pragma once
//------------------------------------------------------------------------------
/**
	Implements a Vulkan implementation of a render texture
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "coregraphics/rendertexture.h"

namespace Vulkan
{

/// generate mip chain (from 0 to number of mips)
void RenderTextureGenerateMipChain(const CoreGraphics::RenderTextureId id);
/// generate mip chain from offset (from N to number of mips)
void RenderTextureGenerateMipChain(const CoreGraphics::RenderTextureId id, IndexT from);
/// generate segment of mip chain
void RenderTextureGenerateMipChain(const CoreGraphics::RenderTextureId id, IndexT from, IndexT to);
/// generate mip maps internally from index to another
void RenderTextureGenerateMipHelper(const CoreGraphics::RenderTextureId id, IndexT from, const CoreGraphics::RenderTextureId target, IndexT to);
/// generate mip from one mip level to another
void RenderTextureBlit(const CoreGraphics::RenderTextureId id, IndexT from, IndexT to, const CoreGraphics::RenderTextureId target = CoreGraphics::RenderTextureId::Invalid());

/// get vk image view
const VkImageView RenderTextureGetVkAttachmentImageView(const CoreGraphics::RenderTextureId id);
/// get vk image view
const VkImageView RenderTextureGetVkSampleImageView(const CoreGraphics::RenderTextureId id);
/// get vk image
const VkImage RenderTextureGetVkImage(const CoreGraphics::RenderTextureId id);


struct VkRenderTextureLoadInfo
{
	VkDevice dev;
	VkImage img;
	VkDeviceMemory mem;
	uint32_t mips;
	uint32_t layers;
	bool isWindow : 1;
	bool relativeSize : 1;
	bool msaa : 1;
	CoreGraphics::WindowId window;
	float widthScale, heightScale, depthScale;
	CoreGraphics::TextureDimensions dims;
	CoreGraphics::PixelFormat::Code format;
};

struct VkRenderTextureRuntimeInfo
{
	VkImageView framebufferView;
	VkImageView sampleView; // only used by depth-stencil targets to sample from
	uint32_t bind;
	CoreGraphics::TextureType type;
};

struct VkRenderTextureMappingInfo
{
	VkBuffer buf;
	VkDeviceMemory mem;
	VkImageCopy region;
	uint32_t mapCount;
};

struct VkRenderTextureWindowInfo
{
	Util::FixedArray<VkImage> swapimages;
	Util::FixedArray<VkImageView> swapviews;
};

typedef Ids::IdAllocator<
	VkRenderTextureLoadInfo,
	VkRenderTextureRuntimeInfo,
	VkRenderTextureMappingInfo,
	VkRenderTextureWindowInfo,
	CoreGraphicsImageLayout
> VkRenderTextureAllocator;
extern VkRenderTextureAllocator renderTextureAllocator;
} // namespace Vulkan