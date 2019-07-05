#pragma once
//------------------------------------------------------------------------------
/**
	Vulkan implementation of a submission context

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <vulkan/vulkan.h>
#include "coregraphics/submissioncontext.h"
#include "ids/id.h"
#include "coregraphics/config.h"
#include "util/fixedarray.h"
#include "ids/idallocator.h"
#include "coregraphics/commandbuffer.h"
namespace Vulkan
{

enum
{
	SubmissionContextCmdBuffer,
	SubmissionContextSemaphore,
	SubmissionContextRetiredCmdBuffer,
	SubmissionContextRetiredSemaphore,
	SubmissionContextFence,
	SubmissionContextFreeBuffers,
	SubmissionContextFreeDeviceMemories,
	SubmissionContextFreeImages,
	SubmissionContextFreeCommandBuffers,
	SubmissionContextFreeHostMemories,
	SubmissionContextCurrentIndex,
	SubmissionContextCmdCreateInfo,
	SubmissionContextName
};

typedef Ids::IdAllocator<
	Util::FixedArray<CoreGraphics::CommandBufferId>,
	Util::FixedArray<CoreGraphics::SemaphoreId>,
	Util::FixedArray<Util::Array<CoreGraphics::CommandBufferId>>,
	Util::FixedArray<Util::Array<CoreGraphics::SemaphoreId>>,
	Util::FixedArray<CoreGraphics::FenceId>,
	Util::Array<std::tuple<VkDevice, VkBuffer>>,
	Util::Array<std::tuple<VkDevice, VkDeviceMemory>>,
	Util::Array<std::tuple<VkDevice, VkImage>>,
	Util::Array<std::tuple<VkDevice, VkCommandPool, VkCommandBuffer>>,
	Util::Array<void*>,
	IndexT,
	CoreGraphics::CommandBufferCreateInfo,
	Util::String
> SubmissionContextAllocator;
extern SubmissionContextAllocator submissionContextAllocator;

/// add buffer for deletion
void SubmissionContextFreeBuffer(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkBuffer buf);
/// add memory for deletion
void SubmissionContextFreeDeviceMemory(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkDeviceMemory mem);
/// add image for deletion
void SubmissionContextFreeImage(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkImage img);
/// add command buffer for deletion
void SubmissionContextFreeCommandBuffer(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkCommandPool pool, VkCommandBuffer buf);

} // namespace Vulkan
