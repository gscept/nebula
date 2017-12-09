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
VkMemoryTexturePool::LoadFromMemory(const Ids::Id24 id, void* info)
{
	VkMemoryTextureInfo* data = (VkMemoryTextureInfo*)info;

	/// during the load-phase, we can safetly get the structs
	this->EnterGet();
	TextureRuntimeInfo& runtimeInfo = this->Get<0>(id);
	TextureLoadInfo& loadInfo = this->Get<1>(id);
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
	scheduler->PushImageUpdate(this->Get<0>(id), imgInfo, 0, 0, data->width * data->height * size, (uint32_t*)data->buffer);

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
	runtimeInfo.type = CoreGraphics::Texture2D;
	runtimeInfo.bind = VkShaderServer::Instance()->RegisterTexture(runtimeInfo.view, CoreGraphics::Texture2D);

	n_assert(this->GetState(id) == Resource::Pending);
	n_assert(loadInfo.img!= VK_NULL_HANDLE);

	return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkMemoryTexturePool::GenerateMipmaps(const CoreGraphics::TextureId id)
{
}

//------------------------------------------------------------------------------
/**
*/
inline bool
VkMemoryTexturePool::Map(const CoreGraphics::TextureId id, IndexT mipLevel, CoreGraphics::GpuBufferTypes::MapType mapType, CoreGraphics::TextureMapInfo & outMapInfo)
{
	Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(id));
	textureAllocator.EnterGet();
	TextureRuntimeInfo& runtime = textureAllocator.Get<0>(resId);
	TextureLoadInfo& load = textureAllocator.Get<1>(resId);
	TextureMappingInfo& map = textureAllocator.Get<2>(resId);

	bool retval = false;
	if (Texture2D == runtime.type)
	{
		VkFormat vkformat = VkTypes::AsVkFormat(load.format);
		VkTypes::VkBlockDimensions blockSize = VkTypes::AsVkBlockSize(vkformat);
		uint32_t size = CoreGraphics::PixelFormat::ToSize(load.format);

		uint32_t mipWidth = (uint32_t)Math::n_max(1.0f, Math::n_ceil(load.dims.width / Math::n_pow(2, (float)mipLevel)));
		uint32_t mipHeight = (uint32_t)Math::n_max(1.0f, Math::n_ceil(load.dims.height / Math::n_pow(2, (float)mipLevel)));

		map.region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		map.region.dstOffset = { 0, 0, 0 };
		map.region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mipLevel, 0, 1 };
		map.region.srcOffset = { 0, 0, 0 };
		map.region.extent = { mipWidth, mipHeight, 1 };
		uint32_t memSize;
		VkUtilities::ReadImage(load.img, load.format, load.dims, runtime.type, map.region, memSize, map.mem, map.buf);

		// the row pitch must be the size of one pixel times the number of pixels in width
		outMapInfo.mipWidth = mipWidth;
		outMapInfo.mipHeight = mipHeight;
		outMapInfo.rowPitch = (int32_t)memSize / mipHeight;
		outMapInfo.depthPitch = (int32_t)memSize;
		VkResult res = vkMapMemory(VkRenderDevice::dev, map.mem, 0, (int32_t)memSize, 0, &outMapInfo.data);
		n_assert(res == VK_SUCCESS);
		retval = res == VK_SUCCESS;
		map.mapCount++;
	}
	else if (Texture3D == runtime.type)
	{
		VkFormat vkformat = VkTypes::AsVkFormat(load.format);
		VkTypes::VkBlockDimensions blockSize = VkTypes::AsVkBlockSize(vkformat);
		uint32_t size = CoreGraphics::PixelFormat::ToSize(load.format);

		uint32_t mipWidth = (uint32_t)Math::n_max(1.0f, Math::n_ceil(load.dims.width / Math::n_pow(2, (float)mipLevel)));
		uint32_t mipHeight = (uint32_t)Math::n_max(1.0f, Math::n_ceil(load.dims.height / Math::n_pow(2, (float)mipLevel)));
		uint32_t mipDepth = (uint32_t)Math::n_max(1.0f, Math::n_ceil(load.dims.depth / Math::n_pow(2, (float)mipLevel)));

		map.region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		map.region.dstOffset = { 0, 0, 0 };
		map.region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mipLevel, 1, 1 };
		map.region.srcOffset = { 0, 0, 0 };
		map.region.extent = { mipWidth, mipHeight, mipDepth };
		uint32_t memSize;
		VkUtilities::ReadImage(load.img, load.format, load.dims, runtime.type, map.region, memSize, map.mem, map.buf);

		// the row pitch must be the size of one pixel times the number of pixels in width
		outMapInfo.mipWidth = mipWidth;
		outMapInfo.mipHeight = mipHeight;
		outMapInfo.rowPitch = (int32_t)memSize / mipWidth;
		outMapInfo.depthPitch = (int32_t)memSize;
		VkResult res = vkMapMemory(VkRenderDevice::dev, map.mem, 0, (int32_t)memSize, 0, &outMapInfo.data);
		n_assert(res == VK_SUCCESS);
		retval = res == VK_SUCCESS;
		map.mapCount++;
	}
	textureAllocator.LeaveGet();
	return retval;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkMemoryTexturePool::Unmap(const CoreGraphics::TextureId id, IndexT mipLevel)
{
	Ids::Id24 resId = Resources::SharedId(id);
	textureAllocator.EnterGet();
	TextureRuntimeInfo& runtime = textureAllocator.Get<0>(resId);
	TextureLoadInfo& load = textureAllocator.Get<1>(resId);
	TextureMappingInfo& map = textureAllocator.Get<2>(resId);

	// unmap and dealloc
	vkUnmapMemory(VkRenderDevice::dev, load.mem);
	VkUtilities::WriteImage(map.buf, load.img, map.region);
	map.mapCount--;
	if (map.mapCount == 0)
	{
		vkFreeMemory(VkRenderDevice::dev, map.mem, nullptr);
		vkDestroyBuffer(VkRenderDevice::dev, map.buf, nullptr);
	}

	textureAllocator.LeaveGet();
}

//------------------------------------------------------------------------------
/**
*/
inline bool
VkMemoryTexturePool::MapCubeFace(const CoreGraphics::TextureId id, CoreGraphics::TextureCubeFace face, IndexT mipLevel, CoreGraphics::GpuBufferTypes::MapType mapType, CoreGraphics::TextureMapInfo & outMapInfo)
{
	Ids::Id24 resId = Resources::SharedId(id);
	textureAllocator.EnterGet();
	TextureRuntimeInfo& runtime = textureAllocator.Get<0>(resId);
	TextureLoadInfo& load = textureAllocator.Get<1>(resId);
	TextureMappingInfo& map = textureAllocator.Get<2>(resId);

	bool retval = false;

	VkFormat vkformat = VkTypes::AsVkFormat(load.format);
	VkTypes::VkBlockDimensions blockSize = VkTypes::AsVkBlockSize(vkformat);
	uint32_t size = CoreGraphics::PixelFormat::ToSize(load.format);

	uint32_t mipWidth = (uint32_t)Math::n_max(1.0f, Math::n_ceil(load.dims.width / Math::n_pow(2, (float)mipLevel)));
	uint32_t mipHeight = (uint32_t)Math::n_max(1.0f, Math::n_ceil(load.dims.height / Math::n_pow(2, (float)mipLevel)));

	map.region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	map.region.dstOffset = { 0, 0, 0 };
	map.region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mipLevel, (uint32_t)face, 1 };
	map.region.srcOffset = { 0, 0, 0 };
	map.region.extent = { mipWidth, mipHeight, 1 };
	uint32_t memSize;
	VkUtilities::ReadImage(load.img, load.format, load.dims, runtime.type, map.region, memSize, map.mem, map.buf);

	// the row pitch must be the size of one pixel times the number of pixels in width
	outMapInfo.mipWidth = mipWidth;
	outMapInfo.mipHeight = mipHeight;
	outMapInfo.rowPitch = (int32_t)memSize / mipWidth;
	outMapInfo.depthPitch = (int32_t)memSize;
	VkResult res = vkMapMemory(VkRenderDevice::dev, map.mem, 0, (int32_t)memSize, 0, &outMapInfo.data);
	n_assert(res == VK_SUCCESS);
	retval = res == VK_SUCCESS;
	map.mapCount++;

	textureAllocator.LeaveGet();

	return retval;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkMemoryTexturePool::UnmapCubeFace(const CoreGraphics::TextureId id, CoreGraphics::TextureCubeFace face, IndexT mipLevel)
{
	Ids::Id24 resId = Resources::SharedId(id);
	textureAllocator.EnterGet();
	TextureRuntimeInfo& runtime = textureAllocator.Get<0>(resId);
	TextureLoadInfo& load = textureAllocator.Get<1>(resId);
	TextureMappingInfo& map = textureAllocator.Get<2>(resId);

	// unmap and dealloc
	vkUnmapMemory(VkRenderDevice::dev, load.mem);
	VkUtilities::WriteImage(map.buf, load.img, map.region);
	map.mapCount--;
	if (map.mapCount == 0)
	{
		vkFreeMemory(VkRenderDevice::dev, map.mem, nullptr);
		vkDestroyBuffer(VkRenderDevice::dev, map.buf, nullptr);
	}

	textureAllocator.LeaveGet();
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkMemoryTexturePool::Update(const CoreGraphics::TextureId id, void * data, SizeT dataSize, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip)
{
	VkBufferImageCopy copy;
	copy.imageExtent.width = width;
	copy.imageExtent.height = height;
	copy.imageExtent.depth = 1;			// hmm, might want this for cube maps and volume textures too
	copy.imageOffset.x = left;
	copy.imageOffset.y = top;
	copy.imageOffset.z = 0;
	copy.imageSubresource.mipLevel = mip;
	copy.imageSubresource.layerCount = 1;
	copy.imageSubresource.baseArrayLayer = 0;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.bufferOffset = 0;
	copy.bufferRowLength = dataSize / width;
	copy.bufferImageHeight = height;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkMemoryTexturePool::Update(const CoreGraphics::TextureId id, CoreGraphics::TextureDimensions dims, void * data, SizeT dataSize, IndexT mip)
{
	VkBufferImageCopy copy;
	copy.imageExtent.width = dims.width;
	copy.imageExtent.height = dims.height;
	copy.imageExtent.depth = 1;			// hmm, might want this for cube maps and volume textures too
	copy.imageOffset.x = 0;
	copy.imageOffset.y = 0;
	copy.imageOffset.z = 0;
	copy.imageSubresource.mipLevel = mip;
	copy.imageSubresource.layerCount = 1;
	copy.imageSubresource.baseArrayLayer = 0;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.bufferOffset = 0;
	copy.bufferRowLength = dataSize / dims.width;
	copy.bufferImageHeight = dims.height;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkMemoryTexturePool::UpdateArray(const CoreGraphics::TextureId id, void * data, SizeT dataSize, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip, IndexT layer)
{
	VkBufferImageCopy copy;
	copy.imageExtent.width = width;
	copy.imageExtent.height = height;
	copy.imageExtent.depth = 1;			// hmm, might want this for cube maps and volume textures too
	copy.imageOffset.x = left;
	copy.imageOffset.y = top;
	copy.imageOffset.z = 0;
	copy.imageSubresource.mipLevel = mip;
	copy.imageSubresource.layerCount = 1;
	copy.imageSubresource.baseArrayLayer = layer;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.bufferOffset = 0;
	copy.bufferRowLength = dataSize / width;
	copy.bufferImageHeight = height;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkMemoryTexturePool::UpdateArray(const CoreGraphics::TextureId id, CoreGraphics::TextureDimensions dims, void * data, SizeT dataSize, IndexT mip, IndexT layer)
{
	VkBufferImageCopy copy;
	copy.imageExtent.width = dims.width;
	copy.imageExtent.height = dims.height;
	copy.imageExtent.depth = 1;			// hmm, might want this for cube maps and volume textures too
	copy.imageOffset.x = 0;
	copy.imageOffset.y = 0;
	copy.imageOffset.z = 0;
	copy.imageSubresource.mipLevel = mip;
	copy.imageSubresource.layerCount = 1;
	copy.imageSubresource.baseArrayLayer = layer;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.bufferOffset = 0;
	copy.bufferRowLength = dataSize / dims.width;
	copy.bufferImageHeight = dims.height;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkMemoryTexturePool::Copy(const CoreGraphics::TextureId from, const CoreGraphics::TextureId to, SizeT width, SizeT height, SizeT depth, IndexT srcMip, IndexT srcLayer, SizeT srcXOffset, SizeT srcYOffset, SizeT srcZOffset, IndexT dstMip, IndexT dstLayer, SizeT dstXOffset, SizeT dstYOffset, SizeT dstZOffset)
{
	VkImageCopy copy;
	copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.srcSubresource.baseArrayLayer = srcLayer;
	copy.srcSubresource.layerCount = 1;
	copy.srcSubresource.mipLevel = srcMip;
	copy.srcOffset = { srcXOffset, srcYOffset, srcZOffset };

	copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.dstSubresource.baseArrayLayer = dstLayer;
	copy.dstSubresource.layerCount = 1;
	copy.dstSubresource.mipLevel = dstMip;
	copy.dstOffset = { dstXOffset, dstYOffset, dstZOffset };

	copy.extent = { width, height, depth };

	TextureLoadInfo& fromLoad = textureAllocator.Get<1>(Resources::SharedId(from));
	TextureLoadInfo& toLoad = textureAllocator.Get<1>(Resources::SharedId(to));

	// begin immediate action, this might actually be delayed but we can't really know from here
	VkCommandBuffer cmdBuf = VkUtilities::BeginImmediateTransfer();
	vkCmdCopyImage(cmdBuf, fromLoad.img, VK_IMAGE_LAYOUT_GENERAL, toLoad.img, VK_IMAGE_LAYOUT_GENERAL, 1, &copy);
	VkUtilities::EndImmediateTransfer(cmdBuf);
}

} // namespace Vulkan