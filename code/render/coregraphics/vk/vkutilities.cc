//------------------------------------------------------------------------------
// vkutilities.cc
// (C) 2017-2020 Individual contributors, see AUTHORS file
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
VkUtilities::ImageUpdate(VkDevice dev, CoreGraphics::CommandBufferId cmd, CoreGraphics::QueueType queue, VkImage img, const VkExtent3D& extent, uint32_t mip, uint32_t layer, VkDeviceSize size, uint32_t* data, VkBuffer& outIntermediateBuffer, CoreGraphics::Alloc& outAlloc)
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

	// allocate temporary buffer
	CoreGraphics::Alloc alloc = AllocateMemory(dev, buf, CoreGraphics::BufferMemory_Temporary);
	vkBindBufferMemory(dev, buf, alloc.mem, alloc.offset);
	char* mapped = (char*)GetMappedMemory(alloc);
	memcpy(mapped, data, alloc.size);

	// perform update of buffer, and stage a copy of buffer data to image
	VkBufferImageCopy copy;
	copy.bufferOffset = 0;
	copy.bufferImageHeight = 0;
	copy.bufferRowLength = 0;
	copy.imageExtent = extent;
	copy.imageOffset = { 0, 0, 0 };
	copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, mip, layer, 1 };
	vkCmdCopyBufferToImage(CommandBufferGetVk(cmd), buf, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

	outIntermediateBuffer = buf;
	outAlloc = alloc;
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
VkUtilities::ImageMemoryBarrier(const VkImage& img, VkImageSubresourceRange subres, CoreGraphics::QueueType fromQueue, CoreGraphics::QueueType toQueue, VkAccessFlags left, VkAccessFlags right, VkImageLayout oldLayout, VkImageLayout newLayout)
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
VkUtilities::ImageMemoryBarrier(const VkImage& img, VkImageSubresourceRange subres, CoreGraphics::QueueType toQueue, VkAccessFlags left, VkAccessFlags right, VkImageLayout oldLayout, VkImageLayout newLayout)
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
VkUtilities::ReadImage(const VkImage tex, CoreGraphics::PixelFormat::Code format, CoreGraphics::TextureDimensions dims, CoreGraphics::TextureType type, VkImageCopy copy, CoreGraphics::Alloc& outMem, VkBuffer& outBuffer)
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

	CoreGraphics::Alloc alloc1 = AllocateMemory(dev, img, CoreGraphics::ImageMemory_Temporary);
	vkBindImageMemory(dev, img, alloc1.mem, alloc1.offset);

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
		alloc1.size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		NULL
	};
	VkBuffer buf;
	res = vkCreateBuffer(dev, &bufInfo, NULL, &buf);
	n_assert(res == VK_SUCCESS);

	CoreGraphics::Alloc alloc2 = AllocateMemory(dev, buf, CoreGraphics::BufferMemory_Temporary);
	vkBindBufferMemory(dev, buf, alloc2.mem, alloc2.offset);

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
	FreeMemory(alloc1);
	vkDestroyImage(dev, img, nullptr);

	outBuffer = buf;
	outMem = alloc2;
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
		false,
		CoreGraphics::CommandBufferPoolId::Invalid() // fixme, or delete this function
	};
	CommandBufferId cmdBuf = CreateCommandBuffer(info);

	// this is slow because we are basically recording and submitting a transfer immediately
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
	res = vkQueueSubmit(Vulkan::GetCurrentQueue(CoreGraphics::GraphicsQueueType), 1, &submit, sync);
	n_assert(res == VK_SUCCESS);

	// wait for fences, this waits for our commands to finish
	res = vkWaitForFences(dev, 1, &sync, true, UINT_MAX);
	n_assert(res == VK_SUCCESS);

	DestroyCommandBuffer(cmdBuf);

	// cleanup fence, buffer and buffer memory
	vkDestroyFence(dev, sync, nullptr);
}

} // namespace Vulkan