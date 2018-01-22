//------------------------------------------------------------------------------
// vkpass.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkpass.h"
#include "vkrenderdevice.h"
#include "vktypes.h"
#include "coregraphics/rendertexture.h"
#include "coregraphics/config.h"
#include "coregraphics/shaderserver.h"
#include "vkconstantbuffer.h"
#include "vkrendertexture.h"
#include "vkshader.h"

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
	Ids::Id32 id = passAllocator.AllocObject();
	VkPassLoadInfo& loadInfo = passAllocator.Get<0>(id);
	VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<1>(id);
	VkRenderPassBeginInfo& beginInfo = passAllocator.Get<2>(id);

	ShaderId sid = ShaderServer::Instance()->GetShader("shd:shared"_atm);
	loadInfo.shaderState = ShaderCreateState(sid, { NEBULAT_PASS_GROUP }, false);
	VkDescriptorSetLayout layout = shaderPool->GetDescriptorSetLayout(sid, NEBULAT_PASS_GROUP);
	runtimeInfo.passPipelineLayout = shaderPool->GetPipelineLayout(sid);

	loadInfo.colorAttachments = info.colorAttachments;
	loadInfo.colorAttachmentClears = info.colorAttachmentClears;
	loadInfo.depthStencilAttachment = info.depthStencilAttachment;
	loadInfo.dev = VkRenderDevice::Instance()->GetCurrentDevice();
	loadInfo.pool = VkRenderDevice::Instance()->GetCurrentDescriptorPool();

	// create descriptor set used by our pass
	VkDescriptorSetAllocateInfo descInfo =
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		loadInfo.pool,
		1,
		&layout
	};
	VkResult res = vkAllocateDescriptorSets(loadInfo.dev, &descInfo, &runtimeInfo.passDescriptorSet);
	n_assert(res == VK_SUCCESS);

	// gather image views
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t layers = 0;
	Util::FixedArray<VkImageView> images;
	images.Resize(info.colorAttachments.Size() + (info.depthStencilAttachment != RenderTextureId::Invalid() ? 1 : 0));
	loadInfo.clearValues.Resize(images.Size());
	loadInfo.rects.Resize(images.Size());
	loadInfo.viewports.Resize(images.Size());

	IndexT i;
	for (i = 0; i < info.colorAttachments.Size(); i++)
	{
		images[i] = RenderTextureGetVkImageView(info.colorAttachments[i]);
		const CoreGraphics::TextureDimensions dims = RenderTextureGetDimensions(info.colorAttachments[i]);
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
		runtimeInfo.subpassViewports[i].Resize(subpass.attachments.Size());
		runtimeInfo.subpassRects[i].Resize(subpass.attachments.Size());

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
		for (j = 0; j < info.colorAttachments.Size(); j++) allAttachments.Append(j);

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
			allAttachments.EraseIndex(allAttachments.FindIndex(ref.attachment));
			if (subpass.resolve) resolves[j] = ref;
		}

		for (; j < info.colorAttachments.Size(); j++)
		{
			VkAttachmentReference& ref = references[j];
			ref.attachment = VK_ATTACHMENT_UNUSED;
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			preserves[preserveAttachments] = allAttachments[preserveAttachments];
			preserveAttachments++;
		}

		for (j = 0; j < subpass.inputs.Size(); j++)
		{
			VkAttachmentReference& ref = inputs[j];
			ref.attachment = subpass.inputs[j];
			ref.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		for (j = 0; j < subpass.dependencies.Size(); j++)
		{
			VkSubpassDependency dep;
			dep.srcSubpass = subpass.dependencies[j];
			dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dep.dstSubpass = i;
			dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
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
			vksubpass.preserveAttachmentCount = preserves.Size();
			vksubpass.pPreserveAttachments = preserves.IsEmpty() ? nullptr : preserves.Begin();
		}
		else
		{
			vksubpass.preserveAttachmentCount = 0;
		}
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
		VkFormat fmt = VkTypes::AsVkFormat(RenderTextureGetPixelFormat(info.colorAttachments[i]));
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
	if (info.depthStencilAttachment != Ids::InvalidId32)
	{
		VkAttachmentDescription& attachment = attachments[i];
		IndexT loadIdx = info.depthStencilFlags & Load ? 2 : info.depthStencilFlags & Clear ? 1 : 0;
		IndexT storeIdx = info.depthStencilFlags & Store ? 1 : 0;
		IndexT stencilLoadIdx = info.depthStencilFlags & LoadStencil ? 2 : info.depthStencilFlags & ClearStencil ? 1 : 0;
		IndexT stencilStoreIdx = info.depthStencilFlags & StoreStencil ? 1 : 0;
		attachment.flags = 0;
		attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		attachment.format = VkTypes::AsVkFormat(RenderTextureGetPixelFormat(info.depthStencilAttachment));
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
	res = vkCreateRenderPass(loadInfo.dev, &rpinfo, nullptr, &loadInfo.pass);
	n_assert(res == VK_SUCCESS);

	if (info.depthStencilAttachment != Ids::InvalidId32)
	{
		images[i] = RenderTextureGetVkImageView(info.depthStencilAttachment);
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
	ConstantBufferCreateInfo cbinfo = { true, loadInfo.shaderState, "PassBlock", 0, 1 };
	loadInfo.passBlockBuffer = CreateConstantBuffer(cbinfo);
	loadInfo.passBlockVar = ShaderStateGetVariable(loadInfo.shaderState, "PassBlock");
	loadInfo.renderTargetDimensionsVar = ConstantBufferCreateShaderVariable(loadInfo.passBlockBuffer, 0, "RenderTargetDimensions");
		//loadInfo.passBlockBuffer->GetVariableByName("RenderTargetDimensions");

	VkWriteDescriptorSet write;
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = nullptr;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.dstBinding = VkShaderGetVkShaderVariableBinding(loadInfo.shaderState, loadInfo.passBlockVar);
	write.dstArrayElement = 0;
	write.dstSet = runtimeInfo.passDescriptorSet;

	VkDescriptorBufferInfo buf;
	buf.buffer = ConstantBufferGetVk(loadInfo.passBlockBuffer);
	buf.offset = 0;
	buf.range = VK_WHOLE_SIZE;
	write.pBufferInfo = &buf;
	write.pImageInfo = nullptr;
	write.pTexelBufferView = nullptr;

	// update descriptor set with attachment
	vkUpdateDescriptorSets(loadInfo.dev, 1, &write, 0, nullptr);

	// update descriptor set based on images attachments
	const CoreGraphics::ShaderVariableId inputAttachmentsVar = ShaderStateGetVariable(loadInfo.shaderState, "InputAttachments");

	// setup input attachments
	Util::FixedArray<Math::float4> dimensions(info.colorAttachments.Size());
	for (i = 0; i < info.colorAttachments.Size(); i++)
	{
		VkWriteDescriptorSet write;
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = NULL;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		write.dstBinding = VkShaderGetVkShaderVariableBinding(loadInfo.shaderState, inputAttachmentsVar);
		write.dstArrayElement = i;
		write.dstSet = runtimeInfo.passDescriptorSet;

		VkDescriptorImageInfo img;
		img.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		img.imageView = RenderTextureGetVkImageView(info.colorAttachments[i]);
		img.sampler = VK_NULL_HANDLE;
		write.pImageInfo = &img;

		// update descriptor set with attachment
		vkUpdateDescriptorSets(loadInfo.dev, 1, &write, 0, NULL);

		// create dimensions float4
		const CoreGraphics::TextureDimensions rtdims = RenderTextureGetDimensions(info.depthStencilAttachment);
		Math::float4& dims = dimensions[i];
		dims.x() = (Math::scalar)rtdims.width;
		dims.y() = (Math::scalar)rtdims.height;
		dims.z() = 1 / dims.x();
		dims.w() = 1 / dims.y();
	}
	ShaderVariableSetArray(loadInfo.renderTargetDimensionsVar, loadInfo.shaderState, dimensions.Begin(), dimensions.Size());
	VkShaderStateSetDescriptorSet(loadInfo.shaderState, runtimeInfo.passDescriptorSet, NEBULAT_PASS_GROUP);
	VkShaderStateSetShared(loadInfo.shaderState, true);

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
DiscardPass(const PassId& id)
{
	VkPassLoadInfo& loadInfo = passAllocator.Get<0>(id.id24);
	VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<1>(id.id24);
	VkRenderPassBeginInfo& beginInfo = passAllocator.Get<2>(id.id24);

	// destroy pass and our descriptor set
	vkFreeDescriptorSets(loadInfo.dev, loadInfo.pool, 1, &runtimeInfo.passDescriptorSet);
	vkDestroyRenderPass(loadInfo.dev, loadInfo.pass, NULL);
	vkDestroyFramebuffer(loadInfo.dev, loadInfo.framebuffer, NULL);
}

//------------------------------------------------------------------------------
/**
*/
void
PassBegin(const PassId& id)
{
	VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<1>(id.id24);
	VkRenderDevice* dev = VkRenderDevice::Instance();

	// bind descriptor set
	static const uint32_t offset = 0;
	dev->BindDescriptorsGraphics(&runtimeInfo.passDescriptorSet, runtimeInfo.passPipelineLayout, NEBULAT_PASS_GROUP, 1, &offset, 1, true);

	// update framebuffer pipeline info to next subpass
	runtimeInfo.currentSubpassIndex = 0;
	runtimeInfo.framebufferPipelineInfo.subpass = runtimeInfo.currentSubpassIndex;
}

//------------------------------------------------------------------------------
/**
*/
void
PassBeginBatch(const PassId& id, Models::FrameBatchType::Code batch)
{
}

//------------------------------------------------------------------------------
/**
*/
void
PassNextSubpass(const PassId& id)
{
	VkPassRuntimeInfo& runtimeInfo = passAllocator.Get<1>(id.id24);
	runtimeInfo.currentSubpassIndex++;
}

//------------------------------------------------------------------------------
/**
*/
void
PassEndBatch(const PassId& id)
{
}

//------------------------------------------------------------------------------
/**
*/
void
PassEnd(const PassId& id)
{

}

//------------------------------------------------------------------------------
/**
*/
void
PassWindowResizeCallback(const PassId& id)
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
		images[i] = RenderTextureGetVkImageView(loadInfo.colorAttachments[i]);
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
	ShaderVariableSetArray(loadInfo.renderTargetDimensionsVar, loadInfo.shaderState, dimensions.Begin(), dimensions.Size());
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CoreGraphics::RenderTextureId>&
PassGetAttachments(const CoreGraphics::PassId& id, const IndexT subpass)
{
	return passAllocator.Get<0>(id.id24).colorAttachments;
}

} // namespace Vulkan