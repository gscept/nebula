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

using namespace CoreGraphics;
namespace Vulkan
{

VkPassAllocator passAllocator(0x00FFFFFF);

//------------------------------------------------------------------------------
/**
*/
const VkRenderPassBeginInfo&
PassGetVkRenderPassBeginInfo(const CoreGraphics::PassId& id)
{
    return passAllocator.Get<Pass_VkRenderPassBeginInfo>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const VkGraphicsPipelineCreateInfo&
PassGetVkFramebufferInfo(const CoreGraphics::PassId& id)
{
    return passAllocator.Get<Pass_VkRuntimeInfo>(id.id).framebufferPipelineInfo;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT 
PassGetVkNumAttachments(const CoreGraphics::PassId& id)
{
    return passAllocator.Get<Pass_VkLoadInfo>(id.id).attachments.Size();
}

//------------------------------------------------------------------------------
/**
*/
const VkDevice
PassGetVkDevice(const CoreGraphics::PassId& id)
{
    return passAllocator.Get<Pass_VkLoadInfo>(id.id).dev;
}

//------------------------------------------------------------------------------
/**
*/
const VkFramebuffer
PassGetVkFramebuffer(const CoreGraphics::PassId& id)
{
    return passAllocator.Get<Pass_VkLoadInfo>(id.id).framebuffer;
}

//------------------------------------------------------------------------------
/**
*/
const VkRenderPass
PassGetVkRenderPass(const CoreGraphics::PassId& id)
{
    return passAllocator.Get<Pass_VkLoadInfo>(id.id).pass;
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
    , Util::Array<uint32>& usedAttachmentCounts
    , Util::FixedArray<VkPipelineViewportStateCreateInfo>& outPipelineInfos
    , uint32& numUsedAttachmentsTotal)
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
        IndexT j = 0;
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
        for (auto attachment : subpass.attachments)
        {
            VkAttachmentReference& ref = subpassInfo.colorReferences.Emplace();
            ref.attachment = attachment;
            ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference& res = subpassInfo.resolves.Emplace();
            res.attachment = VK_ATTACHMENT_UNUSED;
            res.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            consumedAttachments[attachment] = true;
            usedAttachments++;
        }

        for (IndexT k = 0; k < subpass.resolves.Size(); k++)
        {
            VkAttachmentReference& ref = subpassInfo.resolves[k];
            ref.attachment = subpass.resolves[k];
            ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            consumedAttachments[ref.attachment] = true;
        }

        for (auto input : subpass.inputs)
        {
            VkAttachmentReference& ref = subpassInfo.inputs.Emplace();
            ref.attachment = input;

            consumedAttachments[input] = true;
        }

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

    VkAttachmentLoadOp loadOps[] =
    {
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_LOAD_OP_LOAD,
    };

    VkAttachmentStoreOp storeOps[] =
    {
        VK_ATTACHMENT_STORE_OP_NONE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_STORE,
    };

    numUsedAttachmentsTotal = (uint32)loadInfo.attachments.Size();
    outAttachments.Resize(numUsedAttachmentsTotal);
    for (i = 0; i < loadInfo.attachments.Size(); i++)
    {
        TextureId tex = TextureViewGetTexture(loadInfo.attachments[i]);
        TextureUsage usage = TextureGetUsage(tex);
        n_assert(AllBits(usage, RenderTexture));

        VkFormat fmt = VkTypes::AsVkFormat(TextureGetPixelFormat(tex));
        VkAttachmentDescription& attachment = outAttachments[i];
        IndexT loadIdx = 0;
        switch (loadInfo.attachmentFlags[i] & (AttachmentFlagBits::Load | AttachmentFlagBits::Clear))
        {
            case AttachmentFlagBits::Load:
                loadIdx = 2;
                break;
            case AttachmentFlagBits::Clear:
                loadIdx = 1;
                break;
            default:
                loadIdx = 0;
                break;
        }
        IndexT stencilLoadIdx = 0;
        switch (loadInfo.attachmentFlags[i] & (AttachmentFlagBits::LoadStencil | AttachmentFlagBits::ClearStencil))
        {
            case AttachmentFlagBits::LoadStencil:
                stencilLoadIdx = 2;
                break;
            case AttachmentFlagBits::ClearStencil:
                stencilLoadIdx = 1;
                break;
            default:
                stencilLoadIdx = 0;
                break;
        }
        IndexT storeIdx = 0;
        switch (loadInfo.attachmentFlags[i] & (AttachmentFlagBits::Store | AttachmentFlagBits::Discard))
        {
            case AttachmentFlagBits::Store:
                storeIdx = 2;
                break;
            case AttachmentFlagBits::Discard:
                storeIdx = 1;
                break;
            default:
                storeIdx = 0;
                break;
        }
        IndexT stencilStoreIdx = 0;
        switch (loadInfo.attachmentFlags[i] & (AttachmentFlagBits::StoreStencil | AttachmentFlagBits::DiscardStencil))
        {
            case AttachmentFlagBits::StoreStencil:
                stencilStoreIdx = 2;
                break;
            case AttachmentFlagBits::DiscardStencil:
                stencilStoreIdx = 1;
                break;
            default:
                stencilStoreIdx = 0;
                break;
        }

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
        attachment.loadOp = loadOps[loadIdx];
        attachment.storeOp = storeOps[storeIdx];
        attachment.stencilLoadOp = loadOps[stencilLoadIdx];
        attachment.stencilStoreOp = storeOps[stencilStoreIdx];
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
    uint32 numUsedAttachmentsTotal = 0;

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
        ShaderId sid = CoreGraphics::ShaderGet("shd:system_shaders/shared.fxb"_atm);
        loadInfo.passBlockBuffer = CoreGraphics::ShaderCreateConstantBuffer(sid, "PassBlock");
        loadInfo.renderTargetDimensionsVar = ShaderGetConstantBinding(sid, "RenderTargetDimensions");

        CoreGraphics::ResourceTableLayoutId tableLayout = ShaderGetResourceTableLayout(sid, NEBULA_PASS_GROUP);
        runtimeInfo.passDescriptorSet = CreateResourceTable(ResourceTableCreateInfo{ tableLayout, 8 });
        runtimeInfo.passPipelineLayout = ShaderGetResourcePipeline(sid);

        CoreGraphics::ResourceTableBuffer write;
        write.buf = loadInfo.passBlockBuffer;
        write.offset = 0;
        write.size = NEBULA_WHOLE_BUFFER_SIZE;
        write.index = 0;
        write.dynamicOffset = false;
        write.texelBuffer = false;
        write.slot = ShaderGetResourceSlot(sid, "PassBlock");
        ResourceTableSetConstantBuffer(runtimeInfo.passDescriptorSet, write);

        // setup input attachments
        IndexT j = 0;
        for (i = 0; i < loadInfo.attachments.Size(); i++)
        {
            n_assert(j < 16); // only allow 8 input attachments in the shader, so we must limit it
            if (!loadInfo.attachmentIsDepthStencil[i])
            {
                CoreGraphics::ResourceTableInputAttachment write;
                write.tex = loadInfo.attachments[i];
                write.isDepth = false;
                write.sampler = InvalidSamplerId;
                write.slot = Shared::Table_Pass::InputAttachment0_SLOT + j;
                write.index = 0;
                ResourceTableSetInputAttachment(runtimeInfo.passDescriptorSet, write);
            }
        }
        ResourceTableCommitChanges(runtimeInfo.passDescriptorSet);
    }

    // Calculate texture dimensions
    Util::FixedArray<Math::vec4> dimensions(loadInfo.attachments.Size());
    for (i = 0; i < loadInfo.attachments.Size(); i++)
    {
        // update descriptor set based on images attachments
        TextureId tex = TextureViewGetTexture(loadInfo.attachments[i]);
        const CoreGraphics::TextureDimensions rtdims = TextureGetDimensions(tex);
        Math::vec4& dims = dimensions[i];
        dims = Math::vec4((Math::scalar)rtdims.width, (Math::scalar)rtdims.height, 1 / (Math::scalar)rtdims.width, 1 / (Math::scalar)rtdims.height);
    }
    BufferUpdateArray(loadInfo.passBlockBuffer, dimensions.Begin(), dimensions.Size(), loadInfo.renderTargetDimensionsVar);

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
    DestroyBuffer(loadInfo.passBlockBuffer);
    runtimeInfo.passDescriptorSet = ResourceTableId::Invalid();
    loadInfo.passBlockBuffer = BufferId::Invalid();

    DelayedDeletePass(id);
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
