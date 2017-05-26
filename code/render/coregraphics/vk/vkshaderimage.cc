//------------------------------------------------------------------------------
// vkshaderimage.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkshaderimage.h"
#include "vkrenderdevice.h"
#include "vktypes.h"
#include "vkutilities.h"
#include "vkscheduler.h"

namespace Vulkan
{

__ImplementClass(Vulkan::VkShaderImage, 'VKSI', Base::ShaderReadWriteTextureBase);
//------------------------------------------------------------------------------
/**
*/
VkShaderImage::VkShaderImage()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkShaderImage::~VkShaderImage()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderImage::Setup(const SizeT width, const SizeT height, const CoreGraphics::PixelFormat::Code& format, const Resources::ResourceId& id)
{
	Base::ShaderReadWriteTextureBase::Setup(width, height, format, id);

	VkFormat vkformat = VkTypes::AsVkDataFormat(format);
	VkFormatProperties formatProps;
	vkGetPhysicalDeviceFormatProperties(VkRenderDevice::physicalDev, vkformat, &formatProps);
    n_assert(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);
	VkExtent3D extents;
	extents.width = width;
	extents.height = height;
	extents.depth = 1;
	uint32_t queues[] = { VkRenderDevice::Instance()->drawQueueFamily, VkRenderDevice::Instance()->computeQueueFamily };
	VkImageCreateInfo info =
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
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		NULL,
		VK_IMAGE_LAYOUT_UNDEFINED
	};
	VkResult stat = vkCreateImage(VkRenderDevice::dev, &info, NULL, &this->img);
	n_assert(stat == VK_SUCCESS);

	// allocate memory backing
	uint32_t alignedSize;
	VkUtilities::AllocateImageMemory(this->img, this->mem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, alignedSize);
	vkBindImageMemory(VkRenderDevice::dev, this->img, this->mem, 0);

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
		this->img,
		VK_IMAGE_VIEW_TYPE_2D,
		vkformat,
		VkTypes::AsVkMapping(format),
		viewRange
	};
	stat = vkCreateImageView(VkRenderDevice::dev, &viewCreate, NULL, &this->view);
	n_assert(stat == VK_SUCCESS);

	// transition to a useable state
	VkScheduler::Instance()->PushImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(this->img, viewRange, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL));

	// setup texture resource
	this->texture->SetupFromVkTexture(this->img, this->mem, this->view, width, height, format);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderImage::Resize()
{
	// implement me!
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderImage::Clear(const Math::float4& clearColor)
{
	VkClearColorValue clear;
	clear.float32[0] = clearColor.x();
	clear.float32[1] = clearColor.y();
	clear.float32[2] = clearColor.z();
	clear.float32[3] = clearColor.w();

	VkImageSubresourceRange viewRange;
	viewRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewRange.baseMipLevel = 0;
	viewRange.levelCount = 1;
	viewRange.baseArrayLayer = 0;
	viewRange.layerCount = 1;
	VkUtilities::ImageColorClear(this->img, VkDeferredCommand::Graphics, VK_IMAGE_LAYOUT_GENERAL, clear, viewRange);
}

} // namespace Vulkan