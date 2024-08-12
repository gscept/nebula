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



    // we need these stored for resizing
    Util::Array<CoreGraphics::TextureViewId> attachments;
    Util::Array<Math::vec4> attachmentClears;
    Util::Array<CoreGraphics::AttachmentFlagBits> attachmentFlags;
    Util::Array<bool> attachmentIsDepthStencil;
    Util::Array<CoreGraphics::Subpass> subpasses;

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
    // these hold the per-pass shader state
    CoreGraphics::BufferId passBlockBuffer;
    IndexT renderTargetDimensionsVar;

    VkGraphicsPipelineCreateInfo framebufferPipelineInfo;
    VkPipelineMultisampleStateCreateInfo multisampleInfo;
    VkPipelineViewportStateCreateInfo viewportInfo;

    uint32_t currentSubpassIndex;
    CoreGraphics::ResourceTableId passDescriptorSet;
    CoreGraphics::ResourcePipelineId passPipelineLayout;

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

/// Get device creating this pass
const VkDevice PassGetVkDevice(const CoreGraphics::PassId& id);
/// Get framebuffer
const VkFramebuffer PassGetVkFramebuffer(const CoreGraphics::PassId& id);
/// Get pass
const VkRenderPass PassGetVkRenderPass(const CoreGraphics::PassId& id);


} // namespace Vulkan
