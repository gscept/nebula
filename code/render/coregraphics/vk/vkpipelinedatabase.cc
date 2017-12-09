//------------------------------------------------------------------------------
// vkpipelinedatabase.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkpipelinedatabase.h"
#include "vkshaderprogram.h"
#include "vkvertexlayout.h"
#include "vkpass.h"

namespace Vulkan
{

__ImplementSingleton(VkPipelineDatabase);
//------------------------------------------------------------------------------
/**
*/
VkPipelineDatabase::VkPipelineDatabase()
{
	__ConstructSingleton;
	this->Reset();
}

//------------------------------------------------------------------------------
/**
*/
VkPipelineDatabase::~VkPipelineDatabase()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::SetPass(const Ptr<VkPass>& pass)
{
	this->currentPass = pass;
	IndexT index = this->tier1.FindIndex(pass);
	if (index != InvalidIndex)
	{
		this->ct1 = this->tier1.ValueAtIndex(index);
		this->SetSubpass(this->currentSubpass);
	}
	else
	{
		this->ct1 = n_new(Tier1Node);
		this->tier1.Add(pass, this->ct1);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::SetSubpass(uint32_t subpass)
{
	this->currentSubpass = subpass;
	IndexT index = this->ct1->children.FindIndex(subpass);
	if (index != InvalidIndex)
	{
		this->ct2 = this->ct1->children.ValueAtIndex(index);
		this->SetShader(this->currentShaderProgram);
	}
	else
	{
		this->ct2 = n_new(Tier2Node);
		this->ct1->children.Add(subpass, this->ct2);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::SetShader(const uint32_t program)
{
	this->currentShaderProgram = program;
	IndexT index = this->ct2->children.FindIndex(program);
	if (index != InvalidIndex)
	{
		this->ct3 = this->ct2->children.ValueAtIndex(index);
		this->SetVertexLayout(this->currentVertexLayout);
	}
	else
	{
		this->ct3 = n_new(Tier3Node);
		this->ct2->children.Add(program, this->ct3);
		this->SetVertexLayout(this->currentVertexLayout);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::SetVertexLayout(VkPipelineVertexInputStateCreateInfo* layout)
{
	this->currentVertexLayout = layout;
	IndexT index = this->ct3->children.FindIndex(layout);
	if (index != InvalidIndex)
	{
		this->ct4 = this->ct3->children.ValueAtIndex(index);
		this->SetInputLayout(this->currentInputAssemblyInfo);
	}
	else
	{
		this->ct4 = n_new(Tier4Node);
		this->ct3->children.Add(layout, this->ct4);
		this->SetInputLayout(this->currentInputAssemblyInfo);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::SetInputLayout(VkPipelineInputAssemblyStateCreateInfo* input)
{
	this->currentInputAssemblyInfo = input;
	IndexT index = this->ct4->children.FindIndex(input);
	if (index != InvalidIndex)
	{
		this->ct5 = this->ct4->children.ValueAtIndex(index);
	}
	else
	{
		this->ct5 = n_new(Tier5Node);
		this->ct4->children.Add(input, this->ct5);
	}
}

//------------------------------------------------------------------------------
/**
*/
VkPipeline
VkPipelineDatabase::GetCompiledPipeline()
{
	if (this->ct1->initial ||
		this->ct2->initial ||
		this->ct3->initial ||
		this->ct4->initial ||
		this->ct5->initial)
	{
		VkGraphicsPipelineCreateInfo shaderInfo = this->currentShaderProgram->shaderPipelineInfo;
		VkGraphicsPipelineCreateInfo passInfo = this->currentPass->framebufferPipelineInfo;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo = *shaderInfo.pColorBlendState;
		colorBlendInfo.attachmentCount = this->currentPass->GetSubpasses()[this->currentSubpass].attachments.Size();
		VkGraphicsPipelineCreateInfo info =
		{
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			NULL,
			0,
			shaderInfo.stageCount,
			shaderInfo.pStages,
			this->currentVertexLayout,
			this->currentInputAssemblyInfo,
			shaderInfo.pTessellationState,
			passInfo.pViewportState,
			shaderInfo.pRasterizationState,
			shaderInfo.pMultisampleState,
			shaderInfo.pDepthStencilState,
			&colorBlendInfo,
			shaderInfo.pDynamicState,
			shaderInfo.layout,
			this->currentPass->GetVkRenderPass(),
			this->currentSubpass,
			VK_NULL_HANDLE,
			-1
		};
		VkResult res = vkCreateGraphicsPipelines(VkRenderDevice::dev, VkRenderDevice::cache, 1, &info, NULL, &this->currentPipeline);
		n_assert(res == VK_SUCCESS);
		this->ct5->pipeline = this->currentPipeline;

		// DAG path is created, so set entire path to not initial
		this->ct1->initial = this->ct2->initial = this->ct3->initial = this->ct4->initial = this->ct5->initial = false;
	}
	else
	{
		this->currentPipeline = this->ct5->pipeline;
	}

	return this->currentPipeline;
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::Reset()
{
	this->currentPass = 0;
	this->currentSubpass = -1;
	this->currentShaderProgram = 0;
	this->currentVertexLayout = 0;
	this->currentInputAssemblyInfo = 0;
	this->currentPipeline = VK_NULL_HANDLE;
	this->ct1 = NULL;
	this->ct2 = NULL;
	this->ct3 = NULL;
	this->ct4 = NULL;
	this->ct5 = NULL;
}

} // namespace $NAMESPACE$
