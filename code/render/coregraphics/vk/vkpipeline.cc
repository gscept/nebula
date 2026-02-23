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
#include "vkpass.h"
namespace Vulkan
{

Ids::IdAllocator<Pipeline> pipelineAllocator;


//------------------------------------------------------------------------------
/**
*/
VkDevice
PipelineGetVkDevice(const CoreGraphics::PipelineId id)
{
    return pipelineAllocator.Get<0>(id.id).dev;
}

//------------------------------------------------------------------------------
/**
*/
VkPipeline
PipelineGetVkPipeline(const CoreGraphics::PipelineId id)
{
    return pipelineAllocator.Get<0>(id.id).pipeline;
}
};

namespace CoreGraphics
{
using namespace Vulkan;

//------------------------------------------------------------------------------
/**
*/
PipelineId
CreateGraphicsPipeline(const PipelineCreateInfo& info)
{
    VkGraphicsPipelineCreateInfo shaderInfo;
    VkShaderProgramRuntimeInfo& programInfo = shaderProgramAlloc.Get<ShaderProgram_RuntimeInfo>(info.shader.programId);

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
    blendInfo.attachmentCount = PassGetNumSubpassAttachments(info.pass, info.subpass);

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

    VkPipelineVertexInputStateCreateInfo dummyVertexInput{};
    dummyVertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    dummyVertexInput.pNext = nullptr;
    dummyVertexInput.flags = 0x0;

    VkPipelineInputAssemblyStateCreateInfo inputInfo{};
    inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputInfo.pNext = nullptr;
    inputInfo.flags = 0x0;
    inputInfo.topology = Vulkan::VkTypes::AsVkPrimitiveType((CoreGraphics::PrimitiveTopology::Code)info.inputAssembly.topo);
    inputInfo.primitiveRestartEnable = info.inputAssembly.primRestart;

    InputAssemblyKey translatedKey;
    translatedKey.topo = Vulkan::VkTypes::AsVkPrimitiveType((CoreGraphics::PrimitiveTopology::Code)info.inputAssembly.topo);
    translatedKey.primRestart = info.inputAssembly.primRestart;

    VkGraphicsPipelineCreateInfo passInfo = PassGetVkFramebufferInfo(info.pass);
    VkRenderPassBeginInfo passBeginInfo = PassGetVkRenderPassBeginInfo(info.pass);

    shaderInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    shaderInfo.pNext = nullptr;
    shaderInfo.flags = 0x0;
    shaderInfo.basePipelineHandle = VK_NULL_HANDLE;
    shaderInfo.basePipelineIndex = -1;
    shaderInfo.pColorBlendState = &blendInfo;
    shaderInfo.pDepthStencilState = &programInfo.depthStencilInfo;
    shaderInfo.pRasterizationState = &programInfo.rasterizerInfo;
    shaderInfo.pMultisampleState = &multisampleInfo;
    shaderInfo.pDynamicState = &programInfo.graphicsDynamicStateInfo;
    shaderInfo.pTessellationState = &programInfo.tessInfo;
    shaderInfo.layout = programInfo.layout;
    shaderInfo.stageCount = programInfo.stageCount;
    shaderInfo.pStages = programInfo.graphicsShaderInfos;
    shaderInfo.pVertexInputState = &dummyVertexInput;
    shaderInfo.pInputAssemblyState = &inputInfo;
    shaderInfo.pViewportState = passInfo.pViewportState;
    shaderInfo.renderPass = passBeginInfo.renderPass;
    shaderInfo.subpass = info.subpass;

    if (!info.ignoreCache)
    {
        CoreGraphics::PipelineId cachedPipeline = Vulkan::PipelineExists(info.pass, info.subpass, info.shader, translatedKey, shaderInfo);
        if (cachedPipeline == CoreGraphics::InvalidPipelineId)
        {
            Ids::Id32 ret = pipelineAllocator.Alloc();
            Pipeline& obj = pipelineAllocator.Get<0>(ret);
            obj.dev = Vulkan::GetCurrentDevice();

            VkResult res = vkCreateGraphicsPipelines(obj.dev, Vulkan::GetPipelineCache(), 1, &shaderInfo, nullptr, &obj.pipeline);
            n_assert(res == VK_SUCCESS);
            obj.layout = programInfo.layout;
            obj.pass = info.pass;

            Vulkan::CachePipeline(info.pass, info.subpass, info.shader, translatedKey, shaderInfo, ret);
            return PipelineId{ ret };
        }
        else
        {
            return cachedPipeline;
        }
    }
    else
    {
        Ids::Id32 ret = pipelineAllocator.Alloc();
        Pipeline& obj = pipelineAllocator.Get<0>(ret);
        obj.dev = Vulkan::GetCurrentDevice();

        VkResult res = vkCreateGraphicsPipelines(obj.dev, Vulkan::GetPipelineCache(), 1, &shaderInfo, nullptr, &obj.pipeline);
        n_assert(res == VK_SUCCESS);
        obj.layout = programInfo.layout;
        obj.pass = info.pass;
        return PipelineId{ ret };
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyGraphicsPipeline(const PipelineId pipeline)
{
    CoreGraphics::InvalidatePipeline(pipeline);
    CoreGraphics::DelayedDeletePipeline(pipeline);
    pipelineAllocator.Dealloc(pipeline.id);
}

//------------------------------------------------------------------------------
/**
*/
const PipelineRayTracingTable
CreateRaytracingPipeline(const Util::Array<CoreGraphics::ShaderProgramId> programs, const CoreGraphics::QueueType queueType)
{
    PipelineRayTracingTable ret;
    VkDevice dev = CoreGraphics::GetCurrentDevice();

    Util::Array<VkPipeline> libraries;
    Util::Array<char> genHandleData, hitHandleData, missHandleData, callableHandleData;
    Util::Array<char*> groupHandleDatas;
    uint rayPayload, hitAttribute;

    enum ShaderGroup
    {
        Gen,
        Hit,
        Miss,
        Intersect,
        Callable
    };
    Util::Array<ShaderGroup> groups;
    Util::Array<VkPipelineShaderStageCreateInfo> shaders;
    Util::Array<VkRayTracingShaderGroupCreateInfoKHR> groupStack;

    uint rayPayloadSize = 0, hitAttributeSize = 0;
    for (IndexT i = 0; i < programs.Size(); i++)
    {
        if (programs[i] == CoreGraphics::InvalidShaderProgramId)
            continue;

        const auto& runtimeInfo = shaderProgramAlloc.Get<ShaderProgram_RuntimeInfo>(programs[i].programId);

        VkRayTracingShaderGroupCreateInfoKHR genMissCallGroup =
        {
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .pNext = nullptr,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
            .generalShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
            .pShaderGroupCaptureReplayHandle = nullptr
        };

        VkRayTracingShaderGroupCreateInfoKHR intersectionGroup =
        {
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .pNext = nullptr,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
            .generalShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
            .pShaderGroupCaptureReplayHandle = nullptr
        };

        VkRayTracingShaderGroupCreateInfoKHR proceduralGroup =
        {
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .pNext = nullptr,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR,
            .generalShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
            .pShaderGroupCaptureReplayHandle = nullptr
        };

        if (runtimeInfo.rg)
        {
            auto group = genMissCallGroup;
            group.generalShader = shaders.Size();
            groupStack.Append(group);
            groups.Append(ShaderGroup::Gen);

            shaders.Append(
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
                .module = runtimeInfo.rg,
                .pName = "main",
                .pSpecializationInfo = nullptr
            });
        }

        if (runtimeInfo.ca)
        {
            auto group = genMissCallGroup;
            group.generalShader = shaders.Size();
            groupStack.Append(group);
            groups.Append(ShaderGroup::Callable);

            shaders.Append(
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_CALLABLE_BIT_KHR,
                .module = runtimeInfo.ca,
                .pName = "main",
                .pSpecializationInfo = nullptr
            });
        }

        if (runtimeInfo.ra)
        {
            auto group = intersectionGroup;
            group.anyHitShader = shaders.Size();
            groupStack.Append(group);
            groups.Append(ShaderGroup::Hit);

            shaders.Append(
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
                .module = runtimeInfo.ra,
                .pName = "main",
                .pSpecializationInfo = nullptr
            });
        }

        if (runtimeInfo.rc)
        {
            if (runtimeInfo.ra)
            {
                groupStack.Back().closestHitShader = shaders.Size();
            }
            else
            {
                auto intGroup = intersectionGroup;
                intGroup.closestHitShader = shaders.Size();
                groupStack.Append(intGroup);
            }
            groups.Append(ShaderGroup::Hit);
            shaders.Append(
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
                .module = runtimeInfo.rc,
                .pName = "main",
                .pSpecializationInfo = nullptr
            });
        }

        if (runtimeInfo.rm)
        {
            auto group = genMissCallGroup;
            group.generalShader = shaders.Size();
            groupStack.Append(group);
            groups.Append(ShaderGroup::Miss);

            shaders.Append(
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
                .module = runtimeInfo.rm,
                .pName = "main",
                .pSpecializationInfo = nullptr
            });
        }

        if (runtimeInfo.ri)
        {
            auto group = proceduralGroup;
            group.intersectionShader = shaders.Size();
            groupStack.Append(group);
            groups.Append(ShaderGroup::Intersect);

            shaders.Append(
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
                .module = runtimeInfo.ri,
                .pName = "main",
                .pSpecializationInfo = nullptr
            });
        }

        VkShaderProgramGetRaytracingVaryingSizes(programs[i], rayPayload, hitAttribute);
        rayPayloadSize = Math::max(rayPayload, rayPayloadSize);
        hitAttributeSize = Math::max(hitAttribute, hitAttributeSize);
    }

    VkRayTracingPipelineInterfaceCreateInfoKHR interfaceInfo =
    {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_INTERFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .maxPipelineRayPayloadSize = rayPayloadSize,
        .maxPipelineRayHitAttributeSize = hitAttributeSize
    };

    static const VkDynamicState dynamicStates[] =
    {
        VK_DYNAMIC_STATE_RAY_TRACING_PIPELINE_STACK_SIZE_KHR
    };

    VkPipelineDynamicStateCreateInfo dynamicState =
    {
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        nullptr,
        0,
        0,
        dynamicStates
    };

    VkRayTracingPipelineCreateInfoKHR createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = (uint)shaders.Size(),
        .pStages = shaders.Begin(),
        .groupCount = (uint)groupStack.Size(),
        .pGroups = groupStack.Begin(),
        .maxPipelineRayRecursionDepth = Math::min(4u, CoreGraphics::MaxRecursionDepth), // TODO: make configurable
        .pLibraryInfo = nullptr,
        .pLibraryInterface = &interfaceInfo,
        .pDynamicState = &dynamicState,
        .layout = VkShaderProgramGetLayout(programs[0]),
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0
    };
    VkPipeline pipeline;
    VkResult pipelineRes = vkCreateRayTracingPipelinesKHR(dev, nullptr, CoreGraphics::GetPipelineCache(), 1, &createInfo, nullptr, &pipeline);
    n_assert(pipelineRes == VK_SUCCESS);

    size_t dataSize = CoreGraphics::ShaderGroupSize * groups.Size();
    char* buf = (char*)Memory::Alloc(Memory::ResourceHeap, dataSize);
    char* parseBuf = buf;
    VkResult groupFetchRes = vkGetRayTracingShaderGroupHandlesKHR(dev, pipeline, 0, groups.Size(), dataSize, buf);
    n_assert(groupFetchRes == VK_SUCCESS);
    groupHandleDatas.Append(buf);

    for (IndexT i = 0; i < groups.Size(); i++)
    {
        const ShaderGroup group = groups[i];
        switch (group)
        {
            case ShaderGroup::Gen:
                genHandleData.AppendArray(parseBuf, CoreGraphics::ShaderGroupSize);
                break;
            case ShaderGroup::Intersect:
            case ShaderGroup::Hit:
                hitHandleData.AppendArray(parseBuf, CoreGraphics::ShaderGroupSize);
                break;
            case ShaderGroup::Miss:
                missHandleData.AppendArray(parseBuf, CoreGraphics::ShaderGroupSize);
                break;
            case ShaderGroup::Callable:
                callableHandleData.AppendArray(parseBuf, CoreGraphics::ShaderGroupSize);
                break;

        }
        parseBuf += CoreGraphics::ShaderGroupSize;
    }

    Memory::Free(Memory::ResourceHeap, buf);

    auto CreateShaderTableBuffer = [queueType](const Util::Array<char>& data, RayDispatchTable::Entry& tableEntry) -> CoreGraphics::BufferId
    {
        tableEntry.baseAddress = 0xFFFFFFFF;
        tableEntry.numEntries = 0;
        if (data.IsEmpty())
            return CoreGraphics::InvalidBufferId;

        // Create buffers for the different shader groups
        CoreGraphics::BufferCreateInfo tableInfo;
        tableInfo.byteSize = data.Size();
        tableInfo.usageFlags = CoreGraphics::BufferUsage::ShaderTable | CoreGraphics::BufferUsage::ShaderAddress;
        tableInfo.queueSupport = queueType == CoreGraphics::ComputeQueueType ? CoreGraphics::BufferQueueSupport::ComputeQueueSupport : CoreGraphics::BufferQueueSupport::GraphicsQueueSupport;
        tableInfo.mode = CoreGraphics::BufferAccessMode::HostLocal;
        tableInfo.data = data.Begin();
        tableInfo.dataSize = data.Size();
        auto ret = CoreGraphics::CreateBuffer(tableInfo);
        tableEntry.baseAddress = CoreGraphics::BufferGetDeviceAddress(ret);
        tableEntry.numEntries = data.Size() / CoreGraphics::ShaderGroupSize;
        tableEntry.entrySize = data.Size();
        return ret;
    };

    // Create buffers for shader tables
    ret.raygenBindingBuffer = CreateShaderTableBuffer(genHandleData, ret.table.genEntry);
    ret.missBindingBuffer = CreateShaderTableBuffer(missHandleData, ret.table.missEntry);
    ret.hitBindingBuffer = CreateShaderTableBuffer(hitHandleData, ret.table.hitEntry);
    ret.callableBindingBuffer = CreateShaderTableBuffer(callableHandleData, ret.table.callableEntry);

    Ids::Id32 id = pipelineAllocator.Alloc();
    pipelineAllocator.Set<Pipeline_Object>(id, { .pipeline = pipeline, .layout = createInfo.layout, .pass = InvalidPassId });

    PipelineId pipeId = id;
    ret.pipeline = pipeId;

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyRaytracingPipeline(const PipelineRayTracingTable& table)
{
    CoreGraphics::DestroyBuffer(table.raygenBindingBuffer);
    CoreGraphics::DestroyBuffer(table.missBindingBuffer);
    CoreGraphics::DestroyBuffer(table.hitBindingBuffer);
    CoreGraphics::DestroyBuffer(table.callableBindingBuffer);
    Pipeline& obj = pipelineAllocator.Get<0>(table.pipeline.id);
    vkDestroyPipeline(Vulkan::GetCurrentDevice(), obj.pipeline, nullptr);
    pipelineAllocator.Dealloc(table.pipeline.id);
}

} // namespace CoreGraphics
