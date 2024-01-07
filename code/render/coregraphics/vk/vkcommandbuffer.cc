//------------------------------------------------------------------------------
//  vkcommandbuffer.cc
//  (C)2017-2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/config.h"
#include "coregraphics/resourcetable.h"
#include "coregraphics/pipeline.h"

#include "vkcommandbuffer.h"
#include "vkgraphicsdevice.h"
#include "vkbuffer.h"
#include "vktexture.h"
#include "vkvertexlayout.h"
#include "vkshader.h"
#include "vktypes.h"
#include "vkshaderprogram.h"
#include "vkresourcetable.h"
#include "vkevent.h"
#include "vkpass.h"
#include "vkpipeline.h"
#include "vkaccelerationstructure.h"

#include "graphics/globalconstants.h"

namespace Vulkan
{

VkCommandBufferAllocator commandBuffers;
VkCommandBufferPoolAllocator commandBufferPools(0x00FFFFFF);
Threading::CriticalSection commandBufferCritSect;

//------------------------------------------------------------------------------
/**
*/
const VkCommandPool 
CmdBufferPoolGetVk(const CoreGraphics::CmdBufferPoolId id)
{
    return commandBufferPools.Get<CommandBufferPool_VkCommandPool>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const VkDevice 
CmdBufferPoolGetVkDevice(const CoreGraphics::CmdBufferPoolId id)
{
    return commandBufferPools.Get<CommandBufferPool_VkDevice>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const VkCommandBuffer
CmdBufferGetVk(const CoreGraphics::CmdBufferId id)
{
#if NEBULA_DEBUG
    n_assert(id.id8 == CoreGraphics::IdType::CommandBufferIdType);
#endif
    if (id == CoreGraphics::InvalidCmdBufferId) return VK_NULL_HANDLE;
    else                                        return commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const VkCommandPool
CmdBufferGetVkPool(const CoreGraphics::CmdBufferId id)
{
    return commandBuffers.Get<CmdBuffer_VkCommandPool>(id.id24);;
}

//------------------------------------------------------------------------------
/**
*/
const VkDevice
CmdBufferGetVkDevice(const CoreGraphics::CmdBufferId id)
{
    return commandBuffers.Get<CmdBuffer_VkDevice>(id.id24);;
}

} // Vulkan

namespace CoreGraphics
{

using namespace Vulkan;

_IMPL_ACQUIRE_RELEASE(CmdBufferId, commandBuffers);

//------------------------------------------------------------------------------
/**
*/
const CmdBufferPoolId 
CreateCmdBufferPool(const CmdBufferPoolCreateInfo& info)
{
    Ids::Id32 id = commandBufferPools.Alloc();

    VkCommandPoolCreateFlags flags = 0;
    flags |= info.resetable ? VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : 0;
    flags |= info.shortlived ? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT : 0;

    uint32_t queueFamily = CoreGraphics::GetQueueIndex(info.queue);

    VkCommandPoolCreateInfo cmdPoolInfo =
    {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        flags,
        queueFamily
    };
    VkDevice dev = Vulkan::GetCurrentDevice();
    VkResult res = vkCreateCommandPool(dev, &cmdPoolInfo, nullptr, &commandBufferPools.Get<CommandBufferPool_VkCommandPool>(id));
    commandBufferPools.Set<CommandBufferPool_VkDevice>(id, dev);
    n_assert(res == VK_SUCCESS);

    CmdBufferPoolId ret;
    ret.id24 = id;
    ret.id8 = CommandBufferPoolIdType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
DestroyCmdBufferPool(const CmdBufferPoolId pool)
{
    vkDestroyCommandPool(commandBufferPools.Get<CommandBufferPool_VkDevice>(pool.id24), commandBufferPools.Get<CommandBufferPool_VkCommandPool>(pool.id24), nullptr);
}

//------------------------------------------------------------------------------
/**
*/
const CmdBufferId
CreateCmdBuffer(const CmdBufferCreateInfo& info)
{
    n_assert(info.pool != CoreGraphics::InvalidCmdBufferPoolId);
    VkCommandPool pool = CmdBufferPoolGetVk(info.pool);
    VkCommandBufferAllocateInfo vkInfo =
    {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        pool,
        info.subBuffer ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1
    };
    VkDevice dev = CmdBufferPoolGetVkDevice(info.pool);
    Ids::Id32 id = commandBuffers.Alloc();
    VkResult res = vkAllocateCommandBuffers(dev, &vkInfo, &commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id));
    n_assert(res == VK_SUCCESS);
    commandBuffers.Set<CmdBuffer_VkCommandPool>(id, pool);
    commandBuffers.Set<CmdBuffer_VkDevice>(id, dev);
    commandBuffers.Set<CmdBuffer_Usage>(id, info.usage);

    QueryBundle& queryBundles = commandBuffers.Get<CmdBuffer_Query>(id);

    uint bits = (uint)info.queryTypes;
    for (IndexT i = 0; i < CoreGraphics::CmdBufferQueryBits::NumBits; i++)
    {
        uint numQueries = 0;
        queryBundles.enabled[i] = false;
        UNUSED(CoreGraphics::QueryType) type;

        // If bit is not set, continue
        if ((bits & 1) == 0)
        {
            bits >>= 1;
            continue;
        }

        switch (1 << i)
        {
            case CoreGraphics::CmdBufferQueryBits::Occlusion:
                numQueries = 1024; // With occlusion culling, we might have thousands of objects
                type = CoreGraphics::QueryType::OcclusionQueryType;
                break;
            case CoreGraphics::CmdBufferQueryBits::Timestamps:
                numQueries = 64;   // Timestamps will be quite a lot fewer
                type = CoreGraphics::QueryType::TimestampsQueryType;
                break;
            case CoreGraphics::CmdBufferQueryBits::Statistics:
                numQueries = 64;    // Statistics will be just a handful over the whole command buffer
                type = CoreGraphics::QueryType::StatisticsQueryType;
                break;
        }

        queryBundles.states[i].chunkSize = numQueries;
        queryBundles.enabled[i] = true;
    }

    VkPipelineBundle& pipelineBundle = commandBuffers.Get<CmdBuffer_VkPipelineBundle>(id);
    pipelineBundle.blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipelineBundle.blendInfo.pNext = nullptr;
    pipelineBundle.blendInfo.flags = 0;

    pipelineBundle.inputAssembly.topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineBundle.inputAssembly.primRestart = false;
    pipelineBundle.pipelineInfo.pColorBlendState = &pipelineBundle.blendInfo;
    pipelineBundle.pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineBundle.pipelineInfo.basePipelineIndex = 0;

    pipelineBundle.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipelineBundle.multisampleInfo.pNext = nullptr;
    pipelineBundle.multisampleInfo.flags = 0;
    pipelineBundle.multisampleInfo.pSampleMask = nullptr;

    pipelineBundle.computeLayout = VK_NULL_HANDLE;
    pipelineBundle.graphicsLayout = VK_NULL_HANDLE;

    ViewportBundle& viewports = commandBuffers.Get<CmdBuffer_PendingViewports>(id);
    viewports.viewports.Resize(8);
    ScissorBundle& scissors = commandBuffers.Get<CmdBuffer_PendingScissors>(id);
    scissors.scissors.Resize(8);

    CmdBufferId ret;
    ret.id24 = id;
    ret.id8 = CommandBufferIdType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyCmdBuffer(const CmdBufferId id)
{
#if _DEBUG
    n_assert(id.id8 == CommandBufferIdType);
#endif

    __Lock(commandBuffers, id.id24);

#if NEBULA_ENABLE_PROFILING
    QueryBundle& queryBundles = commandBuffers.Get<CmdBuffer_Query>(id.id24);
    queryBundles.chunks[0].Clear();
    queryBundles.chunks[1].Clear();
    queryBundles.chunks[2].Clear();

    CmdBufferMarkerBundle& markers = commandBuffers.Get<CmdBuffer_ProfilingMarkers>(id.id24);
    markers.markerStack.Clear();
    markers.finishedMarkers.Clear();
#endif

    CoreGraphics::DelayedDeleteCommandBuffer(id);
    commandBuffers.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdBeginRecord(const CmdBufferId id, const CmdBufferBeginInfo& info)
{
#if _DEBUG
    n_assert(id.id8 == CommandBufferIdType);
#endif
    VkCommandBufferUsageFlags flags = 0;
    flags |= info.submitOnce ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
    flags |= info.submitDuringPass ? VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : 0;
    flags |= info.resubmittable ? VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT : 0;
    VkCommandBufferBeginInfo begin =
    {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        flags,
        nullptr     // fixme, this part can optimize if used properly!
    };
    VkResult res = vkBeginCommandBuffer(commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24), &begin);
    n_assert(res == VK_SUCCESS);

    // Also write first timestamp
    QueryBundle& queryBundle = commandBuffers.Get<CmdBuffer_Query>(id.id24);

    queryBundle.states[CoreGraphics::TimestampsQueryType].currentChunk = 0;
    VkQueryPool pool = Vulkan::GetQueryPool(CoreGraphics::TimestampsQueryType);
    if (queryBundle.enabled[CoreGraphics::TimestampsQueryType])
    {
        QueryBundle::QueryChunk& chunk = queryBundle.GetChunk(CoreGraphics::TimestampsQueryType);
        vkCmdWriteTimestamp(commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, pool, chunk.offset + chunk.queryCount++);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CmdEndRecord(const CmdBufferId id)
{
#if _DEBUG
    n_assert(id.id8 == CommandBufferIdType);
#endif

    VkResult res = vkEndCommandBuffer(commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24));
    n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdReset(const CmdBufferId id, const CmdBufferClearInfo& info)
{
#if _DEBUG
    n_assert(id.id8 == CommandBufferIdType);
#endif
    VkCommandBufferResetFlags flags = 0;
    flags |= info.allowRelease ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0;
    VkResult res = vkResetCommandBuffer(commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24), flags);
    n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSetVertexBuffer(const CmdBufferId id, IndexT streamIndex, const CoreGraphics::BufferId& buffer, SizeT bufferOffset)
{
#if _DEBUG
    CoreGraphics::QueueType usage = commandBuffers.Get<CmdBuffer_Usage>(id.id24);
    n_assert(usage == QueueType::GraphicsQueueType);
#endif
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    VkBuffer buf = Vulkan::BufferGetVk(buffer);
    VkDeviceSize offset = bufferOffset;
    vkCmdBindVertexBuffers(cmdBuf, streamIndex, 1, &buf, &offset);
}   

//------------------------------------------------------------------------------
/**
*/
void
CmdSetVertexLayout(const CmdBufferId id, const CoreGraphics::VertexLayoutId& vl)
{
#if _DEBUG
    CoreGraphics::QueueType usage = commandBuffers.Get<CmdBuffer_Usage>(id.id24);
    n_assert(usage == QueueType::GraphicsQueueType);
#endif
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    //VkPipelineBundle& pipelineBundle = commandBuffers.Get<CmdBuffer_VkPipelineBundle>(id.id24);
    const VertexLayoutVkBindInfo& bindInfo = VertexLayoutGetVkBindInfo(vl);

    vkCmdSetVertexInputEXT(cmdBuf, bindInfo.binds.Size(), bindInfo.binds.Begin(), bindInfo.attrs.Size(), bindInfo.attrs.Begin());
    //pipelineBundle.pipelineInfo.pVertexInputState = info;
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSetIndexBuffer(const CmdBufferId id, const IndexType::Code indexType, const CoreGraphics::BufferId& buffer, SizeT bufferOffset)
{
#if _DEBUG
    CoreGraphics::QueueType usage = commandBuffers.Get<CmdBuffer_Usage>(id.id24);
    n_assert(usage == QueueType::GraphicsQueueType);
#endif
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    VkBuffer buf = Vulkan::BufferGetVk(buffer);
    VkDeviceSize offset = bufferOffset;
    VkIndexType vkIdxType = indexType == IndexType::Index16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
    vkCmdBindIndexBuffer(cmdBuf, buf, offset, vkIdxType);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSetIndexBuffer(const CmdBufferId id, const CoreGraphics::BufferId& buffer, CoreGraphics::IndexType::Code indexSize, SizeT bufferOffset)
{
#if _DEBUG
    CoreGraphics::QueueType usage = commandBuffers.Get<CmdBuffer_Usage>(id.id24);
    n_assert(usage == QueueType::GraphicsQueueType);
#endif
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    VkBuffer buf = Vulkan::BufferGetVk(buffer);
    VkDeviceSize offset = bufferOffset;
    VkIndexType vkIdxType = indexSize == IndexType::Index16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
    vkCmdBindIndexBuffer(cmdBuf, buf, offset, vkIdxType);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSetPrimitiveTopology(const CmdBufferId id, const CoreGraphics::PrimitiveTopology::Code topo)
{
#if _DEBUG
    CoreGraphics::QueueType usage = commandBuffers.Get<CmdBuffer_Usage>(id.id24);
    n_assert(usage == QueueType::GraphicsQueueType);
#endif

    VkPipelineBundle& pipelineBundle = commandBuffers.Get<CmdBuffer_VkPipelineBundle>(id.id24);
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    VkPrimitiveTopology comp = VkTypes::AsVkPrimitiveType(topo);
    pipelineBundle.inputAssembly.topo = comp;
    pipelineBundle.inputAssembly.primRestart = false;
    vkCmdSetPrimitiveTopology(cmdBuf, comp);
    vkCmdSetPrimitiveRestartEnable(cmdBuf, false);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSetShaderProgram(const CmdBufferId id, const CoreGraphics::ShaderProgramId pro, bool bindGlobals)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    VkPipelineBundle& pipelineBundle = commandBuffers.Get<CmdBuffer_VkPipelineBundle>(id.id24);
    VkShaderProgramRuntimeInfo& info = shaderProgramAlloc.Get<ShaderProgram_RuntimeInfo>(pro.programId);

    IndexT buffer = CoreGraphics::GetBufferedFrameIndex();
    pipelineBundle.program = pro;
    if (info.type == ShaderPipeline::ComputePipeline)
    {
        bool pipelineChange = pipelineBundle.computeLayout != info.layout;
        pipelineBundle.computeLayout = info.layout;
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, info.pipeline);
        if (bindGlobals && pipelineChange)
        {
            CoreGraphics::CmdSetResourceTable(id, Graphics::GetTickResourceTable(buffer), NEBULA_TICK_GROUP, CoreGraphics::ShaderPipeline::ComputePipeline, nullptr);
            CoreGraphics::CmdSetResourceTable(id, Graphics::GetFrameResourceTable(buffer), NEBULA_FRAME_GROUP, CoreGraphics::ShaderPipeline::ComputePipeline, nullptr);
        }
    }
    else if (info.type == ShaderPipeline::RayTracingPipeline)
    {
        bool pipelineChange = pipelineBundle.raytracingLayout != info.layout;
        pipelineBundle.raytracingLayout = info.layout;
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, info.pipeline);
        if (bindGlobals && pipelineChange)
        {
            CoreGraphics::CmdSetResourceTable(id, Graphics::GetTickResourceTable(buffer), NEBULA_TICK_GROUP, CoreGraphics::ShaderPipeline::RayTracingPipeline, nullptr);
            CoreGraphics::CmdSetResourceTable(id, Graphics::GetFrameResourceTable(buffer), NEBULA_FRAME_GROUP, CoreGraphics::ShaderPipeline::RayTracingPipeline, nullptr);
        }
    }
    else
    {
        CmdPipelineBuildBits& bits = commandBuffers.Get<CmdBuffer_PipelineBuildBits>(id.id24);
        bits |= CoreGraphics::CmdPipelineBuildBits::ShaderInfoSet;
        bits &= ~CoreGraphics::CmdPipelineBuildBits::PipelineBuilt;

        // Setup blend info
        pipelineBundle.blendInfo.attachmentCount = info.colorBlendInfo.attachmentCount;
        pipelineBundle.blendInfo.flags = info.colorBlendInfo.flags;
        pipelineBundle.blendInfo.logicOp = info.colorBlendInfo.logicOp;
        pipelineBundle.blendInfo.logicOpEnable = info.colorBlendInfo.logicOpEnable;
        pipelineBundle.blendInfo.pAttachments = info.colorBlendAttachments;
        memcpy(pipelineBundle.blendInfo.blendConstants, info.colorBlendInfo.blendConstants, sizeof(info.colorBlendInfo.blendConstants));

        pipelineBundle.multisampleInfo.alphaToCoverageEnable = info.multisampleInfo.alphaToCoverageEnable;
        pipelineBundle.multisampleInfo.alphaToOneEnable = info.multisampleInfo.alphaToOneEnable;
        pipelineBundle.multisampleInfo.minSampleShading = info.multisampleInfo.minSampleShading;
        pipelineBundle.multisampleInfo.sampleShadingEnable = info.multisampleInfo.sampleShadingEnable;
        pipelineBundle.multisampleInfo.pSampleMask = info.multisampleInfo.pSampleMask;

        // Setup states (excluding pass)
        pipelineBundle.pipelineInfo.pDepthStencilState = &info.depthStencilInfo;
        pipelineBundle.pipelineInfo.pRasterizationState = &info.rasterizerInfo;
        pipelineBundle.pipelineInfo.pMultisampleState = &pipelineBundle.multisampleInfo;
        pipelineBundle.pipelineInfo.pDynamicState = &info.graphicsDynamicStateInfo;
        pipelineBundle.pipelineInfo.pTessellationState = &info.tessInfo;

        // Setup shaders
        pipelineBundle.pipelineInfo.layout = info.layout;
        pipelineBundle.pipelineInfo.stageCount = info.stageCount;
        pipelineBundle.pipelineInfo.pStages = info.graphicsShaderInfos;

        bool pipelineChange = pipelineBundle.graphicsLayout != info.layout;
        pipelineBundle.graphicsLayout = info.layout;
        if (bindGlobals && pipelineChange)
        {
            CoreGraphics::CmdSetResourceTable(id, Graphics::GetTickResourceTable(buffer), NEBULA_TICK_GROUP, CoreGraphics::ShaderPipeline::GraphicsPipeline, nullptr);
            CoreGraphics::CmdSetResourceTable(id, Graphics::GetFrameResourceTable(buffer), NEBULA_FRAME_GROUP, CoreGraphics::ShaderPipeline::GraphicsPipeline, nullptr);
            CoreGraphics::CmdSetResourceTable(id, PassGetResourceTable(pipelineBundle.pass), NEBULA_PASS_GROUP, CoreGraphics::ShaderPipeline::GraphicsPipeline, nullptr);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSetResourceTable(const CmdBufferId id, const CoreGraphics::ResourceTableId table, const IndexT slot, CoreGraphics::ShaderPipeline pipeline, const Util::FixedArray<uint>& offsets)
{
    if (offsets.IsEmpty())
        CmdSetResourceTable(id, table, slot, pipeline, 0, nullptr);
    else
        CmdSetResourceTable(id, table, slot, pipeline, offsets.Size(), offsets.Begin());
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSetResourceTable(const CmdBufferId id, const CoreGraphics::ResourceTableId table, const IndexT slot, CoreGraphics::ShaderPipeline pipeline, uint32 numOffsets, uint32* offsets)
{
    const VkPipelineBundle& pipelineBundle = commandBuffers.Get<CmdBuffer_VkPipelineBundle>(id.id24);
    VkDescriptorSet set = Vulkan::ResourceTableGetVkDescriptorSet(table);
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    switch (pipeline)
    {
        case ShaderPipeline::GraphicsPipeline:
            vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineBundle.graphicsLayout, slot, 1, &set, numOffsets, offsets);
            break;
        case ShaderPipeline::ComputePipeline:
            vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineBundle.computeLayout, slot, 1, &set, numOffsets, offsets);
            break;
        case ShaderPipeline::RayTracingPipeline:
            vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineBundle.raytracingLayout, slot, 1, &set, numOffsets, offsets);
            break;
        default: n_error("unhandled enum"); break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CmdPushConstants(const CmdBufferId id, ShaderPipeline pipeline, uint offset, uint size, const void* data)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    const VkPipelineBundle& pipelineBundle = commandBuffers.Get<CmdBuffer_VkPipelineBundle>(id.id24);
    switch (pipeline)
    {
        case ShaderPipeline::GraphicsPipeline:
            vkCmdPushConstants(cmdBuf, pipelineBundle.graphicsLayout, VK_SHADER_STAGE_ALL_GRAPHICS, offset, size, data);
            break;
        case ShaderPipeline::ComputePipeline:
            vkCmdPushConstants(cmdBuf, pipelineBundle.computeLayout, VK_SHADER_STAGE_COMPUTE_BIT, offset, size, data);
            break;
        case ShaderPipeline::RayTracingPipeline:
            vkCmdPushConstants(cmdBuf, pipelineBundle.raytracingLayout, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR | VK_SHADER_STAGE_CALLABLE_BIT_KHR, offset, size, data);
            break;
        default: n_error("unhandled enum"); break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CmdPushGraphicsConstants(const CmdBufferId id, uint offset, uint size, const void* data)
{
    const VkPipelineBundle& pipelineBundle = commandBuffers.Get<CmdBuffer_VkPipelineBundle>(id.id24);
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    vkCmdPushConstants(cmdBuf, pipelineBundle.graphicsLayout, VK_SHADER_STAGE_ALL_GRAPHICS, offset, size, data);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdPushComputeConstants(const CmdBufferId id, uint offset, uint size, const void* data)
{
    const VkPipelineBundle& pipelineBundle = commandBuffers.Get<CmdBuffer_VkPipelineBundle>(id.id24);
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    vkCmdPushConstants(cmdBuf, pipelineBundle.computeLayout, VK_SHADER_STAGE_COMPUTE_BIT, offset, size, data);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSetGraphicsPipeline(const CmdBufferId id)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);

    CmdPipelineBuildBits& bits = commandBuffers.Get<CmdBuffer_PipelineBuildBits>(id.id24);
    n_assert((bits & CmdPipelineBuildBits::AllInfoSet) != 0);
    if (!AllBits(bits, CmdPipelineBuildBits::PipelineBuilt))
    {
        const VkPipelineBundle& pipelineBundle = commandBuffers.Get<CmdBuffer_VkPipelineBundle>(id.id24);
        VkPipeline pipeline = CoreGraphics::GetOrCreatePipeline(pipelineBundle.pass, pipelineBundle.pipelineInfo.subpass, pipelineBundle.program, pipelineBundle.inputAssembly, pipelineBundle.pipelineInfo);
        bits |= CmdPipelineBuildBits::PipelineBuilt;
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }

    // Set viewport and scissors since Vulkan requires them to be set after the pipeline
    ViewportBundle& viewports = commandBuffers.Get<CmdBuffer_PendingViewports>(id.id24);
    if (viewports.numPending > 0)
    {
        vkCmdSetViewport(cmdBuf, 0, viewports.numPending, viewports.viewports.Begin());
        viewports.numPending = 0;
    }
    ScissorBundle& rects = commandBuffers.Get<CmdBuffer_PendingScissors>(id.id24);
    if (rects.numPending > 0)
    {
        vkCmdSetScissor(cmdBuf, 0, rects.numPending, rects.scissors.Begin());
        rects.numPending = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSetGraphicsPipeline(const CmdBufferId buf, const PipelineId pipeline)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(buf.id24);
    Pipeline& pipelineObj = pipelineAllocator.Get<Pipeline_Object>(pipeline.id24);
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineObj.pipeline);
    VkPipelineBundle& pipelineBundle = commandBuffers.Get<CmdBuffer_VkPipelineBundle>(buf.id24);

    bool pipelineChange = pipelineBundle.graphicsLayout != pipelineObj.layout;
    pipelineBundle.graphicsLayout = pipelineObj.layout;
    if (pipelineChange)
    {
        IndexT buffer = CoreGraphics::GetBufferedFrameIndex();
        CoreGraphics::CmdSetResourceTable(buf, Graphics::GetTickResourceTable(buffer), NEBULA_TICK_GROUP, CoreGraphics::ShaderPipeline::GraphicsPipeline, nullptr);
        CoreGraphics::CmdSetResourceTable(buf, Graphics::GetFrameResourceTable(buffer), NEBULA_FRAME_GROUP, CoreGraphics::ShaderPipeline::GraphicsPipeline, nullptr);
        CoreGraphics::CmdSetResourceTable(buf, PassGetResourceTable(pipelineObj.pass), NEBULA_PASS_GROUP, CoreGraphics::ShaderPipeline::GraphicsPipeline, nullptr);
    }

    // Set viewport and scissors since Vulkan requires them to be set after the pipeline
    ViewportBundle& viewports = commandBuffers.Get<CmdBuffer_PendingViewports>(buf.id24);
    if (viewports.numPending > 0)
    {
        vkCmdSetViewport(cmdBuf, 0, viewports.numPending, viewports.viewports.Begin());
        viewports.numPending = 0;
    }
    ScissorBundle& rects = commandBuffers.Get<CmdBuffer_PendingScissors>(buf.id24);
    if (rects.numPending > 0)
    {
        vkCmdSetScissor(cmdBuf, 0, rects.numPending, rects.scissors.Begin());
        rects.numPending = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSetRayTracingPipeline(const CmdBufferId buf, const PipelineId pipeline)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(buf.id24);
    VkPipelineBundle& pipelineBundle = commandBuffers.Get<CmdBuffer_VkPipelineBundle>(buf.id24);
    Pipeline& pipelineObj = pipelineAllocator.Get<Pipeline_Object>(pipeline.id24);
    pipelineBundle.raytracingLayout = pipelineObj.layout;

    bool pipelineChange = pipelineBundle.graphicsLayout != pipelineObj.layout;
    pipelineBundle.graphicsLayout = pipelineObj.layout;
    if (pipelineChange)
    {
        IndexT buffer = CoreGraphics::GetBufferedFrameIndex();
        CoreGraphics::CmdSetResourceTable(buf, Graphics::GetTickResourceTable(buffer), NEBULA_TICK_GROUP, CoreGraphics::ShaderPipeline::RayTracingPipeline, nullptr);
        CoreGraphics::CmdSetResourceTable(buf, Graphics::GetFrameResourceTable(buffer), NEBULA_FRAME_GROUP, CoreGraphics::ShaderPipeline::RayTracingPipeline, nullptr);
    }

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineObj.pipeline);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdBarrier(
            const CmdBufferId id,
            CoreGraphics::PipelineStage fromStage,
            CoreGraphics::PipelineStage toStage,
            CoreGraphics::BarrierDomain domain,
            const Util::FixedArray<TextureBarrierInfo>& textures,
            const Util::FixedArray<BufferBarrierInfo>& buffers,
            const IndexT fromQueue,
            const IndexT toQueue,
            const char* name)
{
    VkBarrierInfo barrier;
    barrier.name = name;
    barrier.srcFlags = VkTypes::AsVkPipelineStage(fromStage);
    barrier.dstFlags = VkTypes::AsVkPipelineStage(toStage);
    barrier.dep = 0;
    //barrier.dep = domain == CoreGraphics::BarrierDomain::Pass ? VK_DEPENDENCY_BY_REGION_BIT : 0;
    barrier.numBufferBarriers = buffers.Size();
    for (uint32_t i = 0; i < barrier.numBufferBarriers; i++)
    {
        VkBufferMemoryBarrier& vkBar = barrier.bufferBarriers[i];
        BufferBarrierInfo& nebBar = buffers[i];

        vkBar.srcAccessMask = VkTypes::AsVkAccessFlags(fromStage);
        vkBar.dstAccessMask = VkTypes::AsVkAccessFlags(toStage);
        vkBar.buffer = CoreGraphics::BufferGetVk(nebBar.buf);
        vkBar.offset = nebBar.subres.offset;
        vkBar.size = (nebBar.subres.size == -1) ? VK_WHOLE_SIZE : nebBar.subres.size;
        vkBar.srcQueueFamilyIndex = fromQueue == InvalidIndex ? VK_QUEUE_FAMILY_IGNORED : fromQueue;
        vkBar.dstQueueFamilyIndex = toQueue == InvalidIndex ? VK_QUEUE_FAMILY_IGNORED : toQueue;

        vkBar.pNext = nullptr;
        vkBar.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    }
    barrier.numImageBarriers = textures.Size();
    IndexT i, j = 0;
    for (i = 0; i < textures.Size(); i++, j++)
    {
        VkImageMemoryBarrier& vkBar = barrier.imageBarriers[j];
        TextureBarrierInfo& nebBar = textures[i];

        vkBar.srcAccessMask = VkTypes::AsVkAccessFlags(fromStage);
        vkBar.dstAccessMask = VkTypes::AsVkAccessFlags(toStage);

        const TextureSubresourceInfo& subres = nebBar.subres;
        bool isDepth = AnyBits(subres.bits, CoreGraphics::ImageBits::DepthBits);
        vkBar.subresourceRange.aspectMask = VkTypes::AsVkImageAspectFlags(subres.bits);
        vkBar.subresourceRange.baseMipLevel = subres.mip;
        vkBar.subresourceRange.levelCount = subres.mipCount;
        vkBar.subresourceRange.baseArrayLayer = subres.layer;
        vkBar.subresourceRange.layerCount = subres.layerCount;
        vkBar.image = CoreGraphics::TextureGetVkImage(nebBar.tex);
        vkBar.srcQueueFamilyIndex = fromQueue == InvalidIndex ? VK_QUEUE_FAMILY_IGNORED : fromQueue;
        vkBar.dstQueueFamilyIndex = toQueue == InvalidIndex ? VK_QUEUE_FAMILY_IGNORED : toQueue;
        vkBar.oldLayout = VkTypes::AsVkImageLayout(fromStage, isDepth);
        vkBar.newLayout = VkTypes::AsVkImageLayout(toStage, isDepth);

        vkBar.pNext = nullptr;
        vkBar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    }

    // If we have no other barriers, insert a memory barrier
    barrier.numMemoryBarriers = 0;
    if (barrier.numBufferBarriers == 0 && barrier.numImageBarriers == 0)
    {
        barrier.numMemoryBarriers = 1;
        barrier.memoryBarriers[0] =
        {
            VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            nullptr,
            VkTypes::AsVkAccessFlags(fromStage),
            VkTypes::AsVkAccessFlags(toStage)
        };
    }
   
    // Insert barrier
    vkCmdPipelineBarrier(CmdBufferGetVk(id),
        barrier.srcFlags,
        barrier.dstFlags,
        barrier.dep,
        barrier.numMemoryBarriers, barrier.memoryBarriers,
        barrier.numBufferBarriers, barrier.bufferBarriers,
        barrier.numImageBarriers, barrier.imageBarriers);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdHandover(
    const CmdBufferId from
    , const CmdBufferId to
    , CoreGraphics::PipelineStage fromStage
    , CoreGraphics::PipelineStage toStage
    , const Util::FixedArray<TextureBarrierInfo>& textures
    , const Util::FixedArray<BufferBarrierInfo>& buffers
    , const IndexT fromQueue
    , const IndexT toQueue
    , const char* name
)
{
    VkBarrierInfo barrier;
    barrier.name = name;
    barrier.srcFlags = VkTypes::AsVkPipelineStage(fromStage);
    barrier.dstFlags = VkTypes::AsVkPipelineStage(toStage);
    barrier.dep = 0;
    barrier.numBufferBarriers = buffers.Size();
    for (uint32_t i = 0; i < barrier.numBufferBarriers; i++)
    {
        VkBufferMemoryBarrier& vkBar = barrier.bufferBarriers[i];
        BufferBarrierInfo& nebBar = buffers[i];

        vkBar.srcAccessMask = VkTypes::AsVkAccessFlags(fromStage);
        vkBar.dstAccessMask = VkTypes::AsVkAccessFlags(fromStage);
        vkBar.buffer = CoreGraphics::BufferGetVk(nebBar.buf);
        vkBar.offset = nebBar.subres.offset;
        vkBar.size = (nebBar.subres.size == -1) ? VK_WHOLE_SIZE : nebBar.subres.size;
        vkBar.srcQueueFamilyIndex = fromQueue == InvalidIndex ? VK_QUEUE_FAMILY_IGNORED : fromQueue;
        vkBar.dstQueueFamilyIndex = toQueue == InvalidIndex ? VK_QUEUE_FAMILY_IGNORED : toQueue;

        vkBar.pNext = nullptr;
        vkBar.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    }
    barrier.numImageBarriers = textures.Size();
    IndexT i, j = 0;
    for (i = 0; i < textures.Size(); i++, j++)
    {
        VkImageMemoryBarrier& vkBar = barrier.imageBarriers[j];
        TextureBarrierInfo& nebBar = textures[i];

        vkBar.srcAccessMask = VkTypes::AsVkAccessFlags(fromStage);
        vkBar.dstAccessMask = VkTypes::AsVkAccessFlags(fromStage);

        const TextureSubresourceInfo& subres = nebBar.subres;
        bool isDepth = AnyBits(subres.bits, CoreGraphics::ImageBits::DepthBits);
        vkBar.subresourceRange.aspectMask = VkTypes::AsVkImageAspectFlags(subres.bits);
        vkBar.subresourceRange.baseMipLevel = subres.mip;
        vkBar.subresourceRange.levelCount = subres.mipCount;
        vkBar.subresourceRange.baseArrayLayer = subres.layer;
        vkBar.subresourceRange.layerCount = subres.layerCount;
        vkBar.image = CoreGraphics::TextureGetVkImage(nebBar.tex);
        vkBar.srcQueueFamilyIndex = fromQueue == InvalidIndex ? VK_QUEUE_FAMILY_IGNORED : fromQueue;
        vkBar.dstQueueFamilyIndex = toQueue == InvalidIndex ? VK_QUEUE_FAMILY_IGNORED : toQueue;
        vkBar.oldLayout = VkTypes::AsVkImageLayout(fromStage, isDepth);
        vkBar.newLayout = VkTypes::AsVkImageLayout(fromStage, isDepth);

        vkBar.pNext = nullptr;
        vkBar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    }

    // If we have no other barriers, insert a memory barrier
    barrier.numMemoryBarriers = 0;
    if (barrier.numBufferBarriers == 0 && barrier.numImageBarriers == 0)
    {
        barrier.numMemoryBarriers = 1;
        barrier.memoryBarriers[0] =
        {
            VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            nullptr,
            VkTypes::AsVkAccessFlags(fromStage),
            VkTypes::AsVkAccessFlags(toStage)
        };
    }

    VkCommandBuffer fromCmds = CmdBufferGetVk(from);
    VkCommandBuffer toCmds = CmdBufferGetVk(to);
    vkCmdPipelineBarrier(
        fromCmds
        , barrier.srcFlags
        , barrier.srcFlags
        , barrier.dep
        , barrier.numMemoryBarriers, barrier.memoryBarriers
        , barrier.numBufferBarriers, barrier.bufferBarriers
        , barrier.numImageBarriers, barrier.imageBarriers);

    vkCmdPipelineBarrier(
        toCmds
        , barrier.srcFlags
        , barrier.srcFlags
        , barrier.dep
        , barrier.numMemoryBarriers, barrier.memoryBarriers
        , barrier.numBufferBarriers, barrier.bufferBarriers
        , barrier.numImageBarriers, barrier.imageBarriers);

    // Last barrier does not do a handover but simply transitions the resource to the destination layout
    for (IndexT i = 0; i < barrier.numBufferBarriers; i++)
    {
        barrier.bufferBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.bufferBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.bufferBarriers[i].dstAccessMask = VkTypes::AsVkAccessFlags(toStage);
    }

    for (IndexT i = 0; i < barrier.numImageBarriers; i++)
    {
        barrier.imageBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.imageBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.imageBarriers[i].dstAccessMask = VkTypes::AsVkAccessFlags(toStage);

        const TextureSubresourceInfo& subres = textures[i].subres;
        bool isDepth = AnyBits(subres.bits, CoreGraphics::ImageBits::DepthBits);

        barrier.imageBarriers[i].newLayout = VkTypes::AsVkImageLayout(toStage, isDepth);
    }

    vkCmdPipelineBarrier(
        toCmds
        , barrier.srcFlags
        , barrier.dstFlags
        , barrier.dep
        , barrier.numMemoryBarriers, barrier.memoryBarriers
        , barrier.numBufferBarriers, barrier.bufferBarriers
        , barrier.numImageBarriers, barrier.imageBarriers);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdBarrier(const CmdBufferId id, const CoreGraphics::BarrierId barrier)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    const VkBarrierInfo& info = BarrierGetVk(barrier);
    vkCmdPipelineBarrier(cmdBuf,
        info.srcFlags,
        info.dstFlags,
        info.dep,
        info.numMemoryBarriers, info.memoryBarriers,
        info.numBufferBarriers, info.bufferBarriers,
        info.numImageBarriers, info.imageBarriers);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSignalEvent(const CmdBufferId id, const CoreGraphics::EventId ev, const CoreGraphics::PipelineStage stage)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    const VkEventInfo& info = EventGetVk(ev);
    vkCmdSetEvent(cmdBuf, info.event, VkTypes::AsVkPipelineStage(stage));
}

//------------------------------------------------------------------------------
/**
*/
void
CmdWaitEvent(const CmdBufferId id, const EventId ev, const CoreGraphics::PipelineStage waitStage, const CoreGraphics::PipelineStage signalStage)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    const VkEventInfo& info = EventGetVk(ev);
    vkCmdWaitEvents(
        cmdBuf
        , 1
        , &info.event
        , VkTypes::AsVkPipelineStage(waitStage)
        , VkTypes::AsVkPipelineStage(signalStage)
        , info.numMemoryBarriers
        , info.memoryBarriers
        , info.numBufferBarriers
        , info.bufferBarriers
        , info.numImageBarriers
        , info.imageBarriers
    );
}

//------------------------------------------------------------------------------
/**
*/
void
CmdResetEvent(const CmdBufferId id, const CoreGraphics::EventId ev, const CoreGraphics::PipelineStage stage)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    const VkEventInfo& info = EventGetVk(ev);
    vkCmdResetEvent(cmdBuf, info.event, VkTypes::AsVkPipelineStage(stage));
}

//------------------------------------------------------------------------------
/**
*/
void
CmdBeginPass(const CmdBufferId id, const PassId pass)
{
    VkPipelineBundle& pipelineBundle = commandBuffers.Get<CmdBuffer_VkPipelineBundle>(id.id24);
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    const VkRenderPassBeginInfo& info = PassGetVkRenderPassBeginInfo(pass);
    const VkGraphicsPipelineCreateInfo& framebufferInfo = PassGetVkFramebufferInfo(pass);

    CmdPipelineBuildBits& bits = commandBuffers.Get<CmdBuffer_PipelineBuildBits>(id.id24);
    bits |= CoreGraphics::CmdPipelineBuildBits::FramebufferLayoutInfoSet;
    bits &= ~CoreGraphics::CmdPipelineBuildBits::PipelineBuilt;

    // Set viewports and scissors
    auto viewports = PassGetViewports(pass);
    CmdSetViewports(id, viewports);
    auto scissors = PassGetRects(pass);
    CmdSetScissors(id, scissors);

    pipelineBundle.pass = pass;
    pipelineBundle.multisampleInfo.rasterizationSamples = framebufferInfo.pMultisampleState->rasterizationSamples;
    pipelineBundle.pipelineInfo.subpass = 0;
    pipelineBundle.pipelineInfo.renderPass = framebufferInfo.renderPass;
    pipelineBundle.pipelineInfo.pViewportState = framebufferInfo.pViewportState;
    vkCmdBeginRenderPass(cmdBuf, &info, VK_SUBPASS_CONTENTS_INLINE);    
}

//------------------------------------------------------------------------------
/**
*/
void
CmdNextSubpass(const CmdBufferId id)
{
    VkPipelineBundle& pipelineBundle = commandBuffers.Get<CmdBuffer_VkPipelineBundle>(id.id24);
    pipelineBundle.pipelineInfo.subpass++;
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    vkCmdNextSubpass(cmdBuf, VK_SUBPASS_CONTENTS_INLINE);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdEndPass(const CmdBufferId id)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    vkCmdEndRenderPass(cmdBuf);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdResetClipToPass(const CmdBufferId id)
{
    VkPipelineBundle& pipelineBundle = commandBuffers.Get<CmdBuffer_VkPipelineBundle>(id.id24);

    // Set viewports and scissors
    auto viewports = PassGetViewports(pipelineBundle.pass);
    CmdSetViewports(id, viewports);
    auto scissors = PassGetRects(pipelineBundle.pass);
    CmdSetScissors(id, scissors);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdDraw(const CmdBufferId id, const CoreGraphics::PrimitiveGroup& pg)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    if (pg.GetNumIndices() > 0)
        vkCmdDrawIndexed(cmdBuf, pg.GetNumIndices(), 1, pg.GetBaseIndex(), pg.GetBaseVertex(), 0);
    else
        vkCmdDraw(cmdBuf, pg.GetNumVertices(), 1, pg.GetBaseVertex(), 0);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdDraw(const CmdBufferId id, SizeT numInstances, const CoreGraphics::PrimitiveGroup& pg)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    if (pg.GetNumIndices() > 0)
        vkCmdDrawIndexed(cmdBuf, pg.GetNumIndices(), numInstances, pg.GetBaseIndex(), pg.GetBaseVertex(), 0);
    else
        vkCmdDraw(cmdBuf, pg.GetNumVertices(), numInstances, pg.GetBaseVertex(), 0);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdDraw(const CmdBufferId id, SizeT numInstances, IndexT baseInstance, const CoreGraphics::PrimitiveGroup& pg)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    if (pg.GetNumIndices() > 0)
        vkCmdDrawIndexed(cmdBuf, pg.GetNumIndices(), numInstances, pg.GetBaseIndex(), pg.GetBaseVertex(), baseInstance);
    else
        vkCmdDraw(cmdBuf, pg.GetNumVertices(), numInstances, pg.GetBaseVertex(), baseInstance);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdDrawIndirect(const CmdBufferId id, const CoreGraphics::BufferId buffer, IndexT bufferOffset, SizeT numDraws, SizeT stride)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    vkCmdDrawIndirect(cmdBuf, BufferGetVk(buffer), bufferOffset, numDraws, stride);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdDrawIndirectIndexed(const CmdBufferId id, const CoreGraphics::BufferId buffer, IndexT bufferOffset, SizeT numDraws, SizeT stride)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    vkCmdDrawIndexedIndirect(cmdBuf, BufferGetVk(buffer), bufferOffset, numDraws, stride);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdDispatch(const CmdBufferId id, int dimX, int dimY, int dimZ)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    vkCmdDispatch(cmdBuf, dimX, dimY, dimZ);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdResolve(const CmdBufferId id, const CoreGraphics::TextureId source, const CoreGraphics::TextureCopy sourceCopy, const CoreGraphics::TextureId dest, const CoreGraphics::TextureCopy destCopy)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    VkImage vkSrc = TextureGetVkImage(source);
    VkImage vkDst = TextureGetVkImage(dest);

    VkImageResolve resolve;
    resolve.dstOffset = { destCopy.region.left, destCopy.region.top, 0 };
    resolve.srcOffset = { sourceCopy.region.left, sourceCopy.region.top, 0 };
    resolve.extent.width = sourceCopy.region.width();
    resolve.extent.height = sourceCopy.region.height();
    resolve.extent.depth = 1;
    n_assert(destCopy.bits != CoreGraphics::ImageBits::Auto && sourceCopy.bits != CoreGraphics::ImageBits::Auto);
    resolve.srcSubresource.aspectMask = VkTypes::AsVkImageAspectFlags(sourceCopy.bits);
    resolve.srcSubresource.baseArrayLayer = 0;
    resolve.srcSubresource.layerCount = 1;
    resolve.srcSubresource.mipLevel = 0;

    resolve.dstSubresource.aspectMask = VkTypes::AsVkImageAspectFlags(destCopy.bits);
    resolve.dstSubresource.baseArrayLayer = 0;
    resolve.dstSubresource.layerCount = 1;
    resolve.dstSubresource.mipLevel = 0;
    
    vkCmdResolveImage(cmdBuf, vkSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &resolve);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdBuildBlas(const CmdBufferId id, const CoreGraphics::BlasId blas)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    const VkAccelerationStructureBuildGeometryInfoKHR& buildInfo = Vulkan::BlasGetVkBuild(blas);
    const Util::Array<VkAccelerationStructureBuildRangeInfoKHR>& rangeInfo = Vulkan::BlasGetVkRanges(blas);
    const VkAccelerationStructureBuildRangeInfoKHR* ranges[] = { rangeInfo.ConstBegin() };
    vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildInfo, ranges);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdBuildTlas(const CmdBufferId id, const CoreGraphics::TlasId tlas)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    const VkAccelerationStructureBuildGeometryInfoKHR& buildInfo = Vulkan::TlasGetVkBuild(tlas);
    const Util::Array<VkAccelerationStructureBuildRangeInfoKHR>& rangeInfo = Vulkan::TlasGetVkRanges(tlas);
    const VkAccelerationStructureBuildRangeInfoKHR* ranges[] = { rangeInfo.ConstBegin() };
    vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildInfo, ranges);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdRaysDispatch(const CmdBufferId id, const RayDispatchTable& table, int dimX, int dimY, int dimZ)
{
    VkStridedDeviceAddressRegionKHR genRegion, hitRegion, missRegion, callableRegion;
    uint handleSize = CoreGraphics::GetCurrentRaytracingProperties().shaderGroupHandleSize;

    auto RegionSetup = [handleSize](VkStridedDeviceAddressRegionKHR& region, const RayDispatchTable::Entry& entry)
    {
        if (entry.numEntries > 0)
            region =
            {
                .deviceAddress = entry.baseAddress,
                .stride = handleSize,
                .size = entry.entrySize
            };
        else
            region =
            {
                .deviceAddress = 0x0,
                .stride = 0x0,
                .size = 0x0
            };
    };

    RegionSetup(genRegion, table.genEntry);
    RegionSetup(missRegion, table.missEntry);
    RegionSetup(hitRegion, table.hitEntry);
    RegionSetup(callableRegion, table.callableEntry);

    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    vkCmdTraceRaysKHR(cmdBuf, &genRegion, &missRegion, &hitRegion, &callableRegion, dimX, dimY, dimZ);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdDrawMeshlets(const CmdBufferId id, int dimX, int dimY, int dimZ)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    vkCmdDrawMeshTasksEXT(cmdBuf, dimX, dimY, dimZ);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdCopy(
    const CmdBufferId id
    , const CoreGraphics::TextureId fromTexture
    , const Util::Array<CoreGraphics::TextureCopy>& from
    , const CoreGraphics::TextureId toTexture
    , const Util::Array<CoreGraphics::TextureCopy>& to
)
{
    n_assert(from.Size() > 0);
    n_assert(from.Size() == to.Size());

    Util::FixedArray<VkImageCopy> copies(from.Size());
    for (IndexT i = 0; i < copies.Size(); i++)
    {
        n_assert(from[i].bits != ImageBits::Auto && to[i].bits != ImageBits::Auto);
        VkImageCopy& copy = copies[i];
        copy.srcSubresource = { VkTypes::AsVkImageAspectFlags(from[i].bits), (uint32_t)from[i].mip, (uint32_t)from[i].layer, 1 };
        copy.srcOffset = { from[i].region.left, from[i].region.top, 0 };
        copy.dstSubresource = { VkTypes::AsVkImageAspectFlags(to[i].bits), (uint32_t)to[i].mip, (uint32_t)to[i].layer, 1 };
        copy.dstOffset = { to[i].region.left, to[i].region.top, 0 };
        copy.extent = { (uint32_t)to[i].region.width(), (uint32_t)to[i].region.height(), 1 };
    }

    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    vkCmdCopyImage(cmdBuf, TextureGetVkImage(fromTexture), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, TextureGetVkImage(toTexture), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copies.Size(), copies.Begin());
}

//------------------------------------------------------------------------------
/**
*/
void
CmdCopy(
    const CmdBufferId id
    , const CoreGraphics::TextureId fromTexture
    , const Util::Array<CoreGraphics::TextureCopy>& from
    , const CoreGraphics::BufferId toBuffer
    , const Util::Array<CoreGraphics::BufferCopy>& to
)
{
    n_assert(from.Size() > 0);
    n_assert(from.Size() == to.Size());

    Util::FixedArray<VkBufferImageCopy> copies(from.Size());
    for (IndexT i = 0; i < copies.Size(); i++)
    {
        VkBufferImageCopy& copy = copies[i];
        copy.bufferOffset = to[i].offset;
        copy.bufferImageHeight = to[i].imageHeight;
        copy.bufferRowLength = to[i].rowLength;
        copy.imageExtent = { (uint32_t)from[i].region.width(), (uint32_t)from[i].region.height(), 1 };
        copy.imageOffset = { (int32_t)from[i].region.left, (int32_t)from[i].region.top, 0 };
        copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)from[i].mip, (uint32_t)from[i].layer, 1 };
    }

    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    vkCmdCopyImageToBuffer(
        cmdBuf
        , TextureGetVkImage(fromTexture)
        , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        , BufferGetVk(toBuffer)
        , copies.Size()
        , copies.Begin());
}

//------------------------------------------------------------------------------
/**
*/
void
CmdCopy(
    const CmdBufferId id
    , const CoreGraphics::BufferId fromBuffer
    , const Util::Array<CoreGraphics::BufferCopy>& from
    , const CoreGraphics::BufferId toBuffer
    , const Util::Array<CoreGraphics::BufferCopy>& to
    , const SizeT size
)
{
    n_assert(from.Size() > 0);
    n_assert(from.Size() == to.Size());

    Util::FixedArray<VkBufferCopy> copies(from.Size());
    for (IndexT i = 0; i < copies.Size(); i++)
    {
        VkBufferCopy& copy = copies[i];
        copy.srcOffset = from[i].offset;
        copy.dstOffset = to[i].offset;
        copy.size = size;
    }

    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    vkCmdCopyBuffer(cmdBuf, BufferGetVk(fromBuffer), BufferGetVk(toBuffer), copies.Size(), copies.Begin());
}

//------------------------------------------------------------------------------
/**
*/
void
CmdCopy(
    const CmdBufferId id
    , const CoreGraphics::BufferId fromBuffer
    , const Util::Array<CoreGraphics::BufferCopy>& from
    , const CoreGraphics::TextureId toTexture
    , const Util::Array<CoreGraphics::TextureCopy>& to
)
{
    n_assert(from.Size() > 0);
    n_assert(from.Size() == to.Size());

    Util::FixedArray<VkBufferImageCopy> copies(from.Size());
    for (IndexT i = 0; i < copies.Size(); i++)
    {
        VkBufferImageCopy& copy = copies[i];
        copy.bufferOffset = from[i].offset;
        copy.bufferImageHeight = from[i].imageHeight;
        copy.bufferRowLength = from[i].rowLength;
        copy.imageExtent = { (uint32_t)to[i].region.width(), (uint32_t)to[i].region.height(), 1 };
        copy.imageOffset = { (int32_t)to[i].region.left, (int32_t)to[i].region.top, 0 };
        copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)to[i].mip, (uint32_t)to[i].layer, 1 };
    }

    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    vkCmdCopyBufferToImage(
        cmdBuf
        , BufferGetVk(fromBuffer)
        , TextureGetVkImage(toTexture)
        , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        , copies.Size()
        , copies.Begin());
}

//------------------------------------------------------------------------------
/**
*/
void
CmdBlit(
    const CmdBufferId id
    , const CoreGraphics::TextureId from
    , const Math::rectangle<SizeT>& fromRegion
    , const CoreGraphics::ImageBits fromBits
    , IndexT fromMip
    , IndexT fromLayer
    , const CoreGraphics::TextureId to
    , const Math::rectangle<SizeT>& toRegion
    , const CoreGraphics::ImageBits toBits
    , IndexT toMip
    , IndexT toLayer
)
{
    n_assert(from != CoreGraphics::InvalidTextureId && to != CoreGraphics::InvalidTextureId);
    n_assert(fromBits != CoreGraphics::ImageBits::Auto && toBits != CoreGraphics::ImageBits::Auto);

    VkImageBlit blit;
    blit.srcOffsets[0] = { fromRegion.left, fromRegion.top, 0 };
    blit.srcOffsets[1] = { fromRegion.right, fromRegion.bottom, 1 };
    blit.srcSubresource = { VkTypes::AsVkImageAspectFlags(fromBits), (uint32_t)fromMip, (uint32_t)fromLayer, 1 };
    blit.dstOffsets[0] = { toRegion.left, toRegion.top, 0 };
    blit.dstOffsets[1] = { toRegion.right, toRegion.bottom, 1 };
    blit.dstSubresource = { VkTypes::AsVkImageAspectFlags(toBits), (uint32_t)toMip, (uint32_t)toLayer, 1 };

    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    vkCmdBlitImage(
        cmdBuf
        , TextureGetVkImage(from)
        , VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        , TextureGetVkImage(to)
        , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        , 1
        , &blit
        , VK_FILTER_LINEAR);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSetViewports(const CmdBufferId id, Util::FixedArray<Math::rectangle<int>> viewports)
{
    ViewportBundle& pending = commandBuffers.Get<CmdBuffer_PendingViewports>(id.id24);
    pending.numPending = 0;
    for (Math::rectangle<int> viewport : viewports)
    {
        VkViewport vp;
        vp.width = (float)viewport.width();
        vp.height = (float)viewport.height();
        vp.x = (float)viewport.left;
        vp.y = (float)viewport.top;
        vp.minDepth = 0.0f;
        vp.maxDepth = 1.0f;
        pending.viewports[pending.numPending++] = vp;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSetScissors(const CmdBufferId id, Util::FixedArray<Math::rectangle<int>> rects)
{
    ScissorBundle& pending = commandBuffers.Get<CmdBuffer_PendingScissors>(id.id24);
    pending.numPending = 0;
    for (Math::rectangle<int> rect : rects)
    {
        VkRect2D sc;
        sc.extent.width = rect.width();
        sc.extent.height = rect.height();
        sc.offset.x = rect.left;
        sc.offset.y = rect.top;
        pending.scissors[pending.numPending++] = sc;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSetViewport(const CmdBufferId id, const Math::rectangle<int>& rect, int index)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    VkViewport vp;
    vp.width = (float)rect.width();
    vp.height = (float)rect.height();
    vp.x = (float)rect.left;
    vp.y = (float)rect.top;
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuf, index, 1, &vp);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSetScissorRect(const CmdBufferId id, const Math::rectangle<int>& rect, int index)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    VkRect2D sc;
    sc.extent.width = rect.width();
    sc.extent.height = rect.height();
    sc.offset.x = rect.left;
    sc.offset.y = rect.top;
    vkCmdSetScissor(cmdBuf, index, 1, &sc);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSetStencilRef(const CmdBufferId id, const uint frontRef, const uint backRef)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    if (frontRef == backRef)
    {
        vkCmdSetStencilReference(cmdBuf, VK_STENCIL_FACE_FRONT_AND_BACK, frontRef);
    }
    else
    {
        vkCmdSetStencilReference(cmdBuf, VK_STENCIL_FACE_FRONT_BIT, frontRef);
        vkCmdSetStencilReference(cmdBuf, VK_STENCIL_FACE_BACK_BIT, backRef);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSetStencilReadMask(const CmdBufferId id, const uint readMask)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    vkCmdSetStencilCompareMask(cmdBuf, VK_STENCIL_FACE_FRONT_AND_BACK, readMask);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdSetStencilWriteMask(const CmdBufferId id, const uint writeMask)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    vkCmdSetStencilWriteMask(cmdBuf, VK_STENCIL_FACE_FRONT_AND_BACK, writeMask);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdUpdateBuffer(const CmdBufferId id, const CoreGraphics::BufferId buffer, uint offset, uint size, const void* data)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    vkCmdUpdateBuffer(cmdBuf, Vulkan::BufferGetVk(buffer), offset, size, data);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdStartOcclusionQueries(const CmdBufferId id)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    QueryBundle& queryBundle = commandBuffers.Get<CmdBuffer_Query>(id.id24);
    VkQueryPool pool = Vulkan::GetQueryPool(CoreGraphics::OcclusionQueryType);
    n_assert(queryBundle.enabled[CoreGraphics::OcclusionQueryType]);
    QueryBundle::QueryChunk& chunk = queryBundle.GetChunk(CoreGraphics::OcclusionQueryType);
    vkCmdBeginQuery(cmdBuf, pool, chunk.offset + chunk.queryCount++, 0x0);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdEndOcclusionQueries(const CmdBufferId id)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    QueryBundle& queryBundle = commandBuffers.Get<CmdBuffer_Query>(id.id24);
    VkQueryPool pool = Vulkan::GetQueryPool(CoreGraphics::OcclusionQueryType);
    n_assert(queryBundle.enabled[CoreGraphics::OcclusionQueryType]);
    QueryBundle::QueryChunk& chunk = queryBundle.GetChunk(CoreGraphics::OcclusionQueryType);
    vkCmdEndQuery(cmdBuf, pool, chunk.offset + chunk.queryCount++);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdStartPipelineQueries(const CmdBufferId id)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    QueryBundle& queryBundle = commandBuffers.Get<CmdBuffer_Query>(id.id24);
    VkQueryPool pool = Vulkan::GetQueryPool(CoreGraphics::StatisticsQueryType);
    n_assert(queryBundle.enabled[CoreGraphics::StatisticsQueryType]);
    QueryBundle::QueryChunk& chunk = queryBundle.GetChunk(CoreGraphics::StatisticsQueryType);
    vkCmdBeginQuery(cmdBuf, pool, chunk.offset + chunk.queryCount++, 0x0);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdEndPipelineQueries(const CmdBufferId id)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);
    QueryBundle& queryBundle = commandBuffers.Get<CmdBuffer_Query>(id.id24);
    VkQueryPool pool = Vulkan::GetQueryPool(CoreGraphics::StatisticsQueryType);
    n_assert(queryBundle.enabled[CoreGraphics::StatisticsQueryType]);
    QueryBundle::QueryChunk& chunk = queryBundle.GetChunk(CoreGraphics::StatisticsQueryType);
    vkCmdEndQuery(cmdBuf, pool, chunk.offset + chunk.queryCount++);
}

#if NEBULA_GRAPHICS_DEBUG
//------------------------------------------------------------------------------
/**
*/
void
CmdBeginMarker(const CmdBufferId id, const Math::vec4& color, const char* name)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);

#if NEBULA_ENABLE_PROFILING
    CmdBufferMarkerBundle& markers = commandBuffers.Get<CmdBuffer_ProfilingMarkers>(id.id24);
    QueryBundle& queryBundle = commandBuffers.Get<CmdBuffer_Query>(id.id24);
    VkQueryPool pool = Vulkan::GetQueryPool(CoreGraphics::TimestampsQueryType);
    if (queryBundle.enabled[CoreGraphics::TimestampsQueryType])
    {
        QueryBundle::QueryChunk& chunk = queryBundle.GetChunk(CoreGraphics::TimestampsQueryType);
        CoreGraphics::QueueType usage = commandBuffers.Get<CmdBuffer_Usage>(id.id24);
        FrameProfilingMarker marker;
        marker.color = color;
        marker.name = name;
        marker.queue = usage;
        marker.gpuBegin = chunk.offset + chunk.queryCount++;
        vkCmdWriteTimestamp(cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, pool, marker.gpuBegin);
        markers.markerStack.Push(marker);
    }
#endif

    alignas(16) float col[4];
    color.store(col);
    VkDebugUtilsLabelEXT info =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        nullptr,
        name,
        { col[0], col[1], col[2], col[3] }
    };
    VkCmdDebugMarkerBegin(cmdBuf, &info);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdEndMarker(const CmdBufferId id)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);

#if NEBULA_ENABLE_PROFILING
    QueryBundle& queryBundle = commandBuffers.Get<CmdBuffer_Query>(id.id24);
    VkQueryPool pool = Vulkan::GetQueryPool(CoreGraphics::TimestampsQueryType);
    if (queryBundle.enabled[CoreGraphics::TimestampsQueryType])
    {
        QueryBundle::QueryChunk& chunk = queryBundle.GetChunk(CoreGraphics::TimestampsQueryType);
        CmdBufferMarkerBundle& markers = commandBuffers.Get<CmdBuffer_ProfilingMarkers>(id.id24);
        n_assert(!markers.markerStack.IsEmpty());
        FrameProfilingMarker marker = markers.markerStack.Pop();
        marker.gpuEnd = chunk.offset + chunk.queryCount++;
        vkCmdWriteTimestamp(cmdBuf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, pool, marker.gpuEnd);

        // Push marker to finished list
        if (markers.markerStack.IsEmpty())
            markers.finishedMarkers.Append(marker);
        else
            markers.markerStack.Peek().children.Append(marker);
    }
#endif
    VkCmdDebugMarkerEnd(cmdBuf);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdInsertMarker(const CmdBufferId id, const Math::vec4& color, const char* name)
{
    VkCommandBuffer cmdBuf = commandBuffers.Get<CmdBuffer_VkCommandBuffer>(id.id24);

    alignas(16) float col[4];
    color.store(col);
    VkDebugUtilsLabelEXT info =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        nullptr,
        name,
        { col[0], col[1], col[2], col[3] }
    };
    VkCmdDebugMarkerInsert(cmdBuf, &info);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdFinishQueries(const CmdBufferId id)
{
    QueryBundle& queryBundle = commandBuffers.Get<CmdBuffer_Query>(id.id24);
    for (IndexT i = 0; i < CoreGraphics::QueryType::NumQueryTypes; i++)
    {
        // Grab all chunk offsets and counts 
        const Util::Array<QueryBundle::QueryChunk>& chunks = queryBundle.chunks[i];
        if (!chunks.IsEmpty())
        {
            Util::FixedArray<IndexT> offsets(chunks.Size());
            Util::FixedArray<SizeT> counts(chunks.Size());
            for (IndexT j = 0; j < chunks.Size(); j++)
            {
                offsets[j] = chunks[j].offset;
                counts[j] = chunks[j].queryCount;
            }

            // Finally copy over all chunks to the output buffer
            CoreGraphics::FinishQueries(id, (CoreGraphics::QueryType)i, offsets.Begin(), counts.Begin(), chunks.Size());
        }
    }
}
#endif

#if NEBULA_ENABLE_PROFILING
//------------------------------------------------------------------------------
/**
*/
Util::Array<CoreGraphics::FrameProfilingMarker>
CmdCopyProfilingMarkers(const CmdBufferId id)
{
    CoreGraphics::CmdBufferMarkerBundle& markers = commandBuffers.Get<CmdBuffer_ProfilingMarkers>(id.id24);
    return markers.finishedMarkers;
}

//------------------------------------------------------------------------------
/**
*/
uint
CmdGetMarkerOffset(const CmdBufferId id)
{
    CoreGraphics::QueryBundle& queries = commandBuffers.Get<CmdBuffer_Query>(id.id24);
    n_assert(queries.enabled[CoreGraphics::QueryType::TimestampsQueryType]);
    return queries.chunks[CoreGraphics::TimestampsQueryType][0].offset;
}

#endif

} // namespace Vulkan
