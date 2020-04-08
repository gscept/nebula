#pragma once
//------------------------------------------------------------------------------
/**
	Implements a shader variation (shader program within effect) in Vulkan.
	
	(C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "lowlevel/vk/vkprogram.h"
#include "lowlevel/vk/vkrenderstate.h"
#include "lowlevel/afxapi.h"
#include "resources/resourcepool.h"
#include "coregraphics/shaderfeature.h"
#include "coregraphics/resourcetable.h"
#include "coregraphics/shader.h"

namespace Vulkan
{

struct VkShaderProgramSetupInfo
{
	VkDevice dev;
	Util::String name;
	CoreGraphics::ShaderFeature::Mask mask;
};

struct VkShaderProgramRuntimeInfo
{
	uint32_t stageCount;
	VkPipelineVertexInputStateCreateInfo vertexInfo;
	VkPipelineRasterizationStateCreateInfo rasterizerInfo;
	VkPipelineMultisampleStateCreateInfo multisampleInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	VkPipelineColorBlendStateCreateInfo colorBlendInfo;
	VkPipelineDynamicStateCreateInfo dynamicInfo;
	VkPipelineTessellationStateCreateInfo tessInfo;
	VkPipelineShaderStageCreateInfo shaderInfos[5];
	VkShaderModule vs, hs, ds, gs, ps, cs;
	uint stencilFrontRef, stencilBackRef, stencilReadMask, stencilWriteMask;
	VkPipeline pipeline;
	VkPipelineLayout layout;
	CoreGraphics::ShaderPipeline type;
	uint32_t uniqueId;
};

extern uint32_t UniqueIdCounter;

enum
{
	ShaderProgram_SetupInfo,
	ShaderProgram_AnyFXPtr,
	ShaderProgram_RuntimeInfo
};
typedef Ids::IdAllocator<
	VkShaderProgramSetupInfo,		//0 used for setup
	AnyFX::VkProgram*,				//1 program object
	VkShaderProgramRuntimeInfo		//2 used for runtime
> VkShaderProgramAllocator;


/// discard variation
void VkShaderProgramDiscard(VkShaderProgramSetupInfo& info, VkShaderProgramRuntimeInfo& rt, VkPipeline& computePipeline);

/// setup from AnyFX program
void VkShaderProgramSetup(const Ids::Id24 id, const Resources::ResourceName& shaderName, AnyFX::VkProgram* program, const CoreGraphics::ResourcePipelineId& pipelineLayout, VkShaderProgramAllocator& allocator);

/// create shader object
void VkShaderProgramCreateShader(const VkDevice dev, VkShaderModule* shader, unsigned binarySize, char* binary);
/// create this program as a graphics program
void VkShaderProgramSetupAsGraphics(AnyFX::VkProgram* program, const Resources::ResourceName& shaderName, VkShaderProgramRuntimeInfo& runtime);
/// create this program as a compute program (can be done immediately)
void VkShaderProgramSetupAsCompute(VkShaderProgramSetupInfo& setup, VkShaderProgramRuntimeInfo& runtime);
} // namespace Vulkan