//------------------------------------------------------------------------------
//  @file vkpipeline.cc
//  @copyright (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "coregraphics/pipeline.h"
#include "vkshaderprogram.h"
#include "vkshader.h"
#include "vkpipeline.h"
#include "vktypes.h"
#include "vkgraphicsdevice.h"
namespace Vulkan
{
Ids::IdAllocator<Pipeline> pipelineAllocator;
};

namespace CoreGraphics
{
using namespace Vulkan;

//------------------------------------------------------------------------------
/**
*/
PipelineId
CreatePipeline(const PipelineCreateInfo& info)
{
    Ids::Id32 ret = pipelineAllocator.Alloc();
    Pipeline& obj = pipelineAllocator.Get<0>(ret);

    VkGraphicsPipelineCreateInfo shaderInfo;
    VkShaderProgramRuntimeInfo& programInfo = shaderAlloc.Get<Shader_ProgramAllocator>(info.shader.shaderId).Get<ShaderProgram_RuntimeInfo>(info.shader.programId);

    // Setup blend info
    VkPipelineColorBlendStateCreateInfo blendInfo;
    blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendInfo.pNext = nullptr;
    blendInfo.flags = 0x0;
    blendInfo.attachmentCount = programInfo.colorBlendInfo.attachmentCount;
    blendInfo.flags = programInfo.colorBlendInfo.flags;
    blendInfo.logicOp = programInfo.colorBlendInfo.logicOp;
    blendInfo.logicOpEnable = programInfo.colorBlendInfo.logicOpEnable;
    blendInfo.pAttachments = programInfo.colorBlendAttachments;
    memcpy(blendInfo.blendConstants, programInfo.colorBlendInfo.blendConstants, sizeof(programInfo.colorBlendInfo.blendConstants));

    VkPipelineMultisampleStateCreateInfo multisampleInfo;
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.pNext = nullptr;
    multisampleInfo.flags = 0x0;
    multisampleInfo.rasterizationSamples = programInfo.multisampleInfo.rasterizationSamples;
    multisampleInfo.alphaToCoverageEnable = programInfo.multisampleInfo.alphaToCoverageEnable;
    multisampleInfo.alphaToOneEnable = programInfo.multisampleInfo.alphaToOneEnable;
    multisampleInfo.minSampleShading = programInfo.multisampleInfo.minSampleShading;
    multisampleInfo.sampleShadingEnable = programInfo.multisampleInfo.sampleShadingEnable;
    multisampleInfo.pSampleMask = programInfo.multisampleInfo.pSampleMask;

    shaderInfo.pColorBlendState = &blendInfo;
    shaderInfo.pDepthStencilState = &programInfo.depthStencilInfo;
    shaderInfo.pRasterizationState = &programInfo.rasterizerInfo;
    shaderInfo.pMultisampleState = &multisampleInfo;
    shaderInfo.pDynamicState = &programInfo.dynamicInfo;
    shaderInfo.pTessellationState = &programInfo.tessInfo;
    shaderInfo.layout = programInfo.layout;
    shaderInfo.stageCount = programInfo.stageCount;
    shaderInfo.pStages = programInfo.shaderInfos;

    // Since this is the public facing API, we have to convert the primitive type as it's expected to be in CoreGraphics 
    InputAssemblyKey translatedKey;
    translatedKey.topo = Vulkan::VkTypes::AsVkPrimitiveType((CoreGraphics::PrimitiveTopology::Code)info.inputAssembly.topo);
    translatedKey.primRestart = info.inputAssembly.primRestart;
    VkPipeline pipeline = Vulkan::GetOrCreatePipeline(info.pass, info.subpass, info.shader, translatedKey, shaderInfo);

    obj.pipeline = pipeline;
    obj.layout = programInfo.layout;
    obj.pass = info.pass;
    return PipelineId{ ret, 0 };
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyPipeline(const PipelineId pipeline)
{
    Pipeline& obj = pipelineAllocator.Get<0>(pipeline.id24);
    vkDestroyPipeline(Vulkan::GetCurrentDevice(), obj.pipeline, nullptr);
    pipelineAllocator.Dealloc(pipeline.id24);
}

} // namespace CoreGraphics
