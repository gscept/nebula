//------------------------------------------------------------------------------
// vkpass.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "vkpass.h"
#include "vkgraphicsdevice.h"
#include "vktypes.h"
#include "coregraphics/config.h"
#include "coregraphics/shaderserver.h"
#include "vktextureview.h"
#include "coregraphics/pass.h"
#include "coregraphics/vk/vkcommandbuffer.h"

#include "gpulang/render/system_shaders/shared.h"

using namespace CoreGraphics;
namespace Vulkan
{

VkPassAllocator passAllocator(0x000000FF);
VkPassRenderAllocator passRenderAllocator(0x000000FF);
//------------------------------------------------------------------------------
/**
*/
const VkRenderPassBeginInfo&
PassGetVkRenderPassBeginInfo(const CoreGraphics::PassId id)
{
    return passAllocator.Get<Pass_VkRenderPassBeginInfo>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const VkGraphicsPipelineCreateInfo&
PassGetVkFramebufferInfo(const CoreGraphics::PassId id)
{
    return passAllocator.Get<Pass_VkRuntimeInfo>(id.id).framebufferPipelineInfo;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
PassGetVkNumAttachments(const CoreGraphics::PassId id)
{
    return passAllocator.Get<Pass_VkLoadInfo>(id.id).attachments.Size();
}

//------------------------------------------------------------------------------
/**
*/
const VkDevice
PassGetVkDevice(const CoreGraphics::PassId id)
{
    return passAllocator.Get<Pass_VkLoadInfo>(id.id).dev;
}

//------------------------------------------------------------------------------
/**
*/
const VkFramebuffer
PassGetVkFramebuffer(const CoreGraphics::PassId id)
{
    return passAllocator.Get<Pass_VkLoadInfo>(id.id).framebuffer;
}

//------------------------------------------------------------------------------
/**
*/
const VkRenderPass
PassGetVkRenderPass(const CoreGraphics::PassId id)
{
    return passAllocator.Get<Pass_VkLoadInfo>(id.id).pass;
}

//------------------------------------------------------------------------------
/**
*/
const VkRenderingInfo
RenderPassGetVk(const CoreGraphics::RenderPassId id)
{
    return passRenderAllocator.Get<PassRender_BeginInfo>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const VkPipelineRenderingCreateInfo&
RenderPassGetVkPipelineInfo(const CoreGraphics::RenderPassId id)
{
    return passRenderAllocator.Get<PassRender_PipelineInfo>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ResourceTableId
RenderPassGetResourceTable(const CoreGraphics::RenderPassId id)
{
    return passRenderAllocator.Get<PassRender_ShaderInterface>(id.id).table;
}

//------------------------------------------------------------------------------
/**
*/
const VkPipelineViewportStateCreateInfo&
PassGetVkViewportInfo(const CoreGraphics::PassId id, uint32_t subpass)
{
    return passAllocator.Get<Pass_VkRuntimeInfo>(id.id).subpassPipelineInfo[subpass];
}

//------------------------------------------------------------------------------
/**
*/
VkAttachmentLoadOp
AsVkLoadOp(const CoreGraphics::AttachmentFlagBits bits)
{
	switch (bits & (AttachmentFlagBits::Load | AttachmentFlagBits::Clear))
    {
        case AttachmentFlagBits::Load:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        case AttachmentFlagBits::Clear:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        default:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
}

//------------------------------------------------------------------------------
/**
*/
VkAttachmentStoreOp
AsVkStoreOp(const CoreGraphics::AttachmentFlagBits bits)
{
	VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    switch (bits & (AttachmentFlagBits::Store | AttachmentFlagBits::Discard))
    {
        case CoreGraphics::AttachmentFlagBits::Store:
            return VK_ATTACHMENT_STORE_OP_STORE;
        case AttachmentFlagBits::Discard:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        default:
            return VK_ATTACHMENT_STORE_OP_NONE;
    }
}

//------------------------------------------------------------------------------
/**
*/
VkAttachmentLoadOp
AsVkLoadOpStencil(const CoreGraphics::AttachmentFlagBits bits)
{
	switch (bits & (AttachmentFlagBits::LoadStencil | AttachmentFlagBits::ClearStencil))
    {
        case AttachmentFlagBits::Load:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        case AttachmentFlagBits::Clear:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        default:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
}

//------------------------------------------------------------------------------
/**
*/
VkAttachmentStoreOp
AsVkStoreOpStencil(const CoreGraphics::AttachmentFlagBits bits)
{
	VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    switch (bits & (AttachmentFlagBits::StoreStencil | AttachmentFlagBits::DiscardStencil))
    {
        case CoreGraphics::AttachmentFlagBits::Store:
            return VK_ATTACHMENT_STORE_OP_STORE;
        case AttachmentFlagBits::Discard:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        default:
            return VK_ATTACHMENT_STORE_OP_NONE;
    }
}

} // namespace Vulkan

namespace CoreGraphics
{

using namespace Vulkan;

//------------------------------------------------------------------------------
/**
*/
Math::rectangle<int>
VkViewportToRect(const VkViewport& vp)
{
    Math::rectangle<int> ret;
    ret.left = vp.x;
    ret.top = vp.y;
    ret.right = ret.left + vp.width;
    ret.bottom = ret.top + vp.height;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Math::rectangle<int>
VkScissorToRect(const VkRect2D& sc)
{
    Math::rectangle<int> ret;
    ret.left = sc.offset.x;
    ret.top = sc.offset.y;
    ret.right = ret.left + sc.extent.width;
    ret.bottom = ret.top + sc.extent.height;
    return ret;
}

struct SubpassInfo
{
    Util::Array<VkAttachmentReference> colorReferences;
    Util::Array<VkAttachmentReference> resolves;
    Util::Array<VkAttachmentReference> inputs;
    Util::Array<uint32_t> preserves;
    VkAttachmentReference depthReference;
    VkAttachmentReference depthResolveReference;
    VkSubpassDescriptionDepthStencilResolve depthStencilResolve;
};

Util::FixedArray<SubpassInfo> subpassInfos;


//------------------------------------------------------------------------------
/**
*/
void
GetSubpassInfo(
    const VkPassLoadInfo& loadInfo
    , Util::FixedArray<VkSubpassDescription>& outDescs
    , Util::Array<VkSubpassDependency>& outDeps
    , Util::FixedArray<VkAttachmentDescription>& outAttachments
    , Util::Array<uint32_t>& usedAttachmentCounts
    , Util::FixedArray<VkPipelineViewportStateCreateInfo>& outPipelineInfos
    , uint32_t& numUsedAttachmentsTotal)
{
    subpassInfos.Clear();
    subpassInfos.Resize(loadInfo.subpasses.Size());

    Util::FixedArray<bool> consumedAttachments(loadInfo.attachments.Size());

    IndexT i;
    for (i = 0; i < loadInfo.subpasses.Size(); i++)
    {
        uint32_t& usedAttachments = usedAttachmentCounts.Emplace();
        const CoreGraphics::Subpass& subpass = loadInfo.subpasses[i];

        consumedAttachments.Fill(false);

        VkSubpassDescription& vksubpass = outDescs[i];
        vksubpass.flags = 0;
        vksubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        SubpassInfo& subpassInfo = subpassInfos[i];
        subpassInfo.depthStencilResolve.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
        subpassInfo.depthStencilResolve.pNext = nullptr;
        subpassInfo.depthStencilResolve.depthResolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
        subpassInfo.depthStencilResolve.stencilResolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;

        // resize rects
        n_assert(subpass.numViewports >= subpass.attachments.Size());
        n_assert(subpass.numScissors >= subpass.attachments.Size());
        outPipelineInfos[i] =
        {
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            nullptr,
            0,
            (uint32_t)subpass.numViewports, // use the actual amount of viewports we will use (might be changed dynamically later)
            nullptr,
            (uint32_t)subpass.numScissors, // do the same for the scissors
            nullptr
        };

        /*
        if (subpass.depthResolve != InvalidIndex)
        {
            subpassInfo.depthResolveReference.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
            subpassInfo.depthResolveReference.pNext = nullptr;
            subpassInfo.depthResolveReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            subpassInfo.depthResolveReference.attachment = subpass.depthResolve;
            subpassInfo.depthResolveReference.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT;
            subpassInfo.depthStencilResolve.pDepthStencilResolveAttachment = &subpassInfo.depthResolveReference;
            vksubpass.pNext = &subpassInfo.depthStencilResolve;
        }
        */

        // fill list of all attachments, will be removed per subpass attachment
        if (subpass.depth != InvalidIndex)
        {
            VkAttachmentReference& ds = subpassInfo.depthReference;
            ds.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            ds.attachment = subpass.depth;

            consumedAttachments[subpass.depth] = true;
        }
        else
        {
            VkAttachmentReference& ds = subpassInfo.depthReference;
            ds.layout = VK_IMAGE_LAYOUT_UNDEFINED;
            ds.attachment = VK_ATTACHMENT_UNUSED;

        }

        // Add color attachments
        subpassInfo.colorReferences.Reserve(subpass.attachments.Size());
        for (auto attachment : subpass.attachments)
        {
            VkAttachmentReference& ref = subpassInfo.colorReferences.Emplace();
            ref.attachment = attachment;
            ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            consumedAttachments[attachment] = true;
            usedAttachments++;
        }

        IndexT j = 0;

		subpassInfo.resolves.Reserve(subpass.resolves.Size());
        for (j = 0; j < subpass.resolves.Size(); j++)
        {
            VkAttachmentReference& ref = subpassInfo.resolves.Emplace();
            ref.attachment = subpass.resolves[j];
            ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            consumedAttachments[ref.attachment] = true;
        }

		// Update only the attachments we actually bind
        subpassInfo.inputs.Reserve(subpass.inputs.Size());
        for (j = 0; j < subpass.inputs.Size(); j++)
        {
            VkAttachmentReference& ref = subpassInfo.inputs.Emplace();
            ref.attachment = subpass.inputs[j];
			ref.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            consumedAttachments[subpass.inputs[j]] = true;
        }

		subpassInfo.preserves.Reserve(consumedAttachments.Size());
        for (j = 0; j < consumedAttachments.Size(); j++)
        {
            if (!consumedAttachments[j])
            {
                uint32_t& ref = subpassInfo.preserves.Emplace();
                ref = j;
            }
        }

        if (!subpass.dependencies.IsEmpty())
        {
            for (j = 0; j < subpass.dependencies.Size(); j++)
            {
                VkSubpassDependency dep;
                dep.srcSubpass = subpass.dependencies[j];
                dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                dep.dstSubpass = i;
                dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
                outDeps.Append(dep);

                if (subpass.depth != InvalidIndex)
                {
                    VkSubpassDependency dep;
                    dep.srcSubpass = subpass.dependencies[j];
                    dep.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    dep.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    dep.dstSubpass = i;
                    dep.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                    dep.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
                    outDeps.Append(dep);
                }
            }
        }
        else
        {
            if (!subpass.attachments.IsEmpty())
            {
                VkSubpassDependency dep;
                dep.srcSubpass = VK_SUBPASS_EXTERNAL;
                dep.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                dep.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                dep.dstSubpass = 0;
                dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
                outDeps.Append(dep);
            }

            if (subpass.depth != InvalidIndex)
            {
                VkSubpassDependency dep;
                dep.srcSubpass = VK_SUBPASS_EXTERNAL;
                dep.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                dep.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                dep.dstSubpass = 0;
                dep.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                dep.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
                outDeps.Append(dep);
            }
        }

        if (i == loadInfo.subpasses.Size() - 1)
        {
            if (!subpass.attachments.IsEmpty())
            {
                VkSubpassDependency dep;
                dep.srcSubpass = i;
                dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                dep.dstSubpass = VK_SUBPASS_EXTERNAL;
                dep.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                dep.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
                outDeps.Append(dep);
            }

            if (subpass.depth != InvalidIndex)
            {
                VkSubpassDependency dep;
                dep.srcSubpass = i;
                dep.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dep.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                dep.dstSubpass = VK_SUBPASS_EXTERNAL;
                dep.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                dep.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
                outDeps.Append(dep);
            }
        }

        // set color attachments
        vksubpass.colorAttachmentCount = subpassInfo.colorReferences.Size();
        vksubpass.pColorAttachments = subpassInfo.colorReferences.Begin();
        vksubpass.pResolveAttachments = subpassInfo.resolves.Begin();
        vksubpass.inputAttachmentCount = subpassInfo.inputs.Size();
        vksubpass.pInputAttachments = subpassInfo.inputs.Begin();
        vksubpass.preserveAttachmentCount = subpassInfo.preserves.Size();
        vksubpass.pPreserveAttachments = subpassInfo.preserves.Begin();
        vksubpass.pDepthStencilAttachment = &subpassInfo.depthReference;
    }

    numUsedAttachmentsTotal = (uint32_t)loadInfo.attachments.Size();
    outAttachments.Resize(numUsedAttachmentsTotal);
    for (i = 0; i < loadInfo.attachments.Size(); i++)
    {
        TextureId tex = TextureViewGetTexture(loadInfo.attachments[i]);
        TextureUsage usage = TextureGetUsage(tex);
        n_assert(AllBits(usage, TextureUsage::Render));

        VkFormat fmt = VkTypes::AsVkFormat(TextureGetPixelFormat(tex));
        VkAttachmentDescription& attachment = outAttachments[i];
        VkAttachmentLoadOp loadOp = AsVkLoadOp(loadInfo.attachmentFlags[i]);
        VkAttachmentLoadOp stencilLoadOp = AsVkLoadOpStencil(loadInfo.attachmentFlags[i]);
        VkAttachmentStoreOp storeOp = AsVkStoreOp(loadInfo.attachmentFlags[i]);
		VkAttachmentStoreOp stencilStoreOp = AsVkStoreOpStencil(loadInfo.attachmentFlags[i]);

        attachment.flags = 0;
        if (loadInfo.attachmentIsDepthStencil[i])
        {
            if (AllBits(loadInfo.attachmentFlags[i], AttachmentFlagBits::Clear | AttachmentFlagBits::ClearStencil))
            {
                attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            }
            else
            {
                attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }

            attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        }
        else
        {
            if (AllBits(loadInfo.attachmentFlags[i], AttachmentFlagBits::Clear))
            {
                attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            }
            else
            {
                attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }
            attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        attachment.format = fmt;
        attachment.loadOp = loadOp;
        attachment.storeOp = storeOp;
        attachment.stencilLoadOp = stencilLoadOp;
        attachment.stencilStoreOp = stencilStoreOp;
        attachment.samples = VkTypes::AsVkSampleFlags(TextureGetNumSamples(tex));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SetupPass(const PassId pid)
{
    Ids::Id32 id = pid.id;
    VkPassLoadInfo& loadInfo = passAllocator.Get<Pass_VkLoadInfo>(id);
    VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<Pass_VkRuntimeInfo>(id);
    VkRenderPassBeginInfo& beginInfo = passAllocator.Get<Pass_VkRenderPassBeginInfo>(id);
    Util::Array<uint32_t>& subpassAttachmentCounts = passAllocator.Get<Pass_SubpassAttachments>(id);

    Util::FixedArray<VkImageView> images;
    images.Resize(loadInfo.attachments.Size());

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t layers = 0;

    IndexT i = 0;

    SizeT samples = 1;

    // otherwise, use the render targets to decide viewports
    for (i = 0; i < loadInfo.attachments.Size(); i++)
    {
        images[i] = TextureViewGetVk(loadInfo.attachments[i]);
        TextureId tex = TextureViewGetTexture(loadInfo.attachments[i]);
        const CoreGraphics::TextureDimensions dims = TextureGetDimensions(tex);
        width = Math::max(width, (uint32_t)dims.width);
        height = Math::max(height, (uint32_t)dims.height);
        layers = Math::max(layers, (uint32_t)TextureGetNumLayers(tex));
        samples = Math::max(samples, TextureGetNumSamples(tex));

        VkRect2D& rect = loadInfo.rects[i];
        rect.offset.x = 0;
        rect.offset.y = 0;
        rect.extent.width = dims.width;
        rect.extent.height = dims.height;
        VkViewport& viewport = loadInfo.viewports[i];
        viewport.width = (float)dims.width;
        viewport.height = (float)dims.height;
        viewport.minDepth = 0;
        viewport.maxDepth = 1;
        viewport.x = 0;
        viewport.y = 0;
    }

    Util::FixedArray<VkSubpassDescription> subpassDescs;
    Util::Array<VkSubpassDependency> subpassDeps;
    Util::FixedArray<VkAttachmentDescription> attachments;

    // resize subpass contents
    subpassDescs.Resize(loadInfo.subpasses.Size());
    attachments.Resize(loadInfo.attachments.Size() + 1);
    runtimeInfo.subpassPipelineInfo.Resize(loadInfo.subpasses.Size());
    uint32_t numUsedAttachmentsTotal = 0;

    // run subpass info fetcher
    GetSubpassInfo(
        loadInfo
        , subpassDescs
        , subpassDeps
        , attachments
        , subpassAttachmentCounts
        , runtimeInfo.subpassPipelineInfo
        , numUsedAttachmentsTotal);

    // create render pass
    VkRenderPassCreateInfo rpinfo =
    {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        nullptr,
        0,
        numUsedAttachmentsTotal,
        numUsedAttachmentsTotal == 0 ? nullptr : attachments.Begin(),
        (uint32_t)subpassDescs.Size(),
        subpassDescs.IsEmpty() ? nullptr : subpassDescs.Begin(),
        (uint32_t)subpassDeps.Size(),
        subpassDeps.IsEmpty() ? nullptr : subpassDeps.Begin()
    };
    VkResult res = vkCreateRenderPass(loadInfo.dev, &rpinfo, nullptr, &loadInfo.pass);
    n_assert(res == VK_SUCCESS);
    CoreGraphics::ObjectSetName(loadInfo.pass, Util::String::Sprintf("Pass - %s", loadInfo.name.Value()).AsCharPtr());

    // create framebuffer
    VkFramebufferCreateInfo fbInfo =
    {
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        nullptr,
        0,
        loadInfo.pass,
        (uint32_t)images.Size(),
        images.Begin(),
        width,
        height,
        layers
    };
    res = vkCreateFramebuffer(loadInfo.dev, &fbInfo, nullptr, &loadInfo.framebuffer);
    n_assert(res == VK_SUCCESS);

    // setup render area
    loadInfo.renderArea.offset.x = 0;
    loadInfo.renderArea.offset.y = 0;
    loadInfo.renderArea.extent.width = width;
    loadInfo.renderArea.extent.height = height;

    // If the pass descriptor is invalid (which it is when the pass is first created) create a new resource table and constant buffer
    if (runtimeInfo.passDescriptorSet == ResourceTableId::Invalid())
    {
        // setup uniform buffer for render target information
        ShaderId sid = CoreGraphics::ShaderGet("shd:system_shaders/shared.gplb"_atm);
        runtimeInfo.passBlockBuffer = CoreGraphics::ShaderCreateConstantBuffer(sid, "PassUniforms", CoreGraphics::BufferAccessMode::DeviceAndHost);
        runtimeInfo.renderTargetDimensionsVar = offsetof(Shared::PassUniforms::STRUCT, RenderTargets);

        CoreGraphics::ResourceTableLayoutId tableLayout = ShaderGetResourceTableLayout(sid, NEBULA_PASS_GROUP);
        runtimeInfo.passDescriptorSet = CreateResourceTable(ResourceTableCreateInfo{ Util::String::Sprintf("Pass %s Descriptors", loadInfo.name.Value()).AsCharPtr(), tableLayout, 8 });
        runtimeInfo.passPipelineLayout = ShaderGetResourcePipeline(sid);

        CoreGraphics::ResourceTableBuffer write;
        write.buf = runtimeInfo.passBlockBuffer;
        write.offset = 0;
        write.size = NEBULA_WHOLE_BUFFER_SIZE;
        write.index = 0;
        write.dynamicOffset = false;
        write.texelBuffer = false;
        write.slot = ShaderGetResourceSlot(sid, "PassUniforms");
        ResourceTableSetConstantBuffer(runtimeInfo.passDescriptorSet, write);

        // setup input attachments
        IndexT j = 0;
        for (i = 0; i < loadInfo.attachments.Size(); i++)
        {
            n_assert(i < 8); // only allow 8 input attachments in the shader, so we must limit it
            if (!loadInfo.attachmentIsDepthStencil[i])
            {
                CoreGraphics::ResourceTableInputAttachment write;
                write.tex = loadInfo.attachments[i];
                write.isDepth = false;
                write.sampler = InvalidSamplerId;
                write.slot = Shared::InputAttachment0::BINDING + j++;
                write.index = 0;
                ResourceTableSetInputAttachment(runtimeInfo.passDescriptorSet, write);
            }
        }
        ResourceTableCommitChanges(runtimeInfo.passDescriptorSet);
    }

    // Calculate texture dimensions
    Util::FixedArray<Shared::RenderTargetParameters> params(loadInfo.attachments.Size());
    for (i = 0; i < loadInfo.attachments.Size(); i++)
    {
        // update descriptor set based on images attachments
        TextureId tex = TextureViewGetTexture(loadInfo.attachments[i]);
        const CoreGraphics::TextureDimensions rtdims = TextureGetDimensions(tex);
        Shared::RenderTargetParameters& rtParams = params[i];
        Math::vec4 dimensions = Math::vec4((Math::scalar)rtdims.width, (Math::scalar)rtdims.height, 1 / (Math::scalar)rtdims.width, 1 / (Math::scalar)rtdims.height);
        dimensions.storeu(rtParams.Dimensions);
        rtParams.Scale[0] = 1;
        rtParams.Scale[1] = 1;
    }
    BufferUpdateArray(runtimeInfo.passBlockBuffer, params, runtimeInfo.renderTargetDimensionsVar);

    // setup info
    runtimeInfo.framebufferPipelineInfo.renderPass = loadInfo.pass;
    runtimeInfo.framebufferPipelineInfo.subpass = 0;
    runtimeInfo.framebufferPipelineInfo.pViewportState = &runtimeInfo.subpassPipelineInfo[0];
    runtimeInfo.framebufferPipelineInfo.pMultisampleState = &runtimeInfo.multisampleInfo;

    runtimeInfo.multisampleInfo.pNext = nullptr;
    runtimeInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    VkSampleCountFlagBits sampleBits = VkTypes::AsVkSampleFlags(samples);
    runtimeInfo.multisampleInfo.rasterizationSamples = sampleBits;

    beginInfo =
    {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        nullptr,
        loadInfo.pass,
        loadInfo.framebuffer,
        loadInfo.renderArea,
        (uint32_t)loadInfo.clearValues.Size(),
        loadInfo.clearValues.Size() > 0 ? loadInfo.clearValues.Begin() : nullptr
    };
}

//------------------------------------------------------------------------------
/**
*/
const PassId
CreatePass(const PassCreateInfo& info)
{
    n_assert(info.subpasses.Size() > 0);
    Ids::Id32 id = passAllocator.Alloc();
    VkPassLoadInfo& loadInfo = passAllocator.Get<Pass_VkLoadInfo>(id);

    loadInfo.attachments = info.attachments;
    loadInfo.attachmentClears = info.attachmentClears;
    loadInfo.dev = Vulkan::GetCurrentDevice();

    SizeT numImages = info.attachments.Size() == 0 ? 1 : info.attachments.Size();
    loadInfo.clearValues.Resize(numImages);
    loadInfo.rects.Resize(numImages);
    loadInfo.viewports.Resize(numImages);
    loadInfo.name = info.name;

    loadInfo.subpasses = info.subpasses;
    loadInfo.attachmentFlags = info.attachmentFlags;
    loadInfo.attachmentIsDepthStencil = info.attachmentDepthStencil;

    IndexT i;
    for (i = 0; i < info.attachments.Size(); i++)
    {
        const Math::vec4& value = loadInfo.attachmentClears[i];
        VkClearValue& clear = loadInfo.clearValues[i];
        if (info.attachmentDepthStencil[i])
        {
            clear.depthStencil.depth = value.x;
            clear.depthStencil.stencil = value.y;
        }
        else
        {
            clear.color.float32[0] = value.x;
            clear.color.float32[1] = value.y;
            clear.color.float32[2] = value.z;
            clear.color.float32[3] = value.w;
        }
    }

    PassId ret = id;
    SetupPass(ret);

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyPass(const PassId id)
{
    VkPassLoadInfo& loadInfo = passAllocator.Get<Pass_VkLoadInfo>(id.id);
    VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<Pass_VkRuntimeInfo>(id.id);

    for (IndexT i = 0; i < loadInfo.attachments.Size(); i++)
        CoreGraphics::DestroyTextureView(loadInfo.attachments[i]);

    // destroy pass and our descriptor set
    DestroyResourceTable(runtimeInfo.passDescriptorSet);
    DestroyBuffer(runtimeInfo.passBlockBuffer);
    runtimeInfo.passDescriptorSet = ResourceTableId::Invalid();
    runtimeInfo.passBlockBuffer = BufferId::Invalid();
    loadInfo.attachments.Clear();
    loadInfo.attachmentClears.Clear();
    loadInfo.attachmentFlags.Clear();
    loadInfo.attachmentIsDepthStencil.Clear();
    loadInfo.subpasses.Clear();
    loadInfo.rects.Clear();
    loadInfo.viewports.Clear();
    loadInfo.clearValues.Clear();
    passAllocator.Get<Pass_SubpassAttachments>(id.id).Clear();
    runtimeInfo.subpassPipelineInfo.Clear();

    DelayedDeletePass(id);
    passAllocator.Dealloc(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const RenderPassId
CreateRenderPass(const RenderPassCreateInfo& info)
{
    Ids::Id32 id = passRenderAllocator.Alloc();
    VkRenderingInfo& renderInfo = passRenderAllocator.Get<PassRender_BeginInfo>(id);
    Util::FixedArray<VkRenderingAttachmentInfo>& attachmentInfo = passRenderAllocator.Get<PassRender_Attachments>(id);
	VkRenderingAttachmentInfo& depthAttachmentInfo = passRenderAllocator.Get<PassRender_DepthAttachment>(id);
    VkPipelineViewportStateCreateInfo& viewportInfo = passRenderAllocator.Get<PassRender_ViewportInfo>(id);
    VkPipelineRenderingCreateInfo& pipelineInfo = passRenderAllocator.Get<PassRender_PipelineInfo>(id);
    RenderPassShaderInterface& shaderInterface = passRenderAllocator.Get<PassRender_ShaderInterface>(id);
    Util::FixedArray<VkFormat>& pipelineInfoFormats = passRenderAllocator.Get<PassRender_PipelineInfoColorFormats>(id);

    pipelineInfoFormats.Resize(info.colorTargets.Size());

    pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.viewMask = 0x0;
    pipelineInfo.colorAttachmentCount = pipelineInfoFormats.Size();
    pipelineInfo.pColorAttachmentFormats = pipelineInfoFormats.Begin();
    pipelineInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    pipelineInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
	attachmentInfo.Resize(info.colorTargets.Size());
	for (SizeT i = 0; i < info.colorTargets.Size(); i++)
	{
        VkRenderingAttachmentInfo& target = attachmentInfo[i];
		target.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        target.pNext = nullptr;
        target.imageView = TextureViewGetVk(info.colorTargets[i]);
        target.imageLayout = VkTypes::AsVkImageLayout(info.colorTargetLayouts[i]);
        target.resolveMode = VK_RESOLVE_MODE_NONE;
        if (info.resolveTargets[i] != CoreGraphics::InvalidTextureViewId)
		{
			target.resolveImageView = TextureViewGetVk(info.resolveTargets[i]);
			target.resolveImageLayout = VkTypes::AsVkImageLayout(info.resolveLayouts[i]);
		}
		else
		{
            target.resolveImageView = VK_NULL_HANDLE;
            target.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}
        pipelineInfoFormats[i] = VkTypes::AsVkFormat(CoreGraphics::TextureViewGetPixelFormat(info.colorTargets[i]));

		target.loadOp = AsVkLoadOp(info.colorTargetFlags[i]);
        target.storeOp = AsVkStoreOp(info.colorTargetFlags[i]);

		VkClearValue& clear = target.clearValue;
        clear.color.float32[0] = info.colorClearValues[i].x;
		clear.color.float32[1] = info.colorClearValues[i].y;
		clear.color.float32[2] = info.colorClearValues[i].z;
		clear.color.float32[3] = info.colorClearValues[i].w;
	}

    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.pNext = nullptr;
    renderInfo.flags = 0x0;
    renderInfo.renderArea = {
        .offset {.x = info.area.left, .y = info.area.top}, .extent {.width = (uint)info.area.width(), .height = (uint)info.area.height()}
    };
    renderInfo.layerCount = info.layerCount;
    renderInfo.viewMask = 0x0;
    renderInfo.colorAttachmentCount = attachmentInfo.Size();
    renderInfo.pColorAttachments = attachmentInfo.Begin();

	if (info.depthTarget != CoreGraphics::InvalidTextureViewId)
	{
        depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachmentInfo.pNext = nullptr;
        depthAttachmentInfo.imageView = TextureViewGetVk(info.depthTarget);
        depthAttachmentInfo.imageLayout = VkTypes::AsVkImageLayout(info.depthTargetLayout);
        depthAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;

		if (info.depthResolveTarget != CoreGraphics::InvalidTextureViewId)
		{
			depthAttachmentInfo.resolveImageView = TextureViewGetVk(info.depthResolveTarget);
			depthAttachmentInfo.resolveImageLayout = VkTypes::AsVkImageLayout(info.depthResolveTargetLayout);
		}
		else
		{
            depthAttachmentInfo.resolveImageView = VK_NULL_HANDLE;
            depthAttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}

        pipelineInfo.depthAttachmentFormat = VkTypes::AsVkFormat(TextureViewGetPixelFormat(info.depthTarget));
        depthAttachmentInfo.resolveImageView = VK_NULL_HANDLE;
        depthAttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		depthAttachmentInfo.loadOp = AsVkLoadOp(info.depthFlags);
        depthAttachmentInfo.storeOp = AsVkStoreOp(info.depthFlags);

		VkClearValue& clear = depthAttachmentInfo.clearValue;
        clear.depthStencil.depth = info.depthClearValue.x;
        clear.depthStencil.stencil = info.depthClearValue.y;
        renderInfo.pDepthAttachment = &depthAttachmentInfo;
	}

    // setup uniform buffer for render target information
    ShaderId sid = CoreGraphics::ShaderGet("shd:system_shaders/shared.gplb"_atm);
    shaderInterface.constants = CoreGraphics::ShaderCreateConstantBuffer(sid, "PassUniforms", CoreGraphics::BufferAccessMode::DeviceAndHost);
    shaderInterface.renderTargetDimensionsOffset = offsetof(Shared::PassUniforms::STRUCT, RenderTargets);

    CoreGraphics::ResourceTableLayoutId tableLayout = ShaderGetResourceTableLayout(sid, NEBULA_PASS_GROUP);
    shaderInterface.table = CreateResourceTable(ResourceTableCreateInfo{ Util::String::Sprintf("Render Pass %s Descriptors", info.name.Value()).AsCharPtr(), tableLayout, 8 });

    CoreGraphics::ResourceTableBuffer write;
    write.buf = shaderInterface.constants;
    write.offset = 0;
    write.size = NEBULA_WHOLE_BUFFER_SIZE;
    write.index = 0;
    write.dynamicOffset = false;
    write.texelBuffer = false;
    write.slot = ShaderGetResourceSlot(sid, "PassUniforms");
    ResourceTableSetConstantBuffer(shaderInterface.table, write);

    // setup input attachments
    ResourceTableCommitChanges(shaderInterface.table);

    return RenderPassId(id);
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyRenderPass(RenderPassId pass)
{
	Util::FixedArray<VkRenderingAttachmentInfo>& attachmentInfo = passRenderAllocator.Get<PassRender_Attachments>(pass.id);
    attachmentInfo.Clear();
    passRenderAllocator.Dealloc(pass.id);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderPassSetRenderTargetParameters(const CoreGraphics::CmdBufferId cmdBuf, const RenderPassId id, const Util::FixedArray<Shared::RenderTargetParameters>& viewports)
{
    RenderPassShaderInterface& shaderInterface = passRenderAllocator.Get<PassRender_ShaderInterface>(id.id);
    CmdUpdateBuffer(cmdBuf, shaderInterface.constants, shaderInterface.renderTargetDimensionsOffset, viewports.ByteSize(), viewports.Begin());
}

//------------------------------------------------------------------------------
/**
*/
void
PassRenderEnd(CoreGraphics::CmdBufferId cmdBuf)
{
    vkCmdEndRendering(CmdBufferGetVk(cmdBuf));
}

//------------------------------------------------------------------------------
/**
*/
void
PassWindowResizeCallback(const PassId id)
{
    VkPassLoadInfo& loadInfo = passAllocator.Get<Pass_VkLoadInfo>(id.id);

    // destroy pass and our descriptor set
    vkDestroyRenderPass(loadInfo.dev, loadInfo.pass, nullptr);
    vkDestroyFramebuffer(loadInfo.dev, loadInfo.framebuffer, nullptr);

    // update attachments because their underlying textures might have changed
    for (IndexT i = 0; i < loadInfo.attachments.Size(); i++)
        CoreGraphics::TextureViewReload(loadInfo.attachments[i]);

    // setup pass again
    SetupPass(id);
}

//------------------------------------------------------------------------------
/**
*/
void
PassSetRenderTargetParameters(const PassId id, const Util::FixedArray<Shared::RenderTargetParameters>& viewports)
{
    VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<Pass_VkRuntimeInfo>(id.id);
    BufferUpdateArray(runtimeInfo.passBlockBuffer, viewports, runtimeInfo.renderTargetDimensionsVar);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CoreGraphics::TextureViewId>&
PassGetAttachments(const CoreGraphics::PassId id)
{
    return passAllocator.Get<Pass_VkLoadInfo>(id.id).attachments;
}

//------------------------------------------------------------------------------
/**
*/
const uint32_t
PassGetNumSubpassAttachments(const CoreGraphics::PassId id, const IndexT subpass)
{
    return passAllocator.Get<Pass_SubpassAttachments>(id.id)[subpass];
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ResourceTableId
PassGetResourceTable(const CoreGraphics::PassId id)
{
    VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<Pass_VkRuntimeInfo>(id.id);
    return runtimeInfo.passDescriptorSet;
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
PassGetName(const CoreGraphics::PassId id)
{
    return passAllocator.Get<Pass_VkLoadInfo>(id.id).name;
}

} // namespace Vulkan
