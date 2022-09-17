#pragma once
//------------------------------------------------------------------------------
/**
    Implements a shader variation (shader program within effect) in Vulkan.
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "lowlevel/vk/vkprogram.h"
#include "lowlevel/vk/vkrenderstate.h"
#include "lowlevel/afxapi.h"
#include "coregraphics/shaderfeature.h"
#include "coregraphics/resourcetable.h"
#include "coregraphics/shader.h"
#include "util/arraystack.h"

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
    VkPipelineColorBlendAttachmentState colorBlendAttachments[8];
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

struct VkProgramReflectionInfo
{
    Util::ArrayStack<uint32_t, 8> vsInputSlots;
    Util::StringAtom name;
};

enum
{
    ShaderProgram_SetupInfo,
    ShaderProgram_ReflectionInfo,
    ShaderProgram_RuntimeInfo
};
typedef Ids::IdAllocator<
    VkShaderProgramSetupInfo,       // used for setup
    VkProgramReflectionInfo,        // program reflection
    VkShaderProgramRuntimeInfo      // used for runtime
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
