#pragma once
//------------------------------------------------------------------------------
/**
	Vulkan implementation of a submission context

	(C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "vkloader.h"
#include "coregraphics/submissioncontext.h"
#include "ids/id.h"
#include "coregraphics/config.h"
#include "util/fixedarray.h"
#include "ids/idallocator.h"
#include "coregraphics/commandbuffer.h"
#include "vkmemory.h"
namespace Vulkan
{

enum
{
	SubmissionContext_NumCycles,
	SubmissionContext_CmdBuffer,
	SubmissionContext_TimelineIndex,
	SubmissionContext_RetiredCmdBuffer,
	SubmissionContext_FreeBuffers,
	SubmissionContext_FreeImages,
	SubmissionContext_FreeCommandBuffers,
	SubmissionContext_ClearCommandBuffers,
	SubmissionContext_FreeHostMemories,
	SubmissionContext_FreeMemories,
	SubmissionContext_CurrentIndex,
	SubmissionContext_CmdCreateInfo,
	SubmissionContext_Name
};

typedef Ids::IdAllocator<
	SizeT,
	Util::FixedArray<CoreGraphics::CommandBufferId>,
	Util::FixedArray<uint64>,
	Util::FixedArray<Util::Array<CoreGraphics::CommandBufferId>>,
	Util::FixedArray<Util::Array<std::tuple<VkDevice, VkBuffer>>>,
	Util::FixedArray<Util::Array<std::tuple<VkDevice, VkImage>>>,
	Util::FixedArray<Util::Array<CoreGraphics::CommandBufferId>>,
	Util::FixedArray<Util::Array<CoreGraphics::CommandBufferId>>,
	Util::FixedArray<Util::Array<void*>>,
	Util::FixedArray<Util::Array<CoreGraphics::Alloc>>,
	IndexT,
	CoreGraphics::CommandBufferCreateInfo,
	Util::String
> SubmissionContextAllocator;
extern SubmissionContextAllocator submissionContextAllocator;

/// TODO: make these generic
/// add buffer for deletion
void SubmissionContextFreeBuffer(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkBuffer buf);
/// add image for deletion
void SubmissionContextFreeImage(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkImage img);
/// add command buffer for deletion
void SubmissionContextFreeCommandBuffer(const CoreGraphics::SubmissionContextId id, const CoreGraphics::CommandBufferId cmd);
/// add command buffer for reset
void SubmissionContextClearCommandBuffer(const CoreGraphics::SubmissionContextId id, const CoreGraphics::CommandBufferId cmd);
/// add a memory alloc for freeing
void SubmissionContextFreeMemory(const CoreGraphics::SubmissionContextId id, const CoreGraphics::Alloc& alloc);

/// set the submission timeline index for this cycle
void SubmissionContextSetTimelineIndex(const CoreGraphics::SubmissionContextId id, uint64 index);
/// get the submission timline index for the previous cycle
uint64 SubmissionContextGetTimelineIndex(const CoreGraphics::SubmissionContextId id);

} // namespace Vulkan
