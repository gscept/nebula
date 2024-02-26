//------------------------------------------------------------------------------
// vkshaderprogram.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "vkshaderprogram.h"
#include "vkgraphicsdevice.h"
#include "coregraphics/shaderserver.h"
#include "lowlevel/vk/vkrenderstate.h"
#include "vkresourcetable.h"

using namespace Util;

namespace Vulkan
{

uint32_t UniqueIdCounter = 0;
ShaderProgramAllocator shaderProgramAlloc;
//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgramSetup(const Ids::Id24 id, const Resources::ResourceName& shaderName, AnyFX::VkProgram* program, const CoreGraphics::ResourcePipelineId& pipelineLayout)
{
    String mask = program->GetAnnotationString("Mask").c_str();
    String name = program->name.c_str();
    
    VkShaderProgramSetupInfo& setup = shaderProgramAlloc.Get<ShaderProgram_SetupInfo>(id);
    VkProgramReflectionInfo& refl = shaderProgramAlloc.Get<ShaderProgram_ReflectionInfo>(id);
    VkShaderProgramRuntimeInfo& runtime = shaderProgramAlloc.Get<ShaderProgram_RuntimeInfo>(id);
    runtime.layout = ResourcePipelineGetVk(pipelineLayout);
    runtime.pipeline = VK_NULL_HANDLE;
    runtime.uniqueId = UniqueIdCounter++;
    setup.mask = CoreGraphics::ShaderFeatureMask(mask);
    setup.name = name;
    setup.dev = Vulkan::GetCurrentDevice();
    VkShaderProgramCreateShader(setup.dev, &runtime.vs, program->shaderBlock.vs.binarySize, program->shaderBlock.vs.binary);
    VkShaderProgramCreateShader(setup.dev, &runtime.hs, program->shaderBlock.hs.binarySize, program->shaderBlock.hs.binary);
    VkShaderProgramCreateShader(setup.dev, &runtime.ds, program->shaderBlock.ds.binarySize, program->shaderBlock.ds.binary);
    VkShaderProgramCreateShader(setup.dev, &runtime.gs, program->shaderBlock.gs.binarySize, program->shaderBlock.gs.binary);
    VkShaderProgramCreateShader(setup.dev, &runtime.ps, program->shaderBlock.ps.binarySize, program->shaderBlock.ps.binary);
    VkShaderProgramCreateShader(setup.dev, &runtime.cs, program->shaderBlock.cs.binarySize, program->shaderBlock.cs.binary);
    if (CoreGraphics::RayTracingSupported)
    {
        VkShaderProgramCreateShader(setup.dev, &runtime.rg, program->shaderBlock.rg.binarySize, program->shaderBlock.rg.binary);
        VkShaderProgramCreateShader(setup.dev, &runtime.ra, program->shaderBlock.ra.binarySize, program->shaderBlock.ra.binary);
        VkShaderProgramCreateShader(setup.dev, &runtime.rc, program->shaderBlock.rc.binarySize, program->shaderBlock.rc.binary);
        VkShaderProgramCreateShader(setup.dev, &runtime.rm, program->shaderBlock.rm.binarySize, program->shaderBlock.rm.binary);
        VkShaderProgramCreateShader(setup.dev, &runtime.ri, program->shaderBlock.ri.binarySize, program->shaderBlock.ri.binary);
        VkShaderProgramCreateShader(setup.dev, &runtime.ca, program->shaderBlock.ca.binarySize, program->shaderBlock.ca.binary);
    }

    if (CoreGraphics::MeshShadersSupported)
    {
        VkShaderProgramCreateShader(setup.dev, &runtime.ts, program->shaderBlock.ts.binarySize, program->shaderBlock.ts.binary);
        VkShaderProgramCreateShader(setup.dev, &runtime.ms, program->shaderBlock.ms.binarySize, program->shaderBlock.ms.binary);
    }

    for (size_t i = 0; i < program->vsInputSlots.size(); i++)
    {
        refl.vsInputSlots.Append(program->vsInputSlots[i]);
    }
    refl.name = program->name.c_str();

    // if we have a compute shader, it will be the one we use, otherwise use the graphics one
    if (runtime.cs)
    {
#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.cs, Util::String::Sprintf("%s - Program: %s - CS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif

        VkShaderProgramSetupAsCompute(setup, runtime);
    }
    else if (runtime.vs)
        VkShaderProgramSetupAsGraphics(program, shaderName, runtime);
    else if (CoreGraphics::RayTracingSupported &&
             (runtime.rg || runtime.ra || runtime.rc || runtime.rm || runtime.ri || runtime.ca))
        VkShaderProgramSetupAsRaytracing(program, shaderName, setup, runtime);
    else
        runtime.type = CoreGraphics::InvalidPipeline;
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
    // we have to keep track of how many shaders we are using, AnyFX makes every function 'main'
    unsigned shaderIdx = 0;
    static const char* name = "main";
    memset(runtime.graphicsShaderInfos, 0, sizeof(runtime.graphicsShaderInfos));

    runtime.stencilFrontRef = program->renderState->renderSettings.stencilFrontRef;
    runtime.stencilBackRef = program->renderState->renderSettings.stencilBackRef;
    runtime.stencilReadMask = program->renderState->renderSettings.stencilReadMask;
    runtime.stencilWriteMask = program->renderState->renderSettings.stencilWriteMask;

    // attach vertex shader
    if (0 != runtime.vs)
    {
        runtime.graphicsShaderInfos[shaderIdx] =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = runtime.vs,
            .pName = name,
            .pSpecializationInfo = nullptr
        };
        shaderIdx++;

#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.vs, Util::String::Sprintf("%s - Program: %s - VS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    if (0 != runtime.ts)
    {
        runtime.graphicsShaderInfos[shaderIdx] =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_TASK_BIT_EXT,
            .module = runtime.ts,
            .pName = name,
            .pSpecializationInfo = nullptr
        };
        shaderIdx++;

#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.ts, Util::String::Sprintf("%s - Program: %s - TS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    if (0 != runtime.ms)
    {
        runtime.graphicsShaderInfos[shaderIdx] =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_MESH_BIT_EXT,
            .module = runtime.ms,
            .pName = name,
            .pSpecializationInfo = nullptr
        };
        shaderIdx++;

#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.ms, Util::String::Sprintf("%s - Program: %s - MS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    if (0 != runtime.hs)
    {
        runtime.graphicsShaderInfos[shaderIdx] =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
            .module = runtime.hs,
            .pName = name,
            .pSpecializationInfo = nullptr
        };
        shaderIdx++;

#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.hs, Util::String::Sprintf("%s - Program: %s - HS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    if (0 != runtime.ds)
    {
        runtime.graphicsShaderInfos[shaderIdx] =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
            .module = runtime.ds,
            .pName = name,
            .pSpecializationInfo = nullptr
        };
        shaderIdx++;

#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.ds, Util::String::Sprintf("%s - Program: %s - DS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    if (0 != runtime.gs)
    {
        runtime.graphicsShaderInfos[shaderIdx] =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_GEOMETRY_BIT,
            .module = runtime.gs,
            .pName = name,
            .pSpecializationInfo = nullptr
        };
        shaderIdx++;

#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.gs, Util::String::Sprintf("%s - Program: %s - GS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    if (0 != runtime.ps)
    {
        runtime.graphicsShaderInfos[shaderIdx] =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = runtime.ps,
            .pName = name,
            .pSpecializationInfo = nullptr
        };
        shaderIdx++;

#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.ps, Util::String::Sprintf("%s - Program: %s - PS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    if (0 != runtime.ms)
    {
        runtime.graphicsShaderInfos[shaderIdx] =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_MESH_BIT_EXT,
            .module = runtime.ms,
            .pName = name,
            .pSpecializationInfo = nullptr
        };
        shaderIdx++;

#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.ms, Util::String::Sprintf("%s - Program: %s - MS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    runtime.stageCount = shaderIdx;

    // retrieve implementation specific state
    AnyFX::VkRenderState* vkRenderState = static_cast<AnyFX::VkRenderState*>(program->renderState);

    runtime.rasterizerInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        nullptr,
        0
    };
    vkRenderState->SetupRasterization(&runtime.rasterizerInfo);

    runtime.multisampleInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        nullptr,
        0
    };
    vkRenderState->SetupMultisample(&runtime.multisampleInfo);

    runtime.depthStencilInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        nullptr,
        0
    };
    vkRenderState->SetupDepthStencil(&runtime.depthStencilInfo);

    runtime.colorBlendInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        nullptr,
        0
    };
    vkRenderState->SetupBlend(&runtime.colorBlendInfo, runtime.colorBlendAttachments);

    runtime.tessInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        nullptr,
        0,
        program->patchSize
    };

    runtime.vertexInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        nullptr,
        0,
        0,
        nullptr,
        program->numVsInputs,
        nullptr
    };

    // setup dynamic state, we only support dynamic viewports and scissor rects
    static const VkDynamicState dynamicStates[] =
    {
        VK_DYNAMIC_STATE_SCISSOR
        , VK_DYNAMIC_STATE_VIEWPORT
        , VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK
        , VK_DYNAMIC_STATE_STENCIL_WRITE_MASK
        , VK_DYNAMIC_STATE_STENCIL_REFERENCE
        , VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY
        , VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE
        , VK_DYNAMIC_STATE_VERTEX_INPUT_EXT
    };
    runtime.graphicsDynamicStateInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        nullptr,
        0,
        sizeof(dynamicStates) / sizeof(VkDynamicState),
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
        nullptr,
        0,
        VK_SHADER_STAGE_COMPUTE_BIT,
        runtime.cs,
        "main",
        VK_NULL_HANDLE,
    };

    VkComputePipelineCreateInfo pInfo =
    {
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        nullptr,
        0,
        shader,
        runtime.layout,
        VK_NULL_HANDLE, 
        0
    };

    // create pipeline
    VkResult res = vkCreateComputePipelines(setup.dev, Vulkan::GetPipelineCache(), 1, &pInfo, nullptr, &runtime.pipeline);
    n_assert(res == VK_SUCCESS);
    runtime.type = CoreGraphics::ComputePipeline;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgramSetupAsRaytracing(AnyFX::VkProgram* program, const Resources::ResourceName& shaderName, VkShaderProgramSetupInfo& setup, VkShaderProgramRuntimeInfo& runtime)
{
    if (runtime.rg)
    {
#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.rg, Util::String::Sprintf("%s - Program: %s - RGS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    if (runtime.ca)
    {
#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.ca, Util::String::Sprintf("%s - Program: %s - CAS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    if (runtime.ra)
    {
#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.ra, Util::String::Sprintf("%s - Program: %s - RAS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    if (runtime.rc)
    {
#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.rc, Util::String::Sprintf("%s - Program: %s - RCS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    if (runtime.rm)
    {
#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.rm, Util::String::Sprintf("%s - Program: %s - RMS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    if (runtime.ri)
    {
#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.ri, Util::String::Sprintf("%s - Program: %s - RIS", shaderName.Value(), program->name.c_str()).AsCharPtr());
#endif
    }

    // Just setup some data, we actually create the pipeline when we combine shaders later
    runtime.rayPayloadSize = program->rayPayloadSize;
    runtime.hitAttributeSize = program->hitAttributeSize;
    runtime.type = CoreGraphics::RayTracingPipeline;
}

//------------------------------------------------------------------------------
/**
*/
VkPipeline
VkShaderProgramGetRaytracingLibrary(const CoreGraphics::ShaderProgramId id)
{
    const VkShaderProgramRuntimeInfo& runtime = shaderProgramAlloc.Get<ShaderProgram_RuntimeInfo>(id.programId);
    return runtime.pipeline;
}

//------------------------------------------------------------------------------
/**
*/
VkPipelineLayout
VkShaderProgramGetLayout(const CoreGraphics::ShaderProgramId id)
{
    const VkShaderProgramRuntimeInfo& runtime = shaderProgramAlloc.Get<ShaderProgram_RuntimeInfo>(id.programId);
    return runtime.layout;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgramGetRaytracingVaryingSizes(const CoreGraphics::ShaderProgramId id, uint& rayPayloadSize, uint& hitAttributeSize)
{
    const VkShaderProgramRuntimeInfo& runtime = shaderProgramAlloc.Get<ShaderProgram_RuntimeInfo>(id.programId);
    rayPayloadSize = runtime.rayPayloadSize;
    hitAttributeSize = runtime.hitAttributeSize;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgramDiscard(VkShaderProgramSetupInfo& info, VkShaderProgramRuntimeInfo& rt, VkPipeline& pipeline)
{
    if (rt.vs != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.vs, nullptr);
    if (rt.hs != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.hs, nullptr);
    if (rt.ds != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.ds, nullptr);
    if (rt.gs != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.gs, nullptr);
    if (rt.ps != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.ps, nullptr);
    if (rt.cs != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.cs, nullptr);
    if (rt.ts != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.ts, nullptr);
    if (rt.ms != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.ms, nullptr);
    if (rt.rg != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.rg, nullptr);
    if (rt.ra != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.ra, nullptr);
    if (rt.rc != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.rc, nullptr);
    if (rt.rm != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.rm, nullptr);
    if (rt.ri != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.ri, nullptr);
    if (rt.ca != VK_NULL_HANDLE)                    vkDestroyShaderModule(info.dev, rt.ca, nullptr);
    if (pipeline != VK_NULL_HANDLE)                 vkDestroyPipeline(info.dev, pipeline, nullptr);
}

} // namespace Vulkan

namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
*/
RayTracingBits
ShaderProgramGetRaytracingBits(const ShaderProgramId id)
{
    using namespace Vulkan;
    VkShaderProgramRuntimeInfo& runtime = shaderProgramAlloc.Get<ShaderProgram_RuntimeInfo>(id.programId);
    return RayTracingBits
    {
        .bitField
        {
            .hasGen = runtime.rg != VK_NULL_HANDLE,
            .hasCallable = runtime.ca != VK_NULL_HANDLE,
            .hasAnyHit = runtime.ra != VK_NULL_HANDLE,
            .hasClosestHit = runtime.rc != VK_NULL_HANDLE,
            .hasIntersect = runtime.ri != VK_NULL_HANDLE,
            .hasMiss = runtime.rm != VK_NULL_HANDLE,
        }
    };
}

} // namespace CoreGraphics
