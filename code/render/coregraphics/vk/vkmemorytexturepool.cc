//------------------------------------------------------------------------------
// vkmemorytextureloader.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkmemorytexturepool.h"
#include "coregraphics/texture.h"
#include "vkrenderdevice.h"
#include "vktypes.h"
#include "coregraphics/renderdevice.h"
#include "vkutilities.h"
#include "resources/resourcemanager.h"
#include "vkshaderserver.h"

using namespace CoreGraphics;
using namespace Resources;
namespace Vulkan
{

__ImplementClass(Vulkan::VkMemoryTexturePool, 'VKTO', Resources::ResourceMemoryPool);

//------------------------------------------------------------------------------
/**
*/
ResourcePool::LoadStatus
VkMemoryTexturePool::UpdateResource(const Ids::Id24 id, void* info)
{
	VkMemoryTextureInfo* data = (VkMemoryTextureInfo*)info;

	/// during the load-phase, we can safetly get the structs
	this->EnterGet();
	VkTexture::RuntimeInfo& runtimeInfo = this->Get<0>(res);
	VkTexture::LoadInfo& loadInfo = this->Get<1>(res);
	this->LeaveGet();

	VkFormat vkformat = VkTypes::AsVkFormat(data->format);
	uint32_t size = PixelFormat::ToSize(data->format);

	VkFormatProperties formatProps;
	vkGetPhysicalDeviceFormatProperties(VkRenderDevice::physicalDev, vkformat, &formatProps);
	VkExtent3D extents;
	extents.width = data->width;
	extents.height = data->height;
	extents.depth = 1;

	VkImageCreateInfo imgInfo =
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

	VkResult stat = vkCreateImage(VkRenderDevice::dev, &imgInfo, NULL, &loadInfo.img);
	n_assert(stat == VK_SUCCESS);

	// allocate memory backing
	uint32_t alignedSize;
	VkUtilities::AllocateImageMemory(loadInfo.img, loadInfo.mem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, alignedSize);
	vkBindImageMemory(VkRenderDevice::dev, loadInfo.img, loadInfo.mem, 0);

	VkScheduler* scheduler = VkScheduler::Instance();

	// transition into transfer mode
	VkImageSubresourceRange subres;
	subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subres.baseArrayLayer = 0;
	subres.baseMipLevel = 0;
	subres.layerCount = 1;
	subres.levelCount = 1;
	scheduler->PushImageLayoutTransition(VkDeferredCommand::Transfer, VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));

	VkBufferImageCopy copy;
	copy.bufferOffset = 0;
	copy.bufferImageHeight = data->height;
	copy.bufferRowLength = data->width * size;
	copy.imageExtent.width = data->width;
	copy.imageExtent.height = data->height;
	copy.imageExtent.depth = 1;
	copy.imageOffset = { 0, 0, 0 };
	copy.imageSubresource.baseArrayLayer = 0;
	copy.imageSubresource.layerCount = 1;
	copy.imageSubresource.mipLevel = 0;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	// update image while in transfer optimal
	scheduler->PushImageUpdate(this->alloc.allocator.Get<0>(id), imgInfo, 0, 0, data->width * data->height * size, (uint32_t*)data->buffer);

	// transition image to shader variable
	scheduler->PushImageLayoutTransition(VkDeferredCommand::Transfer, VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
	scheduler->PushImageOwnershipChange(VkDeferredCommand::Transfer, VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, VkDeferredCommand::Transfer, VkDeferredCommand::Graphics, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

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
		loadInfo.img,
		viewType,
		vkformat,
		VkTypes::AsVkMapping(data->format),
		viewRange
	};
	stat = vkCreateImageView(VkRenderDevice::dev, &viewCreate, NULL, &runtimeInfo.view);
	n_assert(stat == VK_SUCCESS);

	loadInfo.dims.width = data->width;
	loadInfo.dims.height = data->height;
	loadInfo.dims.depth = 1;
	loadInfo.mips = 1;
	loadInfo.format = data->format;
	runtimeInfo.type = Texture::Texture2D;
	runtimeInfo.bind = VkShaderServer::Instance()->RegisterTexture(runtimeInfo.view, Texture::Texture2D);

	n_assert(this->GetState(id) == Resource::Pending);
	n_assert(loadInfo.img!= VK_NULL_HANDLE);

	return ResourcePool::Success;
}


} // namespace Vulkan