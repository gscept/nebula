#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan implementation of a submission context

    @copyright
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
    SubmissionContext_Fences,
    SubmissionContext_FreeBuffers,
    SubmissionContext_FreeImages,
    SubmissionContext_FreeImageViews,
    SubmissionContext_FreeResourceTables,
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
    Util::FixedArray<CoreGraphics::FenceId>,
    Util::FixedArray<Util::Array<Util::Tuple<VkDevice, VkBuffer>>>,
    Util::FixedArray<Util::Array<Util::Tuple<VkDevice, VkImage>>>,
    Util::FixedArray<Util::Array<Util::Tuple<VkDevice, VkImageView>>>,
    Util::FixedArray<Util::Array<Util::Tuple<VkDevice, CoreGraphics::ResourceTableLayoutId, VkDescriptorSet, IndexT>>>,
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
void SubmissionContextFreeVkBuffer(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkBuffer buf);
/// add image for deletion
void SubmissionContextFreeVkImage(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkImage img);
/// add image view for deletion
void SubmissionContextFreeVkImageView(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkImageView img);
/// 

/// set the submission timeline index for this cycle
void SubmissionContextSetTimelineIndex(const CoreGraphics::SubmissionContextId id, uint64 index);
/// get the submission timline index for the previous cycle
uint64 SubmissionContextGetTimelineIndex(const CoreGraphics::SubmissionContextId id);

} // namespace Vulkan
