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

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgram::Apply(VkShaderProgramRuntimeInfo& info)
{
	// if we are compute, we can set the pipeline straight away, otherwise we have to accumulate the infos
	if (info.type == Compute)		VkRenderDevice::Instance()->BindComputePipeline(info.pipeline, info.layout);
	else if (info.type == Graphics)	VkRenderDevice::Instance()->BindGraphicsPipelineInfo(info.info, info.id);
	else							VkRenderDevice::Instance()->UnbindPipeline();
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgram::Setup(const Ids::Id24 id, AnyFX::VkProgram* program, VkPipelineLayout& pipelineLayout, ProgramAllocator& allocator)
{
	allocator.Get<1>(id) = program;
	String mask = program->GetAnnotationString("Mask").c_str();
	String name = program->name.c_str();
	
	VkShaderProgramSetupInfo& setup = allocator.Get<0>(id);
	VkShaderProgramRuntimeInfo& runtime = allocator.Get<2>(id);
	runtime.layout = pipelineLayout;
	runtime.id = VkShaderProgram::uniqueIdCounter++;
	runtime.pipeline = VK_NULL_HANDLE;
	setup.mask = CoreGraphics::ShaderServer::Instance()->FeatureStringToMask(mask);
	setup.name = name;
	setup.dev = VkRenderDevice::Instance()->GetCurrentDevice();
	VkShaderProgram::CreateShader(setup.dev, &setup.vs, program->shaderBlock.vsBinarySize, program->shaderBlock.vsBinary);
	VkShaderProgram::CreateShader(setup.dev, &setup.hs, program->shaderBlock.hsBinarySize, program->shaderBlock.hsBinary);
	VkShaderProgram::CreateShader(setup.dev, &setup.ds, program->shaderBlock.dsBinarySize, program->shaderBlock.dsBinary);
	VkShaderProgram::CreateShader(setup.dev, &setup.gs, program->shaderBlock.gsBinarySize, program->shaderBlock.gsBinary);
	VkShaderProgram::CreateShader(setup.dev, &setup.ps, program->shaderBlock.psBinarySize, program->shaderBlock.psBinary);
	VkShaderProgram::CreateShader(setup.dev, &setup.cs, program->shaderBlock.csBinarySize, program->shaderBlock.csBinary);

	// if we have a compute shader, it will be the one we use, otherwise use the graphics one
	if (setup.cs)		VkShaderProgram::SetupAsCompute(setup, runtime);
	else if (setup.vs)	VkShaderProgram::SetupAsGraphics(program, setup, runtime);
	else				runtime.type = PipelineType::InvalidType;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgram::CreateShader(const VkDevice dev, VkShaderModule* shader, unsigned binarySize, char* binary)
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
		VkResult res = vkCreateShaderModule(dev, &info, NULL, shader);
		assert(res == VK_SUCCESS);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgram::SetupAsGraphics(AnyFX::VkProgram* program, VkShaderProgramSetupInfo& setup, VkShaderProgramRuntimeInfo& runtime)
{
	// we have to keep track of how MANY shaders we are using, AnyFX makes every function 'main'
	unsigned shaderIdx = 0;
	static const char* name = "main";
	memset(setup.shaderInfos, 0, sizeof(setup.shaderInfos));

	// attach vertex shader
	if (0 != setup.vs)
	{
		setup.shaderInfos[shaderIdx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		setup.shaderInfos[shaderIdx].pNext = NULL;
		setup.shaderInfos[shaderIdx].flags = 0;
		setup.shaderInfos[shaderIdx].stage = VK_SHADER_STAGE_VERTEX_BIT;
		setup.shaderInfos[shaderIdx].module = setup.vs;
		setup.shaderInfos[shaderIdx].pName = name;
		setup.shaderInfos[shaderIdx].pSpecializationInfo = NULL;
		shaderIdx++;
	}

	if (0 != setup.hs)
	{
		setup.shaderInfos[shaderIdx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		setup.shaderInfos[shaderIdx].pNext = NULL;
		setup.shaderInfos[shaderIdx].flags = 0;
		setup.shaderInfos[shaderIdx].stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		setup.shaderInfos[shaderIdx].module = setup.hs;
		setup.shaderInfos[shaderIdx].pName = name;
		setup.shaderInfos[shaderIdx].pSpecializationInfo = NULL;
		shaderIdx++;
	}

	if (0 != setup.ds)
	{
		setup.shaderInfos[shaderIdx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		setup.shaderInfos[shaderIdx].pNext = NULL;
		setup.shaderInfos[shaderIdx].flags = 0;
		setup.shaderInfos[shaderIdx].stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		setup.shaderInfos[shaderIdx].module = setup.ds;
		setup.shaderInfos[shaderIdx].pName = name;
		setup.shaderInfos[shaderIdx].pSpecializationInfo = NULL;
		shaderIdx++;
	}

	if (0 != setup.gs)
	{
		setup.shaderInfos[shaderIdx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		setup.shaderInfos[shaderIdx].pNext = NULL;
		setup.shaderInfos[shaderIdx].flags = 0;
		setup.shaderInfos[shaderIdx].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		setup.shaderInfos[shaderIdx].module = setup.gs;
		setup.shaderInfos[shaderIdx].pName = name;
		setup.shaderInfos[shaderIdx].pSpecializationInfo = NULL;
		shaderIdx++;
	}

	if (0 != setup.ps)
	{
		setup.shaderInfos[shaderIdx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		setup.shaderInfos[shaderIdx].pNext = NULL;
		setup.shaderInfos[shaderIdx].flags = 0;
		setup.shaderInfos[shaderIdx].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		setup.shaderInfos[shaderIdx].module = setup.ps;
		setup.shaderInfos[shaderIdx].pName = name;
		setup.shaderInfos[shaderIdx].pSpecializationInfo = NULL;
		shaderIdx++;
	}

	// retrieve implementation specific state
	AnyFX::VkRenderState* vkRenderState = static_cast<AnyFX::VkRenderState*>(program->renderState);

	setup.rasterizerInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		NULL,
		0
	};
	vkRenderState->SetupRasterization(&setup.rasterizerInfo);

	setup.multisampleInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		NULL,
		0
	};
	vkRenderState->SetupMultisample(&setup.multisampleInfo);

	setup.depthStencilInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		NULL,
		0
	};
	vkRenderState->SetupDepthStencil(&setup.depthStencilInfo);

	setup.colorBlendInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		NULL,
		0
	};
	vkRenderState->SetupBlend(&setup.colorBlendInfo);

	setup.tessInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		NULL,
		0,
		program->patchSize
	};

	setup.vertexInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		NULL,
		0,
		0,
		NULL,
		program->numVsInputs,
		NULL
	};

	// setup dynamic state, we only support dynamic viewports and scissor rects
	static const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT };
	setup.dynamicInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		NULL,
		0,
		2,
		dynamicStates
	};

	// setup pipeline information regarding the shader state
	runtime.info =
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		NULL,
		VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT,
		shaderIdx,
		setup.shaderInfos,
		&setup.vertexInfo,			// we only save how many vs inputs we allow here
		NULL,						// this is input type related (triangles, patches etc)
		program->supportsTessellation ? &setup.tessInfo : VK_NULL_HANDLE,
		NULL,						// this is our viewport and is setup by the framebuffer
		&setup.rasterizerInfo,
		&setup.multisampleInfo,
		&setup.depthStencilInfo,
		&setup.colorBlendInfo,
		&setup.dynamicInfo,
		runtime.layout,
		NULL,							// pass specific stuff, keep as NULL, handled by the framebuffer
		0,
		VK_NULL_HANDLE, 0				// base pipeline is kept as NULL too, because this is the base for all derivatives
	};

	// be sure to flag compute shader as null
	runtime.type = Graphics;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgram::SetupAsCompute(VkShaderProgramSetupInfo& setup, VkShaderProgramRuntimeInfo& runtime)
{
	// create 6 shader info stages for each shader type
	n_assert(0 != setup.cs);

	VkPipelineShaderStageCreateInfo shader =
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		NULL,
		0,
		VK_SHADER_STAGE_COMPUTE_BIT,
		setup.cs,
		"main",
		VK_NULL_HANDLE,
	};

	VkComputePipelineCreateInfo pInfo =
	{
		VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		NULL,
		0,
		shader,
		runtime.layout,
		VK_NULL_HANDLE, 
		0
	};

	// create pipeline
	VkResult res = vkCreateComputePipelines(setup.dev, VkRenderDevice::cache, 1, &pInfo, NULL, &runtime.pipeline);
	n_assert(res == VK_SUCCESS);
	runtime.type = Compute;
}

//------------------------------------------------------------------------------
/**
*/
const uint32_t
VkShaderProgram::GetNumVertexInputs(AnyFX::VkProgram* program)
{
	return program->numVsInputs;
}

//------------------------------------------------------------------------------
/**
*/
const uint32_t
VkShaderProgram::GetNumPixelOutputs(AnyFX::VkProgram* program)
{
	return program->numPsOutputs;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgram::Discard(VkShaderProgramSetupInfo& info, VkPipeline& computePipeline)
{
	if (info.vs != VK_NULL_HANDLE)					vkDestroyShaderModule(info.dev, info.vs, NULL);
	if (info.hs != VK_NULL_HANDLE)					vkDestroyShaderModule(info.dev, info.hs, NULL);
	if (info.ds != VK_NULL_HANDLE)					vkDestroyShaderModule(info.dev, info.ds, NULL);
	if (info.gs != VK_NULL_HANDLE)					vkDestroyShaderModule(info.dev, info.gs, NULL);
	if (info.ps != VK_NULL_HANDLE)					vkDestroyShaderModule(info.dev, info.ps, NULL);
	if (info.cs != VK_NULL_HANDLE)					vkDestroyShaderModule(info.dev, info.cs, NULL);
	if (computePipeline != VK_NULL_HANDLE)			vkDestroyPipeline(info.dev, computePipeline, NULL);
}

} // namespace Vulkan