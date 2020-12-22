//------------------------------------------------------------------------------
// vkshaderprogram.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshaderprogram.h"
#include "vkgraphicsdevice.h"
#include "coregraphics/shaderserver.h"
#include "lowlevel/vk/vkrenderstate.h"
#include "lowlevel/vk/vksampler.h"
#include "vkresourcetable.h"

using namespace Util;
namespace Vulkan
{

uint32_t UniqueIdCounter = 0;

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgramSetup(const Ids::Id24 id, const Resources::ResourceName& shaderName, AnyFX::VkProgram* program, const CoreGraphics::ResourcePipelineId& pipelineLayout, VkShaderProgramAllocator& allocator)
{
    allocator.Get<1>(id) = program;
    String mask = program->GetAnnotationString("Mask").c_str();
    String name = program->name.c_str();
    
    VkShaderProgramSetupInfo& setup = allocator.Get<0>(id);
    VkShaderProgramRuntimeInfo& runtime = allocator.Get<2>(id);
    runtime.layout = ResourcePipelineGetVk(pipelineLayout);
    runtime.pipeline = VK_NULL_HANDLE;
    runtime.uniqueId = UniqueIdCounter++;
    setup.mask = CoreGraphics::ShaderServer::Instance()->FeatureStringToMask(mask);
    setup.name = name;
    setup.dev = Vulkan::GetCurrentDevice();
    VkShaderProgramCreateShader(setup.dev, &runtime.vs, program->shaderBlock.vsBinarySize, program->shaderBlock.vsBinary);
    VkShaderProgramCreateShader(setup.dev, &runtime.hs, program->shaderBlock.hsBinarySize, program->shaderBlock.hsBinary);
    VkShaderProgramCreateShader(setup.dev, &runtime.ds, program->shaderBlock.dsBinarySize, program->shaderBlock.dsBinary);
    VkShaderProgramCreateShader(setup.dev, &runtime.gs, program->shaderBlock.gsBinarySize, program->shaderBlock.gsBinary);
    VkShaderProgramCreateShader(setup.dev, &runtime.ps, program->shaderBlock.psBinarySize, program->shaderBlock.psBinary);
    VkShaderProgramCreateShader(setup.dev, &runtime.cs, program->shaderBlock.csBinarySize, program->shaderBlock.csBinary);

    // if we have a compute shader, it will be the one we use, otherwise use the graphics one
    if (runtime.cs)
    {
#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.cs, Util::String::Sprintf("%s - Program: %s - CS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif

        VkShaderProgramSetupAsCompute(setup, runtime);
    }
    else if (runtime.vs)    VkShaderProgramSetupAsGraphics(program, shaderName, runtime);
    else                    runtime.type = CoreGraphics::InvalidPipeline;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgramCreateShader(const VkDevice dev, VkShaderModule* shader, unsigned binarySize, char* binary)
{
    if (binarySize > 0)
    {
        VkShaderModuleCreateInfo info =
        {
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            NULL,
            0,                                      // flags
            binarySize, // Vulkan expects the binary to be uint32, so we must assume size is in units of 4 bytes
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
VkShaderProgramSetupAsGraphics(AnyFX::VkProgram* program, const Resources::ResourceName& shaderName, VkShaderProgramRuntimeInfo& runtime)
{
    // we have to keep track of how MANY shaders we are using, AnyFX makes every function 'main'
    unsigned shaderIdx = 0;
    static const char* name = "main";
    memset(runtime.shaderInfos, 0, sizeof(runtime.shaderInfos));

    runtime.stencilFrontRef = program->renderState->renderSettings.stencilFrontRef;
    runtime.stencilBackRef = program->renderState->renderSettings.stencilBackRef;
    runtime.stencilReadMask = program->renderState->renderSettings.stencilReadMask;
    runtime.stencilWriteMask = program->renderState->renderSettings.stencilWriteMask;

    // attach vertex shader
    if (0 != runtime.vs)
    {
        runtime.shaderInfos[shaderIdx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        runtime.shaderInfos[shaderIdx].pNext = NULL;
        runtime.shaderInfos[shaderIdx].flags = 0;
        runtime.shaderInfos[shaderIdx].stage = VK_SHADER_STAGE_VERTEX_BIT;
        runtime.shaderInfos[shaderIdx].module = runtime.vs;
        runtime.shaderInfos[shaderIdx].pName = name;
        runtime.shaderInfos[shaderIdx].pSpecializationInfo = NULL;
        shaderIdx++;

#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.vs, Util::String::Sprintf("%s - Program: %s - VS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    if (0 != runtime.hs)
    {
        runtime.shaderInfos[shaderIdx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        runtime.shaderInfos[shaderIdx].pNext = NULL;
        runtime.shaderInfos[shaderIdx].flags = 0;
        runtime.shaderInfos[shaderIdx].stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        runtime.shaderInfos[shaderIdx].module = runtime.hs;
        runtime.shaderInfos[shaderIdx].pName = name;
        runtime.shaderInfos[shaderIdx].pSpecializationInfo = NULL;
        shaderIdx++;

#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.hs, Util::String::Sprintf("%s - Program: %s - HS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    if (0 != runtime.ds)
    {
        runtime.shaderInfos[shaderIdx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        runtime.shaderInfos[shaderIdx].pNext = NULL;
        runtime.shaderInfos[shaderIdx].flags = 0;
        runtime.shaderInfos[shaderIdx].stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        runtime.shaderInfos[shaderIdx].module = runtime.ds;
        runtime.shaderInfos[shaderIdx].pName = name;
        runtime.shaderInfos[shaderIdx].pSpecializationInfo = NULL;
        shaderIdx++;

#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.ds, Util::String::Sprintf("%s - Program: %s - DS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    if (0 != runtime.gs)
    {
        runtime.shaderInfos[shaderIdx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        runtime.shaderInfos[shaderIdx].pNext = NULL;
        runtime.shaderInfos[shaderIdx].flags = 0;
        runtime.shaderInfos[shaderIdx].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
        runtime.shaderInfos[shaderIdx].module = runtime.gs;
        runtime.shaderInfos[shaderIdx].pName = name;
        runtime.shaderInfos[shaderIdx].pSpecializationInfo = NULL;
        shaderIdx++;

#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.gs, Util::String::Sprintf("%s - Program: %s - GS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    if (0 != runtime.ps)
    {
        runtime.shaderInfos[shaderIdx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        runtime.shaderInfos[shaderIdx].pNext = NULL;
        runtime.shaderInfos[shaderIdx].flags = 0;
        runtime.shaderInfos[shaderIdx].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        runtime.shaderInfos[shaderIdx].module = runtime.ps;
        runtime.shaderInfos[shaderIdx].pName = name;
        runtime.shaderInfos[shaderIdx].pSpecializationInfo = NULL;
        shaderIdx++;

#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.ps, Util::String::Sprintf("%s - Program: %s - PS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    runtime.stageCount = shaderIdx;

    // retrieve implementation specific state
    AnyFX::VkRenderState* vkRenderState = static_cast<AnyFX::VkRenderState*>(program->renderState);

    runtime.rasterizerInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        NULL,
        0
    };
    vkRenderState->SetupRasterization(&runtime.rasterizerInfo);

    runtime.multisampleInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        NULL,
        0
    };
    vkRenderState->SetupMultisample(&runtime.multisampleInfo);

    runtime.depthStencilInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        NULL,
        0
    };
    vkRenderState->SetupDepthStencil(&runtime.depthStencilInfo);

    runtime.colorBlendInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        NULL,
        0
    };
    vkRenderState->SetupBlend(&runtime.colorBlendInfo);

    runtime.tessInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        NULL,
        0,
        program->patchSize
    };

    runtime.vertexInfo =
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
    static const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK, VK_DYNAMIC_STATE_STENCIL_WRITE_MASK, VK_DYNAMIC_STATE_STENCIL_REFERENCE };
    runtime.dynamicInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        NULL,
        0,
        5,
        dynamicStates
    };

    // be sure to flag compute shader as null
    runtime.type = CoreGraphics::GraphicsPipeline;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgramSetupAsCompute(VkShaderProgramSetupInfo& setup, VkShaderProgramRuntimeInfo& runtime)
{
    // create 6 shader info stages for each shader type
    n_assert(0 != runtime.cs);


    VkPipelineShaderStageCreateInfo shader =
    {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        NULL,
        0,
        VK_SHADER_STAGE_COMPUTE_BIT,
        runtime.cs,
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
    VkResult res = vkCreateComputePipelines(setup.dev, Vulkan::GetPipelineCache(), 1, &pInfo, NULL, &runtime.pipeline);
    n_assert(res == VK_SUCCESS);
    runtime.type = CoreGraphics::ComputePipeline;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgramDiscard(VkShaderProgramSetupInfo& info, VkShaderProgramRuntimeInfo& rt, VkPipeline& computePipeline)
{
    if (rt.vs != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.vs, NULL);
    if (rt.hs != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.hs, NULL);
    if (rt.ds != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.ds, NULL);
    if (rt.gs != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.gs, NULL);
    if (rt.ps != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.ps, NULL);
    if (rt.cs != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.cs, NULL);
    if (computePipeline != VK_NULL_HANDLE)          vkDestroyPipeline(info.dev, computePipeline, NULL);
}

} // namespace Vulkan