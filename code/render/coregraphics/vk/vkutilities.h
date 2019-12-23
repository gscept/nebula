#pragma once
//------------------------------------------------------------------------------
/**
	Implements some Vulkan related utilities
	
	(C) 2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "vkdeferredcommand.h"
#include "coregraphics/gpubuffertypes.h"
#include "coregraphics/pixelformat.h"
#include "coregraphics/texture.h"
#include "coregraphics/commandbuffer.h"
#include "math/rectangle.h"
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

	/// image transition on a constant buffer
	static void ImageBarrier(CoreGraphics::CommandBufferId cmd, CoreGraphics::BarrierStage left, CoreGraphics::BarrierStage right, VkImageMemoryBarrier barrier);
	/// update buffer memory from CPU
	static void BufferUpdate(CoreGraphics::CommandBufferId cmd, VkBuffer buf, VkDeviceSize offset, VkDeviceSize size, const void* data);
	/// update image memory from CPU
	static void ImageUpdate(VkDevice dev, CoreGraphics::CommandBufferId cmd, CoreGraphics::QueueType queue, VkImage img, const VkImageCreateInfo& info, uint32_t mip, uint32_t face, VkDeviceSize size, uint32_t* data, VkBuffer& outIntermediateBuffer, VkDeviceMemory& outIntermediateMemory);
	/// perform image color clear
	static void ImageColorClear(CoreGraphics::CommandBufferId cmd, const VkImage& image, VkImageLayout layout, VkClearColorValue clearValue, VkImageSubresourceRange subres);
	/// perform image depth stencil clear
	static void ImageDepthStencilClear(CoreGraphics::CommandBufferId cmd, const VkImage& image, VkImageLayout layout, VkClearDepthStencilValue clearValue, VkImageSubresourceRange subres);
	/// do actual copy (see coregraphics namespace for helper functions)
	static void Copy(CoreGraphics::CommandBufferId cmd, const VkImage from, Math::rectangle<SizeT> fromRegion, const VkImage to, Math::rectangle<SizeT> toRegion);
	/// perform actual blit (see coregraphics namespace for helper functions)
	static void Blit(CoreGraphics::CommandBufferId cmd, const VkImage from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const VkImage to, Math::rectangle<SizeT> toRegion, IndexT toMip);


	/// create image memory barrier
	static VkImageMemoryBarrier ImageMemoryBarrier(const VkImage& img, VkImageSubresourceRange subres, VkAccessFlags left, VkAccessFlags right, VkImageLayout oldLayout, VkImageLayout newLayout);
	/// create image memory barrier with an explicit ownership change
	static VkImageMemoryBarrier ImageMemoryBarrier(const VkImage& img, VkImageSubresourceRange subres, CoreGraphics::QueueType fromQueue, CoreGraphics::QueueType toQueue, VkAccessFlags left, VkAccessFlags right, VkImageLayout oldLayout, VkImageLayout newLayout);
	/// create image memory barrier with an implicit ownership change
	static VkImageMemoryBarrier ImageMemoryBarrier(const VkImage& img, VkImageSubresourceRange subres, CoreGraphics::QueueType toQueue, VkAccessFlags left, VkAccessFlags right, VkImageLayout oldLayout, VkImageLayout newLayout);
	/// create buffer memory barrier
	static VkBufferMemoryBarrier BufferMemoryBarrier(const VkBuffer& buf, VkDeviceSize offset, VkDeviceSize size, VkAccessFlags srcAccess, VkAccessFlags dstAccess);

	/// allocate a buffer memory storage
	static void AllocateBufferMemory(const VkDevice dev, const VkBuffer& buf, VkDeviceMemory& bufmem, VkMemoryPropertyFlagBits flags, uint32_t& bufsize);
	/// allocate an image memory storage
	static void AllocateImageMemory(const VkDevice dev, const VkImage& img, VkDeviceMemory& imgmem, VkMemoryPropertyFlagBits flags, uint32_t& imgsize);
	/// figure out which memory type fits given memory bits and required properties
	static VkResult GetMemoryType(uint32_t bits, VkMemoryPropertyFlags flags, uint32_t& index);

	/// perform image read-back, and saves to buffer (SLOW!)
	static void ReadImage(const VkImage tex, CoreGraphics::PixelFormat::Code format, CoreGraphics::TextureDimensions dims, CoreGraphics::TextureType type, VkImageCopy copy, uint32_t& outMemSize, VkDeviceMemory& outMem, VkBuffer& outBuffer);
	/// perform image write-back, transitions data from buffer to image (SLOW!)
	static void WriteImage(const VkBuffer& srcImg, const VkImage& dstImg, VkImageCopy copy);
	/// helper to begin immediate transfer
	static CoreGraphics::CommandBufferId BeginImmediateTransfer();
	/// helper to end immediate transfer
	static void EndImmediateTransfer(CoreGraphics::CommandBufferId cmdBuf);
};
} // namespace Vulkan