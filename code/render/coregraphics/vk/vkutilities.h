#pragma once
//------------------------------------------------------------------------------
/**
	Implements some Vulkan related utilities
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "vkdeferredcommand.h"
#include "core/refcounted.h"
namespace Vulkan
{
class VkTexture;
class VkUtilities
{
public:
	/// constructor
	VkUtilities();
	/// destructor
	virtual ~VkUtilities();

	/// perform image layout transition immediately
	static void ImageLayoutTransition(VkDeferredCommand::CommandQueueType queue, VkImageMemoryBarrier barrier);
	/// perform image layout transition immediately
	static void ImageLayoutTransition(VkCommandBuffer buf, VkImageMemoryBarrier barrier);
	/// create image memory barrier
	static VkImageMemoryBarrier ImageMemoryBarrier(const VkImage& img, VkImageSubresourceRange subres, VkImageLayout oldLayout, VkImageLayout newLayout);
	/// create image ownership change
	static VkImageMemoryBarrier ImageMemoryBarrier(const VkImage& img, VkImageSubresourceRange subres, VkDeferredCommand::CommandQueueType fromQueue, VkDeferredCommand::CommandQueueType toQueue, VkImageLayout layout);
	/// create buffer memory barrier
	static VkBufferMemoryBarrier BufferMemoryBarrier(const VkBuffer& buf, VkDeviceSize offset, VkDeviceSize size, VkAccessFlags srcAccess, VkAccessFlags dstAccess);
	/// transition image between layouts
	static void ChangeImageLayout(const VkImageMemoryBarrier& barrier, const VkDeferredCommand::CommandQueueType& type);
	/// transition image ownership
	static void ImageOwnershipChange(VkDeferredCommand::CommandQueueType queue, VkImageMemoryBarrier barrier);
	/// perform image color clear
	static void ImageColorClear(const VkImage& image, const VkDeferredCommand::CommandQueueType& queue, VkImageLayout layout, VkClearColorValue clearValue, VkImageSubresourceRange subres);
	/// perform image depth stencil clear
	static void ImageDepthStencilClear(const VkImage& image, const VkDeferredCommand::CommandQueueType& queue, VkImageLayout layout, VkClearDepthStencilValue clearValue, VkImageSubresourceRange subres);

	/// allocate a buffer memory storage, num is a multiplier for how many times the size needs to be duplicated
	static void AllocateBufferMemory(const VkBuffer& buf, VkDeviceMemory& bufmem, VkMemoryPropertyFlagBits flags, uint32_t& bufsize);
	/// allocate an image memory storage, num is a multiplier for how many times the size needs to be duplicated
	static void AllocateImageMemory(const VkImage& img, VkDeviceMemory& imgmem, VkMemoryPropertyFlagBits flags, uint32_t& imgsize);
	/// figure out which memory type fits given memory bits and required properties
	static VkResult GetMemoryType(uint32_t bits, VkMemoryPropertyFlags flags, uint32_t& index);

	/// update buffer memory from CPU
	static void BufferUpdate(const VkBuffer& buf, VkDeviceSize offset, VkDeviceSize size, const void* data);
	/// update buffer memory from CPU
	static void BufferUpdate(VkCommandBuffer cmd, const VkBuffer& buf, VkDeviceSize offset, VkDeviceSize size, const void* data);
	/// update image memory from CPU
	static void ImageUpdate(const VkImage& img, const VkImageCreateInfo& info, uint32_t mip, uint32_t face, VkDeviceSize size, uint32_t* data);

	/// perform image read-back, and saves to buffer (SLOW!)
	static void ReadImage(const Ptr<VkTexture>& tex, VkImageCopy copy, uint32_t& outMemSize, VkDeviceMemory& outMem, VkBuffer& outBuffer);
	/// perform image write-back, transitions data from buffer to image (SLOW!)
	static void WriteImage(const VkBuffer& srcImg, const VkImage& dstImg, VkImageCopy copy);
	/// helper to begin immediate transfer
	static VkCommandBuffer BeginImmediateTransfer();
	/// helper to end immediate transfer
	static void EndImmediateTransfer(VkCommandBuffer cmdBuf);
};
} // namespace Vulkan