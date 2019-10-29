//------------------------------------------------------------------------------
// vkpass.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkpass.h"
#include "vkgraphicsdevice.h"
#include "vktypes.h"
#include "coregraphics/rendertexture.h"
#include "coregraphics/config.h"
#include "coregraphics/shaderserver.h"
#include "vkconstantbuffer.h"
#include "vkrendertexture.h"
#include "vkshader.h"
#include "vkresourcetable.h"

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
	return passAllocator.Get<2>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const VkGraphicsPipelineCreateInfo&
PassGetVkFramebufferInfo(const CoreGraphics::PassId& id)
{
	return passAllocator.Get<1>(id.id24).framebufferPipelineInfo;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT 
PassGetVkNumAttachments(const CoreGraphics::PassId& id)
{
	return passAllocator.Get<0>(id.id24).colorAttachments.Size();
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<VkRect2D>&
PassGetVkRects(const CoreGraphics::PassId& id)
{
	const VkPassRuntimeInfo& info = passAllocator.Get<1>(id.id24);
	return info.subpassRects[info.currentSubpassIndex];
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<VkViewport>&
PassGetVkViewports(const CoreGraphics::PassId& id)
{
	const VkPassRuntimeInfo& info = passAllocator.Get<1>(id.id24);
	return info.subpassViewports[info.currentSubpassIndex];
}

} // namespace Vulkan

namespace CoreGraphics
{

using namespace Vulkan;
//------------------------------------------------------------------------------
/**
*/
const PassId
CreatePass(const PassCreateInfo& info)
{
	n_assert(info.subpasses.Size() > 0);
	Ids::Id32 id = passAllocator.Alloc();
	VkPassLoadInfo& loadInfo = passAllocator.Get<0>(id);
	VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<1>(id);
	VkRenderPassBeginInfo& beginInfo = passAllocator.Get<2>(id);
	Util::Array<uint32_t>& subpassAttachmentCounts = passAllocator.Get<3>(id);

	ShaderId sid = ShaderServer::Instance()->GetShader("shd:shared.fxb"_atm);

	loadInfo.colorAttachments = info.colorAttachments;
	loadInfo.colorAttachmentClears = info.colorAttachmentClears;
	loadInfo.depthStencilAttachment = info.depthStencilAttachment;
	loadInfo.dev = Vulkan::GetCurrentDevice();

	// gather image views
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t layers = 0;
	Util::FixedArray<VkImageView> images;
	images.Resize(info.colorAttachments.Size() + (info.depthStencilAttachment != RenderTextureId::Invalid() ? 1 : 0));
	loadInfo.clearValues.Resize(images.Size());
	loadInfo.rects.Resize(images.Size());
	loadInfo.viewports.Resize(images.Size());
	loadInfo.name = info.name;

	IndexT i;
	for (i = 0; i < info.colorAttachments.Size(); i++)
	{
		images[i] = RenderTextureGetVkAttachmentImageView(info.colorAttachments[i]);
		const CoreGraphics::TextureDimensions dims = RenderTextureGetDimensions(info.colorAttachments[i]);
		width = Math::n_max(width, (uint32_t)dims.width);
		height = Math::n_max(height, (uint32_t)dims.height);
		layers = Math::n_max(layers, (uint32_t)RenderTextureGetNumLayers(info.colorAttachments[i]));

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

		const Math::float4& value = info.colorAttachmentClears[i];
		VkClearValue& clear = loadInfo.clearValues[i];
		clear.color.float32[0] = value.x();
		clear.color.float32[1] = value.y();
		clear.color.float32[2] = value.z();
		clear.color.float32[3] = value.w();
	}

	Util::FixedArray<VkSubpassDescription> subpassDescs;
	Util::FixedArray<Util::FixedArray<VkAttachmentReference>> subpassReferences;
	Util::FixedArray<Util::FixedArray<VkAttachmentReference>> subpassInputs;
	Util::FixedArray<Util::FixedArray<uint32_t>> subpassPreserves;
	Util::FixedArray<Util::FixedArray<VkAttachmentReference>> subpassResolves;
	Util::FixedArray<VkAttachmentReference> subpassDepthStencils;
	Util::Array<VkSubpassDependency> subpassDeps;

	// resize subpass contents
	subpassDescs.Resize(info.subpasses.Size());
	subpassReferences.Resize(info.subpasses.Size());
	subpassInputs.Resize(info.subpasses.Size());
	subpassPreserves.Resize(info.subpasses.Size());
	subpassResolves.Resize(info.subpasses.Size());
	subpassDepthStencils.Resize(info.subpasses.Size());
	runtimeInfo.subpassViewports.Resize(info.subpasses.Size());
	runtimeInfo.subpassRects.Resize(info.subpasses.Size());
	runtimeInfo.subpassPipelineInfo.Resize(info.subpasses.Size());
	Util::FixedArray<bool> subpassHasDependencies(info.subpasses.Size(), false);

	for (i = 0; i < info.subpasses.Size(); i++)
	{
		const CoreGraphics::Subpass& subpass = info.subpasses[i];

		VkSubpassDescription& vksubpass = subpassDescs[i];
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
		runtimeInfo.subpassViewports[i].Resize(subpass.attachments.Size());
		runtimeInfo.subpassRects[i].Resize(subpass.attachments.Size());
		runtimeInfo.subpassPipelineInfo[i] =
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
		references.Resize(info.colorAttachments.Size());
		inputs.Resize(subpass.inputs.Size());
		preserves.Resize(info.colorAttachments.Size() - subpass.attachments.Size());
		if (subpass.resolve) resolves.Resize(subpass.attachments.Size());

		// if subpass binds depth, the slot for the depth-stencil buffer is color attachments + 1
		if (subpass.bindDepth)
		{
			n_assert(info.depthStencilAttachment != RenderTextureId::Invalid());
			VkAttachmentReference& ds = subpassDepthStencils[i];
			ds.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			ds.attachment = info.colorAttachments.Size();
			vksubpass.pDepthStencilAttachment = &ds;
		}
		else
		{
			VkAttachmentReference& ds = subpassDepthStencils[i];
			ds.layout = VK_IMAGE_LAYOUT_UNDEFINED;
			ds.attachment = VK_ATTACHMENT_UNUSED;
			vksubpass.pDepthStencilAttachment = &ds;
		}

		// fill list of all attachments, will be removed per subpass attachment
		IndexT j;
		Util::Array<IndexT> allAttachments;
		for (j = 0; j < info.colorAttachments.Size(); j++)
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
			runtimeInfo.subpassViewports[i][j] = loadInfo.viewports[ref.attachment];
			runtimeInfo.subpassRects[i][j] = loadInfo.rects[ref.attachment];
			usedAttachments++;

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

		if (subpass.dependencies.Size() > 0) for (j = 0; j < subpass.dependencies.Size(); j++)
		{
			VkSubpassDependency dep;
			subpassHasDependencies[subpass.dependencies[j]] = true;
			dep.srcSubpass = subpass.dependencies[j];
			dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dep.dstSubpass = i;
			dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			subpassDeps.Append(dep);
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

		subpassAttachmentCounts.Append(vksubpass.colorAttachmentCount);
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

	uint32_t numUsedAttachments = (uint32_t)info.colorAttachments.Size();
	Util::FixedArray<VkAttachmentDescription> attachments;
	attachments.Resize(info.colorAttachments.Size() + 1);
	for (i = 0; i < info.colorAttachments.Size(); i++)
	{
		VkFormat fmt = VkTypes::AsVkFramebufferFormat(RenderTextureGetPixelFormat(info.colorAttachments[i]));
		VkAttachmentDescription& attachment = attachments[i];
		IndexT loadIdx = info.colorAttachmentFlags[i] & Load ? 2 : info.colorAttachmentFlags[i] & Clear ? 1 : 0;
		IndexT storeIdx = info.colorAttachmentFlags[i] & Store ? 1 : 0;
		attachment.flags = 0;
		attachment.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		attachment.format = fmt;
		attachment.loadOp = loadOps[loadIdx];
		attachment.storeOp = storeOps[storeIdx];
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.samples = RenderTextureGetMSAA(info.colorAttachments[i]) ? VK_SAMPLE_COUNT_16_BIT : VK_SAMPLE_COUNT_1_BIT;
	}

	// use depth stencil attachments if pointer is not null
	if (info.depthStencilAttachment != CoreGraphics::RenderTextureId::Invalid())
	{
		VkAttachmentDescription& attachment = attachments[i];
		IndexT loadIdx = info.depthStencilFlags & Load ? 2 : info.depthStencilFlags & Clear ? 1 : 0;
		IndexT storeIdx = info.depthStencilFlags & Store ? 1 : 0;
		IndexT stencilLoadIdx = info.depthStencilFlags & LoadStencil ? 2 : info.depthStencilFlags & ClearStencil ? 1 : 0;
		IndexT stencilStoreIdx = info.depthStencilFlags & StoreStencil ? 1 : 0;
		attachment.flags = 0;
		attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		attachment.format = VkTypes::AsVkFramebufferFormat(RenderTextureGetPixelFormat(info.depthStencilAttachment));
		attachment.loadOp = loadOps[loadIdx];
		attachment.storeOp = storeOps[storeIdx];
		attachment.stencilLoadOp = loadOps[stencilLoadIdx];
		attachment.stencilStoreOp = storeOps[stencilStoreIdx];
		attachment.samples = RenderTextureGetMSAA(info.depthStencilAttachment) ? VK_SAMPLE_COUNT_16_BIT : VK_SAMPLE_COUNT_1_BIT;
		numUsedAttachments++;
	}

	// create render pass
	VkRenderPassCreateInfo rpinfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr,
		0,
		numUsedAttachments,
		numUsedAttachments == 0 ? nullptr : attachments.Begin(),
		(uint32_t)subpassDescs.Size(),
		subpassDescs.IsEmpty() ? nullptr : subpassDescs.Begin(),
		(uint32_t)subpassDeps.Size(),
		subpassDeps.IsEmpty() ? nullptr : subpassDeps.Begin()
	};
	VkResult res = vkCreateRenderPass(loadInfo.dev, &rpinfo, nullptr, &loadInfo.pass);
	n_assert(res == VK_SUCCESS);

	if (info.depthStencilAttachment != CoreGraphics::RenderTextureId::Invalid())
	{
		images[i] = RenderTextureGetVkAttachmentImageView(info.depthStencilAttachment);
		const CoreGraphics::TextureDimensions dims = RenderTextureGetDimensions(info.depthStencilAttachment);
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

		VkClearValue& clear = loadInfo.clearValues[i];
		clear.depthStencil.depth = info.clearDepth;
		clear.depthStencil.stencil = info.clearStencil;
	}

	// setup render area
	loadInfo.renderArea.offset.x = 0;
	loadInfo.renderArea.offset.y = 0;
	loadInfo.renderArea.extent.width = width;
	loadInfo.renderArea.extent.height = height;

	// setup viewport info
	runtimeInfo.viewportInfo.pNext = nullptr;
	runtimeInfo.viewportInfo.flags = 0;
	runtimeInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	runtimeInfo.viewportInfo.scissorCount = loadInfo.rects.Size();
	runtimeInfo.viewportInfo.pScissors = loadInfo.rects.Size() > 0 ? loadInfo.rects.Begin() : 0;
	runtimeInfo.viewportInfo.viewportCount = loadInfo.viewports.Size();
	runtimeInfo.viewportInfo.pViewports = loadInfo.viewports.Size() > 0 ? loadInfo.viewports.Begin() : 0;

	// setup uniform buffer for render target information
	loadInfo.passBlockBuffer = CoreGraphics::ShaderCreateConstantBuffer(sid, "PassBlock");
	loadInfo.renderTargetDimensionsVar = ShaderGetConstantBinding(sid, "RenderTargetDimensions");

	CoreGraphics::ResourceTableLayoutId tableLayout = ShaderGetResourceTableLayout(sid, NEBULA_PASS_GROUP);
	runtimeInfo.passDescriptorSet = CreateResourceTable(ResourceTableCreateInfo{ tableLayout });
	runtimeInfo.passPipelineLayout = ShaderGetResourcePipeline(sid);

	CoreGraphics::ResourceTableConstantBuffer write;
	write.buf = loadInfo.passBlockBuffer;
	write.offset = 0;
	write.size = -1;
	write.index = 0;
	write.dynamicOffset = false;
	write.texelBuffer = false;
	write.slot = ConstantBufferGetSlot(loadInfo.passBlockBuffer);
	ResourceTableSetConstantBuffer(runtimeInfo.passDescriptorSet, write);

	// setup input attachments
	Util::FixedArray<Math::float4> dimensions(info.colorAttachments.Size());
	for (i = 0; i < info.colorAttachments.Size(); i++)
	{
		// update descriptor set based on images attachments
		IndexT inputAttachmentLocation = ShaderGetResourceSlot(sid, Util::String::Sprintf("InputAttachment%d", i));
		n_assert(inputAttachmentLocation != InvalidIndex);

		n_assert(info.colorAttachments.Size() < 16); // only allow 8 input attachments in the shader, so we must limit it
		CoreGraphics::ResourceTableInputAttachment write;
		write.tex = info.colorAttachments[i];
		write.isDepth = false;
		write.sampler = SamplerId::Invalid();
		write.slot = inputAttachmentLocation;
		write.index = 0;
		ResourceTableSetInputAttachment(runtimeInfo.passDescriptorSet, write);

		// create dimensions float4
		const CoreGraphics::TextureDimensions rtdims = RenderTextureGetDimensions(info.colorAttachments[i]);
		Math::float4& dims = dimensions[i];
		dims.x() = (Math::scalar)rtdims.width;
		dims.y() = (Math::scalar)rtdims.height;
		dims.z() = 1 / dims.x();
		dims.w() = 1 / dims.y();
	}
	if (info.depthStencilAttachment != CoreGraphics::RenderTextureId::Invalid())
	{
		// update descriptor set based on images attachments
		IndexT inputAttachmentLocation = ShaderGetResourceSlot(sid, Util::String::Sprintf("DepthAttachment", i));
		n_assert(inputAttachmentLocation != InvalidIndex);

		CoreGraphics::ResourceTableInputAttachment write;
		write.tex = info.depthStencilAttachment;
		write.isDepth = true;
		write.sampler = SamplerId::Invalid();
		write.slot = inputAttachmentLocation;
		write.index = 0;
		ResourceTableSetInputAttachment(runtimeInfo.passDescriptorSet, write);
	}
	ConstantBufferUpdateArray(loadInfo.passBlockBuffer, dimensions.Begin(), dimensions.Size(), loadInfo.renderTargetDimensionsVar);
	ResourceTableCommitChanges(runtimeInfo.passDescriptorSet);

	// create framebuffer
	VkFramebufferCreateInfo fbInfo =
	{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		NULL,
		0,
		loadInfo.pass,
		(uint32_t)images.Size(),
		images.Begin(),
		width,
		height,
		layers
	};
	res = vkCreateFramebuffer(loadInfo.dev, &fbInfo, NULL, &loadInfo.framebuffer);
	n_assert(res == VK_SUCCESS);

	// setup info
	runtimeInfo.framebufferPipelineInfo.renderPass = loadInfo.pass;
	runtimeInfo.framebufferPipelineInfo.subpass = 0;
	runtimeInfo.framebufferPipelineInfo.pViewportState = &runtimeInfo.viewportInfo;

	beginInfo = 
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		NULL,
		loadInfo.pass,
		loadInfo.framebuffer,
		loadInfo.renderArea,
		(uint32_t)loadInfo.clearValues.Size(),
		loadInfo.clearValues.Size() > 0 ? loadInfo.clearValues.Begin() : NULL
	};

	PassId ret;
	ret.id24 = id;
	ret.id8 = PassIdType;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DiscardPass(const PassId id)
{
	VkPassLoadInfo& loadInfo = passAllocator.Get<0>(id.id24);
	VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<1>(id.id24);
	VkRenderPassBeginInfo& beginInfo = passAllocator.Get<2>(id.id24);

	// destroy pass and our descriptor set
	DestroyResourceTable(runtimeInfo.passDescriptorSet);
	DestroyConstantBuffer(loadInfo.passBlockBuffer);
	vkDestroyRenderPass(loadInfo.dev, loadInfo.pass, NULL);
	vkDestroyFramebuffer(loadInfo.dev, loadInfo.framebuffer, NULL);
}

//------------------------------------------------------------------------------
/**
*/
void
PassBegin(const PassId id)
{
	VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<1>(id.id24);

	// bind descriptor set
	CoreGraphics::SetResourceTable(runtimeInfo.passDescriptorSet, NEBULA_PASS_GROUP, CoreGraphics::GraphicsPipeline, nullptr);

	// update framebuffer pipeline info to next subpass
	runtimeInfo.currentSubpassIndex = 0;
	runtimeInfo.framebufferPipelineInfo.subpass = 0;
	runtimeInfo.framebufferPipelineInfo.pViewportState = &runtimeInfo.subpassPipelineInfo[0];

	const Util::FixedArray<VkViewport>& viewports = runtimeInfo.subpassViewports[0];
	CoreGraphics::SetVkViewports(viewports.Begin(), viewports.Size());
	const Util::FixedArray<VkRect2D>& scissors = runtimeInfo.subpassRects[0];
	CoreGraphics::SetVkScissorRects(scissors.Begin(), scissors.Size());

	CoreGraphics::BeginPass(id);
}

//------------------------------------------------------------------------------
/**
*/
void
PassBeginBatch(const PassId id, Frame::FrameBatchType::Code batch)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
PassNextSubpass(const PassId id)
{
	VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<1>(id.id24);
	runtimeInfo.currentSubpassIndex++;
	runtimeInfo.framebufferPipelineInfo.subpass = runtimeInfo.currentSubpassIndex;
	runtimeInfo.framebufferPipelineInfo.pViewportState = &runtimeInfo.subpassPipelineInfo[runtimeInfo.currentSubpassIndex];

	const Util::FixedArray<VkViewport>& viewports = runtimeInfo.subpassViewports[runtimeInfo.currentSubpassIndex];
	CoreGraphics::SetVkViewports(viewports.Begin(), viewports.Size());
	const Util::FixedArray<VkRect2D>& scissors = runtimeInfo.subpassRects[runtimeInfo.currentSubpassIndex];
	CoreGraphics::SetVkScissorRects(scissors.Begin(), scissors.Size());

	CoreGraphics::SetToNextSubpass();
}

//------------------------------------------------------------------------------
/**
*/
void
PassEndBatch(const PassId id)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
PassEnd(const PassId id)
{
	CoreGraphics::EndPass();
}

//------------------------------------------------------------------------------
/**
*/
void 
PassApplyClipSettings(const PassId id)
{
	VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<1>(id.id24);
	const Util::FixedArray<VkViewport>& viewports = runtimeInfo.subpassViewports[runtimeInfo.currentSubpassIndex];
	SetVkViewports(viewports.Begin(), viewports.Size());

	const Util::FixedArray<VkRect2D>& scissors = runtimeInfo.subpassRects[runtimeInfo.currentSubpassIndex];
	SetVkScissorRects(scissors.Begin(), scissors.Size());
}

//------------------------------------------------------------------------------
/**
*/
void
PassWindowResizeCallback(const PassId id)
{
	VkPassLoadInfo& loadInfo = passAllocator.Get<0>(id.id24);
	VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<1>(id.id24);
	VkRenderPassBeginInfo& beginInfo = passAllocator.Get<2>(id.id24);

	// gather image views
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t layers = 0;
	Util::FixedArray<VkImageView> images;
	images.Resize(loadInfo.colorAttachments.Size() + (loadInfo.depthStencilAttachment == Ids::InvalidId32 ? 1 : 0));

	IndexT i;
	for (i = 0; i < loadInfo.colorAttachments.Size(); i++)
	{
		const CoreGraphics::TextureDimensions dims = RenderTextureGetDimensions(loadInfo.colorAttachments[i]);
		images[i] = RenderTextureGetVkAttachmentImageView(loadInfo.colorAttachments[i]);
		width = Math::n_max(width, (uint32_t)dims.width);
		height = Math::n_max(height, (uint32_t)dims.height);
		layers = Math::n_max(layers, (uint32_t)dims.depth);

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

		const Math::float4& value = loadInfo.colorAttachmentClears[i];
		VkClearValue& clear = loadInfo.clearValues[i];
		clear.color.float32[0] = value.x();
		clear.color.float32[1] = value.y();
		clear.color.float32[2] = value.z();
		clear.color.float32[3] = value.w();
	}

	// destroy old framebuffer
	vkDestroyFramebuffer(loadInfo.dev, loadInfo.framebuffer, nullptr);

	// create framebuffer
	VkFramebufferCreateInfo fbInfo =
	{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		NULL,
		0,
		loadInfo.pass,
		(uint32_t)images.Size(),
		images.Begin(),
		width,
		height,
		layers
	};

	// we need to recreate the framebuffer object because the image handles might have changed, and the dimensions to render to is different
	VkResult res;
	res = vkCreateFramebuffer(loadInfo.dev, &fbInfo, nullptr, &loadInfo.framebuffer);
	n_assert(res == VK_SUCCESS);

	// setup input attachments
	Util::FixedArray<Math::float4> dimensions(loadInfo.colorAttachments.Size());
	for (i = 0; i < loadInfo.colorAttachments.Size(); i++)
	{
		// create dimensions float4
		const CoreGraphics::TextureDimensions rtdims = RenderTextureGetDimensions(loadInfo.colorAttachments[i]);
		Math::float4& dims = dimensions[i];
		dims.x() = (Math::scalar)rtdims.width;
		dims.y() = (Math::scalar)rtdims.height;
		dims.z() = 1 / dims.x();
		dims.w() = 1 / dims.y();
	}
	ConstantBufferUpdateArray(loadInfo.passBlockBuffer, dimensions.Begin(), dimensions.Size(), loadInfo.renderTargetDimensionsVar);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CoreGraphics::RenderTextureId>&
PassGetAttachments(const CoreGraphics::PassId id)
{
	return passAllocator.Get<0>(id.id24).colorAttachments;
}

//------------------------------------------------------------------------------
/**
*/
const uint32_t
PassGetNumSubpassAttachments(const CoreGraphics::PassId id, const IndexT subpass)
{
	return passAllocator.Get<3>(id.id24)[subpass];
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom 
PassGetName(const CoreGraphics::PassId id)
{
	return passAllocator.Get<0>(id.id24).name;
}

} // namespace Vulkan