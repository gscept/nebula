//------------------------------------------------------------------------------
// vkshaderprogram.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "vkshaderprogram.h"
#include "vkgraphicsdevice.h"
#include "coregraphics/shaderserver.h"
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
VkShaderProgramSetup(
    const Ids::Id24 id,
    const Resources::ResourceName& shaderName,
    GPULang::Deserialize::Program* program,
    const CoreGraphics::ResourcePipelineId& pipelineLayout
)
{
    String mask, name;
    for (size_t i = 0; i < program->annotationCount; i++)
    {
        String annot = String(program->annotations[i].name, program->annotations[i].nameLength);
        if (annot == "Mask")
            mask = String(program->annotations[i].data.s.string, program->annotations[i].data.s.length);
    }
    name = String(program->name, program->nameLength);
    
    VkShaderProgramSetupInfo& setup = shaderProgramAlloc.Get<ShaderProgram_SetupInfo>(id);
    VkProgramReflectionInfo& refl = shaderProgramAlloc.Get<ShaderProgram_ReflectionInfo>(id);
    VkShaderProgramRuntimeInfo& runtime = shaderProgramAlloc.Get<ShaderProgram_RuntimeInfo>(id);
    runtime.layout = ResourcePipelineGetVk(pipelineLayout);
    runtime.pipeline = VK_NULL_HANDLE;
    runtime.uniqueId = UniqueIdCounter++;
    setup.mask = CoreGraphics::ShaderFeatureMask(mask.IsEmpty() ? name : mask);
    setup.name = name;
    setup.dev = Vulkan::GetCurrentDevice();
    VkShaderProgramCreateShader(setup.dev, &runtime.vs, &program->shaders[GPULang::Deserialize::Program::ShaderStages::VertexShader]);
    VkShaderProgramCreateShader(setup.dev, &runtime.hs, &program->shaders[GPULang::Deserialize::Program::ShaderStages::HullShader]);
    VkShaderProgramCreateShader(setup.dev, &runtime.ds, &program->shaders[GPULang::Deserialize::Program::ShaderStages::DomainShader]);
    VkShaderProgramCreateShader(setup.dev, &runtime.gs, &program->shaders[GPULang::Deserialize::Program::ShaderStages::GeometryShader]);
    VkShaderProgramCreateShader(setup.dev, &runtime.ps, &program->shaders[GPULang::Deserialize::Program::ShaderStages::PixelShader]);
    VkShaderProgramCreateShader(setup.dev, &runtime.cs, &program->shaders[GPULang::Deserialize::Program::ShaderStages::ComputeShader]);
    if (CoreGraphics::RayTracingSupported)
    {
        VkShaderProgramCreateShader(setup.dev, &runtime.rg, &program->shaders[GPULang::Deserialize::Program::ShaderStages::RayGenShader]);
        VkShaderProgramCreateShader(setup.dev, &runtime.ra, &program->shaders[GPULang::Deserialize::Program::ShaderStages::RayAnyHitShader]);
        VkShaderProgramCreateShader(setup.dev, &runtime.rc, &program->shaders[GPULang::Deserialize::Program::ShaderStages::RayClosestHitShader]);
        VkShaderProgramCreateShader(setup.dev, &runtime.rm, &program->shaders[GPULang::Deserialize::Program::ShaderStages::RayMissShader]);
        VkShaderProgramCreateShader(setup.dev, &runtime.ri, &program->shaders[GPULang::Deserialize::Program::ShaderStages::RayIntersectionShader]);
        VkShaderProgramCreateShader(setup.dev, &runtime.ca, &program->shaders[GPULang::Deserialize::Program::ShaderStages::RayCallableShader]);
    }

    if (CoreGraphics::MeshShadersSupported)
    {
        VkShaderProgramCreateShader(setup.dev, &runtime.ts, &program->shaders[GPULang::Deserialize::Program::ShaderStages::TaskShader]);
        VkShaderProgramCreateShader(setup.dev, &runtime.ms, &program->shaders[GPULang::Deserialize::Program::ShaderStages::MeshShader]);
    }

    for (size_t i = 0; i < program->vsInputLength; i++)
    {
        refl.vsInputSlots.Append(program->vsInputs[i]);
    }
    refl.name = name;

    // if we have a compute shader, it will be the one we use, otherwise use the graphics one
    if (runtime.cs)
    {
#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.cs, Util::String::Sprintf("%s - Program: %s - CS", shaderName.Value(), name.AsCharPtr()).AsCharPtr());
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
VkShaderProgramCreateShader(const VkDevice dev, VkShaderModule* shader, GPULang::Deserialize::Program::Shader* binary)
{
    if (binary->binaryLength > 0)
    {
        VkShaderModuleCreateInfo info =
        {
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            NULL,
            0,                                      // flags
            binary->binaryLength, // Vulkan expects the binary to be uint32, so we must assume size is in units of 4 bytes
            binary->binary
        };

        // create shader
        VkResult res = vkCreateShaderModule(dev, &info, NULL, shader);
        assert(res == VK_SUCCESS);
    }
}

//------------------------------------------------------------------------------
/**
*/
VkStencilOp
StencilOpFromGPULang(GPULang::Serialization::StencilOp op)
{
    switch (op)
    {
        case GPULang::Serialization::StencilOp::StencilKeepOp: return VkStencilOp::VK_STENCIL_OP_KEEP;
        case GPULang::Serialization::StencilOp::StencilZeroOp: return VkStencilOp::VK_STENCIL_OP_ZERO;
        case GPULang::Serialization::StencilOp::StencilReplaceOp: return VkStencilOp::VK_STENCIL_OP_REPLACE;
        case GPULang::Serialization::StencilOp::StencilIncrementClampOp: return VkStencilOp::VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case GPULang::Serialization::StencilOp::StencilDecrementClampOp: return VkStencilOp::VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case GPULang::Serialization::StencilOp::StencilInvertOp: return VkStencilOp::VK_STENCIL_OP_INVERT;
        case GPULang::Serialization::StencilOp::StencilIncrementWrapOp: return VkStencilOp::VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case GPULang::Serialization::StencilOp::StencilDecrementWrapOp: return VkStencilOp::VK_STENCIL_OP_DECREMENT_AND_WRAP;
    }
    n_error("Invalid GPULang stencil op");
    return VkStencilOp::VK_STENCIL_OP_KEEP;
}

//------------------------------------------------------------------------------
/**
*/
VkCompareOp
CompareOpFromGPULang(GPULang::Serialization::CompareMode mode)
{
    switch (mode)
    {
        case GPULang::Serialization::CompareMode::NeverCompare: return VK_COMPARE_OP_NEVER;
        case GPULang::Serialization::CompareMode::LessCompare: return VK_COMPARE_OP_LESS;
        case GPULang::Serialization::CompareMode::LessEqualCompare: return VK_COMPARE_OP_LESS_OR_EQUAL;
        case GPULang::Serialization::CompareMode::GreaterCompare: return VK_COMPARE_OP_GREATER;
        case GPULang::Serialization::CompareMode::GreaterEqualCompare: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case GPULang::Serialization::CompareMode::NotEqualCompare: return VK_COMPARE_OP_NOT_EQUAL;
        case GPULang::Serialization::CompareMode::EqualCompare: return VK_COMPARE_OP_EQUAL;
        case GPULang::Serialization::CompareMode::AlwaysCompare: return VK_COMPARE_OP_ALWAYS;
    }
    n_error("Invalid GPULang compare mode");
    return VK_COMPARE_OP_ALWAYS;
}

//------------------------------------------------------------------------------
/**
*/
VkPolygonMode
PolygonModeFromGPULang(GPULang::Serialization::RasterizationMode mode)
{
    switch (mode)
    {
        case GPULang::Serialization::RasterizationMode::FillMode: return VK_POLYGON_MODE_FILL;
        case GPULang::Serialization::RasterizationMode::LineMode: return VK_POLYGON_MODE_LINE;
        case GPULang::Serialization::RasterizationMode::PointMode: return VK_POLYGON_MODE_POINT;
    }
    n_error("Invalid GPULang polygon mode");
    return VK_POLYGON_MODE_FILL;
}

//------------------------------------------------------------------------------
/**
*/
VkFrontFace
PolygonModeFromGPULang(GPULang::Serialization::WindingOrderMode mode)
{
    switch (mode)
    {
        case GPULang::Serialization::WindingOrderMode::ClockwiseMode: return VK_FRONT_FACE_CLOCKWISE;
        case GPULang::Serialization::WindingOrderMode::CounterClockwiseMode: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }
    n_error("Invalid GPULang winding order mode");
    return VK_FRONT_FACE_CLOCKWISE;
}

//------------------------------------------------------------------------------
/**
*/
VkLogicOp
LogicOpFromGPULang(GPULang::Serialization::LogicOp op)
{
    switch (op)
    {
        case GPULang::Serialization::LogicOp::LogicClearOp: return VK_LOGIC_OP_CLEAR;
        case GPULang::Serialization::LogicOp::LogicAndOp: return VK_LOGIC_OP_AND;
        case GPULang::Serialization::LogicOp::LogicAndReverseOp: return VK_LOGIC_OP_AND_REVERSE;
        case GPULang::Serialization::LogicOp::LogicCopyOp: return VK_LOGIC_OP_COPY;
        case GPULang::Serialization::LogicOp::LogicAndInvertedOp: return VK_LOGIC_OP_AND_INVERTED;
        case GPULang::Serialization::LogicOp::LogicNoOp: return VK_LOGIC_OP_NO_OP;
        case GPULang::Serialization::LogicOp::LogicXorOp: return VK_LOGIC_OP_XOR;
        case GPULang::Serialization::LogicOp::LogicOrOp: return VK_LOGIC_OP_OR;
        case GPULang::Serialization::LogicOp::LogicNorOp: return VK_LOGIC_OP_NOR;
        case GPULang::Serialization::LogicOp::LogicEquivalentOp: return VK_LOGIC_OP_EQUIVALENT;
        case GPULang::Serialization::LogicOp::LogicInvertOp: return VK_LOGIC_OP_INVERT;
        case GPULang::Serialization::LogicOp::LogicOrReverseOp: return VK_LOGIC_OP_OR_REVERSE;
        case GPULang::Serialization::LogicOp::LogicCopyInvertedOp: return VK_LOGIC_OP_COPY_INVERTED;
        case GPULang::Serialization::LogicOp::LogicOrInvertedOp: return VK_LOGIC_OP_OR_INVERTED;
        case GPULang::Serialization::LogicOp::LogicNandOp: return VK_LOGIC_OP_NAND;
        case GPULang::Serialization::LogicOp::LogicSetOp: return VK_LOGIC_OP_SET;
    }

    n_error("Invalid GPULang logic op");
    return VK_LOGIC_OP_CLEAR;
}

//------------------------------------------------------------------------------
/**
*/
VkBlendFactor
BlendFactorFromGPULang(GPULang::Serialization::BlendFactor factor)
{
    switch (factor)
    {
        case GPULang::Serialization::BlendFactor::ZeroFactor: return VK_BLEND_FACTOR_ZERO;
        case GPULang::Serialization::BlendFactor::OneFactor: return VK_BLEND_FACTOR_ONE;
        case GPULang::Serialization::BlendFactor::SourceColorFactor: return VK_BLEND_FACTOR_SRC_COLOR;
        case GPULang::Serialization::BlendFactor::OneMinusSourceColorFactor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case GPULang::Serialization::BlendFactor::DestinationColorFactor: return VK_BLEND_FACTOR_DST_COLOR;
        case GPULang::Serialization::BlendFactor::OneMinusDestinationColorFactor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case GPULang::Serialization::BlendFactor::SourceAlphaFactor: return VK_BLEND_FACTOR_SRC_ALPHA;
        case GPULang::Serialization::BlendFactor::OneMinusSourceAlphaFactor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case GPULang::Serialization::BlendFactor::DestinationAlphaFactor: return VK_BLEND_FACTOR_DST_ALPHA;
        case GPULang::Serialization::BlendFactor::OneMinusDestinationAlphaFactor: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case GPULang::Serialization::BlendFactor::ConstantColorFactor: return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case GPULang::Serialization::BlendFactor::OneMinusConstantColorFactor: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case GPULang::Serialization::BlendFactor::ConstantAlphaFactor: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case GPULang::Serialization::BlendFactor::OneMinusConstantAlphaFactor: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        case GPULang::Serialization::BlendFactor::SourceAlphaSaturateFactor: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case GPULang::Serialization::BlendFactor::Source1ColorFactor: return VK_BLEND_FACTOR_SRC1_COLOR;
        case GPULang::Serialization::BlendFactor::OneMinusSource1ColorFactor: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        case GPULang::Serialization::BlendFactor::Source1AlphaFactor: return VK_BLEND_FACTOR_SRC1_ALPHA;
        case GPULang::Serialization::BlendFactor::OneMinusSource1AlphaFactor: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
    }
    n_error("Invalid GPULang blend factor");
    return VK_BLEND_FACTOR_ZERO; // Handle invalid cases gracefully
}

//------------------------------------------------------------------------------
/**
*/
VkBlendOp
BlendOpFromGPULang(GPULang::Serialization::BlendOp op)
{
    switch (op)
    {
        case GPULang::Serialization::BlendOp::AddOp: return VK_BLEND_OP_ADD;
        case GPULang::Serialization::BlendOp::SubtractOp: return VK_BLEND_OP_SUBTRACT;
        case GPULang::Serialization::BlendOp::ReverseSubtractOp: return VK_BLEND_OP_REVERSE_SUBTRACT;
        case GPULang::Serialization::BlendOp::MinOp: return VK_BLEND_OP_MIN;
        case GPULang::Serialization::BlendOp::MaxOp: return VK_BLEND_OP_MAX;
    }
    n_error("Invalid GPULang blend op");
    return VK_BLEND_OP_ADD;
}

//------------------------------------------------------------------------------
/**
*/
VkCullModeFlags
CullModeFromGPULang(GPULang::Serialization::CullMode mode)
{
    switch (mode)
    {
        case GPULang::Serialization::CullMode::BackMode: return VK_CULL_MODE_BACK_BIT;
        case GPULang::Serialization::CullMode::FrontMode:return VK_CULL_MODE_FRONT_BIT;
        case GPULang::Serialization::CullMode::FrontAndBackMode: return VK_CULL_MODE_FRONT_AND_BACK;
        case GPULang::Serialization::CullMode::NoCullMode: return VK_CULL_MODE_NONE;
    }

    n_error("Invalid GPULang cull mode");
    return VK_CULL_MODE_BACK_BIT;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgramSetupAsGraphics(GPULang::Deserialize::Program* program, const Resources::ResourceName& shaderName, VkShaderProgramRuntimeInfo& runtime)
{
    // we have to keep track of how many shaders we are using, AnyFX makes every function 'main'
    unsigned shaderIdx = 0;
    static const char* name = "main";
    memset(runtime.graphicsShaderInfos, 0, sizeof(runtime.graphicsShaderInfos));

    runtime.stencilFrontRef = program->renderState->frontStencilState.referenceMask;
    runtime.stencilBackRef = program->renderState->backStencilState.referenceMask;
    runtime.stencilReadMask = program->renderState->frontStencilState.compareMask;
    runtime.stencilWriteMask = program->renderState->frontStencilState.writeMask;

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
        CoreGraphics::ObjectSetName(runtime.vs, Util::String::Sprintf("%s - Program: %s - VS", shaderName.Value(), program->name).AsCharPtr());
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
        CoreGraphics::ObjectSetName(runtime.ts, Util::String::Sprintf("%s - Program: %s - TS", shaderName.Value(), program->name).AsCharPtr());
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
        CoreGraphics::ObjectSetName(runtime.ms, Util::String::Sprintf("%s - Program: %s - MS", shaderName.Value(), program->name).AsCharPtr());
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
        CoreGraphics::ObjectSetName(runtime.hs, Util::String::Sprintf("%s - Program: %s - HS", shaderName.Value(), program->name).AsCharPtr());
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
        CoreGraphics::ObjectSetName(runtime.ds, Util::String::Sprintf("%s - Program: %s - DS", shaderName.Value(), program->name).AsCharPtr());
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
        CoreGraphics::ObjectSetName(runtime.gs, Util::String::Sprintf("%s - Program: %s - GS", shaderName.Value(), program->name).AsCharPtr());
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
        CoreGraphics::ObjectSetName(runtime.ps, Util::String::Sprintf("%s - Program: %s - PS", shaderName.Value(), program->name).AsCharPtr());
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
        CoreGraphics::ObjectSetName(runtime.ms, Util::String::Sprintf("%s - Program: %s - MS", shaderName.Value(), program->name).AsCharPtr());
#endif
    }

    runtime.stageCount = shaderIdx;
    runtime.rasterizerInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        nullptr,
        0
    };
    runtime.rasterizerInfo.depthClampEnable = program->renderState->depthClampEnabled;
    runtime.rasterizerInfo.rasterizerDiscardEnable = program->renderState->noPixels;
    runtime.rasterizerInfo.polygonMode = PolygonModeFromGPULang(program->renderState->rasterizationMode);
    runtime.rasterizerInfo.cullMode = CullModeFromGPULang(program->renderState->cullMode);
    runtime.rasterizerInfo.frontFace = PolygonModeFromGPULang(program->renderState->windingOrderMode);
    runtime.rasterizerInfo.depthBiasEnable = program->renderState->depthBiasEnabled;
    runtime.rasterizerInfo.depthBiasConstantFactor = program->renderState->depthBiasFactor;
    runtime.rasterizerInfo.depthBiasClamp = program->renderState->depthBiasClamp;
    runtime.rasterizerInfo.depthBiasSlopeFactor = program->renderState->depthBiasSlopeFactor;
    runtime.rasterizerInfo.lineWidth = program->renderState->lineWidth;

    runtime.multisampleInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        nullptr,
        0,
        (VkSampleCountFlagBits)program->renderState->samples,
        program->renderState->sampleShadingEnabled,
        program->renderState->minSampleShading,
        nullptr, // FIX
        program->renderState->alphaToCoverageEnabled,
        program->renderState->alphaToOneEnabled
    };

    runtime.depthStencilInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        nullptr,
        0,
        program->renderState->depthTestEnabled,
        program->renderState->depthWriteEnabled,
        CompareOpFromGPULang(program->renderState->depthCompare),
        program->renderState->depthBoundsTestEnabled,
        program->renderState->stencilEnabled,
        {
            StencilOpFromGPULang(program->renderState->frontStencilState.fail),
            StencilOpFromGPULang(program->renderState->frontStencilState.pass),
            StencilOpFromGPULang(program->renderState->frontStencilState.depthFail),
            CompareOpFromGPULang(program->renderState->frontStencilState.compare),
            program->renderState->frontStencilState.compareMask,
            program->renderState->frontStencilState.writeMask,
            program->renderState->frontStencilState.referenceMask,
        },
        {
            StencilOpFromGPULang(program->renderState->backStencilState.fail),
            StencilOpFromGPULang(program->renderState->backStencilState.pass),
            StencilOpFromGPULang(program->renderState->backStencilState.depthFail),
            CompareOpFromGPULang(program->renderState->backStencilState.compare),
            program->renderState->backStencilState.compareMask,
            program->renderState->backStencilState.writeMask,
            program->renderState->backStencilState.referenceMask,
        },
        program->renderState->minDepthBounds,
        program->renderState->maxDepthBounds,
    };

    runtime.colorBlendInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        nullptr,
        0
    };
    runtime.colorBlendInfo.logicOp = LogicOpFromGPULang(program->renderState->logicOp);
    runtime.colorBlendInfo.logicOpEnable = program->renderState->logicOpEnabled;
    runtime.colorBlendInfo.blendConstants[0] = program->renderState->blendConstants[0];
    runtime.colorBlendInfo.blendConstants[1] = program->renderState->blendConstants[1];
    runtime.colorBlendInfo.blendConstants[2] = program->renderState->blendConstants[2];
    runtime.colorBlendInfo.blendConstants[3] = program->renderState->blendConstants[3];
    for (uint i = 0; i < 8; i++)
    {
        runtime.colorBlendAttachments[i] =
        {
            program->renderState->blendStates[i].blendEnabled,
            BlendFactorFromGPULang(program->renderState->blendStates[i].sourceColorBlendFactor),
            BlendFactorFromGPULang(program->renderState->blendStates[i].destinationColorBlendFactor),
            BlendOpFromGPULang(program->renderState->blendStates[i].colorBlendOp),
            BlendFactorFromGPULang(program->renderState->blendStates[i].sourceAlphaBlendFactor),
            BlendFactorFromGPULang(program->renderState->blendStates[i].destinationAlphaBlendFactor),
            BlendOpFromGPULang(program->renderState->blendStates[i].alphaBlendOp),
            program->renderState->blendStates[i].colorComponentMask
        };
    }

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
        (uint32_t)program->vsInputLength,
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
}(

//------------------------------------------------------------------------------
/**
*/
void
VkShaderProgramSetupAsRaytracing(
    GPULang::Deserialize::Program* program,
    const Resources::ResourceName& shaderName,
    VkShaderProgramSetupInfo& setup,
    VkShaderProgramRuntimeInfo& runtime
)
{
     if (runtime.rg)
    {
#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.rg, Util::String::Sprintf("%s - Program: %s - RGS", shaderName.Value(), program->name).AsCharPtr());
#endif
    }

    if (runtime.ca)
    {
#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.ca, Util::String::Sprintf("%s - Program: %s - CAS", shaderName.Value(), program->name).AsCharPtr());
#endif
    }

    if (runtime.ra)
    {
#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.ra, Util::String::Sprintf("%s - Program: %s - RAS", shaderName.Value(), program->name).AsCharPtr());
#endif
    }

    if (runtime.rc)
    {
#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.rc, Util::String::Sprintf("%s - Program: %s - RCS", shaderName.Value(), program->name).AsCharPtr());
#endif
    }

    if (runtime.rm)
    {
#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.rm, Util::String::Sprintf("%s - Program: %s - RMS", shaderName.Value(), program->name).AsCharPtr());
#endif
    }

    if (runtime.ri)
    {
#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::ObjectSetName(runtime.ri, Util::String::Sprintf("%s - Program: %s - RIS", shaderName.Value(), program->name).AsCharPtr());
#endif
    }

    // Just setup some data, we actually create the pipeline when we combine shaders later
    runtime.rayPayloadSize = program->rayPayloadSize;
    runtime.hitAttributeSize = program->rayHitAttributeSize;
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
