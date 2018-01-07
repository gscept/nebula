#pragma once
//------------------------------------------------------------------------------
/**
	Implements a Vulkan pass, which translates into a VkRenderPass.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/passbase.h"
#include "coregraphics/shader.h"
#include "coregraphics/constantbuffer.h"

namespace Vulkan
{
class VkPass : public Base::PassBase
{
	__DeclareClass(VkPass);
public:
	/// constructor
	VkPass();
	/// destructor
	virtual ~VkPass();

	/// setup pass
	void Setup();
	/// discard pass
	void Discard();

	/// starts pass
	void Begin();
	/// progress to next subpass
	void NextSubpass();
	/// ends pass
	void End();

	/// get render pass
	const VkRenderPass GetVkRenderPass() const;
	/// get render area
	const VkRect2D& GetVkRenderArea() const;
	/// get framebuffer
	const VkFramebuffer GetVkFramebuffer() const;
	/// get graphics pipeline info for pass
	const VkGraphicsPipelineCreateInfo& GetVkFramebufferPipelineInfo();
	/// get list of clear values
	const Util::FixedArray<VkClearValue>& GetVkClearValues() const;

	/// get scissor rects
	const Util::FixedArray<VkRect2D>& GetVkScissorRects() const;
	/// get viewports
	const Util::FixedArray<VkViewport>& GetVkViewports() const;
	/// get scissor rects for subpass
	const Util::FixedArray<VkRect2D>& GetVkScissorRects(IndexT i) const;
	/// get viewports for subpass
	const Util::FixedArray<VkViewport>& GetVkViewports(IndexT i) const;

	/// handle window resizing
	void OnWindowResized();

private:
	friend class VkPipelineDatabase;

	VkDevice dev;
	CoreGraphics::ShaderStateId shaderState;
	CoreGraphics::ConstantBufferId passBlockBuffer;
	CoreGraphics::ShaderVariableId passBlockVar;
	CoreGraphics::ShaderVariableId renderTargetDimensionsVar;
	VkDescriptorSet passDescriptorSet;
	VkPipelineLayout passPipelineLayout;
	Util::FixedArray<VkClearValue> clearValues;
	VkGraphicsPipelineCreateInfo framebufferPipelineInfo;
	VkPipelineViewportStateCreateInfo viewportInfo;
	VkRect2D renderArea;
	VkRenderPass pass;
	VkFramebuffer framebuffer;

	Util::FixedArray<VkRect2D> scissorRects;
	Util::FixedArray<VkViewport> viewports;

	Util::FixedArray<Util::FixedArray<VkRect2D>> subpassRects;
	Util::FixedArray<Util::FixedArray<VkViewport>> subpassViewports;
};

//------------------------------------------------------------------------------
/**
*/
inline const VkRenderPass
VkPass::GetVkRenderPass() const
{
	return this->pass;
}

//------------------------------------------------------------------------------
/**
*/
inline const VkRect2D&
VkPass::GetVkRenderArea() const
{
	return this->renderArea;
}

//------------------------------------------------------------------------------
/**
*/
inline const VkFramebuffer
VkPass::GetVkFramebuffer() const
{
	return this->framebuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline const VkGraphicsPipelineCreateInfo&
VkPass::GetVkFramebufferPipelineInfo()
{
	return this->framebufferPipelineInfo;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::FixedArray<VkClearValue>&
VkPass::GetVkClearValues() const
{
	return this->clearValues;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::FixedArray<VkRect2D>&
VkPass::GetVkScissorRects() const
{
	return this->scissorRects;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::FixedArray<VkViewport>&
VkPass::GetVkViewports() const
{
	return this->viewports;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::FixedArray<VkRect2D>&
VkPass::GetVkScissorRects(IndexT i) const
{
	return this->subpassRects[i];
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::FixedArray<VkViewport>&
VkPass::GetVkViewports(IndexT i) const
{
	return this->subpassViewports[i];
}

} // namespace Vulkan