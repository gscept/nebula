//------------------------------------------------------------------------------
// vkshaderrwtexture.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshaderrwtexture.h"
#include "vkgraphicsdevice.h"
#include "vktypes.h"
#include "vkutilities.h"
#include "vkscheduler.h"
#include "coregraphics/config.h"
#include "coregraphics/shaderserver.h"
#include <array>


namespace Vulkan
{

ShaderRWTextureAllocator shaderRWTextureAllocator(0x00FFFFFF);

//------------------------------------------------------------------------------
/**
*/
const VkImageView
ShaderRWTextureGetVkImageView(const CoreGraphics::ShaderRWTextureId id)
{
	return shaderRWTextureAllocator.Get<1>(id.id24).view;
}

//------------------------------------------------------------------------------
/**
*/
const VkImage
ShaderRWTextureGetVkImage(const CoreGraphics::ShaderRWTextureId id)
{
	return shaderRWTextureAllocator.Get<0>(id.id24).img;
}

} // namespace Vulkan

namespace CoreGraphics
{

using namespace Vulkan;
//------------------------------------------------------------------------------
/**
*/
const ShaderRWTextureId
CreateShaderRWTexture(const ShaderRWTextureCreateInfo& info)
{
	Ids::Id32 id = shaderRWTextureAllocator.AllocObject();

	VkShaderRWTextureLoadInfo& loadInfo = shaderRWTextureAllocator.Get<0>(id);
	VkShaderRWTextureRuntimeInfo& runtimeInfo = shaderRWTextureAllocator.Get<1>(id);
	CoreGraphicsImageLayout& layout = shaderRWTextureAllocator.Get<2>(id);

	ShaderRWTextureInfo adjustedInfo = ShaderRWTextureInfoSetupHelper(info);
	loadInfo.dims.width = adjustedInfo.width;
	loadInfo.dims.height = adjustedInfo.height;
	loadInfo.dims.depth = 1;
	loadInfo.dev = Vulkan::GetCurrentDevice();

	VkPhysicalDevice physicalDev = Vulkan::GetCurrentPhysicalDevice();
	VkFormat vkformat = VkTypes::AsVkDataFormat(adjustedInfo.format);
	VkFormatProperties formatProps;
	vkGetPhysicalDeviceFormatProperties(physicalDev, vkformat, &formatProps);
	n_assert(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);
	VkExtent3D extents;
	extents.width = adjustedInfo.width;
	extents.height = adjustedInfo.height;
	extents.depth = 1;

	const Util::Set<uint32_t>& queues = Vulkan::GetQueueFamilies();
	VkImageCreateInfo crinfo =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		NULL,
		0,
		VK_IMAGE_TYPE_2D,
		vkformat,
		extents,
		1,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_SHARING_MODE_CONCURRENT,
		(uint32_t)queues.Size(),
		queues.KeysAsArray().Begin(),
		VK_IMAGE_LAYOUT_UNDEFINED
	};
	VkResult stat = vkCreateImage(loadInfo.dev, &crinfo, NULL, &loadInfo.img);
	n_assert(stat == VK_SUCCESS);

	// allocate memory backing
	uint32_t alignedSize;
	VkUtilities::AllocateImageMemory(loadInfo.dev, loadInfo.img, loadInfo.mem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, alignedSize);
	vkBindImageMemory(loadInfo.dev, loadInfo.img, loadInfo.mem, 0);

	VkImageSubresourceRange viewRange;
	viewRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewRange.baseMipLevel = 0;
	viewRange.baseArrayLayer = 0;
	viewRange.levelCount = VK_REMAINING_MIP_LEVELS;
	viewRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	VkImageViewCreateInfo viewCreate =
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		NULL,
		0,
		loadInfo.img,
		VK_IMAGE_VIEW_TYPE_2D,
		vkformat,
		VkTypes::AsVkMapping(adjustedInfo.format),
		viewRange
	};
	stat = vkCreateImageView(loadInfo.dev, &viewCreate, NULL, &runtimeInfo.view);
	n_assert(stat == VK_SUCCESS);

	// transition to a useable state
	VkScheduler::Instance()->PushImageLayoutTransition(GraphicsQueueType, CoreGraphics::BarrierStage::Host, CoreGraphics::BarrierStage::AllGraphicsShaders, VkUtilities::ImageMemoryBarrier(loadInfo.img, viewRange, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VkTypes::AsVkImageLayout(info.layout)));

	layout = info.layout;
	ShaderRWTextureId ret;
	ret.id24 = id;
	ret.id8 = ShaderRWTextureIdType;

	if (info.registerBindless)
		runtimeInfo.bind = VkShaderServer::Instance()->RegisterTexture(ret, Texture2D);
	else
		runtimeInfo.bind = 0; // use placeholder

#if NEBULA_GRAPHICS_DEBUG
	ObjectSetName(ret, info.name.Value());
#endif

	CoreGraphics::RegisterShaderRWTexture(info.name, ret);

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyShaderRWTexture(const ShaderRWTextureId id)
{
	VkShaderRWTextureLoadInfo& loadInfo = shaderRWTextureAllocator.Get<0>(id.id24);
	VkShaderRWTextureRuntimeInfo& runtimeInfo = shaderRWTextureAllocator.Get<1>(id.id24);

	// bind 0 is reserved for the placeholder texture, meaning we never registered one
	if (runtimeInfo.bind != 0)
		VkShaderServer::Instance()->UnregisterTexture(runtimeInfo.bind, Texture2D);

	vkDestroyImageView(loadInfo.dev, runtimeInfo.view, nullptr);
	vkDestroyImage(loadInfo.dev, loadInfo.img, nullptr);
	vkFreeMemory(loadInfo.dev, loadInfo.mem, nullptr);

	shaderRWTextureAllocator.DeallocObject(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderRWTextureResize(const ShaderRWTextureId id, const ShaderRWTextureResizeInfo& info)
{
	n_error("Implement me!");
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderRWTextureWindowResized(const ShaderRWTextureId id)
{
	n_error("Implement me!");
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderRWWindowResized(const ShaderRWTextureId id)
{
	n_error("Implement me!");
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderRWTextureClear(const ShaderRWTextureId id, const Math::float4& color)
{
	VkShaderRWTextureLoadInfo& loadInfo = shaderRWTextureAllocator.Get<0>(id.id24);
	VkClearColorValue clear;
	clear.float32[0] = color.x();
	clear.float32[1] = color.y();
	clear.float32[2] = color.z();
	clear.float32[3] = color.w();

	VkImageSubresourceRange viewRange;
	viewRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewRange.baseMipLevel = 0;
	viewRange.levelCount = 1;
	viewRange.baseArrayLayer = 0;
	viewRange.layerCount = 1;
	VkUtilities::ImageColorClear(loadInfo.img, GraphicsQueueType, VK_IMAGE_LAYOUT_GENERAL, clear, viewRange);
}

//------------------------------------------------------------------------------
/**
*/
const TextureDimensions
ShaderRWTextureGetDimensions(const ShaderRWTextureId id)
{
	return shaderRWTextureAllocator.Get<0>(id.id24).dims;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
ShaderRWTextureGetNumMips(const ShaderRWTextureId id)
{
	return 1;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphicsImageLayout
ShaderRWTextureGetLayout(const ShaderRWTextureId id)
{
	return shaderRWTextureAllocator.Get<2>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
uint 
ShaderRWTextureGetBindlessHandle(const ShaderRWTextureId id)
{
	return shaderRWTextureAllocator.Get<1>(id.id24).bind;
}

} // namespace CoreGraphics