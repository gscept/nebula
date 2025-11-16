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
#include "coregraphics/shaderfeature.h"
#include "coregraphics/resourcetable.h"
#include "util/arraystack.h"
#include "util/serialize.h"


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
    VkPipelineDynamicStateCreateInfo graphicsDynamicStateInfo;
    VkPipelineTessellationStateCreateInfo tessInfo;
    VkPipelineShaderStageCreateInfo graphicsShaderInfos[5];
    VkPipelineDynamicStateCreateInfo raytracingDynamicStateInfo;
    VkPipelineShaderStageCreateInfo raytracingShaderInfos[6];
    VkShaderModule vs, hs, ds, gs, ps, cs, ts, ms, rg, ra, rc, rm, ri, ca;
    uint stencilFrontRef, stencilBackRef, stencilReadMask, stencilWriteMask;
    VkPipeline pipeline;
    uint rayPayloadSize, hitAttributeSize;
    VkPipelineLayout layout;
    CoreGraphics::ShaderPipeline type;
    uint32_t uniqueId;
};

extern uint32_t UniqueIdCounter;

struct VkProgramReflectionInfo
{
    Util::StackArray<uint32_t, 8> vsInputSlots;
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
> ShaderProgramAllocator;

extern ShaderProgramAllocator shaderProgramAlloc;


/// discard variation
void VkShaderProgramDiscard(VkShaderProgramSetupInfo& info, VkShaderProgramRuntimeInfo& rt, VkPipeline& computePipeline);

/// setup from AnyFX program
void VkShaderProgramSetup(const Ids::Id24 id, const Resources::ResourceName& shaderName, AnyFX::VkProgram* program, const CoreGraphics::ResourcePipelineId& pipelineLayout);
/// setup from AnyFX program
void VkShaderProgramSetup(const Ids::Id24 id, const Resources::ResourceName& shaderName, GPULang::Deserialize::Program* program, const CoreGraphics::ResourcePipelineId& pipelineLayout);

/// create shader object
void VkShaderProgramCreateShader(const VkDevice dev, VkShaderModule* shader, unsigned binarySize, char* binary);
/// create shader object
void VkShaderProgramCreateShader(const VkDevice dev, VkShaderModule* shader, GPULang::Deserialize::Program::Shader* binary);
/// create this program as a graphics program
void VkShaderProgramSetupAsGraphics(AnyFX::VkProgram* program, const Resources::ResourceName& shaderName, VkShaderProgramRuntimeInfo& runtime);
/// create this program as a graphics program
void VkShaderProgramSetupAsGraphics(GPULang::Deserialize::Program* program, const Resources::ResourceName& shaderName, VkShaderProgramRuntimeInfo& runtime);
/// create this program as a compute program (can be done immediately)
void VkShaderProgramSetupAsCompute(VkShaderProgramSetupInfo& setup, VkShaderProgramRuntimeInfo& runtime);
/// create this program as a compute program (can be done immediately)
void VkShaderProgramSetupAsRaytracing(AnyFX::VkProgram* program, const Resources::ResourceName& shaderName, VkShaderProgramSetupInfo& setup, VkShaderProgramRuntimeInfo& runtime);
/// create this program as a compute program (can be done immediately)
void VkShaderProgramSetupAsRaytracing(GPULang::Deserialize::Program* program, const Resources::ResourceName& shaderName, VkShaderProgramSetupInfo& setup, VkShaderProgramRuntimeInfo& runtime);
/// Get raytracing library pipepline
VkPipeline VkShaderProgramGetRaytracingLibrary(const CoreGraphics::ShaderProgramId id);
/// Get resource layout of shader program
VkPipelineLayout VkShaderProgramGetLayout(const CoreGraphics::ShaderProgramId id);
/// Get ray payload and hit attribute sizes
void VkShaderProgramGetRaytracingVaryingSizes(const CoreGraphics::ShaderProgramId id, uint& rayPayloadSize, uint& hitAttributeSize);

} // namespace Vulkan
