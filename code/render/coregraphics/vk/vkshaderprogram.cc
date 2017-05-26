//------------------------------------------------------------------------------
// vkshaderprogram.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkshaderprogram.h"
#include "vkrenderdevice.h"
#include "coregraphics/shaderserver.h"
#include "lowlevel/vk/vkrenderstate.h"
#include "lowlevel/vk/vksampler.h"

using namespace Util;
namespace Vulkan
{

uint32_t VkShaderProgram::uniqueIdCounter = 0;
__ImplementClass(Vulkan::VkShaderProgram, 'VKSP', Base::ShaderVariationBase);
//------------------------------------------------------------------------------
/**
*/
VkShaderProgram::VkShaderProgram() :
	vs(VK_NULL_HANDLE),
	hs(VK_NULL_HANDLE),
	ds(VK_NULL_HANDLE),
	gs(VK_NULL_HANDLE),
	ps(VK_NULL_HANDLE),
	cs(VK_NULL_HANDLE),
	pipelineType(InvalidType),
	computePipeline(VK_NULL_HANDLE)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkShaderProgram::~VkShaderProgram()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgram::Apply()
{
	n_assert(this->program);

	// if we are compute, we can set the pipeline straight away, otherwise we have to accumulate the infos
	if (this->pipelineType == Compute)			VkRenderDevice::Instance()->BindComputePipeline(this->computePipeline, this->pipelineLayout);
	else if (this->pipelineType == Graphics)	VkRenderDevice::Instance()->BindGraphicsPipelineInfo(this->shaderPipelineInfo, this);
	else										VkRenderDevice::Instance()->UnbindPipeline();
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgram::Commit()
{
	n_assert(this->program);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgram::Setup(AnyFX::VkProgram* program, const VkPipelineLayout& layout)
{
	this->program = program;
	String mask = program->GetAnnotationString("Mask").c_str();
	String name = program->name.c_str();
	this->pipelineLayout = layout;
	this->uniqueId = VkShaderProgram::uniqueIdCounter++;

	this->CreateShader(&this->vs, program->shaderBlock.vsBinarySize, program->shaderBlock.vsBinary);
	this->CreateShader(&this->hs, program->shaderBlock.hsBinarySize, program->shaderBlock.hsBinary);
	this->CreateShader(&this->ds, program->shaderBlock.dsBinarySize, program->shaderBlock.dsBinary);
	this->CreateShader(&this->gs, program->shaderBlock.gsBinarySize, program->shaderBlock.gsBinary);
	this->CreateShader(&this->ps, program->shaderBlock.psBinarySize, program->shaderBlock.psBinary);
	this->CreateShader(&this->cs, program->shaderBlock.csBinarySize, program->shaderBlock.csBinary);

	// if we have a compute shader, it will be the one we use, otherwise use the graphics one
	if (this->cs)		this->SetupAsCompute();
	else if (this->vs)	this->SetupAsGraphics();
	else				this->SetupAsEmpty();

	// setup feature mask and name
	this->SetFeatureMask(CoreGraphics::ShaderServer::Instance()->FeatureStringToMask(mask));
	this->SetName(name);
	this->SetNumPasses(1);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgram::CreateShader(VkShaderModule* shader, unsigned binarySize, char* binary)
{
	if (binarySize > 0)
	{
		VkShaderModuleCreateInfo info =
		{
			VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			NULL,
			0,										// flags
			binarySize,	// Vulkan expects the binary to be uint32, so we must assume size is in units of 4 bytes
			(unsigned*)binary
		};

		// create shader
		VkResult res = vkCreateShaderModule(VkRenderDevice::dev, &info, NULL, shader);
		assert(res == VK_SUCCESS);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgram::SetupAsGraphics()
{
	// we have to keep track of how MANY shaders we are using too
	unsigned shaderIdx = 0;
	static const char* name = "main";
	memset(this->shaderInfos, 0, sizeof(this->shaderInfos));

	// attach vertex shader
	if (0 != this->vs)
	{
		this->shaderInfos[shaderIdx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		this->shaderInfos[shaderIdx].pNext = NULL;
		this->shaderInfos[shaderIdx].flags = 0;
		this->shaderInfos[shaderIdx].stage = VK_SHADER_STAGE_VERTEX_BIT;
		this->shaderInfos[shaderIdx].module = this->vs;
		this->shaderInfos[shaderIdx].pName = name;
		this->shaderInfos[shaderIdx].pSpecializationInfo = NULL;
		shaderIdx++;
	}

	if (0 != this->hs)
	{
		this->shaderInfos[shaderIdx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		this->shaderInfos[shaderIdx].pNext = NULL;
		this->shaderInfos[shaderIdx].flags = 0;
		this->shaderInfos[shaderIdx].stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		this->shaderInfos[shaderIdx].module = this->hs;
		this->shaderInfos[shaderIdx].pName = name;
		this->shaderInfos[shaderIdx].pSpecializationInfo = NULL;
		shaderIdx++;
	}

	if (0 != this->ds)
	{
		this->shaderInfos[shaderIdx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		this->shaderInfos[shaderIdx].pNext = NULL;
		this->shaderInfos[shaderIdx].flags = 0;
		this->shaderInfos[shaderIdx].stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		this->shaderInfos[shaderIdx].module = this->ds;
		this->shaderInfos[shaderIdx].pName = name;
		this->shaderInfos[shaderIdx].pSpecializationInfo = NULL;
		shaderIdx++;
	}

	if (0 != this->gs)
	{
		this->shaderInfos[shaderIdx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		this->shaderInfos[shaderIdx].pNext = NULL;
		this->shaderInfos[shaderIdx].flags = 0;
		this->shaderInfos[shaderIdx].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		this->shaderInfos[shaderIdx].module = this->gs;
		this->shaderInfos[shaderIdx].pName = name;
		this->shaderInfos[shaderIdx].pSpecializationInfo = NULL;
		shaderIdx++;
	}

	if (0 != this->ps)
	{
		this->shaderInfos[shaderIdx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		this->shaderInfos[shaderIdx].pNext = NULL;
		this->shaderInfos[shaderIdx].flags = 0;
		this->shaderInfos[shaderIdx].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		this->shaderInfos[shaderIdx].module = this->ps;
		this->shaderInfos[shaderIdx].pName = name;
		this->shaderInfos[shaderIdx].pSpecializationInfo = NULL;
		shaderIdx++;
	}

	// retrieve implementation specific state
	AnyFX::VkRenderState* vkRenderState = static_cast<AnyFX::VkRenderState*>(this->program->renderState);

	this->rasterizerInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		NULL,
		0
	};
	vkRenderState->SetupRasterization(&this->rasterizerInfo);

	this->multisampleInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		NULL,
		0
	};
	vkRenderState->SetupMultisample(&this->multisampleInfo);

	this->depthStencilInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		NULL,
		0
	};
	vkRenderState->SetupDepthStencil(&this->depthStencilInfo);

	this->colorBlendInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		NULL,
		0
	};
	vkRenderState->SetupBlend(&this->colorBlendInfo);

	this->tessInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		NULL,
		0,
		this->program->patchSize
	};

	this->vertexInfo = 
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		NULL,
		0,
		0,
		NULL,
		this->program->numVsInputs,
		NULL
	};

	// setup dynamic state, we only support dynamic viewports and scissor rects
	static const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT };
	this->dynamicInfo = 
	{
		VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		NULL,
		0,
		2,
		dynamicStates
	};

	// setup pipeline information regarding the shader state
	this->shaderPipelineInfo =
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		NULL,
		VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT,
		shaderIdx,
		this->shaderInfos,
		&this->vertexInfo,			// we only save how many vs inputs we allow here
		NULL,						// this is input type related (triangles, patches etc)
		program->supportsTessellation ? &this->tessInfo : VK_NULL_HANDLE,
		NULL,						// this is our viewport and is setup by the framebuffer
		&this->rasterizerInfo,
		&this->multisampleInfo,
		&this->depthStencilInfo,
		&this->colorBlendInfo,
		&this->dynamicInfo,					
		this->pipelineLayout,
		NULL,							// pass specific stuff, keep as NULL, handled by the framebuffer
		0,
		VK_NULL_HANDLE, 0				// base pipeline is kept as NULL too, because this is the base for all derivatives
	};

	// be sure to flag compute shader as null
	this->pipelineType = Graphics;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgram::SetupAsCompute()
{
	// create 6 shader info stages for each shader type
	n_assert(0 != this->cs);

	VkPipelineShaderStageCreateInfo shader =
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		NULL,
		0,
		VK_SHADER_STAGE_COMPUTE_BIT,
		this->cs,
		"main",
		VK_NULL_HANDLE,
	};

	VkComputePipelineCreateInfo info =
	{
		VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		NULL,
		0,
		shader,
		this->pipelineLayout,
		VK_NULL_HANDLE, 
		0
	};

	// create pipeline
	VkResult res = vkCreateComputePipelines(VkRenderDevice::dev, VkRenderDevice::cache, 1, &info, NULL, &this->computePipeline);
	n_assert(res == VK_SUCCESS);
	this->pipelineType = Compute;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgram::SetupAsEmpty()
{
	this->pipelineType = InvalidType;
}

//------------------------------------------------------------------------------
/**
*/
const uint32_t
VkShaderProgram::GetNumVertexInputs() const
{
	return this->program->numVsInputs;
}

//------------------------------------------------------------------------------
/**
*/
const uint32_t
VkShaderProgram::GetNumPixelOutputs() const
{
	return this->program->numPsOutputs;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgram::Discard()
{
	ShaderVariationBase::Discard();
	if (this->vs != VK_NULL_HANDLE)					vkDestroyShaderModule(VkRenderDevice::dev, this->vs, NULL);
	if (this->hs != VK_NULL_HANDLE)					vkDestroyShaderModule(VkRenderDevice::dev, this->hs, NULL);
	if (this->ds != VK_NULL_HANDLE)					vkDestroyShaderModule(VkRenderDevice::dev, this->ds, NULL);
	if (this->gs != VK_NULL_HANDLE)					vkDestroyShaderModule(VkRenderDevice::dev, this->gs, NULL);
	if (this->ps != VK_NULL_HANDLE)					vkDestroyShaderModule(VkRenderDevice::dev, this->ps, NULL);
	if (this->cs != VK_NULL_HANDLE)					vkDestroyShaderModule(VkRenderDevice::dev, this->cs, NULL);
	if (this->computePipeline != VK_NULL_HANDLE)	vkDestroyPipeline(VkRenderDevice::dev, this->computePipeline, NULL);
}

} // namespace Vulkan