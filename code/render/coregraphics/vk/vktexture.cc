//------------------------------------------------------------------------------
// vktexture.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vktexture.h"
#include "vkrenderdevice.h"
#include "coregraphics/pixelformat.h"
#include "vktypes.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/config.h"
#include "vkshaderserver.h"
#include "vkutilities.h"

namespace Vulkan
{

VkTexture::VkTextureAllocator VkTexture::textureAllocator(0x00FFFFFF);

//------------------------------------------------------------------------------
/**
*/
bool
VkTexture::Map(const Resources::ResourceId id, IndexT mipLevel, Base::GpuResourceBase::MapType mapType, Base::TextureBase::MapInfo& outMapInfo)
{
	Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(id));
	VkTexture::textureAllocator.EnterGet();
	VkTexture::RuntimeInfo& runtime = VkTexture::textureAllocator.Get<0>(resId);
	VkTexture::LoadInfo& load = VkTexture::textureAllocator.Get<1>(resId);
	VkTexture::MappingInfo& map = VkTexture::textureAllocator.Get<2>(resId);	

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
	VkTexture::textureAllocator.LeaveGet();
	return retval;
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::Unmap(const Resources::ResourceId id, IndexT mipLevel)
{
	Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(id));
	VkTexture::textureAllocator.EnterGet();
	VkTexture::RuntimeInfo& runtime = VkTexture::textureAllocator.Get<0>(resId);
	VkTexture::LoadInfo& load = VkTexture::textureAllocator.Get<1>(resId);
	VkTexture::MappingInfo& map = VkTexture::textureAllocator.Get<2>(resId);

	// unmap and dealloc
	vkUnmapMemory(VkRenderDevice::dev, load.mem);
	VkUtilities::WriteImage(map.buf, load.img, map.region);
	map.mapCount--;
	if (map.mapCount == 0)
	{
		vkFreeMemory(VkRenderDevice::dev, map.mem, nullptr);
		vkDestroyBuffer(VkRenderDevice::dev, map.buf, nullptr);
	}

	VkTexture::textureAllocator.LeaveGet();
}

//------------------------------------------------------------------------------
/**
*/
bool
VkTexture::MapCubeFace(const Resources::ResourceId id, Base::TextureBase::CubeFace face, IndexT mipLevel, Base::GpuResourceBase::MapType mapType, Base::TextureBase::MapInfo& outMapInfo)
{
	Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(id));
	VkTexture::textureAllocator.EnterGet();
	VkTexture::RuntimeInfo& runtime = VkTexture::textureAllocator.Get<0>(resId);
	VkTexture::LoadInfo& load = VkTexture::textureAllocator.Get<1>(resId);
	VkTexture::MappingInfo& map = VkTexture::textureAllocator.Get<2>(resId);

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
	
	VkTexture::textureAllocator.LeaveGet();

	return retval;
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::UnmapCubeFace(const Resources::ResourceId id, Base::TextureBase::CubeFace face, IndexT mipLevel)
{
	Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(id));
	VkTexture::textureAllocator.EnterGet();
	VkTexture::RuntimeInfo& runtime = VkTexture::textureAllocator.Get<0>(resId);
	VkTexture::LoadInfo& load = VkTexture::textureAllocator.Get<1>(resId);
	VkTexture::MappingInfo& map = VkTexture::textureAllocator.Get<2>(resId);

	// unmap and dealloc
	vkUnmapMemory(VkRenderDevice::dev, load.mem);
	VkUtilities::WriteImage(map.buf, load.img, map.region);
	map.mapCount--;
	if (map.mapCount == 0)
	{
		vkFreeMemory(VkRenderDevice::dev, map.mem, nullptr);
		vkDestroyBuffer(VkRenderDevice::dev, map.buf, nullptr);
	}

	VkTexture::textureAllocator.LeaveGet();
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::GenerateMipmaps()
{
	// implement some way to generate mipmaps here, perhaps a compute shader is the easiest...
	// hmm, perhaps we can use a sequence of subpasses here to create our mip chain
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::Update(VkImage img, void* data, SizeT dataSize, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip)
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

	// push update
	//VkRenderDevice::Instance()->PushImageUpdate(img, copy, dataSize, (uint32_t*)data);
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::Update(VkImage img, TextureBase::Dimensions dims, void* data, SizeT dataSize, IndexT mip)
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

	// push update
	//VkRenderDevice::Instance()->PushImageUpdate(this->img, copy, dataSize, (uint32_t*)data);
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::UpdateArray(VkImage img, void* data, SizeT dataSize, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip, IndexT layer)
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

	// push update
	//VkRenderDevice::Instance()->PushImageUpdate(this->img, copy, dataSize, (uint32_t*)data);
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::UpdateArray(VkImage img, TextureBase::Dimensions dims, void* data, SizeT dataSize, IndexT mip, IndexT layer)
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

	// push update
	//VkRenderDevice::Instance()->PushImageUpdate(this->img, copy, dataSize, (uint32_t*)data);
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::Copy(const VkImage from, const VkImage to, uint32_t width, uint32_t height, uint32_t depth,
	uint32_t srcMip, uint32_t srcLayer, int32_t srcXOffset, int32_t srcYOffset, int32_t srcZOffset,
	uint32_t dstMip, uint32_t dstLayer, int32_t dstXOffset, int32_t dstYOffset, int32_t dstZOffset)
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

	// begin immediate action, this might actually be delayed but we can't really know from here
	VkCommandBuffer cmdBuf = VkUtilities::BeginImmediateTransfer();
	vkCmdCopyImage(cmdBuf, from, VK_IMAGE_LAYOUT_GENERAL, to, VK_IMAGE_LAYOUT_GENERAL, 1, &copy);
	VkUtilities::EndImmediateTransfer(cmdBuf);
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::Unload(const VkDeviceMemory mem, const VkImage img, const VkImageView view)
{
	vkFreeMemory(VkRenderDevice::dev, mem, nullptr);
	vkDestroyImage(VkRenderDevice::dev, img, nullptr);
	vkDestroyImageView(VkRenderDevice::dev, img, nullptr);
}

} // namespace Vulkan
