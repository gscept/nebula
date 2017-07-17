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

using namespace CoreGraphics;
namespace Vulkan
{

__ImplementClass(Vulkan::VkPass, 'VKFR', Base::PassBase);
//------------------------------------------------------------------------------
/**
*/
VkPass::VkPass()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkPass::~VkPass()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkPass::Setup()
{
	// setup base class
	PassBase::Setup();

	// create shader state for input attachments and render target dimensions
	this->shaderState = ShaderServer::Instance()->CreateShaderState("shd:shared", { NEBULAT_PASS_GROUP });
	VkDescriptorSetLayout layout = this->shaderState->GetShader()->GetDescriptorLayout(NEBULAT_PASS_GROUP);
	this->passPipelineLayout = this->shaderState->GetShader()->GetPipelineLayout();

	// create descriptor set used by our pass
	VkDescriptorSetAllocateInfo descInfo =
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		NULL,
		VkRenderDevice::descPool,
		1,
		&layout
	};
	VkResult res = vkAllocateDescriptorSets(VkRenderDevice::dev, &descInfo, &this->passDescriptorSet);
	n_assert(res == VK_SUCCESS);

	// gather image views
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t layers = 0;
	Util::FixedArray<VkImageView> images;
	images.Resize(this->colorAttachments.Size() + (this->depthStencilAttachment.isvalid() ? 1 : 0));
	this->clearValues.Resize(images.Size());
	this->scissorRects.Resize(images.Size());
	this->viewports.Resize(images.Size());

	IndexT i;
	for (i = 0; i < this->colorAttachments.Size(); i++)
	{
		images[i] = this->colorAttachments[i]->GetVkImageView();
		width = Math::n_max(width, this->colorAttachments[i]->GetWidth());
		height = Math::n_max(height, this->colorAttachments[i]->GetHeight());
		layers = Math::n_max(layers, this->colorAttachments[i]->GetLayers());

		VkRect2D& rect = scissorRects[i];
		rect.offset.x = 0;
		rect.offset.y = 0;
		rect.extent.width = this->colorAttachments[i]->GetWidth();
		rect.extent.height = this->colorAttachments[i]->GetHeight();
		VkViewport& viewport = viewports[i];
		viewport.width = (float)this->colorAttachments[i]->GetWidth();
		viewport.height = (float)this->colorAttachments[i]->GetHeight();
		viewport.minDepth = 0;
		viewport.maxDepth = 1;
		viewport.x = 0;
		viewport.y = 0;

		const Math::float4& value = this->colorAttachmentClears[i];
		VkClearValue& clear = this->clearValues[i];
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
	subpassDescs.Resize(this->subpasses.Size());
	subpassReferences.Resize(this->subpasses.Size());
	subpassInputs.Resize(this->subpasses.Size());
	subpassPreserves.Resize(this->subpasses.Size());
	subpassResolves.Resize(this->subpasses.Size());
	subpassDepthStencils.Resize(this->subpasses.Size());
	this->subpassViewports.Resize(this->subpasses.Size());
	this->subpassRects.Resize(this->subpasses.Size());

	for (i = 0; i < this->subpasses.Size(); i++)
	{
		const PassBase::Subpass& subpass = this->subpasses[i];

		VkSubpassDescription& vksubpass = subpassDescs[i];
		vksubpass.flags = 0;
		vksubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		vksubpass.pColorAttachments = NULL;
		vksubpass.pDepthStencilAttachment = NULL;
		vksubpass.pPreserveAttachments = NULL;
		vksubpass.pResolveAttachments = NULL;
		vksubpass.pInputAttachments = NULL;

		// resize rects
		this->subpassViewports[i].Resize(subpass.attachments.Size());
		this->subpassRects[i].Resize(subpass.attachments.Size());

		// get references to fixed arrays
		Util::FixedArray<VkAttachmentReference>& references = subpassReferences[i];
		Util::FixedArray<VkAttachmentReference>& inputs = subpassInputs[i];
		Util::FixedArray<uint32_t>& preserves = subpassPreserves[i];
		Util::FixedArray<VkAttachmentReference>& resolves = subpassResolves[i];

		// resize arrays straight away since we already know the size
		references.Resize(this->colorAttachments.Size());
		inputs.Resize(subpass.inputs.Size());
		preserves.Resize(this->colorAttachments.Size() - subpass.attachments.Size());
		if (subpass.resolve) resolves.Resize(subpass.attachments.Size());

		// if subpass binds depth, the slot for the depth-stencil buffer is color attachments + 1
		if (subpass.bindDepth)
		{
			VkAttachmentReference& ds = subpassDepthStencils[i];
			ds.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			ds.attachment = this->colorAttachments.Size();
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
		for (j = 0; j < this->colorAttachments.Size(); j++) allAttachments.Append(j);

		IndexT idx = 0;
		SizeT preserveAttachments = 0;
		SizeT usedAttachments = 0;
		for (j = 0; j < subpass.attachments.Size(); j++)
		{
			VkAttachmentReference& ref = references[j];
			ref.attachment = subpass.attachments[j];
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			subpassViewports[i][j] = this->viewports[ref.attachment];
			subpassRects[i][j] = this->scissorRects[ref.attachment];
			usedAttachments++;

			// remove from all attachments list
			allAttachments.EraseIndex(allAttachments.FindIndex(ref.attachment));
			if (subpass.resolve) resolves[j] = ref;
		}
	
		for (; j < this->colorAttachments.Size(); j++)
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
			vksubpass.pInputAttachments = inputs.IsEmpty() ? NULL : inputs.Begin();
		}
		else
		{
			vksubpass.inputAttachmentCount = 0;
		}
		
		// the rest are automatically preserve
		if (preserves.Size() > 0)
		{ 
			vksubpass.preserveAttachmentCount = preserves.Size();
			vksubpass.pPreserveAttachments = preserves.IsEmpty() ? NULL : preserves.Begin();
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

	uint32_t numUsedAttachments = (uint32_t)this->colorAttachments.Size();
	Util::FixedArray<VkAttachmentDescription> attachments;
	attachments.Resize(this->colorAttachments.Size() + 1);
	for (i = 0; i < this->colorAttachments.Size(); i++)
	{
		VkFormat fmt = VkTypes::AsVkFormat(this->colorAttachments[i]->GetPixelFormat());
		VkAttachmentDescription& attachment = attachments[i];
		IndexT loadIdx = this->colorAttachmentFlags[i] & Load ? 2 : this->colorAttachmentFlags[i] & Clear ? 1 : 0;
		IndexT storeIdx = this->colorAttachmentFlags[i] & Store ? 1 : 0;
		attachment.flags = 0;
		attachment.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		attachment.format = fmt;
		attachment.loadOp = loadOps[loadIdx];
		attachment.storeOp = storeOps[storeIdx];
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.samples = this->colorAttachments[i]->GetEnableMSAA() ? VK_SAMPLE_COUNT_16_BIT : VK_SAMPLE_COUNT_1_BIT;
	}

	// use depth stencil attachments if pointer is not null
	if (this->depthStencilAttachment.isvalid())
	{
		VkAttachmentDescription& attachment = attachments[i];
		IndexT loadIdx = this->depthStencilFlags & Load ? 2 : this->depthStencilFlags & Clear ? 1 : 0;
		IndexT storeIdx = this->depthStencilFlags & Store ? 1 : 0;
		IndexT stencilLoadIdx = this->depthStencilFlags & LoadStencil ? 2 : this->depthStencilFlags & ClearStencil ? 1 : 0;
		IndexT stencilStoreIdx = this->depthStencilFlags & StoreStencil ? 1 : 0;
		attachment.flags = 0;
		attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		attachment.format = VkTypes::AsVkFormat(this->depthStencilAttachment->GetPixelFormat());
		attachment.loadOp = loadOps[loadIdx];
		attachment.storeOp = storeOps[storeIdx];
		attachment.stencilLoadOp = loadOps[stencilLoadIdx];
		attachment.stencilStoreOp = storeOps[stencilStoreIdx];
		attachment.samples = this->depthStencilAttachment->GetEnableMSAA() ? VK_SAMPLE_COUNT_16_BIT : VK_SAMPLE_COUNT_1_BIT;
		numUsedAttachments++;
	}
	
	// create render pass
	VkRenderPassCreateInfo info =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		NULL,
		0,
		numUsedAttachments,
		numUsedAttachments == 0 ? NULL : attachments.Begin(),
		(uint32_t)subpassDescs.Size(),
		subpassDescs.IsEmpty() ? NULL : subpassDescs.Begin(),
		(uint32_t)subpassDeps.Size(),
		subpassDeps.IsEmpty() ? NULL : subpassDeps.Begin()
	};
	res = vkCreateRenderPass(VkRenderDevice::dev, &info, NULL, &this->pass);
	n_assert(res == VK_SUCCESS);


	if (this->depthStencilAttachment.isvalid())
	{
		images[i] = this->depthStencilAttachment->GetVkImageView();
		VkRect2D& rect = scissorRects[i];
		rect.offset.x = 0;
		rect.offset.y = 0;
		rect.extent.width = this->depthStencilAttachment->GetWidth();
		rect.extent.height = this->depthStencilAttachment->GetHeight();
		VkViewport& viewport = viewports[i];
		viewport.width = (float)this->depthStencilAttachment->GetWidth();
		viewport.height = (float)this->depthStencilAttachment->GetHeight();
		viewport.minDepth = 0;
		viewport.maxDepth = 1;
		viewport.x = 0;
		viewport.y = 0;

		VkClearValue& clear = this->clearValues[i];
		clear.depthStencil.depth = this->clearDepth;
		clear.depthStencil.stencil = this->clearStencil;
	}

	// setup render area
	this->renderArea.offset.x = 0;
	this->renderArea.offset.y = 0;
	this->renderArea.extent.width = width;
	this->renderArea.extent.height = height;

	// setup viewport info
	this->viewportInfo.pNext = NULL;
	this->viewportInfo.flags = 0;
	this->viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	this->viewportInfo.scissorCount = this->scissorRects.Size();
	this->viewportInfo.pScissors = this->scissorRects.Begin();
	this->viewportInfo.viewportCount = this->viewports.Size();
	this->viewportInfo.pViewports = this->viewports.Begin();

	// setup uniform buffer for render target information
	this->passBlockBuffer = ConstantBuffer::Create();
	this->passBlockBuffer->SetupFromBlockInShader(this->shaderState, "PassBlock", 1);
	this->passBlockVar = this->shaderState->GetVariableByName("PassBlock");
	this->renderTargetDimensionsVar = this->passBlockBuffer->GetVariableByName("RenderTargetDimensions");

	VkWriteDescriptorSet write;
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = NULL;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.dstBinding = this->passBlockVar->binding;
	write.dstArrayElement = 0;
	write.dstSet = this->passDescriptorSet;

	VkDescriptorBufferInfo buf;
	buf.buffer = this->passBlockBuffer->GetVkBuffer();
	buf.offset = 0;
	buf.range = VK_WHOLE_SIZE;
	write.pBufferInfo = &buf;
	write.pImageInfo = NULL;
	write.pTexelBufferView = NULL;

	// update descriptor set with attachment
	vkUpdateDescriptorSets(VkRenderDevice::dev, 1, &write, 0, NULL);

	// update descriptor set based on images attachments
	const Ptr<CoreGraphics::ShaderVariable>& inputAttachmentsVar = this->shaderState->GetVariableByName("InputAttachments");
	
	// setup input attachments
	Util::FixedArray<Math::float4> dimensions(this->colorAttachments.Size());
	for (i = 0; i < this->colorAttachments.Size(); i++)
	{
		VkWriteDescriptorSet write;
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = NULL;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		write.dstBinding = inputAttachmentsVar->binding;
		write.dstArrayElement = i;
		write.dstSet = this->passDescriptorSet;

		VkDescriptorImageInfo img;
		img.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		img.imageView = this->colorAttachments[i]->GetVkImageView();
		img.sampler = VK_NULL_HANDLE;
		write.pImageInfo = &img;

		// update descriptor set with attachment
		vkUpdateDescriptorSets(VkRenderDevice::dev, 1, &write, 0, NULL);

		// create dimensions float4
		Math::float4& dims = dimensions[i];
		dims.x() = (Math::scalar)this->colorAttachments[i]->GetWidth();
		dims.y() = (Math::scalar)this->colorAttachments[i]->GetHeight();
		dims.z() = 1 / dims.x();
		dims.w() = 1 / dims.y();
	}
	this->renderTargetDimensionsVar->SetFloat4Array(dimensions.Begin(), dimensions.Size());
	this->shaderState->SetDescriptorSet(this->passDescriptorSet, NEBULAT_PASS_GROUP);
	this->shaderState->SetApplyShared(true);

	// create framebuffer
	VkFramebufferCreateInfo fbInfo =
	{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		NULL,
		0,
		this->pass,
		(uint32_t)images.Size(),
		images.Begin(),
		width,
		height,
		layers
	};
	res = vkCreateFramebuffer(VkRenderDevice::dev, &fbInfo, NULL, &this->framebuffer);
	n_assert(res == VK_SUCCESS);

	// setup info
	this->framebufferPipelineInfo.renderPass = this->pass;
	this->framebufferPipelineInfo.subpass = 0;
	this->framebufferPipelineInfo.pViewportState = &this->viewportInfo;	
}

//------------------------------------------------------------------------------
/**
*/
void
VkPass::Discard()
{
	// release shader state
	this->shaderState->Discard();
	this->shaderState = nullptr;

	// destroy pass and our descriptor set
	vkFreeDescriptorSets(VkRenderDevice::dev, VkRenderDevice::descPool, 1, &this->passDescriptorSet);
	vkDestroyRenderPass(VkRenderDevice::dev, this->pass, NULL);
	vkDestroyFramebuffer(VkRenderDevice::dev, this->framebuffer, NULL);

	// run base class discard
	PassBase::Discard();
}

//------------------------------------------------------------------------------
/**
*/
void
VkPass::Begin()
{
	PassBase::Begin();
	VkRenderDevice* dev = VkRenderDevice::Instance();

	// bind descriptor set
	static const uint32_t offset = 0;
	dev->BindDescriptorsGraphics(&this->passDescriptorSet, this->passPipelineLayout, NEBULAT_PASS_GROUP, 1, &offset, 1, true);

	// update framebuffer pipeline info to next subpass
	this->framebufferPipelineInfo.subpass = this->currentSubpass;
}

//------------------------------------------------------------------------------
/**
*/
void
VkPass::NextSubpass()
{
	PassBase::NextSubpass();
	this->framebufferPipelineInfo.subpass = this->currentSubpass;
}

//------------------------------------------------------------------------------
/**
*/
void
VkPass::End()
{
	PassBase::End();
}

//------------------------------------------------------------------------------
/**
*/
void
VkPass::OnWindowResized()
{
	n_assert(this->renderTargetDimensionsVar.isvalid());

	// gather image views
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t layers = 0;
	Util::FixedArray<VkImageView> images;
	images.Resize(this->colorAttachments.Size() + (this->depthStencilAttachment.isvalid() ? 1 : 0));

	IndexT i;
	for (i = 0; i < this->colorAttachments.Size(); i++)
	{
		images[i] = this->colorAttachments[i]->GetVkImageView();
		width = Math::n_max(width, this->colorAttachments[i]->GetWidth());
		height = Math::n_max(height, this->colorAttachments[i]->GetHeight());
		layers = Math::n_max(layers, this->colorAttachments[i]->GetLayers());

		VkRect2D& rect = scissorRects[i];
		rect.offset.x = 0;
		rect.offset.y = 0;
		rect.extent.width = this->colorAttachments[i]->GetWidth();
		rect.extent.height = this->colorAttachments[i]->GetHeight();
		VkViewport& viewport = viewports[i];
		viewport.width = (float)this->colorAttachments[i]->GetWidth();
		viewport.height = (float)this->colorAttachments[i]->GetHeight();
		viewport.minDepth = 0;
		viewport.maxDepth = 1;
		viewport.x = 0;
		viewport.y = 0;

		const Math::float4& value = this->colorAttachmentClears[i];
		VkClearValue& clear = this->clearValues[i];
		clear.color.float32[0] = value.x();
		clear.color.float32[1] = value.y();
		clear.color.float32[2] = value.z();
		clear.color.float32[3] = value.w();
	}

	// destroy old framebuffer
	vkDestroyFramebuffer(VkRenderDevice::dev, this->framebuffer, nullptr);

	// create framebuffer
	VkFramebufferCreateInfo fbInfo =
	{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		NULL,
		0,
		this->pass,
		(uint32_t)images.Size(),
		images.Begin(),
		width,
		height,
		layers
	};

	// we need to recreate the framebuffer object because the image handles might have changed, and the dimensions to render to is different
	VkResult res;
	res = vkCreateFramebuffer(VkRenderDevice::dev, &fbInfo, nullptr, &this->framebuffer);
	n_assert(res == VK_SUCCESS);

	// setup input attachments
	Util::FixedArray<Math::float4> dimensions(this->colorAttachments.Size());
	for (i = 0; i < this->colorAttachments.Size(); i++)
	{
		// create dimensions float4
		Math::float4& dims = dimensions[i];
		dims.x() = (Math::scalar)this->colorAttachments[i]->GetWidth();
		dims.y() = (Math::scalar)this->colorAttachments[i]->GetHeight();
		dims.z() = 1 / dims.x();
		dims.w() = 1 / dims.y();
	}
	this->renderTargetDimensionsVar->SetFloat4Array(dimensions.Begin(), dimensions.Size());
}

} // namespace Vulkan