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
namespace Vulkan
{

enum
{
	SubmissionContext_NumCycles,
	SubmissionContext_CmdBuffer,
	SubmissionContext_RetiredCmdBuffer,
	SubmissionContext_Fence,
	SubmissionContext_FreeBuffers,
	SubmissionContext_FreeDeviceMemories,
	SubmissionContext_FreeImages,
	SubmissionContext_FreeCommandBuffers,
	SubmissionContext_ClearCommandBuffers,
	SubmissionContext_FreeHostMemories,
	SubmissionContext_CurrentIndex,
	SubmissionContext_CmdCreateInfo,
	SubmissionContext_Name
};

typedef Ids::IdAllocator<
	SizeT,
	Util::FixedArray<CoreGraphics::CommandBufferId>,
	Util::FixedArray<Util::Array<CoreGraphics::CommandBufferId>>,
	Util::FixedArray<CoreGraphics::FenceId>,
	Util::FixedArray<Util::Array<std::tuple<VkDevice, VkBuffer>>>,
	Util::FixedArray<Util::Array<std::tuple<VkDevice, VkDeviceMemory>>>,
	Util::FixedArray<Util::Array<std::tuple<VkDevice, VkImage>>>,
	Util::FixedArray<Util::Array<CoreGraphics::CommandBufferId>>,
	Util::FixedArray<Util::Array<CoreGraphics::CommandBufferId>>,
	Util::FixedArray<Util::Array<void*>>,
	IndexT,
	CoreGraphics::CommandBufferCreateInfo,
	Util::String
> SubmissionContextAllocator;
extern SubmissionContextAllocator submissionContextAllocator;


/// FIXME, change these to be non-vulkan specific types and move them to generic submissioncontext

/// add buffer for deletion
void SubmissionContextFreeBuffer(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkBuffer buf);
/// add memory for deletion
void SubmissionContextFreeDeviceMemory(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkDeviceMemory mem);
/// add image for deletion
void SubmissionContextFreeImage(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkImage img);
/// add command buffer for deletion
void SubmissionContextFreeCommandBuffer(const CoreGraphics::SubmissionContextId id, const CoreGraphics::CommandBufferId cmd);
/// add command buffer for reset
void SubmissionContextClearCommandBuffer(const CoreGraphics::SubmissionContextId id, const CoreGraphics::CommandBufferId cmd);

} // namespace Vulkan
