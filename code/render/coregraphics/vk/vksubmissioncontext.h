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
	SubmissionContextNumCycles,
	SubmissionContextCmdBuffer,
	SubmissionContextRetiredCmdBuffer,
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
	SizeT,
	Util::FixedArray<CoreGraphics::CommandBufferId>,
	Util::FixedArray<Util::Array<CoreGraphics::CommandBufferId>>,
	Util::FixedArray<CoreGraphics::FenceId>,
	Util::FixedArray<Util::Array<std::tuple<VkDevice, VkBuffer>>>,
	Util::FixedArray<Util::Array<std::tuple<VkDevice, VkDeviceMemory>>>,
	Util::FixedArray<Util::Array<std::tuple<VkDevice, VkImage>>>,
	Util::FixedArray<Util::Array<std::tuple<VkDevice, VkCommandPool, VkCommandBuffer>>>,
	Util::FixedArray<Util::Array<void*>>,
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
