#pragma once
//------------------------------------------------------------------------------
/**
    Implements a Vulkan pass, which translates into a VkRenderPass.
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/shader.h"
#include "coregraphics/pass.h"
#include "coregraphics/buffer.h"
#include "coregraphics/resourcetable.h"

namespace Vulkan
{

struct VkPassLoadInfo
{
    VkDevice dev;
    Util::StringAtom name;

    // these hold the per-pass shader state
    CoreGraphics::BufferId passBlockBuffer;
    IndexT renderTargetDimensionsVar;

    // we need these stored for resizing
    Util::Array<CoreGraphics::TextureViewId> colorAttachments;
    Util::Array<Math::vec4> colorAttachmentClears;
    CoreGraphics::TextureViewId depthStencilAttachment;
    Util::Array<CoreGraphics::AttachmentFlagBits> colorAttachmentFlags;
    Util::Array<CoreGraphics::Subpass> subpasses;
    CoreGraphics::AttachmentFlagBits depthStencilFlags;

    // we store these so we retain the data for when we need to bind it
    VkRect2D renderArea;
    VkFramebuffer framebuffer;
    VkRenderPass pass;
    Util::FixedArray<VkRect2D> rects;
    Util::FixedArray<VkViewport> viewports;
    Util::FixedArray<VkClearValue> clearValues;
};

struct VkPassRuntimeInfo
{
    VkGraphicsPipelineCreateInfo framebufferPipelineInfo;
    VkPipelineViewportStateCreateInfo viewportInfo;

    uint32_t currentSubpassIndex;
    CoreGraphics::ResourceTableId passDescriptorSet;
    CoreGraphics::ResourcePipelineId passPipelineLayout;

    Util::FixedArray<Util::FixedArray<Math::rectangle<int>>> subpassRects;
    Util::FixedArray<Util::FixedArray<Math::rectangle<int>>> subpassViewports;
    Util::FixedArray<VkPipelineViewportStateCreateInfo> subpassPipelineInfo;
    CoreGraphics::PassRecordMode recordMode;
};

enum
{
    Pass_VkLoadInfo
    , Pass_VkRuntimeInfo
    , Pass_VkRenderPassBeginInfo
    , Pass_SubpassAttachments
};

typedef Ids::IdAllocator<
    VkPassLoadInfo,
    VkPassRuntimeInfo,
    VkRenderPassBeginInfo,
    Util::Array<uint32_t>   // subpass attachments
> VkPassAllocator;
extern VkPassAllocator passAllocator;

/// get vk render pass
const VkRenderPassBeginInfo& PassGetVkRenderPassBeginInfo(const CoreGraphics::PassId& id);
/// get vk framebuffer info
const VkGraphicsPipelineCreateInfo& PassGetVkFramebufferInfo(const CoreGraphics::PassId& id);
/// get number of pass attachments
const SizeT PassGetVkNumAttachments(const CoreGraphics::PassId& id);


} // namespace Vulkan
