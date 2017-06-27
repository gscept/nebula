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

__ImplementClass(Vulkan::VkTexture, 'VKTX', Base::TextureBase);
//------------------------------------------------------------------------------
/**
*/
VkTexture::VkTexture() :
	mapCount(0),
	mappedImg(VK_NULL_HANDLE),
	mappedMem(VK_NULL_HANDLE),
	id(NULL)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkTexture::~VkTexture()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::Unload()
{
	n_assert(this->mapCount == 0);
	vkDestroyImage(VkRenderDevice::dev, this->img, NULL);
	vkDestroyImageView(VkRenderDevice::dev, this->imgView, NULL);
	vkFreeMemory(VkRenderDevice::dev, this->mem, NULL);
	TextureBase::Unload();
}

//------------------------------------------------------------------------------
/**
*/
bool
VkTexture::Map(IndexT mipLevel, MapType mapType, MapInfo& outMapInfo)
{
	bool retval = false;
	if (Texture2D == this->type)
	{		
		VkFormat format = VkTypes::AsVkFormat(this->pixelFormat);
		VkTypes::VkBlockDimensions blockSize = VkTypes::AsVkBlockSize(this->pixelFormat);
		uint32_t size = CoreGraphics::PixelFormat::ToSize(this->pixelFormat);

		uint32_t mipWidth = (uint32_t)Math::n_max(1.0f, Math::n_ceil(this->width / Math::n_pow(2, (float)mipLevel)));
		uint32_t mipHeight = (uint32_t)Math::n_max(1.0f, Math::n_ceil(this->height / Math::n_pow(2, (float)mipLevel)));

		this->mappedBufferLayout.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		this->mappedBufferLayout.dstOffset = { 0, 0, 0 };
		this->mappedBufferLayout.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mipLevel, 0, 1 };
		this->mappedBufferLayout.srcOffset = { 0, 0, 0 };
		this->mappedBufferLayout.extent = { mipWidth, mipHeight, 1 };
		uint32_t memSize;
		VkUtilities::ReadImage(this, this->mappedBufferLayout, memSize, this->mappedMem, this->mappedImg);

		// the row pitch must be the size of one pixel times the number of pixels in width
		outMapInfo.mipWidth = mipWidth;
		outMapInfo.mipHeight = mipHeight;
		outMapInfo.rowPitch = (int32_t)memSize / mipHeight;
		outMapInfo.depthPitch = (int32_t)memSize;
		VkResult res = vkMapMemory(VkRenderDevice::dev, this->mappedMem, 0, (int32_t)memSize, 0, &this->mappedData);
		n_assert(res == VK_SUCCESS);

		outMapInfo.data = this->mappedData;
		retval = res == VK_SUCCESS;
		this->mapCount++;
	}
	else if (Texture3D == this->type)
	{
		VkFormat format = VkTypes::AsVkFormat(this->pixelFormat);
		VkTypes::VkBlockDimensions blockSize = VkTypes::AsVkBlockSize(this->pixelFormat);
		uint32_t size = CoreGraphics::PixelFormat::ToSize(this->pixelFormat);

		uint32_t mipWidth = (uint32_t)Math::n_max(1.0f, Math::n_ceil(this->width / Math::n_pow(2, (float)mipLevel)));
		uint32_t mipHeight = (uint32_t)Math::n_max(1.0f, Math::n_ceil(this->height / Math::n_pow(2, (float)mipLevel)));
		uint32_t mipDepth = (uint32_t)Math::n_max(1.0f, Math::n_ceil(this->depth / Math::n_pow(2, (float)mipLevel)));

		this->mappedBufferLayout.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		this->mappedBufferLayout.dstOffset = { 0, 0, 0 };
		this->mappedBufferLayout.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mipLevel, 1, 1 };
		this->mappedBufferLayout.srcOffset = { 0, 0, 0 };
		this->mappedBufferLayout.extent = { mipWidth, mipHeight, mipDepth };
		uint32_t memSize;
		VkUtilities::ReadImage(this, this->mappedBufferLayout, memSize, this->mappedMem, this->mappedImg);

		// the row pitch must be the size of one pixel times the number of pixels in width
		outMapInfo.mipWidth = mipWidth;
		outMapInfo.mipHeight = mipHeight;
		outMapInfo.rowPitch = (int32_t)memSize / mipWidth;
		outMapInfo.depthPitch = (int32_t)memSize;
		VkResult res = vkMapMemory(VkRenderDevice::dev, this->mappedMem, 0, (int32_t)memSize, 0, &this->mappedData);
		n_assert(res == VK_SUCCESS);

		outMapInfo.data = this->mappedData;
		retval = res == VK_SUCCESS;
		this->mapCount++;
	}
	return retval;
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::Unmap(IndexT mipLevel)
{
	n_assert(this->mapCount > 0);

	// unmap and dealloc
	vkUnmapMemory(VkRenderDevice::dev, this->mappedMem);
	VkUtilities::WriteImage(this->mappedImg, this->img, this->mappedBufferLayout);
	this->mapCount--;

	if (this->mapCount == 0)
	{
		vkFreeMemory(VkRenderDevice::dev, this->mappedMem, NULL);
		vkDestroyBuffer(VkRenderDevice::dev, this->mappedImg, NULL);

		this->mappedData = 0;
		this->mappedImg = VK_NULL_HANDLE;
		this->mappedMem = VK_NULL_HANDLE;
		this->mappedBufferLayout = VkImageCopy();
	}
}

//------------------------------------------------------------------------------
/**
*/
bool
VkTexture::MapCubeFace(CubeFace face, IndexT mipLevel, MapType mapType, MapInfo& outMapInfo)
{
	bool retval = false;
	
	VkFormat format = VkTypes::AsVkFormat(this->pixelFormat);
	VkTypes::VkBlockDimensions blockSize = VkTypes::AsVkBlockSize(this->pixelFormat);
	uint32_t size = CoreGraphics::PixelFormat::ToSize(this->pixelFormat);

	uint32_t mipWidth = (uint32_t)Math::n_max(1.0f, Math::n_ceil(this->width / Math::n_pow(2, (float)mipLevel)));
	uint32_t mipHeight = (uint32_t)Math::n_max(1.0f, Math::n_ceil(this->height / Math::n_pow(2, (float)mipLevel)));

	this->mappedBufferLayout.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	this->mappedBufferLayout.dstOffset = { 0, 0, 0 };
	this->mappedBufferLayout.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mipLevel, (uint32_t)face, 1 };
	this->mappedBufferLayout.srcOffset = { 0, 0, 0 };
	this->mappedBufferLayout.extent = { mipWidth, mipHeight, 1 };
	uint32_t memSize;
	VkUtilities::ReadImage(this, this->mappedBufferLayout, memSize, this->mappedMem, this->mappedImg);

	// the row pitch must be the size of one pixel times the number of pixels in width
	outMapInfo.mipWidth = mipWidth;
	outMapInfo.mipHeight = mipHeight;
	outMapInfo.rowPitch = (int32_t)memSize / mipWidth;
	outMapInfo.depthPitch = (int32_t)memSize;
	VkResult res = vkMapMemory(VkRenderDevice::dev, this->mappedMem, 0, (int32_t)memSize, 0, &this->mappedData);
	n_assert(res == VK_SUCCESS);

	outMapInfo.data = this->mappedData;
	retval = res == VK_SUCCESS;
	this->mapCount++;

	return retval;
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::UnmapCubeFace(CubeFace face, IndexT mipLevel)
{
	n_assert(this->mapCount > 0);
	// unmap and dealloc
	vkUnmapMemory(VkRenderDevice::dev, this->mappedMem);
	VkUtilities::WriteImage(this->mappedImg, this->img, this->mappedBufferLayout);
	this->mapCount--;

	if (this->mapCount == 0)
	{
		vkFreeMemory(VkRenderDevice::dev, this->mappedMem, NULL);
		vkDestroyBuffer(VkRenderDevice::dev, this->mappedImg, NULL);

		this->mappedData = 0;
		this->mappedImg = VK_NULL_HANDLE;
		this->mappedMem = VK_NULL_HANDLE;
		this->mappedBufferLayout = VkImageCopy();
	}	
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
VkTexture::Update(void* data, SizeT dataSize, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip)
{
	n_assert(this->img != VK_NULL_HANDLE);
	n_assert(this->mem != VK_NULL_HANDLE);

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
	//VkRenderDevice::Instance()->PushImageUpdate(this->img, copy, dataSize, (uint32_t*)data);
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::Update(void* data, SizeT dataSize, IndexT mip)
{
	n_assert(this->img != VK_NULL_HANDLE);
	n_assert(this->mem != VK_NULL_HANDLE);

	VkBufferImageCopy copy;
	copy.imageExtent.width = this->width;
	copy.imageExtent.height = this->height;
	copy.imageExtent.depth = 1;			// hmm, might want this for cube maps and volume textures too
	copy.imageOffset.x = 0;
	copy.imageOffset.y = 0;
	copy.imageOffset.z = 0;
	copy.imageSubresource.mipLevel = mip;
	copy.imageSubresource.layerCount = 1;
	copy.imageSubresource.baseArrayLayer = 0;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.bufferOffset = 0;
	copy.bufferRowLength = dataSize / this->width;
	copy.bufferImageHeight = this->height;

	// push update
	//VkRenderDevice::Instance()->PushImageUpdate(this->img, copy, dataSize, (uint32_t*)data);
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::UpdateArray(void* data, SizeT dataSize, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip, IndexT layer)
{
	n_assert(this->img != VK_NULL_HANDLE);
	n_assert(this->mem != VK_NULL_HANDLE);

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
VkTexture::UpdateArray(void* data, SizeT dataSize, IndexT mip, IndexT layer)
{
	n_assert(this->img != VK_NULL_HANDLE);
	n_assert(this->mem != VK_NULL_HANDLE);

	VkBufferImageCopy copy;
	copy.imageExtent.width = this->width;
	copy.imageExtent.height = this->height;
	copy.imageExtent.depth = 1;			// hmm, might want this for cube maps and volume textures too
	copy.imageOffset.x = 0;
	copy.imageOffset.y = 0;
	copy.imageOffset.z = 0;
	copy.imageSubresource.mipLevel = mip;
	copy.imageSubresource.layerCount = 1;
	copy.imageSubresource.baseArrayLayer = layer;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.bufferOffset = 0;
	copy.bufferRowLength = dataSize / this->width;
	copy.bufferImageHeight = this->height;

	// push update
	//VkRenderDevice::Instance()->PushImageUpdate(this->img, copy, dataSize, (uint32_t*)data);
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::SetupFromVkTexture(VkImage img, VkDeviceMemory mem, VkImageView imgView, uint32_t width, uint32_t height, CoreGraphics::PixelFormat::Code format, uint32_t numMips /*= 0*/, const bool setLoaded /*= true*/, const bool isAttachment /*= false*/)
{
	this->type = Texture2D;
	this->pixelFormat = format;
	this->img = img;
	this->numMipLevels = numMips;
	this->mem = mem;
	this->imgView = imgView;
	this->id = VkShaderServer::Instance()->RegisterTexture(this);

	this->SetType(VkTexture::Texture2D);
	this->SetWidth(width);
	this->SetHeight(height);
	this->SetDepth(1);
	this->SetNumMipLevels(Math::n_max(1, numMips));
	this->SetPixelFormat(format);
	this->access = GpuResourceBase::AccessRead;
	this->isRenderTargetAttachment = isAttachment;
	if (setLoaded)
	{
		this->SetState(Resource::Loaded);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::SetupFromVkMultisampleTexture(VkImage img, VkDeviceMemory mem, VkImageView imgView, uint32_t width, uint32_t height, CoreGraphics::PixelFormat::Code format, uint32_t numMips /*= 0*/, const bool setLoaded /*= true*/, const bool isAttachment /*= false*/)
{
	this->type = Texture2D;
	this->pixelFormat = format;
	this->img = img;
	this->numMipLevels = numMips;
	this->mem = mem;
	this->imgView = imgView;
	this->id = VkShaderServer::Instance()->RegisterTexture(this);

	this->SetWidth(width);
	this->SetHeight(height);
	this->SetDepth(1);
	this->SetNumMipLevels(Math::n_max(1, numMips));
	this->SetPixelFormat(format);
	this->access = GpuResourceBase::AccessRead;
	this->isRenderTargetAttachment = isAttachment;
	if (setLoaded)
	{
		this->SetState(Resource::Loaded);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::SetupFromVkCubeTexture(VkImage img, VkDeviceMemory mem, VkImageView imgView, uint32_t width, uint32_t height, CoreGraphics::PixelFormat::Code format, uint32_t numMips /*= 0*/, const bool setLoaded /*= true*/, const bool isAttachment /*= false*/)
{
	this->type = TextureCube;
	this->pixelFormat = format;
	this->img = img;
	this->numMipLevels = numMips;
	this->mem = mem;
	this->imgView = imgView;
	this->id = VkShaderServer::Instance()->RegisterTexture(this);

	this->SetWidth(width);
	this->SetHeight(height);
	this->SetDepth(1);
	this->SetNumMipLevels(Math::n_max(1, numMips));
	this->SetPixelFormat(format);
	this->access = GpuResourceBase::AccessRead;
	this->isRenderTargetAttachment = isAttachment;
	if (setLoaded)
	{
		this->SetState(Resource::Loaded);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::SetupFromVkVolumeTexture(VkImage img, VkDeviceMemory mem, VkImageView imgView, uint32_t width, uint32_t height, uint32_t depth, CoreGraphics::PixelFormat::Code format, uint32_t numMips /*= 0*/, const bool setLoaded /*= true*/, const bool isAttachment /*= false*/)
{
	this->type = Texture3D;
	this->pixelFormat = format;
	this->img = img;
	this->numMipLevels = numMips;
	this->mem = mem;
	this->imgView = imgView;
	this->id = VkShaderServer::Instance()->RegisterTexture(this);

	this->SetWidth(width);
	this->SetHeight(height);
	this->SetDepth(depth);
	this->SetNumMipLevels(Math::n_max(1, numMips));
	this->SetPixelFormat(format);
	this->access = GpuResourceBase::AccessRead;
	this->isRenderTargetAttachment = isAttachment;
	if (setLoaded)
	{
		this->SetState(Resource::Loaded);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::SetupFromVkBackbuffer(VkImage img, uint32_t width, uint32_t height, uint32_t depth, CoreGraphics::PixelFormat::Code format)
{
	this->type = Texture2D;
	this->pixelFormat = format;
	this->img = img;
	this->numMipLevels = 1;
	this->mem = VK_NULL_HANDLE;
	this->imgView = VK_NULL_HANDLE;

	this->SetWidth(width);
	this->SetHeight(height);
	this->SetDepth(depth);
	this->SetNumMipLevels(1);
	this->SetPixelFormat(format);
	this->access = GpuResourceBase::AccessRead;
	this->isRenderTargetAttachment = true;
	this->SetState(Resource::Loaded);
}

//------------------------------------------------------------------------------
/**
*/
void
VkTexture::Copy(const Ptr<CoreGraphics::Texture>& from, const Ptr<CoreGraphics::Texture>& to, uint32_t width, uint32_t height, uint32_t depth,
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
	vkCmdCopyImage(cmdBuf, from->GetVkImage(), VK_IMAGE_LAYOUT_GENERAL, to->GetVkImage(), VK_IMAGE_LAYOUT_GENERAL, 1, &copy);
	VkUtilities::EndImmediateTransfer(cmdBuf);
}

} // namespace Vulkan
