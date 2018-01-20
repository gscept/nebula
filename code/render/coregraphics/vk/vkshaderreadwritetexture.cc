//------------------------------------------------------------------------------
// vkshaderreadwritetexture.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkshaderreadwritetexture.h"
#include "vkrenderdevice.h"
#include "vktypes.h"
#include "vkutilities.h"
#include "vkscheduler.h"
#include "coregraphics/config.h"
#include <array>


namespace Vulkan
{

ShaderRWTextureAllocator shaderRWTextureAllocator(0x00FFFFFF);

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

	loadInfo.dev = VkRenderDevice::Instance()->GetCurrentDevice();

	VkPhysicalDevice physicalDev = VkRenderDevice::Instance()->GetCurrentPhysicalDevice();
	VkFormat vkformat = VkTypes::AsVkDataFormat(info.format);
	VkFormatProperties formatProps;
	vkGetPhysicalDeviceFormatProperties(physicalDev, vkformat, &formatProps);
	n_assert(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);
	VkExtent3D extents;
	extents.width = info.width;
	extents.height = info.height;
	extents.depth = 1;

	const std::array<uint32_t, 4> queues = VkRenderDevice::Instance()->GetQueueFamilies();
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
		(uint32_t)queues.size(),
		queues.data(),
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
		VkTypes::AsVkMapping(info.format),
		viewRange
	};
	stat = vkCreateImageView(loadInfo.dev, &viewCreate, NULL, &runtimeInfo.view);
	n_assert(stat == VK_SUCCESS);

	// transition to a useable state
	VkScheduler::Instance()->PushImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(loadInfo.img, viewRange, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL));

	ShaderRWTextureId ret;
	ret.id24 = id;
	ret.id8 = ShaderRWTextureIdType;
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

}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureWindowResized(const ShaderRWTextureId id)
{

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
	VkUtilities::ImageColorClear(loadInfo.img, VkDeferredCommand::Graphics, VK_IMAGE_LAYOUT_GENERAL, clear, viewRange);
}

} // namespace CoreGraphics