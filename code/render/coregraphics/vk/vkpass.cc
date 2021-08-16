//------------------------------------------------------------------------------
// vkpass.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkpass.h"
#include "vkgraphicsdevice.h"
#include "vktypes.h"
#include "coregraphics/config.h"
#include "coregraphics/shaderserver.h"
#include "vkbuffer.h"
#include "vkshader.h"
#include "vkresourcetable.h"
#include "vktextureview.h"

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
    return passAllocator.Get<Pass_BeginInfo>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const VkGraphicsPipelineCreateInfo&
PassGetVkFramebufferInfo(const CoreGraphics::PassId& id)
{
    return passAllocator.Get<Pass_RuntimeInfo>(id.id24).framebufferPipelineInfo;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT 
PassGetVkNumAttachments(const CoreGraphics::PassId& id)
{
    return passAllocator.Get<Pass_LoadInfo>(id.id24).colorAttachments.Size();
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<VkRect2D>&
PassGetVkRects(const CoreGraphics::PassId& id)
{
    const VkPassRuntimeInfo& info = passAllocator.Get<Pass_RuntimeInfo>(id.id24);
    return info.subpassRects[info.currentSubpassIndex];
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<VkViewport>&
PassGetVkViewports(const CoreGraphics::PassId& id)
{
    const VkPassRuntimeInfo& info = passAllocator.Get<Pass_RuntimeInfo>(id.id24);
    return info.subpassViewports[info.currentSubpassIndex];
}

} // namespace Vulkan

namespace CoreGraphics
{

using namespace Vulkan;

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
    , Util::FixedArray<Util::FixedArray<VkViewport>>& outViewports
    , Util::FixedArray<Util::FixedArray<VkRect2D>>& outScissorRects
    , Util::FixedArray<VkPipelineViewportStateCreateInfo>& outPipelineInfos
    , uint32& numUsedAttachmentsTotal)
{
    Util::FixedArray<Util::FixedArray<VkAttachmentReference>> subpassReferences;
    Util::FixedArray<Util::FixedArray<VkAttachmentReference>> subpassInputs;
    Util::FixedArray<Util::FixedArray<uint32_t>> subpassPreserves;
    Util::FixedArray<Util::FixedArray<VkAttachmentReference>> subpassResolves;
    Util::FixedArray<VkAttachmentReference> subpassDepthStencils;

    subpassReferences.Resize(loadInfo.subpasses.Size());
    subpassInputs.Resize(loadInfo.subpasses.Size());
    subpassPreserves.Resize(loadInfo.subpasses.Size());
    subpassResolves.Resize(loadInfo.subpasses.Size());
    subpassDepthStencils.Resize(loadInfo.subpasses.Size());

    IndexT i;
    for (i = 0; i < loadInfo.subpasses.Size(); i++)
    {
        const CoreGraphics::Subpass& subpass = loadInfo.subpasses[i];

        VkSubpassDescription& vksubpass = outDescs[i];
        vksubpass.flags = 0;
        vksubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        vksubpass.pColorAttachments = nullptr;
        vksubpass.pDepthStencilAttachment = nullptr;
        vksubpass.pPreserveAttachments = nullptr;
        vksubpass.pResolveAttachments = nullptr;
        vksubpass.pInputAttachments = nullptr;

        // resize rects
        n_assert(subpass.numViewports >= subpass.attachments.Size());
        n_assert(subpass.numScissors >= subpass.attachments.Size());
        outViewports[i].Resize(subpass.numViewports);
        outScissorRects[i].Resize(subpass.numScissors);
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

        // get references to fixed arrays
        Util::FixedArray<VkAttachmentReference>& references = subpassReferences[i];
        Util::FixedArray<VkAttachmentReference>& inputs = subpassInputs[i];
        Util::FixedArray<uint32_t>& preserves = subpassPreserves[i];
        Util::FixedArray<VkAttachmentReference>& resolves = subpassResolves[i];

        // resize arrays straight away since we already know the size
        references.Resize(loadInfo.colorAttachments.Size());
        inputs.Resize(subpass.inputs.Size());
        preserves.Resize(loadInfo.colorAttachments.Size() - subpass.attachments.Size());
        if (subpass.resolve)
            resolves.Resize(subpass.attachments.Size());

        // if subpass binds depth, the slot for the depth-stencil buffer is color attachments + 1
        if (subpass.bindDepth)
        {
            n_assert(loadInfo.depthStencilAttachment != InvalidTextureViewId);
            VkAttachmentReference& ds = subpassDepthStencils[i];
            ds.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            ds.attachment = loadInfo.colorAttachments.Size();
            vksubpass.pDepthStencilAttachment = &ds;

            // if we have no attachments in the subpass, use the depth stencil viewports
            if (subpass.attachments.Size() == 0)
            {
                outViewports[i][0] = loadInfo.viewports[ds.attachment];
                outScissorRects[i][0] = loadInfo.rects[ds.attachment];
            }
        }
        else
        {
            // we are not allowed to have a subpass that doesn't use any attachments
            n_assert(subpass.attachments.Size() != 0);
            VkAttachmentReference& ds = subpassDepthStencils[i];
            ds.layout = VK_IMAGE_LAYOUT_UNDEFINED;
            ds.attachment = VK_ATTACHMENT_UNUSED;
            vksubpass.pDepthStencilAttachment = &ds;
        }

        // fill list of all attachments, will be removed per subpass attachment
        IndexT j;
        Util::Array<IndexT> allAttachments;
        for (j = 0; j < loadInfo.colorAttachments.Size(); j++)
        {
            allAttachments.Append(j);
        }

        IndexT idx = 0;
        SizeT preserveAttachments = 0;
        SizeT usedAttachments = 0;

        for (j = 0; j < subpass.attachments.Size(); j++)
        {
            VkAttachmentReference& ref = references[j];
            ref.attachment = subpass.attachments[j];
            ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            usedAttachments++;

            outViewports[i][j] = loadInfo.viewports[ref.attachment];
            outScissorRects[i][j] = loadInfo.rects[ref.attachment];

            // remove from all attachments list
            IndexT idx = allAttachments.FindIndex(ref.attachment);
            allAttachments.EraseIndex(idx);
            if (subpass.resolve) resolves[j] = ref;
        }

        for (j = 0; j < subpass.inputs.Size(); j++)
        {
            VkAttachmentReference& ref = inputs[j];
            ref.attachment = subpass.inputs[j];
            ref.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            IndexT index = allAttachments.FindIndex(ref.attachment);
            n_assert_fmt(index != InvalidIndex, "Input attachment %d is already being used as an output attachment", ref.attachment);
            allAttachments.EraseIndex(index);
        }

        for (j = 0; j < allAttachments.Size(); j++)
        {
            VkAttachmentReference& ref = references[usedAttachments + j];
            ref.attachment = allAttachments[j];
            ref.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            preserves[preserveAttachments] = allAttachments[j];
            preserveAttachments++;
        }

        for (j = 0; j < subpass.dependencies.Size(); j++)
        {
            VkSubpassDependency dep;
            dep.srcSubpass = subpass.dependencies[j];
            dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dep.dstSubpass = i;
            dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            outDeps.Append(dep);
        }

        // set color attachments
        vksubpass.colorAttachmentCount = usedAttachments;
        vksubpass.pColorAttachments = references.Begin();

        // if we have subpass inputs, use them
        if (inputs.Size() > 0)
        {
            vksubpass.inputAttachmentCount = inputs.Size();
            vksubpass.pInputAttachments = inputs.IsEmpty() ? nullptr : inputs.Begin();
        }
        else
        {
            vksubpass.inputAttachmentCount = 0;
        }

        // the rest are automatically preserve
        if (preserves.Size() > 0)
        {
            vksubpass.preserveAttachmentCount = preserveAttachments;
            vksubpass.pPreserveAttachments = preserves.IsEmpty() ? nullptr : preserves.Begin();
        }
        else
        {
            vksubpass.preserveAttachmentCount = 0;
        }

        usedAttachmentCounts.Append(vksubpass.colorAttachmentCount);
    }

    VkAttachmentLoadOp loadOps[] =
    {
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_LOAD_OP_LOAD,
    };

    VkAttachmentStoreOp storeOps[] =
    {
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_STORE,
    };

    numUsedAttachmentsTotal = (uint32)loadInfo.colorAttachments.Size();
    for (i = 0; i < loadInfo.colorAttachments.Size(); i++)
    {
        TextureId tex = TextureViewGetTexture(loadInfo.colorAttachments[i]);
        TextureUsage usage = TextureGetUsage(tex);
        n_assert(CheckBits(usage, RenderTexture));

        VkFormat fmt = VkTypes::AsVkFormat(TextureGetPixelFormat(tex));
        VkAttachmentDescription& attachment = outAttachments[i];
        IndexT loadIdx = CheckBits(loadInfo.colorAttachmentFlags[i], AttachmentFlagBits::Load) ? 2 : CheckBits(loadInfo.colorAttachmentFlags[i], AttachmentFlagBits::Clear) ? 1 : 0;
        IndexT storeIdx = CheckBits(loadInfo.colorAttachmentFlags[i], AttachmentFlagBits::Store) ? 1 : 0;
        attachment.flags = 0;
        attachment.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        attachment.format = fmt;
        attachment.loadOp = loadOps[loadIdx];
        attachment.storeOp = storeOps[storeIdx];
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.samples = VkTypes::AsVkSampleFlags(TextureGetNumSamples(tex));
    }

    // use depth stencil attachments if pointer is not null
    if (loadInfo.depthStencilAttachment != CoreGraphics::InvalidTextureViewId)
    {
        TextureId tex = TextureViewGetTexture(loadInfo.depthStencilAttachment);
        TextureUsage usage = TextureGetUsage(tex);
        n_assert(CheckBits(usage, RenderTexture));

        VkAttachmentDescription& attachment = outAttachments[i];
        IndexT loadIdx = CheckBits(loadInfo.depthStencilFlags, AttachmentFlagBits::Load) ? 2 : CheckBits(loadInfo.depthStencilFlags, AttachmentFlagBits::Clear) ? 1 : 0;
        IndexT storeIdx = CheckBits(loadInfo.depthStencilFlags, AttachmentFlagBits::Store) ? 1 : 0;
        IndexT stencilLoadIdx = CheckBits(loadInfo.depthStencilFlags, AttachmentFlagBits::LoadStencil) ? 2 : CheckBits(loadInfo.depthStencilFlags, AttachmentFlagBits::ClearStencil) ? 1 : 0;
        IndexT stencilStoreIdx = CheckBits(loadInfo.depthStencilFlags, AttachmentFlagBits::StoreStencil) ? 1 : 0;
        attachment.flags = 0;
        attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        attachment.format = VkTypes::AsVkFormat(TextureGetPixelFormat(tex));
        attachment.loadOp = loadOps[loadIdx];
        attachment.storeOp = storeOps[storeIdx];
        attachment.stencilLoadOp = loadOps[stencilLoadIdx];
        attachment.stencilStoreOp = storeOps[stencilStoreIdx];
        attachment.samples = VkTypes::AsVkSampleFlags(TextureGetNumSamples(tex));
        numUsedAttachmentsTotal++;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SetupPass(const PassId pid)
{
    Ids::Id32 id = pid.id24;
    VkPassLoadInfo& loadInfo = passAllocator.Get<Pass_LoadInfo>(id);
    VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<Pass_RuntimeInfo>(id);
    VkRenderPassBeginInfo& beginInfo = passAllocator.Get<Pass_BeginInfo>(id);
    Util::Array<uint32_t>& subpassAttachmentCounts = passAllocator.Get<Pass_SubpassAttachments>(id);

    Util::FixedArray<VkImageView> images;
    images.Resize(loadInfo.colorAttachments.Size() + (loadInfo.depthStencilAttachment != InvalidTextureViewId ? 1 : 0));

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t layers = 0;

    IndexT i = 0;

    // if we have no attachments, use the depth attachment to setup the viewport
    if (loadInfo.colorAttachments.Size() == 0)
    {
        n_assert(loadInfo.depthStencilAttachment != CoreGraphics::InvalidTextureViewId);
        TextureId tex = TextureViewGetTexture(loadInfo.depthStencilAttachment);
        const CoreGraphics::TextureDimensions dims = TextureGetDimensions(tex);
        VkRect2D& rect = loadInfo.rects[0];
        rect.offset.x = 0;
        rect.offset.y = 0;
        rect.extent.width = dims.width;
        rect.extent.height = dims.height;
        VkViewport& viewport = loadInfo.viewports[0];
        viewport.width = (float)dims.width;
        viewport.height = (float)dims.height;
        viewport.minDepth = 0;
        viewport.maxDepth = 1;
        viewport.x = 0;
        viewport.y = 0;
    }
    else
    {
        // otherwise, use the render targets to decide viewports
        for (i = 0; i < loadInfo.colorAttachments.Size(); i++)
        {
            images[i] = TextureViewGetVk(loadInfo.colorAttachments[i]);
            TextureId tex = TextureViewGetTexture(loadInfo.colorAttachments[i]);
            const CoreGraphics::TextureDimensions dims = TextureGetDimensions(tex);
            width = Math::max(width, (uint32_t)dims.width);
            height = Math::max(height, (uint32_t)dims.height);
            layers = Math::max(layers, (uint32_t)TextureGetNumLayers(tex));

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
    }

    Util::FixedArray<VkSubpassDescription> subpassDescs;
    Util::Array<VkSubpassDependency> subpassDeps;
    Util::FixedArray<VkAttachmentDescription> attachments;

    // resize subpass contents
    subpassDescs.Resize(loadInfo.subpasses.Size());
    attachments.Resize(loadInfo.colorAttachments.Size() + 1);
    runtimeInfo.subpassViewports.Resize(loadInfo.subpasses.Size());
    runtimeInfo.subpassRects.Resize(loadInfo.subpasses.Size());
    runtimeInfo.subpassPipelineInfo.Resize(loadInfo.subpasses.Size());
    uint32 numUsedAttachmentsTotal = 0;

    // run subpass info fetcher
    GetSubpassInfo(
        loadInfo
        , subpassDescs
        , subpassDeps
        , attachments
        , subpassAttachmentCounts
        , runtimeInfo.subpassViewports
        , runtimeInfo.subpassRects
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

    if (loadInfo.depthStencilAttachment != CoreGraphics::InvalidTextureViewId)
    {
        TextureId tex = TextureViewGetTexture(loadInfo.depthStencilAttachment);
        images[i] = TextureViewGetVk(loadInfo.depthStencilAttachment);
        const CoreGraphics::TextureDimensions dims = TextureGetDimensions(tex);

        width = Math::max(width, (uint32_t)dims.width);
        height = Math::max(height, (uint32_t)dims.height);
        layers = Math::max(layers, (uint32_t)TextureGetNumLayers(tex));
    }

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

    // setup uniform buffer for render target information
    ShaderId sid = ShaderServer::Instance()->GetShader("shd:shared.fxb"_atm);
    loadInfo.passBlockBuffer = CoreGraphics::ShaderCreateConstantBuffer(sid, "PassBlock");
    loadInfo.renderTargetDimensionsVar = ShaderGetConstantBinding(sid, "RenderTargetDimensions");

    CoreGraphics::ResourceTableLayoutId tableLayout = ShaderGetResourceTableLayout(sid, NEBULA_PASS_GROUP);
    runtimeInfo.passDescriptorSet = CreateResourceTable(ResourceTableCreateInfo{ tableLayout });
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
    Util::FixedArray<Math::vec4> dimensions(loadInfo.colorAttachments.Size());
    for (i = 0; i < loadInfo.colorAttachments.Size(); i++)
    {
        // update descriptor set based on images attachments
        IndexT inputAttachmentLocation = ShaderGetResourceSlot(sid, Util::String::Sprintf("InputAttachment%d", i));
        n_assert(inputAttachmentLocation != InvalidIndex);

        n_assert(loadInfo.colorAttachments.Size() < 16); // only allow 8 input attachments in the shader, so we must limit it
        CoreGraphics::ResourceTableInputAttachment write;
        write.tex = loadInfo.colorAttachments[i];
        write.isDepth = false;
        write.sampler = InvalidSamplerId;
        write.slot = inputAttachmentLocation;
        write.index = 0;
        ResourceTableSetInputAttachment(runtimeInfo.passDescriptorSet, write);

        // create dimensions vec4
        TextureId tex = TextureViewGetTexture(loadInfo.colorAttachments[i]);
        const CoreGraphics::TextureDimensions rtdims = TextureGetDimensions(tex);
        Math::vec4& dims = dimensions[i];
        dims = Math::vec4((Math::scalar)rtdims.width, (Math::scalar)rtdims.height, 1 / (Math::scalar)rtdims.width, 1 / (Math::scalar)rtdims.height);
    }
    if (loadInfo.depthStencilAttachment != CoreGraphics::InvalidTextureViewId)
    {
        // update descriptor set based on images attachments
        IndexT inputAttachmentLocation = ShaderGetResourceSlot(sid, Util::String::Sprintf("DepthAttachment", i));
        n_assert(inputAttachmentLocation != InvalidIndex);
        
        CoreGraphics::ResourceTableInputAttachment write;
        write.tex = loadInfo.depthStencilAttachment;
        write.isDepth = true;
        write.sampler = InvalidSamplerId;
        write.slot = inputAttachmentLocation;
        write.index = 0;
        ResourceTableSetInputAttachment(runtimeInfo.passDescriptorSet, write);
    }
    BufferUpdateArray(loadInfo.passBlockBuffer, dimensions.Begin(), dimensions.Size(), loadInfo.renderTargetDimensionsVar);
    ResourceTableCommitChanges(runtimeInfo.passDescriptorSet);

    // setup info
    runtimeInfo.framebufferPipelineInfo.renderPass = loadInfo.pass;
    runtimeInfo.framebufferPipelineInfo.subpass = 0;
    runtimeInfo.framebufferPipelineInfo.pViewportState = &runtimeInfo.subpassPipelineInfo[0];

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
    VkPassLoadInfo& loadInfo = passAllocator.Get<Pass_LoadInfo>(id);

    loadInfo.colorAttachments = info.colorAttachments;
    loadInfo.colorAttachmentClears = info.colorAttachmentClears;
    loadInfo.depthStencilAttachment = info.depthStencilAttachment;
    loadInfo.dev = Vulkan::GetCurrentDevice();

    SizeT numImages = info.colorAttachments.Size() == 0 ? 1 : info.colorAttachments.Size();
    loadInfo.clearValues.Resize(numImages + (loadInfo.depthStencilAttachment != CoreGraphics::InvalidTextureViewId ? 1 : 0));
    loadInfo.rects.Resize(numImages);
    loadInfo.viewports.Resize(numImages);
    loadInfo.name = info.name;
    
    loadInfo.subpasses = info.subpasses;
    loadInfo.colorAttachmentFlags = info.colorAttachmentFlags;
    loadInfo.depthStencilFlags = info.depthStencilFlags;
    
    IndexT i;
    for (i = 0; i < info.colorAttachments.Size(); i++)
    {
        const Math::vec4& value = loadInfo.colorAttachmentClears[i];
        VkClearValue& clear = loadInfo.clearValues[i];
        clear.color.float32[0] = value.x;
        clear.color.float32[1] = value.y;
        clear.color.float32[2] = value.z;
        clear.color.float32[3] = value.w;
    }
    
    if (loadInfo.depthStencilAttachment != CoreGraphics::InvalidTextureViewId)
    {
        VkClearValue& clear = loadInfo.clearValues[i];
        clear.depthStencil.depth = info.clearDepth;
        clear.depthStencil.stencil = info.clearStencil;
    }

    PassId ret;
    ret.id24 = id;
    ret.id8 = PassIdType;

    SetupPass(ret);

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyPass(const PassId id)
{
    VkPassLoadInfo& loadInfo = passAllocator.Get<Pass_LoadInfo>(id.id24);
    VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<Pass_RuntimeInfo>(id.id24);
    VkRenderPassBeginInfo& beginInfo = passAllocator.Get<Pass_BeginInfo>(id.id24);

    for (IndexT i = 0; i < loadInfo.colorAttachments.Size(); i++)
        CoreGraphics::DestroyTextureView(loadInfo.colorAttachments[i]);
    if (loadInfo.depthStencilAttachment != CoreGraphics::InvalidTextureViewId)
        CoreGraphics::DestroyTextureView(loadInfo.depthStencilAttachment);

    // destroy pass and our descriptor set
    DestroyResourceTable(runtimeInfo.passDescriptorSet);
    DestroyBuffer(loadInfo.passBlockBuffer);
    vkDestroyRenderPass(loadInfo.dev, loadInfo.pass, nullptr);
    vkDestroyFramebuffer(loadInfo.dev, loadInfo.framebuffer, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
void
PassBegin(const PassId id, PassRecordMode recordMode)
{
    VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<Pass_RuntimeInfo>(id.id24);

    // bind descriptor set for pass resources
    CoreGraphics::SetResourceTable(runtimeInfo.passDescriptorSet, NEBULA_PASS_GROUP, CoreGraphics::GraphicsPipeline, nullptr);

    // update framebuffer pipeline info to next subpass
    runtimeInfo.currentSubpassIndex = 0;
    runtimeInfo.framebufferPipelineInfo.subpass = 0;
    runtimeInfo.framebufferPipelineInfo.pViewportState = &runtimeInfo.subpassPipelineInfo[0];
    runtimeInfo.recordMode = recordMode;

    CoreGraphics::BeginPass(id, recordMode);
}

//------------------------------------------------------------------------------
/**
*/
void
PassNextSubpass(const PassId id)
{
    VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<Pass_RuntimeInfo>(id.id24);
    runtimeInfo.currentSubpassIndex++;
    runtimeInfo.framebufferPipelineInfo.subpass = runtimeInfo.currentSubpassIndex;
    runtimeInfo.framebufferPipelineInfo.pViewportState = &runtimeInfo.subpassPipelineInfo[runtimeInfo.currentSubpassIndex];

    CoreGraphics::SetToNextSubpass(runtimeInfo.recordMode);
}

//------------------------------------------------------------------------------
/**
*/
void
PassEnd(const PassId id)
{
    VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<Pass_RuntimeInfo>(id.id24);
    CoreGraphics::EndPass(runtimeInfo.recordMode);
}

//------------------------------------------------------------------------------
/**
*/
void 
PassApplyClipSettings(const PassId id)
{
    VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<Pass_RuntimeInfo>(id.id24);
    const Util::FixedArray<VkViewport>& viewports = runtimeInfo.subpassViewports[runtimeInfo.currentSubpassIndex];
    CoreGraphics::SetVkViewports(viewports.Begin(), viewports.Size());

    const Util::FixedArray<VkRect2D>& scissors = runtimeInfo.subpassRects[runtimeInfo.currentSubpassIndex];
    CoreGraphics::SetVkScissorRects(scissors.Begin(), scissors.Size());
}

//------------------------------------------------------------------------------
/**
*/
void
PassWindowResizeCallback(const PassId id)
{
    VkPassLoadInfo& loadInfo = passAllocator.Get<Pass_LoadInfo>(id.id24);
    VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<Pass_RuntimeInfo>(id.id24);

    // destroy pass and our descriptor set
    DestroyResourceTable(runtimeInfo.passDescriptorSet);
    DestroyBuffer(loadInfo.passBlockBuffer);
    vkDestroyRenderPass(loadInfo.dev, loadInfo.pass, nullptr);
    vkDestroyFramebuffer(loadInfo.dev, loadInfo.framebuffer, nullptr);

    // update attachments because their underlying textures might have changed
    for (IndexT i = 0; i < loadInfo.colorAttachments.Size(); i++)
    {
        CoreGraphics::TextureViewReload(loadInfo.colorAttachments[i]);
    }
    if (loadInfo.depthStencilAttachment != CoreGraphics::InvalidTextureViewId)
    {
        CoreGraphics::TextureViewReload(loadInfo.depthStencilAttachment);
    }

    // setup pass again
    SetupPass(id);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CoreGraphics::TextureViewId>&
PassGetAttachments(const CoreGraphics::PassId id)
{
    return passAllocator.Get<Pass_LoadInfo>(id.id24).colorAttachments;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::TextureViewId 
PassGetDepthStencilAttachment(const CoreGraphics::PassId id)
{
    return passAllocator.Get<Pass_LoadInfo>(id.id24).depthStencilAttachment;
}

//------------------------------------------------------------------------------
/**
*/
const uint32_t
PassGetNumSubpassAttachments(const CoreGraphics::PassId id, const IndexT subpass)
{
    return passAllocator.Get<Pass_SubpassAttachments>(id.id24)[subpass];
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom 
PassGetName(const CoreGraphics::PassId id)
{
    return passAllocator.Get<Pass_LoadInfo>(id.id24).name;
}

} // namespace Vulkan
