//------------------------------------------------------------------------------
// vkutilities.cc
// (C) 2017-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkutilities.h"
#include "vkgraphicsdevice.h"
#include "coregraphics/config.h"
#include "vkcommandbuffer.h"
#include "vktypes.h"

namespace Vulkan
{

//------------------------------------------------------------------------------
/**
*/
VkUtilities::VkUtilities()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkUtilities::~VkUtilities()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkUtilities::ImageBarrier(CoreGraphics::CommandBufferId cmd, CoreGraphics::BarrierStage left, CoreGraphics::BarrierStage right, VkImageMemoryBarrier barrier)
{
	// execute command
	vkCmdPipelineBarrier(CommandBufferGetVk(cmd),
		VkTypes::AsVkPipelineFlags(left), VkTypes::AsVkPipelineFlags(right),
		0,
		0, VK_NULL_HANDLE,
		0, VK_NULL_HANDLE,
		1, &barrier);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkUtilities::BufferUpdate(CoreGraphics::CommandBufferId cmd, VkBuffer buf, VkDeviceSize offset, VkDeviceSize size, const void* data)
{
	VkDeviceSize totalSize = size;
	VkDeviceSize totalOffset = offset;
	while (totalSize > 0)
	{
		const uint8_t* ptr = (const uint8_t*)data + totalOffset;
		VkDeviceSize uploadSize = totalSize < 65536 ? totalSize : 65536;
		vkCmdUpdateBuffer(CommandBufferGetVk(cmd), buf, totalOffset, uploadSize, (const uint32_t*)ptr);
		totalSize -= uploadSize;
		totalOffset += uploadSize;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
VkUtilities::ImageUpdate(VkDevice dev, CoreGraphics::CommandBufferId cmd, CoreGraphicsQueueType queue, VkImage img, const VkImageCreateInfo& info, uint32_t mip, uint32_t face, VkDeviceSize size, uint32_t* data, VkBuffer& outIntermediateBuffer, VkDeviceMemory& outIntermediateMemory)
{
	// create transfer buffer
	const uint32_t qfamily = Vulkan::GetQueueFamily(queue);
	VkBufferCreateInfo bufInfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		NULL,
		0,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		1,
		&qfamily
	};
	VkBuffer buf;
	vkCreateBuffer(dev, &bufInfo, NULL, &buf);

	// allocate memory
	VkDeviceMemory bufMem;
	uint32_t bufsize;
	VkUtilities::AllocateBufferMemory(dev, buf, bufMem, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, bufsize);
	vkBindBufferMemory(dev, buf, bufMem, 0);

	// map memory
	void* mapped;
	VkResult res = vkMapMemory(dev, bufMem, 0, size, 0, &mapped);
	n_assert(res == VK_SUCCESS);
	memcpy(mapped, data, VK_DEVICE_SIZE_CONV(size));
	vkUnmapMemory(dev, bufMem);

	// perform update of buffer, and stage a copy of buffer data to image
	VkBufferImageCopy copy;
	copy.bufferOffset = 0;
	copy.bufferImageHeight = 0;
	copy.bufferRowLength = 0;
	copy.imageExtent = info.extent;
	copy.imageOffset = { 0, 0, 0 };
	copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, mip, face, 1 };
	vkCmdCopyBufferToImage(CommandBufferGetVk(cmd), buf, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

	outIntermediateBuffer = buf;
	outIntermediateMemory = bufMem;
}

//------------------------------------------------------------------------------
/**
*/
void 
VkUtilities::ImageColorClear(CoreGraphics::CommandBufferId cmd, const VkImage& image, VkImageLayout layout, VkClearColorValue clearValue, VkImageSubresourceRange subres)
{
	vkCmdClearColorImage(CommandBufferGetVk(cmd), image, layout, &clearValue, 1, &subres);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkUtilities::ImageDepthStencilClear(CoreGraphics::CommandBufferId cmd, const VkImage& image, VkImageLayout layout, VkClearDepthStencilValue clearValue, VkImageSubresourceRange subres)
{
	vkCmdClearDepthStencilImage(CommandBufferGetVk(cmd), image, layout, &clearValue, 1, &subres);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkUtilities::Copy(CoreGraphics::CommandBufferId cmd, const VkImage from, Math::rectangle<SizeT> fromRegion, const VkImage to, Math::rectangle<SizeT> toRegion)
{
	VkImageCopy region;
	region.dstOffset = { fromRegion.left, fromRegion.top, 0 };
	region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	region.extent = { (uint32_t)toRegion.width(), (uint32_t)toRegion.height(), 1 };
	region.srcOffset = { toRegion.left, toRegion.top, 0 };
	region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	vkCmdCopyImage(CommandBufferGetVk(cmd), from, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, to, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkUtilities::Blit(CoreGraphics::CommandBufferId cmd, const VkImage from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const VkImage to, Math::rectangle<SizeT> toRegion, IndexT toMip)
{
	VkImageBlit blit;
	blit.srcOffsets[0] = { fromRegion.left, fromRegion.top, 0 };
	blit.srcOffsets[1] = { fromRegion.right, fromRegion.bottom, 1 };
	blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)fromMip, 0, 1 };
	blit.dstOffsets[0] = { toRegion.left, toRegion.top, 0 };
	blit.dstOffsets[1] = { toRegion.right, toRegion.bottom, 1 };
	blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)toMip, 0, 1 };
	vkCmdBlitImage(CommandBufferGetVk(cmd), from, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, to, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
}

//------------------------------------------------------------------------------
/**
*/
VkImageMemoryBarrier
VkUtilities::ImageMemoryBarrier(const VkImage& img, VkImageSubresourceRange subres, VkAccessFlags left, VkAccessFlags right, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier barrier;
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = NULL;
	barrier.image = img;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcAccessMask = left;
	barrier.dstAccessMask = right;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange = subres;
	return barrier;
}

//------------------------------------------------------------------------------
/**
*/
VkImageMemoryBarrier
VkUtilities::ImageMemoryBarrier(const VkImage& img, VkImageSubresourceRange subres, CoreGraphicsQueueType fromQueue, CoreGraphicsQueueType toQueue, VkAccessFlags left, VkAccessFlags right, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	uint32_t from = Vulkan::GetQueueFamily(fromQueue);
	uint32_t to = Vulkan::GetQueueFamily(toQueue);

	VkImageMemoryBarrier barrier;
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = NULL;
	barrier.image = img;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcAccessMask = left;
	barrier.dstAccessMask = right;
	barrier.srcQueueFamilyIndex = from;
	barrier.dstQueueFamilyIndex = to;
	barrier.subresourceRange = subres;
	return barrier;
}

//------------------------------------------------------------------------------
/**
*/
VkImageMemoryBarrier
VkUtilities::ImageMemoryBarrier(const VkImage& img, VkImageSubresourceRange subres, CoreGraphicsQueueType toQueue, VkAccessFlags left, VkAccessFlags right, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	uint32_t to = Vulkan::GetQueueFamily(toQueue);

	VkImageMemoryBarrier barrier;
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = NULL;
	barrier.image = img;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcAccessMask = left;
	barrier.dstAccessMask = right;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = to;
	barrier.subresourceRange = subres;
	return barrier;
}

//------------------------------------------------------------------------------
/**
*/
VkBufferMemoryBarrier
VkUtilities::BufferMemoryBarrier(const VkBuffer& buf, VkDeviceSize offset, VkDeviceSize size, VkAccessFlags srcAccess, VkAccessFlags dstAccess)
{
	VkBufferMemoryBarrier barrier;
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.pNext = NULL;
	barrier.buffer = buf;
	barrier.dstAccessMask = dstAccess;
	barrier.srcAccessMask = srcAccess;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.size = size;
	barrier.offset = offset;
	return barrier;
}

//------------------------------------------------------------------------------
/**
*/
void
VkUtilities::AllocateBufferMemory(const VkDevice dev, const VkBuffer& buf, VkDeviceMemory& bufmem, VkMemoryPropertyFlagBits flags, uint32_t& bufsize)
{
	// now attain memory requirements so we get a properly aligned memory storage
	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(dev, buf, &req);

	uint32_t memtype;
	VkResult err = VkUtilities::GetMemoryType(req.memoryTypeBits, flags, memtype);
	n_assert(err == VK_SUCCESS);
	VkMemoryAllocateInfo meminfo =
	{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		NULL,
		req.size,
		memtype
	};

	// now allocate memory
	err = vkAllocateMemory(dev, &meminfo, NULL, &bufmem);
	if (err == VK_ERROR_OUT_OF_DEVICE_MEMORY || err == VK_ERROR_OUT_OF_HOST_MEMORY)
	{
		#if __X64__
		n_error("VkRenderDevice::AllocateBufferMemory(): Could not allocate '%lld' bytes, out of memory\n.", req.size);
		#else
		n_error("VkRenderDevice::AllocateBufferMemory(): Could not allocate '%d' bytes, out of memory\n.", req.size);
		#endif
	}
	n_assert(err == VK_SUCCESS);
	bufsize = (uint32_t)req.size;
}

//------------------------------------------------------------------------------
/**
*/
void
VkUtilities::AllocateImageMemory(const VkDevice dev, const VkImage& img, VkDeviceMemory& imgmem, VkMemoryPropertyFlagBits flags, uint32_t& imgsize)
{
	// now attain memory requirements so we get a properly aligned memory storage
	VkMemoryRequirements req;
	vkGetImageMemoryRequirements(dev, img, &req);

	uint32_t memtype;
	VkResult err = VkUtilities::GetMemoryType(req.memoryTypeBits, flags, memtype);
	n_assert(err == VK_SUCCESS);
	VkMemoryAllocateInfo meminfo =
	{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		NULL,
		req.size,
		memtype
	};

	// now allocate memory
	err = vkAllocateMemory(dev, &meminfo, NULL, &imgmem);
	if (err == VK_ERROR_OUT_OF_DEVICE_MEMORY || err == VK_ERROR_OUT_OF_HOST_MEMORY)
	{
		#if __X64__
		n_error("VkRenderDevice::AllocateImageMemory(): Could not allocate '%lld' bytes, out of memory\n.", req.size);
		#else
		n_error("VkRenderDevice::AllocateImageMemory(): Could not allocate '%d' bytes, out of memory\n.", req.size);
		#endif
	}
	n_assert(err == VK_SUCCESS);
	imgsize = (uint32_t)req.size;
}


//------------------------------------------------------------------------------
/**
*/
VkResult
VkUtilities::GetMemoryType(uint32_t bits, VkMemoryPropertyFlags flags, uint32_t& index)
{
	VkPhysicalDeviceMemoryProperties props = Vulkan::GetMemoryProperties();
	for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
	{
		if ((bits & 1) == 1)
		{
			if ((props.memoryTypes[i].propertyFlags & flags) == flags)
			{
				index = i;
				return VK_SUCCESS;
			}
		}
		bits >>= 1;
	}
	return VK_ERROR_FEATURE_NOT_PRESENT;
}

//------------------------------------------------------------------------------
/**
*/
void
VkUtilities::ReadImage(const VkImage tex, CoreGraphics::PixelFormat::Code format, CoreGraphics::TextureDimensions dims, CoreGraphics::TextureType type, VkImageCopy copy, uint32_t& outMemSize, VkDeviceMemory& outMem, VkBuffer& outBuffer)
{
	VkDevice dev = Vulkan::GetCurrentDevice();
	CoreGraphics::CommandBufferId cmdBuf = VkUtilities::BeginImmediateTransfer();

	// find format of equal size so that we can decompress later
	VkFormat fmt = VkTypes::AsVkFormat(format);
	bool isCompressed = VkTypes::IsCompressedFormat(fmt);
	VkTypes::VkBlockDimensions bdims = VkTypes::AsVkBlockSize(format);
	VkExtent3D dstExtent;
	dstExtent = copy.extent;
	VkImageCreateInfo info =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		NULL,
		0,
		type == CoreGraphics::Texture2D ? VK_IMAGE_TYPE_2D :
		type == CoreGraphics::Texture3D ? VK_IMAGE_TYPE_3D :
		type == CoreGraphics::TextureCube ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_1D,
		fmt,
		dstExtent,
		1,
		type == CoreGraphics::TextureCube ? 6u : type == CoreGraphics::Texture3D ? (uint32_t)dims.depth : 1u,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		NULL,
		VK_IMAGE_LAYOUT_UNDEFINED
	};
	VkImage img;
	VkResult res = vkCreateImage(dev, &info, NULL, &img);
	n_assert(res == VK_SUCCESS);

	// allocate memory
	VkDeviceMemory imgMem;
	uint32_t memSize;
	VkUtilities::AllocateImageMemory(dev, img, imgMem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memSize);
	vkBindImageMemory(dev, img, imgMem, 0);

	VkImageSubresourceRange srcSubres;
	srcSubres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	srcSubres.baseArrayLayer = copy.srcSubresource.baseArrayLayer;
	srcSubres.layerCount = copy.srcSubresource.layerCount;
	srcSubres.baseMipLevel = copy.srcSubresource.mipLevel;
	srcSubres.levelCount = 1;
	VkImageSubresourceRange dstSubres;
	dstSubres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	dstSubres.baseArrayLayer = copy.dstSubresource.baseArrayLayer;
	dstSubres.layerCount = copy.dstSubresource.layerCount;
	dstSubres.baseMipLevel = copy.dstSubresource.mipLevel;
	dstSubres.levelCount = 1;

	VkImageBlit blit;
	blit.srcSubresource = copy.srcSubresource;
	blit.srcOffsets[0] = copy.srcOffset;
	blit.srcOffsets[1] = { (int32_t)copy.extent.width, (int32_t)copy.extent.height, (int32_t)copy.extent.depth };

	blit.dstSubresource = copy.dstSubresource;
	blit.dstOffsets[0] = copy.dstOffset;
	blit.dstOffsets[1] = { (int32_t)dstExtent.width, (int32_t)dstExtent.height, (int32_t)dstExtent.depth };

	// perform update of buffer, and stage a copy of buffer data to image
	VkCommandBuffer cbuf = CommandBufferGetVk(cmdBuf);
	VkUtilities::ImageBarrier(cmdBuf, CoreGraphics::BarrierStage::Host, CoreGraphics::BarrierStage::Transfer, VkUtilities::ImageMemoryBarrier(img, dstSubres, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
	VkUtilities::ImageBarrier(cmdBuf, CoreGraphics::BarrierStage::AllGraphicsShaders, CoreGraphics::BarrierStage::Transfer, VkUtilities::ImageMemoryBarrier(tex, srcSubres, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL));
	vkCmdCopyImage(cbuf, tex, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
	VkUtilities::ImageBarrier(cmdBuf, CoreGraphics::BarrierStage::Transfer, CoreGraphics::BarrierStage::AllGraphicsShaders, VkUtilities::ImageMemoryBarrier(tex, srcSubres, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
	VkUtilities::ImageBarrier(cmdBuf, CoreGraphics::BarrierStage::Transfer, CoreGraphics::BarrierStage::Transfer, VkUtilities::ImageMemoryBarrier(img, dstSubres, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL));

	VkBufferCreateInfo bufInfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		NULL,
		0,
		memSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		NULL
	};
	VkBuffer buf;
	res = vkCreateBuffer(dev, &bufInfo, NULL, &buf);
	n_assert(res == VK_SUCCESS);

	VkDeviceMemory bufMem;
	uint32_t bufMemSize;
	VkUtilities::AllocateBufferMemory(dev, buf, bufMem, VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), bufMemSize);
	vkBindBufferMemory(dev, buf, bufMem, 0);

	VkBufferImageCopy cp;
	cp.bufferOffset = 0;
	cp.bufferRowLength = 0;
	cp.bufferImageHeight = 0;
	cp.imageExtent = dstExtent;
	cp.imageOffset = copy.dstOffset;
	cp.imageSubresource = copy.dstSubresource;
	vkCmdCopyImageToBuffer(cbuf, img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buf, 1, &cp);

	// end immediate command buffer
	VkUtilities::EndImmediateTransfer(cmdBuf);

	// delete image
	vkFreeMemory(dev, imgMem, nullptr);
	vkDestroyImage(dev, img, nullptr);

	outBuffer = buf;
	outMem = bufMem;
	outMemSize = VK_DEVICE_SIZE_CONV(bufMemSize);
}

//------------------------------------------------------------------------------
/**
*/
void
VkUtilities::WriteImage(const VkBuffer& srcImg, const VkImage& dstImg, VkImageCopy copy)
{
	// implement me!
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::CommandBufferId
VkUtilities::BeginImmediateTransfer()
{
	using namespace CoreGraphics;
	CommandBufferCreateInfo info =
	{
		false, false, true, CommandTransfer
	};
	CommandBufferId cmdBuf = CreateCommandBuffer(info);

	// this is why this is slow, we must perform a begin-end-submit of the command buffer for this to work
	VkCommandBufferBeginInfo begin =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		NULL,
		0,
		NULL
	};
	vkBeginCommandBuffer(CommandBufferGetVk(cmdBuf), &begin);
	return cmdBuf;
}

//------------------------------------------------------------------------------
/**
*/
void
VkUtilities::EndImmediateTransfer(CoreGraphics::CommandBufferId cmdBuf)
{
	// end command
	const VkCommandBuffer buf = CommandBufferGetVk(cmdBuf);
	vkEndCommandBuffer(buf);

	VkSubmitInfo submit =
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		nullptr,
		0, nullptr, nullptr,
		1, &buf,
		0, nullptr
	};

	VkFenceCreateInfo fence =
	{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		nullptr,
		0
	};

	VkDevice dev = Vulkan::GetCurrentDevice();

	// create a fence we can wait for, and execute this very tiny command buffer
	VkResult res;
	VkFence sync;
	res = vkCreateFence(dev, &fence, nullptr, &sync);
	n_assert(res == VK_SUCCESS);
	res = vkQueueSubmit(Vulkan::GetCurrentQueue(GraphicsQueueType), 1, &submit, sync);
	n_assert(res == VK_SUCCESS);

	// wait for fences, this waits for our commands to finish
	res = vkWaitForFences(dev, 1, &sync, true, UINT_MAX);
	n_assert(res == VK_SUCCESS);

	DestroyCommandBuffer(cmdBuf);

	// cleanup fence, buffer and buffer memory
	vkDestroyFence(dev, sync, nullptr);
}

} // namespace Vulkan