#pragma once
//------------------------------------------------------------------------------
/**
	Implements a shader variation (shader program within effect) in Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "lowlevel/vk/vkprogram.h"
#include "lowlevel/vk/vkrenderstate.h"
#include "lowlevel/afxapi.h"
#include "resources/resourcepool.h"
#include "coregraphics/shaderfeature.h"

namespace Vulkan
{

enum VkShaderProgramPipelineType
{
	InvalidType,
	Compute,
	Graphics
};

struct VkShaderProgramSetupInfo
{
	VkDevice dev;
	VkPipelineVertexInputStateCreateInfo vertexInfo;
	VkPipelineRasterizationStateCreateInfo rasterizerInfo;
	VkPipelineMultisampleStateCreateInfo multisampleInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	VkPipelineColorBlendStateCreateInfo colorBlendInfo;
	VkPipelineDynamicStateCreateInfo dynamicInfo;
	VkPipelineTessellationStateCreateInfo tessInfo;
	VkPipelineShaderStageCreateInfo shaderInfos[5];
	VkShaderModule vs, hs, ds, gs, ps, cs;
	Util::String name;
	CoreGraphics::ShaderFeature::Mask mask;
};

struct VkShaderProgramRuntimeInfo
{
	VkGraphicsPipelineCreateInfo info;
	VkPipeline pipeline;
	VkPipelineLayout layout;
	VkShaderProgramPipelineType type;
	uint32_t uniqueId;
};

extern uint32_t UniqueIdCounter;
typedef Ids::IdAllocator<
	VkShaderProgramSetupInfo,		//0 used for setup
	AnyFX::VkProgram*,				//1 program object
	VkShaderProgramRuntimeInfo		//2 used for runtime
> VkShaderProgramAllocator;


/// applies program
void VkShaderProgramApply(VkShaderProgramRuntimeInfo& info);
/// discard variation
void VkShaderProgramDiscard(VkShaderProgramSetupInfo& info, VkPipeline& computePipeline);

/// setup from AnyFX program
void VkShaderProgramSetup(const Ids::Id24 id, AnyFX::VkProgram* program, VkPipelineLayout& pipelineLayout, VkShaderProgramAllocator& allocator);

/// create shader object
void VkShaderProgramCreateShader(const VkDevice dev, VkShaderModule* shader, unsigned binarySize, char* binary);
/// create this program as a graphics program
void VkShaderProgramSetupAsGraphics(AnyFX::VkProgram* program, VkShaderProgramSetupInfo& setup, VkShaderProgramRuntimeInfo& runtime);
/// create this program as a compute program (can be done immediately)
void VkShaderProgramSetupAsCompute(VkShaderProgramSetupInfo& setup, VkShaderProgramRuntimeInfo& runtime);
} // namespace Vulkan