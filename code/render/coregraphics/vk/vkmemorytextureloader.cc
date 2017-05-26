//------------------------------------------------------------------------------
// vkmemorytextureloader.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkmemorytextureloader.h"
#include "coregraphics/texture.h"
#include "vkrenderdevice.h"
#include "vktypes.h"
#include "coregraphics/renderdevice.h"
#include "vkutilities.h"

using namespace CoreGraphics;
using namespace Resources;
namespace Vulkan
{

__ImplementClass(Vulkan::VkMemoryTextureLoader, 'VKTO', Resources::ResourceLoader);
//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTextureLoader::SetImageBuffer(const void* buffer, SizeT width, SizeT height, CoreGraphics::PixelFormat::Code format)
{
	this->width = width;
	this->height = height;
	this->format = format;
	VkFormat vkformat = VkTypes::AsVkFormat(format);
	uint32_t size = PixelFormat::ToSize(format);

	VkFormatProperties formatProps;
	vkGetPhysicalDeviceFormatProperties(VkRenderDevice::physicalDev, vkformat, &formatProps);
	VkExtent3D extents;
	extents.width = width;
	extents.height = height;
	extents.depth = 1;
	
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
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		NULL,
		VK_IMAGE_LAYOUT_UNDEFINED
	};
	
	VkResult stat = vkCreateImage(VkRenderDevice::dev, &info, NULL, &this->image);
	n_assert(stat == VK_SUCCESS);

	// allocate memory backing
	uint32_t alignedSize;
	VkUtilities::AllocateImageMemory(this->image, this->mem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, alignedSize);
	vkBindImageMemory(VkRenderDevice::dev, this->image, this->mem, 0);

	VkScheduler* scheduler = VkScheduler::Instance();

	// transition into transfer mode
	VkImageSubresourceRange subres;
	subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subres.baseArrayLayer = 0;
	subres.baseMipLevel = 0;
	subres.layerCount = 1;
	subres.levelCount = 1;
	scheduler->PushImageLayoutTransition(VkDeferredCommand::Transfer, VkUtilities::ImageMemoryBarrier(this->image, subres, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));

	VkBufferImageCopy copy;
	copy.bufferOffset = 0;
	copy.bufferImageHeight = height;
	copy.bufferRowLength = width * size;
	copy.imageExtent.width = width;
	copy.imageExtent.height = height;
	copy.imageExtent.depth = 1;
	copy.imageOffset = { 0, 0, 0 };
	copy.imageSubresource.baseArrayLayer = 0;
	copy.imageSubresource.layerCount = 1;
	copy.imageSubresource.mipLevel = 0;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	// update image while in transfer optimal
	scheduler->PushImageUpdate(this->image, info, 0, 0, width * height * size, (uint32_t*)buffer);

	// transition image to shader variable
	scheduler->PushImageLayoutTransition(VkDeferredCommand::Transfer, VkUtilities::ImageMemoryBarrier(this->image, subres, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
	scheduler->PushImageOwnershipChange(VkDeferredCommand::Transfer, VkUtilities::ImageMemoryBarrier(this->image, subres, VkDeferredCommand::Transfer, VkDeferredCommand::Graphics, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

	// create view
	VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
	VkImageSubresourceRange viewRange;
	viewRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewRange.baseMipLevel = 0;
	viewRange.levelCount = 1;
	viewRange.baseArrayLayer = 0;
	viewRange.layerCount = 1;
	VkImageViewCreateInfo viewCreate =
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		NULL,
		0,
		this->image,
		viewType,
		vkformat,
		VkTypes::AsVkMapping(format),
		viewRange
	};
	stat = vkCreateImageView(VkRenderDevice::dev, &viewCreate, NULL, &this->view);
	n_assert(stat == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
bool
VkMemoryTextureLoader::OnLoadRequested()
{
	n_assert(this->resource->IsA(Texture::RTTI));
	n_assert(this->image != 0);
	const Ptr<Texture>& res = this->resource.downcast<Texture>();
	n_assert(!res->IsLoaded());
	res->SetupFromVkTexture(this->image, this->mem, this->view, this->width, this->height, this->format);
	res->SetState(Resource::Loaded);
	this->SetState(Resource::Loaded);
	return true;
}

} // namespace Vulkan