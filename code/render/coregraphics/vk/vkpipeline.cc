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
    shaderInfo.pDynamicState = &programInfo.graphicsDynamicStateInfo;
    shaderInfo.pTessellationState = &programInfo.tessInfo;
    shaderInfo.layout = programInfo.layout;
    shaderInfo.stageCount = programInfo.stageCount;
    shaderInfo.pStages = programInfo.graphicsShaderInfos;

    // Since this is the public facing API, we have to convert the primitive type as it's expected to be in CoreGraphics 
    InputAssemblyKey translatedKey;
    translatedKey.topo = Vulkan::VkTypes::AsVkPrimitiveType((CoreGraphics::PrimitiveTopology::Code)info.inputAssembly.topo);
    translatedKey.primRestart = info.inputAssembly.primRestart;
    VkPipeline pipeline = Vulkan::GetOrCreatePipeline(info.pass, info.subpass, info.shader, translatedKey, shaderInfo);

    obj.pipeline = pipeline;
    obj.layout = programInfo.layout;
    obj.pass = info.pass;
    return PipelineId{ ret, PipelineIdType };
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

//------------------------------------------------------------------------------
/**
*/
const PipelineRayTracingTable
CreateRaytracingPipeline(const Util::Array<CoreGraphics::ShaderProgramId> programs)
{
    PipelineRayTracingTable ret;
    VkDevice dev = CoreGraphics::GetCurrentDevice();

    Util::Array<VkPipeline> libraries;
    Util::Array<char> genHandleData, hitHandleData, missHandleData, callableHandleData;
    Util::Array<char*> groupHandleDatas;
    uint handleSize = CoreGraphics::GetCurrentRaytracingProperties().shaderGroupHandleSize;

    uint rayPayloadSize = 0, hitAttributeSize = 0;
    for (IndexT i = 0; i < programs.Size(); i++)
    {
        VkPipeline pipeline = VkShaderProgramGetRaytracingLibrary(programs[i]);
        libraries.Append(pipeline);

        uint rayPayload, hitAttribute;
        VkShaderProgramGetRaytracingVaryingSizes(programs[i], rayPayload, hitAttribute);
        rayPayloadSize = Math::max(rayPayload, rayPayloadSize);
        hitAttributeSize = Math::max(hitAttribute, hitAttributeSize);

        // Get raytracing bits
        CoreGraphics::RayTracingBits bits = CoreGraphics::ShaderProgramGetRaytracingBits(programs[i]);

        // Count the amount of groups
        
        uint numGroups = 0;
        if (bits.bitField.hasGen)
            numGroups++;
        if (bits.bitField.hasCallable)
            numGroups++;
        if (bits.bitField.hasAnyHit || bits.bitField.hasClosestHit)
            numGroups++;
        if (bits.bitField.hasIntersect)
            numGroups++;
        if (bits.bitField.hasMiss)
            numGroups++;

        size_t dataSize = handleSize * numGroups;
        char* buf = (char*)Memory::Alloc(Memory::ResourceHeap, dataSize);
        char* parseBuf = buf;
        VkResult res = vkGetRayTracingShaderGroupHandlesKHR(dev, pipeline, 0, numGroups, dataSize, buf);
        n_assert(res == VK_SUCCESS);
        groupHandleDatas.Append(buf);

        if (bits.bitField.hasGen)
        {
            genHandleData.AppendArray(parseBuf, handleSize);
            parseBuf += handleSize;
        }
        if (bits.bitField.hasCallable)
        {
            callableHandleData.AppendArray(parseBuf, handleSize);
            parseBuf += handleSize;
        }
        if (bits.bitField.hasAnyHit || bits.bitField.hasClosestHit)
        {
            hitHandleData.AppendArray(parseBuf, handleSize);
            parseBuf += handleSize;
        }
        if (bits.bitField.hasIntersect)
        {
            hitHandleData.AppendArray(parseBuf, handleSize);
            parseBuf += handleSize;
        }
        if (bits.bitField.hasMiss)
        {
            missHandleData.AppendArray(parseBuf, handleSize);
            parseBuf += handleSize;
        }
        Memory::Free(Memory::ResourceHeap, buf);
    }

    auto CreateShaderTableBuffer = [handleSize](const Util::Array<char>& data, RayDispatchTable::Entry& tableEntry) -> CoreGraphics::BufferId
    {
        tableEntry.baseAddress = 0xFFFFFFFF;
        tableEntry.numEntries = 0;
        if (data.IsEmpty())
            return CoreGraphics::InvalidBufferId;

        // Create buffers for the different shader groups
        CoreGraphics::BufferCreateInfo tableInfo;
        tableInfo.byteSize = data.Size();
        tableInfo.usageFlags = CoreGraphics::BufferUsageFlag::ShaderTable | CoreGraphics::BufferUsageFlag::ShaderAddress;
        tableInfo.queueSupport = CoreGraphics::BufferQueueSupport::GraphicsQueueSupport;
        tableInfo.mode = CoreGraphics::BufferAccessMode::HostLocal;
        tableInfo.data = data.Begin();
        tableInfo.dataSize = data.Size();
        auto ret = CoreGraphics::CreateBuffer(tableInfo);
        tableEntry.baseAddress = CoreGraphics::BufferGetDeviceAddress(ret);
        tableEntry.numEntries = data.Size() / handleSize;
        tableEntry.entrySize = data.Size();
        return ret;
    };

    // Create buffers for shader tables
    ret.raygenBindingBuffer = CreateShaderTableBuffer(genHandleData, ret.table.genEntry);
    ret.missBindingBuffer = CreateShaderTableBuffer(missHandleData, ret.table.missEntry);
    ret.hitBindingBuffer = CreateShaderTableBuffer(hitHandleData, ret.table.hitEntry);
    ret.callableBindingBuffer = CreateShaderTableBuffer(callableHandleData, ret.table.callableEntry);

    VkPipelineLibraryCreateInfoKHR libraryInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR,
        .pNext = nullptr,
        .libraryCount = (uint)libraries.Size(),
        .pLibraries = libraries.Begin()
    };
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
        .stageCount = 0,
        .pStages = nullptr,
        .groupCount = 0,
        .pGroups = nullptr,
        .maxPipelineRayRecursionDepth = Math::min(4u, Vulkan::GetCurrentRaytracingProperties().maxRayRecursionDepth), // TODO: make configurable
        .pLibraryInfo = &libraryInfo,
        .pLibraryInterface = &interfaceInfo,
        .pDynamicState = &dynamicState,
        .layout = VkShaderProgramGetLayout(programs[0]),
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0
    };
    VkPipeline pipeline;
    VkResult res = vkCreateRayTracingPipelinesKHR(dev, nullptr, CoreGraphics::GetPipelineCache(), 1, &createInfo, nullptr, &pipeline);
    n_assert(res == VK_SUCCESS);

    Ids::Id32 id = pipelineAllocator.Alloc();
    pipelineAllocator.Set<Pipeline_Object>(id, { .pipeline = pipeline, .layout = createInfo.layout, .pass = InvalidPassId });

    PipelineId pipeId;
    pipeId.id24 = id;
    pipeId.id8 = PipelineIdType;
    ret.pipeline = pipeId;

    return ret;
}

} // namespace CoreGraphics
