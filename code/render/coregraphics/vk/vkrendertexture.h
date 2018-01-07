#pragma once
//------------------------------------------------------------------------------
/**
	Implements a Vulkan implementation of a render texture
	
	(C) 2016 Individual contributors, see AUTHORS file
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
/// swap buffers, only valid if this is a window texture
void RenderTextureSwapBuffers(const CoreGraphics::RenderTextureId id);

/// get vk image view
const VkImageView RenderTextureGetVkImageView(const CoreGraphics::RenderTextureId id);
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
	bool dynamicSize : 1;
	bool relativeSize : 1;
	bool msaa : 1;
	CoreGraphics::WindowId window;
	float widthScale, heightScale, depthScale;
	CoreGraphics::TextureDimensions dims;
	CoreGraphics::PixelFormat::Code format;
};

struct VkRenderTextureRuntimeInfo
{
	VkImageView view;
	CoreGraphics::TextureType type;
	bool inpass : 1;
	uint32_t bind : 31; // just for alignment
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
	VkRenderTextureWindowInfo
> VkRenderTextureAllocator;
extern VkRenderTextureAllocator renderTextureAllocator;
} // namespace Vulkan