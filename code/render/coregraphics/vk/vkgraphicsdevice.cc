//------------------------------------------------------------------------------
//  vkgraphicsdevice.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/config.h"
#include "vkgraphicsdevice.h"
#include "vkmemory.h"
#include "coregraphics/commandbuffer.h"
#include "vkshaderprogram.h"
#include "vkpipelinedatabase.h"
#include "vkcommandbuffer.h"
#include "vktransformdevice.h"
#include "vkresourcetable.h"
#include "vkshaderserver.h"
#include "vkpass.h"
#include "vkbarrier.h"
#include "vkbuffer.h"
#include "coregraphics/displaydevice.h"
#include "app/application.h"
#include "util/bit.h"
#include "io/ioserver.h"
#include "vkevent.h"
#include "vkfence.h"
#include "vktypes.h"
#include "vkutilities.h"
#include "coregraphics/vertexsignaturecache.h"
#include "coregraphics/glfw/glfwwindow.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/vk/vksemaphore.h"
#include "coregraphics/vk/vkfence.h"
#include "coregraphics/vk/vksubmissioncontext.h"
#include "coregraphics/submissioncontext.h"
#include "resources/resourceserver.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/drawthread.h"
#include "profiling/profiling.h"

namespace Vulkan
{

enum class PipelineBuildBits : uint
{
    NoInfoSet                       = 0,
    ShaderInfoSet                   = N_BIT(0),
    VertexLayoutInfoSet             = N_BIT(1),
    FramebufferLayoutInfoSet        = N_BIT(2),
    InputLayoutInfoSet              = N_BIT(3),

    AllInfoSet                      = ShaderInfoSet | VertexLayoutInfoSet | FramebufferLayoutInfoSet | InputLayoutInfoSet,

    PipelineBuilt                   = N_BIT(4)
};
__ImplementEnumBitOperators(PipelineBuildBits);
__ImplementEnumComparisonOperators(PipelineBuildBits);

struct GraphicsDeviceState : CoreGraphics::GraphicsDeviceState
{
    uint32_t adapter;
    uint32_t frameId;
    VkPhysicalDeviceMemoryProperties memoryProps;

    VkInstance instance;
    VkDescriptorPool descPool;
    VkPipelineCache cache;
    VkAllocationCallbacks alloc;

    CoreGraphics::ShaderPipeline currentBindPoint;

    // only support threaded command buffer building 
    CoreGraphics::CommandBufferId drawThreadCommands;

    Util::FixedArray<VkCommandBufferThread::VkDescriptorsCommand> propagateDescriptorSets;
    SizeT numCallsLastFrame;
    SizeT numUsedThreads;

    VkCommandBufferInheritanceInfo passInfo;
    VkPipelineInputAssemblyStateCreateInfo inputInfo;
    VkPipelineColorBlendStateCreateInfo blendInfo;

    static const uint MaxVertexStreams = 16;
    CoreGraphics::BufferId vboStreams[MaxVertexStreams];
    IndexT vboStreamOffsets[MaxVertexStreams];
    CoreGraphics::BufferId ibo;
    IndexT iboOffset;

    struct ConstantsRingBuffer
    {
        // handle global constant memory
        AtomicCounter cboGfxStartAddress[CoreGraphics::GlobalConstantBufferType::NumConstantBufferTypes];
        AtomicCounter cboGfxEndAddress[CoreGraphics::GlobalConstantBufferType::NumConstantBufferTypes];
        AtomicCounter cboComputeStartAddress[CoreGraphics::GlobalConstantBufferType::NumConstantBufferTypes];
        AtomicCounter cboComputeEndAddress[CoreGraphics::GlobalConstantBufferType::NumConstantBufferTypes];
    };
    Util::FixedArray<ConstantsRingBuffer> constantBufferRings;

    VkSemaphore waitForPresentSemaphore;

    CoreGraphics::QueueType mainSubmitQueue;

    uint maxNumBufferedFrames;
    uint32_t currentBufferedFrameIndex;

    VkExtensionProperties physicalExtensions[64];

    uint32_t usedPhysicalExtensions;
    const char* deviceExtensionStrings[64];

    uint32_t usedExtensions;
    const char* extensions[64];


    uint32_t drawQueueFamily;
    uint32_t computeQueueFamily;
    uint32_t transferQueueFamily;
    uint32_t sparseQueueFamily;
    uint32_t drawQueueIdx;
    uint32_t computeQueueIdx;
    uint32_t transferQueueIdx;
    uint32_t sparseQueueIdx;
    Util::Set<uint32_t> usedQueueFamilies;
    Util::FixedArray<uint32_t> queueFamilyMap;

    // setup management classes
    VkSubContextHandler subcontextHandler;
    VkPipelineDatabase database;

    // device handling (multi GPU?!?!)
    Util::FixedArray<VkDevice> devices;
    Util::FixedArray<VkPhysicalDevice> physicalDevices;
    Util::FixedArray<VkPhysicalDeviceProperties> deviceProps;
    Util::FixedArray<VkPhysicalDeviceFeatures> deviceFeatures;
    Util::FixedArray<uint32_t> numCaps;
    Util::FixedArray<Util::FixedArray<VkExtensionProperties>> caps;
    Util::FixedArray<Util::FixedArray<const char*>> deviceFeatureStrings;
    IndexT currentDevice;

    Util::FixedArray<CoreGraphics::ShaderProgramId> currentShaderPrograms;
    CoreGraphics::ShaderFeature::Mask currentShaderMask;

    CoreGraphics::VertexLayoutId currentVertexLayout;
    CoreGraphics::ShaderProgramId currentVertexLayoutShader;

    VkGraphicsPipelineCreateInfo currentPipelineInfo;
    VkPipelineLayout currentGraphicsPipelineLayout;
    VkPipelineLayout currentComputePipelineLayout;
    VkPipeline currentPipeline;
    PipelineBuildBits currentPipelineBits;
    uint currentStencilFrontRef, currentStencilBackRef, currentStencilReadMask, currentStencilWriteMask;

    Util::FixedArray<Util::Array<VkBuffer>> delayedDeleteBuffers;
    Util::FixedArray<Util::Array<VkImage>> delayedDeleteImages;
    Util::FixedArray<Util::Array<VkImageView>> delayedDeleteImageViews;
    Util::FixedArray<Util::Array<CoreGraphics::Alloc>> delayedFreeMemories;

    static const SizeT MaxQueriesPerFrame = 1024;
    VkQueryPool queryPoolsByType[CoreGraphics::NumQueryTypes];
    CoreGraphics::BufferId queryResultBuffers[CoreGraphics::NumQueryTypes];
    Util::FixedArray<IndexT> queryStartIndexByType[CoreGraphics::NumQueryTypes];
    Util::FixedArray<SizeT> queryCountsByType[CoreGraphics::NumQueryTypes];
    Util::FixedArray<CoreGraphics::Query> queries;
    SizeT numUsedQueries;
    IndexT queriesRingOffset;

    VkCommandBufferThread::VkScissorRectArrayCommand scissorArrayCommand;
    VkCommandBufferThread::VkViewportArrayCommand viewportArrayCommand;

    _declare_counter(NumPipelinesBuilt);
    _declare_timer(DebugTimer);

} state;

VkDebugUtilsMessengerEXT VkErrorDebugMessageHandle = nullptr;
PFN_vkCreateDebugUtilsMessengerEXT VkCreateDebugMessenger = nullptr;
PFN_vkDestroyDebugUtilsMessengerEXT VkDestroyDebugMessenger = nullptr;

PFN_vkSetDebugUtilsObjectNameEXT VkDebugObjectName = nullptr;
PFN_vkSetDebugUtilsObjectTagEXT VkDebugObjectTag = nullptr;
PFN_vkQueueBeginDebugUtilsLabelEXT VkQueueBeginLabel = nullptr;
PFN_vkQueueEndDebugUtilsLabelEXT VkQueueEndLabel = nullptr;
PFN_vkQueueInsertDebugUtilsLabelEXT VkQueueInsertLabel = nullptr;
PFN_vkCmdBeginDebugUtilsLabelEXT VkCmdDebugMarkerBegin = nullptr;
PFN_vkCmdEndDebugUtilsLabelEXT VkCmdDebugMarkerEnd = nullptr;
PFN_vkCmdInsertDebugUtilsLabelEXT VkCmdDebugMarkerInsert = nullptr;

//------------------------------------------------------------------------------
/**
*/
void
SetupAdapter()
{
    // retrieve available GPUs
    uint32_t gpuCount;
    VkResult res;
    res = vkEnumeratePhysicalDevices(state.instance, &gpuCount, NULL);
    n_assert(res == VK_SUCCESS);

    state.devices.Resize(gpuCount);
    state.physicalDevices.Resize(gpuCount);
    state.numCaps.Resize(gpuCount);
    state.caps.Resize(gpuCount);
    state.deviceFeatureStrings.Resize(gpuCount);
    state.deviceProps.Resize(gpuCount);
    state.deviceFeatures.Resize(gpuCount);


    if (gpuCount > 0)
    {
        res = vkEnumeratePhysicalDevices(state.instance, &gpuCount, state.physicalDevices.Begin());
        n_assert(res == VK_SUCCESS);

        if (gpuCount > 1)
            n_printf("Found %d GPUs, which is more than 1! Perhaps the Render Device should be able to use it?\n", gpuCount);

        IndexT i;
        for (i = 0; i < (IndexT)gpuCount; i++)
        {
            res = vkEnumerateDeviceExtensionProperties(state.physicalDevices[i], nullptr, &state.numCaps[i], nullptr);
            n_assert(res == VK_SUCCESS);

            if (state.numCaps[i] > 0)
            {
                state.caps[i].Resize(state.numCaps[i]);
                state.deviceFeatureStrings[i].Resize(state.numCaps[i]);

                res = vkEnumerateDeviceExtensionProperties(state.physicalDevices[i], nullptr, &state.numCaps[i], state.caps[i].Begin());
                n_assert(res == VK_SUCCESS);

                static const Util::String wantedExtensions[] =
                {
                    "VK_KHR_swapchain",
                    "VK_KHR_maintenance1",
                    "VK_KHR_maintenance2",
                    "VK_KHR_maintenance3",
                    "VK_EXT_host_query_reset",
                    "VK_EXT_descriptor_indexing",
                    "VK_EXT_robustness2"
                };

                uint32_t newNumCaps = 0;
                state.deviceFeatureStrings[i][newNumCaps++] = wantedExtensions[0].AsCharPtr();
                state.deviceFeatureStrings[i][newNumCaps++] = wantedExtensions[1].AsCharPtr();
                state.deviceFeatureStrings[i][newNumCaps++] = wantedExtensions[2].AsCharPtr();
                state.deviceFeatureStrings[i][newNumCaps++] = wantedExtensions[3].AsCharPtr();
                state.numCaps[i] = newNumCaps;
            }

            // get device props and features
            vkGetPhysicalDeviceProperties(state.physicalDevices[0], &state.deviceProps[i]);
            vkGetPhysicalDeviceFeatures(state.physicalDevices[0], &state.deviceFeatures[i]);
        }
    }
    else
    {
        n_error("VkGraphicsDevice::SetupAdapter(): No GPU available.\n");
    }
}

//------------------------------------------------------------------------------
/**
*/
VkInstance
GetInstance()
{
    return state.instance;
}

//------------------------------------------------------------------------------
/**
*/
VkDevice
GetCurrentDevice()
{
    return state.devices[state.currentDevice];
}

//------------------------------------------------------------------------------
/**
*/
VkPhysicalDevice
GetCurrentPhysicalDevice()
{
    return state.physicalDevices[state.currentDevice];
}

//------------------------------------------------------------------------------
/**
*/
VkPhysicalDeviceProperties
GetCurrentProperties()
{
    return state.deviceProps[state.currentDevice];
}

//------------------------------------------------------------------------------
/**
*/
VkPhysicalDeviceFeatures 
GetCurrentFeatures()
{
    return state.deviceFeatures[state.currentDevice];;
}

//------------------------------------------------------------------------------
/**
*/
VkPipelineCache 
GetPipelineCache()
{
    return state.cache;
}

//------------------------------------------------------------------------------
/**
*/
VkPhysicalDeviceMemoryProperties
GetMemoryProperties()
{
    return state.memoryProps;
}

//------------------------------------------------------------------------------
/**
*/
VkCommandBuffer 
GetMainBuffer(const CoreGraphics::QueueType queue)
{
    n_assert(state.drawThreadCommands == CoreGraphics::InvalidCommandBufferId);

    switch (queue)
    {
    case CoreGraphics::GraphicsQueueType: return CommandBufferGetVk(state.gfxCmdBuffer);
    case CoreGraphics::ComputeQueueType: return CommandBufferGetVk(state.computeCmdBuffer);
    }
    return VK_NULL_HANDLE;
}

//------------------------------------------------------------------------------
/**
*/
VkSemaphore 
GetRenderingSemaphore()
{
    return SemaphoreGetVk(state.renderingFinishedSemaphores[state.currentBufferedFrameIndex]);
}

//------------------------------------------------------------------------------
/**
*/
VkFence
GetPresentFence()
{
    return FenceGetVk(state.presentFences[state.currentBufferedFrameIndex]);
}

//------------------------------------------------------------------------------
/**
*/
void 
WaitForPresent(VkSemaphore sem)
{
    n_assert(state.waitForPresentSemaphore == VK_NULL_HANDLE);
    state.waitForPresentSemaphore = sem;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Set<uint32_t>& 
GetQueueFamilies()
{
    return state.usedQueueFamilies;
}

//------------------------------------------------------------------------------
/**
*/
const uint32_t 
GetQueueFamily(const CoreGraphics::QueueType type)
{
    return state.queueFamilyMap[type];
}

//------------------------------------------------------------------------------
/**
*/
const VkQueue 
GetQueue(const CoreGraphics::QueueType type, const IndexT index)
{
    switch (type)
    {
    case CoreGraphics::GraphicsQueueType:
        return state.subcontextHandler.drawQueues[index];
        break;
    case CoreGraphics::ComputeQueueType:
        return state.subcontextHandler.computeQueues[index];
        break;
    case CoreGraphics::TransferQueueType:
        return state.subcontextHandler.transferQueues[index];
        break;
    case CoreGraphics::SparseQueueType:
        return state.subcontextHandler.sparseQueues[index];
        break;
    }
    return VK_NULL_HANDLE;
}

//------------------------------------------------------------------------------
/**
*/
const VkQueue 
GetCurrentQueue(const CoreGraphics::QueueType type)
{
    return state.subcontextHandler.GetQueue(type);
}

//------------------------------------------------------------------------------
/**
*/
void
InsertBarrier(const VkBarrierInfo& barrier, const CoreGraphics::QueueType queue)
{
    if (state.drawThread)
    {
        VkMemoryBarrier* memBarriers = nullptr;
        if (barrier.numMemoryBarriers)
        {
            memBarriers = n_new_array(VkMemoryBarrier, barrier.numMemoryBarriers);
            Memory::Copy(barrier.memoryBarriers, memBarriers, sizeof(VkMemoryBarrier) * barrier.numMemoryBarriers);
        }
        VkBufferMemoryBarrier* bufBarriers = nullptr;
        if (barrier.numBufferBarriers)
        {
            bufBarriers = n_new_array(VkBufferMemoryBarrier, barrier.numBufferBarriers);
            Memory::Copy(barrier.bufferBarriers, bufBarriers, sizeof(VkBufferMemoryBarrier) * barrier.numBufferBarriers);
        }
        VkImageMemoryBarrier* imgBarriers = nullptr;
        if (barrier.numImageBarriers)
        {
            imgBarriers = n_new_array(VkImageMemoryBarrier, barrier.numImageBarriers);
            Memory::Copy(barrier.imageBarriers, imgBarriers, sizeof(VkImageMemoryBarrier) * barrier.numImageBarriers);
        }

        VkCommandBufferThread::VkBarrierCommand cmd;
        cmd.srcMask = barrier.srcFlags;
        cmd.dstMask = barrier.dstFlags;
        cmd.dep = barrier.dep;
        cmd.memoryBarrierCount = barrier.numMemoryBarriers;
        cmd.memoryBarriers = memBarriers;
        cmd.bufferBarrierCount = barrier.numBufferBarriers;
        cmd.bufferBarriers = bufBarriers;
        cmd.imageBarrierCount = barrier.numImageBarriers;
        cmd.imageBarriers = imgBarriers;
        state.drawThread->Push(cmd);
    }
    else
    {
        n_assert(state.drawThreadCommands == CoreGraphics::InvalidCommandBufferId);
        VkCommandBuffer buf = GetMainBuffer(queue);
        vkCmdPipelineBarrier(buf,
            barrier.srcFlags,
            barrier.dstFlags,
            barrier.dep,
            barrier.numMemoryBarriers, barrier.memoryBarriers,
            barrier.numBufferBarriers, barrier.bufferBarriers,
            barrier.numImageBarriers, barrier.imageBarriers);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
Copy(const VkImage from, Math::rectangle<SizeT> fromRegion, const VkImage to, Math::rectangle<SizeT> toRegion)
{
    n_assert(!state.inBeginPass);
    n_assert(state.drawThreadCommands == CoreGraphics::InvalidCommandBufferId);
    VkImageCopy region;
    region.dstOffset = { fromRegion.left, fromRegion.top, 0 };
    region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    region.extent = { (uint32_t)toRegion.width(), (uint32_t)toRegion.height(), 1 };
    region.srcOffset = { toRegion.left, toRegion.top, 0 };
    region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    vkCmdCopyImage(GetMainBuffer(CoreGraphics::GraphicsQueueType), from, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, to, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

//------------------------------------------------------------------------------
/**
*/
void 
Blit(const VkImage from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const VkImage to, Math::rectangle<SizeT> toRegion, IndexT toMip)
{
    n_assert(!state.inBeginPass);
    n_assert(state.drawThreadCommands == CoreGraphics::InvalidCommandBufferId);
    VkImageBlit blit;
    blit.srcOffsets[0] = { fromRegion.left, fromRegion.top, 0 };
    blit.srcOffsets[1] = { fromRegion.right, fromRegion.bottom, 1 };
    blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)fromMip, 0, 1 };
    blit.dstOffsets[0] = { toRegion.left, toRegion.top, 0 };
    blit.dstOffsets[1] = { toRegion.right, toRegion.bottom, 1 };
    blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)toMip, 0, 1 };
    vkCmdBlitImage(GetMainBuffer(CoreGraphics::GraphicsQueueType), from, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, to, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);
}

//------------------------------------------------------------------------------
/**
*/
void 
BindDescriptorsGraphics(const VkDescriptorSet* descriptors, uint32_t baseSet, uint32_t setCount, const uint32_t* offsets, uint32_t offsetCount, bool propagate)
{
    // if we are setting descriptors before we have a pipeline, add them for later submission
    if (state.currentShaderPrograms[CoreGraphics::GraphicsQueueType] == CoreGraphics::InvalidShaderProgramId)
    {
        for (uint32_t i = 0; i < setCount; i++)
        {
            VkCommandBufferThread::VkDescriptorsCommand cmd;
            cmd.baseSet = baseSet;
            cmd.numSets = 1;
            cmd.sets = &descriptors[i];
            cmd.numOffsets = offsetCount;
            cmd.offsets = offsets;
            cmd.type = VK_PIPELINE_BIND_POINT_GRAPHICS;
            state.propagateDescriptorSets[baseSet + i] = cmd;
        }
    }
    else
    {
        // if batching, draws goes to thread
        n_assert(state.currentShaderPrograms[CoreGraphics::GraphicsQueueType] != CoreGraphics::InvalidShaderProgramId);
        if (state.drawThread)
        {
            n_assert(state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId);
            VkCommandBufferThread::VkDescriptorsCommand cmd;
            cmd.baseSet = baseSet;
            cmd.numSets = setCount;
            cmd.sets = descriptors;
            cmd.numOffsets = offsetCount;
            cmd.offsets = offsets;
            cmd.type = VK_PIPELINE_BIND_POINT_GRAPHICS;
            state.propagateDescriptorSets[baseSet].baseSet = -1;
            state.drawThread->Push(cmd);
        }
        else
        {
            // otherwise they go on the main draw
            vkCmdBindDescriptorSets(GetMainBuffer(CoreGraphics::GraphicsQueueType), VK_PIPELINE_BIND_POINT_GRAPHICS, state.currentGraphicsPipelineLayout, baseSet, setCount, descriptors, offsetCount, offsets);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
BindDescriptorsCompute(const VkDescriptorSet* descriptors, uint32_t baseSet, uint32_t setCount, const uint32_t* offsets, uint32_t offsetCount, const CoreGraphics::QueueType queue)
{
    n_assert(state.inBeginFrame);
    n_assert(state.drawThreadCommands == CoreGraphics::InvalidCommandBufferId);
    vkCmdBindDescriptorSets(GetMainBuffer(queue), VK_PIPELINE_BIND_POINT_COMPUTE, state.currentComputePipelineLayout, baseSet, setCount, descriptors, offsetCount, offsets);
}

//------------------------------------------------------------------------------
/**
*/
void 
UpdatePushRanges(const VkShaderStageFlags& stages, const VkPipelineLayout& layout, uint32_t offset, uint32_t size, const byte* data)
{
    if (state.drawThread)
    {
        n_assert(state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId);     
        VkCommandBufferThread::VkPushConstantsCommand cmd;
        cmd.layout = layout;
        cmd.offset = offset;
        cmd.size = size;
        cmd.stages = stages;
        memcpy(cmd.data, data, size);
        state.drawThread->Push(cmd);
    }
    else
    {
        vkCmdPushConstants(GetMainBuffer(CoreGraphics::GraphicsQueueType), layout, stages, offset, size, data);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
BindGraphicsPipelineInfo(const VkGraphicsPipelineCreateInfo& shader, const CoreGraphics::ShaderProgramId programId)
{
    if (state.currentShaderPrograms[CoreGraphics::GraphicsQueueType] != programId || !AllBits(state.currentPipelineBits, PipelineBuildBits::ShaderInfoSet))
    {
        state.database.SetShader(programId, shader);
        state.currentPipelineBits |= PipelineBuildBits::ShaderInfoSet;

        state.blendInfo.pAttachments = shader.pColorBlendState->pAttachments;
        memcpy(state.blendInfo.blendConstants, shader.pColorBlendState->blendConstants, sizeof(float) * 4);
        state.blendInfo.logicOp = shader.pColorBlendState->logicOp;
        state.blendInfo.logicOpEnable = shader.pColorBlendState->logicOpEnable;

        state.currentPipelineInfo.pDepthStencilState = shader.pDepthStencilState;
        state.currentPipelineInfo.pRasterizationState = shader.pRasterizationState;
        state.currentPipelineInfo.pMultisampleState = shader.pMultisampleState;
        state.currentPipelineInfo.pDynamicState = shader.pDynamicState;
        state.currentPipelineInfo.pTessellationState = shader.pTessellationState;
        state.currentPipelineInfo.stageCount = shader.stageCount;
        state.currentPipelineInfo.pStages = shader.pStages;
        state.currentPipelineInfo.layout = shader.layout;
        state.currentPipelineBits &= ~PipelineBuildBits::PipelineBuilt;
        state.currentShaderPrograms[CoreGraphics::GraphicsQueueType] = programId;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SetVertexLayoutPipelineInfo(VkPipelineVertexInputStateCreateInfo* vertexLayout)
{
    if (state.currentPipelineInfo.pVertexInputState != vertexLayout || !AllBits(state.currentPipelineBits, PipelineBuildBits::VertexLayoutInfoSet))
    {
        state.database.SetVertexLayout(vertexLayout);
        state.currentPipelineBits |= PipelineBuildBits::VertexLayoutInfoSet;
        state.currentPipelineInfo.pVertexInputState = vertexLayout;

        state.currentPipelineBits &= ~PipelineBuildBits::PipelineBuilt;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SetFramebufferLayoutInfo(const VkGraphicsPipelineCreateInfo& framebufferLayout)
{
    state.currentPipelineBits |= PipelineBuildBits::FramebufferLayoutInfoSet;
    state.currentPipelineInfo.renderPass = framebufferLayout.renderPass;
    state.currentPipelineInfo.subpass = framebufferLayout.subpass;
    state.currentPipelineInfo.pViewportState = framebufferLayout.pViewportState;
    state.currentPipelineBits &= ~PipelineBuildBits::PipelineBuilt;
}

//------------------------------------------------------------------------------
/**
*/
void 
SetInputLayoutInfo(VkPipelineInputAssemblyStateCreateInfo* inputLayout)
{
    if (state.currentPipelineInfo.pInputAssemblyState != inputLayout || !AllBits(state.currentPipelineBits, PipelineBuildBits::InputLayoutInfoSet))
    {
        state.database.SetInputLayout(inputLayout);
        state.currentPipelineBits |= PipelineBuildBits::InputLayoutInfoSet;
        state.currentPipelineInfo.pInputAssemblyState = inputLayout;
        state.currentPipelineBits &= ~PipelineBuildBits::PipelineBuilt;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
CreateAndBindGraphicsPipeline()
{
    VkPipeline pipeline = state.database.GetCompiledPipeline();
    _incr_counter(state.NumPipelinesBuilt, 1);

    if (state.drawThread)
    { 
        n_assert(state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId)

        // send pipeline bind command, this is the first step in our procedure, so we use this as a trigger to switch threads
        VkCommandBufferThread::VkGfxPipelineBindCommand pipeCommand;
        pipeCommand.pipeline = pipeline;
        pipeCommand.layout = state.currentGraphicsPipelineLayout;
        state.drawThread->Push(pipeCommand);

        // update stencil stuff
        VkCommandBufferThread::VkStencilRefCommand stencilRefCommand;
        stencilRefCommand.frontRef = state.currentStencilFrontRef;
        stencilRefCommand.backRef = state.currentStencilBackRef;
        state.drawThread->Push(stencilRefCommand);

        VkCommandBufferThread::VkStencilReadMaskCommand stencilReadMaskCommand;
        stencilReadMaskCommand.mask = state.currentStencilReadMask;
        state.drawThread->Push(stencilReadMaskCommand);

        VkCommandBufferThread::VkStencilWriteMaskCommand stencilWriteMaskCommand;
        stencilWriteMaskCommand.mask = state.currentStencilWriteMask;
        state.drawThread->Push(stencilWriteMaskCommand);

        // bind textures and camera descriptors
        CoreGraphics::SetResourceTable(state.tickResourceTable, NEBULA_TICK_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
        CoreGraphics::SetResourceTable(state.frameResourceTable, NEBULA_FRAME_GROUP, CoreGraphics::GraphicsPipeline, nullptr);

        // push propagation descriptors
        for (IndexT i = 0; i < state.propagateDescriptorSets.Size(); i++)
            if (state.propagateDescriptorSets[i].baseSet != -1)
                state.drawThread->Push(state.propagateDescriptorSets[i]);

        // push viewport and scissor arrays
        state.drawThread->Push(state.scissorArrayCommand);
        state.drawThread->Push(state.viewportArrayCommand);
    }
    else
    {
        // push propagation descriptors
        for (IndexT i = 0; i < state.propagateDescriptorSets.Size(); i++)
            if (state.propagateDescriptorSets[i].baseSet != -1)
                vkCmdBindDescriptorSets(
                    GetMainBuffer(CoreGraphics::GraphicsQueueType),
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    state.currentGraphicsPipelineLayout,
                    state.propagateDescriptorSets[i].baseSet,
                    state.propagateDescriptorSets[i].numSets,
                    state.propagateDescriptorSets[i].sets,
                    state.propagateDescriptorSets[i].numOffsets,
                    state.propagateDescriptorSets[i].offsets);

        // bind pipeline
        vkCmdBindPipeline(GetMainBuffer(CoreGraphics::GraphicsQueueType), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        // set viewport stuff
        vkCmdSetViewport(GetMainBuffer(CoreGraphics::GraphicsQueueType), 0, state.viewportArrayCommand.num, state.viewportArrayCommand.vps);
        vkCmdSetScissor(GetMainBuffer(CoreGraphics::GraphicsQueueType), 0, state.scissorArrayCommand.num, state.scissorArrayCommand.scs);

        // set stencil stuff
        vkCmdSetStencilCompareMask(GetMainBuffer(CoreGraphics::GraphicsQueueType), VK_STENCIL_FACE_FRONT_AND_BACK, state.currentStencilReadMask);
        vkCmdSetStencilWriteMask(GetMainBuffer(CoreGraphics::GraphicsQueueType), VK_STENCIL_FACE_FRONT_AND_BACK, state.currentStencilWriteMask);
        vkCmdSetStencilReference(GetMainBuffer(CoreGraphics::GraphicsQueueType), VK_STENCIL_FACE_FRONT_BIT, state.currentStencilFrontRef);
        vkCmdSetStencilReference(GetMainBuffer(CoreGraphics::GraphicsQueueType), VK_STENCIL_FACE_BACK_BIT, state.currentStencilBackRef);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
BindComputePipeline(const VkPipeline& pipeline, const CoreGraphics::QueueType queue)
{

}

//------------------------------------------------------------------------------
/**
*/
void 
UnbindPipeline()
{
    state.currentBindPoint = CoreGraphics::InvalidPipeline;
    state.currentPipelineBits &= ~PipelineBuildBits::ShaderInfoSet;
}

//------------------------------------------------------------------------------
/**
*/
void
SetVkViewports(VkViewport* viewports, SizeT num)
{
    VkCommandBufferThread::VkViewportArrayCommand& cmd = state.viewportArrayCommand;
    cmd.first = 0;
    cmd.num = num;
    cmd.vps = viewports;
    if (state.currentShaderPrograms[CoreGraphics::GraphicsQueueType] != CoreGraphics::InvalidShaderProgramId)
    {
        if (state.drawThread)
        {
            if (state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId)
            {

                state.drawThread->Push(cmd);
            }
        }
        else
        {
            // activate this code when we have main thread secondary buffers
            vkCmdSetViewport(GetMainBuffer(CoreGraphics::GraphicsQueueType), 0, num, viewports);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SetVkScissorRects(VkRect2D* scissors, SizeT num)
{
    VkCommandBufferThread::VkScissorRectArrayCommand& cmd = state.scissorArrayCommand;
    cmd.first = 0;
    cmd.num = num;
    cmd.scs = scissors;

    if (state.currentShaderPrograms[CoreGraphics::GraphicsQueueType] != CoreGraphics::InvalidShaderProgramId)
    {
        if (state.drawThread)
        {
            if (state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId)
            {
                state.drawThread->Push(cmd);
            }
        }
        else
        {
            // activate this code when we have main thread secondary buffers
            vkCmdSetScissor(GetMainBuffer(CoreGraphics::GraphicsQueueType), 0, num, scissors);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SparseTextureBind(const VkImage img, const Util::Array<VkSparseMemoryBind>& opaqueBinds, const Util::Array<VkSparseImageMemoryBind>& pageBinds)
{
    state.subcontextHandler.AppendSparseBind(CoreGraphics::SparseQueueType, img, opaqueBinds, pageBinds);
    state.sparseSubmitActive = true;
    state.sparseWaitHandled = false;
}

#if NEBULA_GRAPHICS_DEBUG
//------------------------------------------------------------------------------
/**
*/
void 
CommandBufferBeginMarker(VkCommandBuffer buf, const Math::vec4& color, const char* name)
{
    alignas(16) float col[4];
    color.store(col);
    VkDebugUtilsLabelEXT info =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        nullptr,
        name,
        { col[0], col[1], col[2], col[3] }
    };
    VkCmdDebugMarkerBegin(buf, &info);
}

//------------------------------------------------------------------------------
/**
*/
void 
CommandBufferEndMarker(VkCommandBuffer buf)
{
    VkCmdDebugMarkerEnd(buf);
}

//------------------------------------------------------------------------------
/**
*/
void 
DelayedDeleteBuffer(const VkBuffer buf)
{
    state.delayedDeleteBuffers[state.currentBufferedFrameIndex].Append(buf);
}

//------------------------------------------------------------------------------
/**
*/
void 
DelayedDeleteImage(const VkImage img)
{
    state.delayedDeleteImages[state.currentBufferedFrameIndex].Append(img);
}

//------------------------------------------------------------------------------
/**
*/
void 
DelayedDeleteImageView(const VkImageView view)
{
    state.delayedDeleteImageViews[state.currentBufferedFrameIndex].Append(view);
}

//------------------------------------------------------------------------------
/**
*/
void 
DelayedFreeMemory(const CoreGraphics::Alloc alloc)
{
    state.delayedFreeMemories[state.currentBufferedFrameIndex].Append(alloc);
}

//------------------------------------------------------------------------------
/**
*/
void 
_ProcessQueriesBeginFrame()
{
    N_SCOPE(ProcessQueries, Render);
    using namespace CoreGraphics;

    SubmissionContextNextCycle(state.queryGraphicsSubmissionContext, nullptr);
    SubmissionContextNextCycle(state.queryComputeSubmissionContext, nullptr);

    // start query queue for graphics
    CommandBufferBeginInfo beginInfo{ true, false, false };
    SubmissionContextNewBuffer(state.queryGraphicsSubmissionContext, state.queryGraphicsSubmissionCmdBuffer);
    CommandBufferBeginRecord(state.queryGraphicsSubmissionCmdBuffer, beginInfo);

    // start query queue for compute
    SubmissionContextNewBuffer(state.queryComputeSubmissionContext, state.queryComputeSubmissionCmdBuffer);
    CommandBufferBeginRecord(state.queryComputeSubmissionCmdBuffer, beginInfo);

    bool submitGraphics = false, submitCompute = false;

    // copy queries and reset pools based on type
    for (IndexT i = 0; i < CoreGraphics::NumQueryTypes; i++)
    {
        QueueType queue = (i <= QueryType::QueryGraphicsMax) ? GraphicsQueueType : ComputeQueueType;

        // get command buffer
        VkCommandBuffer buf;

        if (queue == GraphicsQueueType)
        {
            buf = CommandBufferGetVk(state.queryGraphicsSubmissionCmdBuffer);
            submitGraphics = true;
        }
        else
        {
            buf = CommandBufferGetVk(state.queryComputeSubmissionCmdBuffer);
            submitCompute = true;
        }

        // if the query index is offset
        if (state.queryCountsByType[i][state.currentBufferedFrameIndex] > 0)
        {
            // pipeline statistics produce 2 integers
            VkDeviceSize stride = (i == CoreGraphics::PipelineStatisticsComputeQuery || i == CoreGraphics::PipelineStatisticsGraphicsQuery) ? 16 : 8;

            // make sure our buffer is finished
            VkBufferMemoryBarrier barr =
            {
                VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                nullptr,
                VK_ACCESS_HOST_READ_BIT,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                Vulkan::BufferGetVk(state.queryResultBuffers[i]),
                state.queryStartIndexByType[i][state.currentBufferedFrameIndex] * sizeof(uint64_t),
                state.queryCountsByType[i][state.currentBufferedFrameIndex] * sizeof(uint64_t)
            };

            vkCmdPipelineBarrier(
                buf, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                1, &barr,
                0, nullptr
            );

            // issue copy of results to buffer
            vkCmdCopyQueryPoolResults(
                buf,
                state.queryPoolsByType[i],
                state.queryStartIndexByType[i][state.currentBufferedFrameIndex],
                state.queryCountsByType[i][state.currentBufferedFrameIndex],
                Vulkan::BufferGetVk(state.queryResultBuffers[i]),
                state.queryStartIndexByType[i][state.currentBufferedFrameIndex] * sizeof(uint64_t),
                stride,
                VK_QUERY_RESULT_64_BIT);

            barr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barr.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
            vkCmdPipelineBarrier(
                buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                0,
                0, nullptr,
                1, &barr,
                0, nullptr
            );

            vkCmdResetQueryPool(buf, state.queryPoolsByType[i], state.queryStartIndexByType[i][state.currentBufferedFrameIndex], state.queryCountsByType[i][state.currentBufferedFrameIndex]);

            // reset query count since we copied them over on this submission
            state.queryCountsByType[i][state.currentBufferedFrameIndex] = 0;
        }
    }

#if NEBULA_ENABLE_PROFILING
    // write beginning of frame timestamps
    Vulkan::__Timestamp(state.queryGraphicsSubmissionCmdBuffer, GraphicsQueueType, CoreGraphics::BarrierStage::Top, "BeginFrameGraphics");
    Vulkan::__Timestamp(state.queryComputeSubmissionCmdBuffer, ComputeQueueType, CoreGraphics::BarrierStage::Top, "BeginFrameCompute");
#endif  

    // append submission
    if (submitGraphics)
        state.subcontextHandler.AppendSubmissionTimeline(GraphicsQueueType, CommandBufferGetVk(state.queryGraphicsSubmissionCmdBuffer), false);
    if (submitCompute)
        state.subcontextHandler.AppendSubmissionTimeline(ComputeQueueType, CommandBufferGetVk(state.queryComputeSubmissionCmdBuffer), false);

    // end record of query reset commands
    CommandBufferEndRecord(state.queryGraphicsSubmissionCmdBuffer);
    CommandBufferEndRecord(state.queryComputeSubmissionCmdBuffer);
}

//------------------------------------------------------------------------------
/**
*/
void
_ProcessProfilingMarkersRecursive(CoreGraphics::FrameProfilingMarker& marker, CoreGraphics::QueryType type)
{
    //state.queryResultsByType[type]
    uint64_t* buffer = ((uint64_t*)CoreGraphics::BufferMap(state.queryResultBuffers[type]));
    const CoreGraphics::Query& query = type == CoreGraphics::QueryType::GraphicsTimestampQuery ? state.queries[state.queriesRingOffset] : state.queries[state.queriesRingOffset+1];
    uint64_t beginFrame = buffer[state.queriesRingOffset];
    uint64_t time1 = buffer[marker.gpuBegin];
    uint64_t time2 = buffer[marker.gpuEnd];
    marker.start = (time1 - beginFrame) * state.deviceProps[state.currentDevice].limits.timestampPeriod - query.cpuTime;
    marker.duration = (time2 - time1) * state.deviceProps[state.currentDevice].limits.timestampPeriod;

    for (IndexT i = 0; i < marker.children.Size(); i++)
    {
        _ProcessProfilingMarkersRecursive(marker.children[i], type);
    }
    CoreGraphics::BufferUnmap(state.queryResultBuffers[type]);
}

//------------------------------------------------------------------------------
/**
*/
void 
_ProcessQueriesEndFrame()
{
    using namespace CoreGraphics;

#if NEBULA_ENABLE_PROFILING
    // clear this frames markers
    state.frameProfilingMarkers.Clear();
#endif

    for (IndexT i = 0; i < CoreGraphics::QueryType::NumQueryTypes; i++)
    {
        // invalidate buffer for readback
        CoreGraphics::BufferInvalidate(state.queryResultBuffers[i]);
    }

    for (IndexT i = 0; i < state.profilingMarkersPerFrame[state.currentBufferedFrameIndex].Size(); i++)
    {
        CoreGraphics::FrameProfilingMarker marker = state.profilingMarkersPerFrame[state.currentBufferedFrameIndex][i];
        CoreGraphics::QueryType type = marker.queue == CoreGraphics::GraphicsQueueType ? CoreGraphics::GraphicsTimestampQuery : CoreGraphics::ComputeTimestampQuery;

        _ProcessProfilingMarkersRecursive(marker, type);
        state.frameProfilingMarkers.Append(marker);
    }

    // reset used queries and ring offset
    state.numUsedQueries = 0;
    state.profilingMarkersPerFrame[state.currentBufferedFrameIndex].Reset();
}

//------------------------------------------------------------------------------
/**
*/
void 
__Timestamp(CoreGraphics::CommandBufferId buf, CoreGraphics::QueueType queue, const CoreGraphics::BarrierStage stage, const char* name)
{
    // convert to vulkan flags, force bits set to only be 1
    VkPipelineStageFlags flags = VkTypes::AsVkPipelineFlags(stage);
    n_assert(Util::CountBits(flags) == 1);

    // chose query based on queue
    CoreGraphics::QueryType type = (queue == CoreGraphics::GraphicsQueueType) ? CoreGraphics::GraphicsTimestampQuery : CoreGraphics::ComputeTimestampQuery;

    // get current query, and get the index
    SizeT& count = state.queryCountsByType[type][state.currentBufferedFrameIndex];
    IndexT idx = state.queryStartIndexByType[type][state.currentBufferedFrameIndex] + count;
    count++;
    n_assert(idx < state.MaxQueriesPerFrame * (SizeT)(state.currentBufferedFrameIndex + 1));

    CoreGraphics::Query query;
    query.type = type;
    query.idx = idx;
    query.cpuTime = Profiling::ProfilingGetTime();
    state.queries[state.queriesRingOffset + state.numUsedQueries++] = query;

    // write time stamp
    n_assert(state.drawThreadCommands == CoreGraphics::InvalidCommandBufferId);
    vkCmdWriteTimestamp(CommandBufferGetVk(buf), (VkPipelineStageFlagBits)flags, state.queryPoolsByType[type], idx);
}
#endif

} // namespace Vulkan

#include "debug/stacktrace.h"

//------------------------------------------------------------------------------
/**
*/
VKAPI_ATTR VkBool32 VKAPI_CALL
NebulaVulkanErrorDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    const int ignore[] =
    {
        307231540,
        -90772850,
        -564812795,
        1345731725,
        1303270965
    };

    /*
    for (IndexT i = 0; i < sizeof(ignore) / sizeof(int); i++)
    {
        if (callbackData->messageIdNumber == ignore[i])
            return VK_FALSE;
    }
    */

    n_warning("%s\n", callbackData->pMessage);
    return VK_FALSE;
}

namespace CoreGraphics
{
using namespace Vulkan;

#if NEBULA_GRAPHICS_DEBUG
template<> void ObjectSetName(const CoreGraphics::CommandBufferId id, const char* name);
#endif

//------------------------------------------------------------------------------
/**
*/
bool
CreateGraphicsDevice(const GraphicsDeviceCreateInfo& info)
{
    DisplayDevice* displayDevice = DisplayDevice::Instance();
    n_assert(displayDevice->IsOpen());

    state.enableValidation = info.enableValidation;

    // create result
    VkResult res;

    // setup application
    VkApplicationInfo appInfo =
    {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        App::Application::Instance()->GetAppTitle().AsCharPtr(),
        2,															// application version
        "Nebula",													// engine name
        4,															// engine version
        VK_API_VERSION_1_2											// API version
    };

    state.usedExtensions = 0;
    uint32_t requiredExtensionsNum;
    const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsNum);
    uint32_t i;
    for (i = 0; i < (uint32_t)requiredExtensionsNum; i++)
    {
        state.extensions[state.usedExtensions++] = requiredExtensions[i];
    }

    const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
    int numLayers = 0;
    const char** usedLayers = nullptr;

#if NEBULA_GRAPHICS_DEBUG
    if (info.enableValidation)
    {
        usedLayers = &layers[0];
        numLayers = 1;
    }
    else
    {
        // don't use any layers, but still load the debug utils so we can put markers
        numLayers = 0;
    }
    state.extensions[state.usedExtensions++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#else
    if (info.enableValidation)
    {
        usedLayers = &layers[0];
        numLayers = 1;
        state.extensions[state.usedExtensions++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }
#endif

    // load layers
    Vulkan::InitVulkan();

    // setup instance
    VkInstanceCreateInfo instanceInfo =
    {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,		// type of struct
        nullptr,										// pointer to next
        0,											// flags
        &appInfo,									// application
        (uint32_t)numLayers,
        usedLayers,
        state.usedExtensions,
        state.extensions
    };

    // create instance
    res = vkCreateInstance(&instanceInfo, nullptr, &state.instance);
    if (res == VK_ERROR_INCOMPATIBLE_DRIVER)
    {
        n_error("Your GPU driver is not compatible with Vulkan.\n");
    }
    else if (res == VK_ERROR_EXTENSION_NOT_PRESENT)
    {
        n_error("Vulkan extension failed to load.\n");
    }
    else if (res == VK_ERROR_LAYER_NOT_PRESENT)
    {
        n_error("Vulkan layer failed to load.\n");
    }
    n_assert(res == VK_SUCCESS);

    // load instance functions
    Vulkan::InitInstance(state.instance);

    // setup adapter
    SetupAdapter();
    state.currentDevice = 0;

#if NEBULA_GRAPHICS_DEBUG
#else
    if (info.enableValidation)
#endif
    {
        VkCreateDebugMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(state.instance, "vkCreateDebugUtilsMessengerEXT");
        VkDestroyDebugMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(state.instance, "vkDestroyDebugUtilsMessengerEXT");
        VkDebugUtilsMessengerCreateInfoEXT dbgInfo;
        dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        dbgInfo.flags = 0;
        dbgInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        dbgInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dbgInfo.pNext = nullptr;
        dbgInfo.pfnUserCallback = NebulaVulkanErrorDebugCallback;
        dbgInfo.pUserData = nullptr;
        res = VkCreateDebugMessenger(state.instance, &dbgInfo, NULL, &VkErrorDebugMessageHandle);
        n_assert(res == VK_SUCCESS);
    }

#if NEBULA_GRAPHICS_DEBUG
    VkDebugObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(state.instance, "vkSetDebugUtilsObjectNameEXT");
    VkDebugObjectTag = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetInstanceProcAddr(state.instance, "vkSetDebugUtilsObjectTagEXT");
    VkQueueBeginLabel = (PFN_vkQueueBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(state.instance, "vkQueueBeginDebugUtilsLabelEXT");
    VkQueueEndLabel = (PFN_vkQueueEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(state.instance, "vkQueueEndDebugUtilsLabelEXT");
    VkQueueInsertLabel = (PFN_vkQueueInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(state.instance, "vkQueueInsertDebugUtilsLabelEXT");
    VkCmdDebugMarkerBegin = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(state.instance, "vkCmdBeginDebugUtilsLabelEXT");
    VkCmdDebugMarkerEnd = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(state.instance, "vkCmdEndDebugUtilsLabelEXT");
    VkCmdDebugMarkerInsert = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(state.instance, "vkCmdInsertDebugUtilsLabelEXT");
#endif

    uint32_t numQueues;
    vkGetPhysicalDeviceQueueFamilyProperties(state.physicalDevices[state.currentDevice], &numQueues, NULL);
    n_assert(numQueues > 0);

    // now get queues from device
    VkQueueFamilyProperties* queuesProps = n_new_array(VkQueueFamilyProperties, numQueues);
    vkGetPhysicalDeviceQueueFamilyProperties(state.physicalDevices[state.currentDevice], &numQueues, queuesProps);
    vkGetPhysicalDeviceMemoryProperties(state.physicalDevices[state.currentDevice], &state.memoryProps);

    state.drawQueueIdx = UINT32_MAX;
    state.computeQueueIdx = UINT32_MAX;
    state.transferQueueIdx = UINT32_MAX;
    state.sparseQueueIdx = UINT32_MAX;

    // create three queues for each family
    Util::FixedArray<uint> indexMap;
    indexMap.Resize(numQueues);
    indexMap.Fill(0);
    for (i = 0; i < numQueues; i++)
    {
        for (uint32_t j = 0; j < queuesProps[i].queueCount; j++)
        {
            // just pick whichever queue supports graphics, it will most likely only be 1
            if (AllBits(queuesProps[i].queueFlags, VK_QUEUE_GRAPHICS_BIT)
                && state.drawQueueIdx == UINT32_MAX)
            {
                state.drawQueueFamily = i;
                state.drawQueueIdx = j;
                indexMap[i]++;
                continue;
            }

            // find a compute queue which is not for graphics
            if (AllBits(queuesProps[i].queueFlags, VK_QUEUE_COMPUTE_BIT)
                && state.computeQueueIdx == UINT32_MAX)
            {
                state.computeQueueFamily = i;
                state.computeQueueIdx = j;
                indexMap[i]++;
                continue;
            }

            // find a transfer queue that is purely for transfers
            if (AllBits(queuesProps[i].queueFlags, VK_QUEUE_TRANSFER_BIT)
                && state.transferQueueIdx == UINT32_MAX)
            {
                state.transferQueueFamily = i;
                state.transferQueueIdx = j;
                indexMap[i]++;
                continue;
            }

            // find a sparse or transfer queue that supports sparse binding
            if (AllBits(queuesProps[i].queueFlags, VK_QUEUE_SPARSE_BINDING_BIT)
                && state.sparseQueueIdx == UINT32_MAX)
            {
                state.sparseQueueFamily = i;
                state.sparseQueueIdx = j;
                indexMap[i]++;
                continue;
            }
        }
    }

    // could not find pure compute queue
    if (state.computeQueueIdx == UINT32_MAX)
    {
        // assert that the graphics queue can handle computes
        n_assert(queuesProps[state.drawQueueFamily].queueFlags& VK_QUEUE_COMPUTE_BIT);
        state.computeQueueFamily = state.drawQueueFamily;
        state.computeQueueIdx = state.drawQueueIdx;
    }

    // could not find pure transfer queue
    if (state.transferQueueIdx == UINT32_MAX)
    {
        // assert the draw queue can handle transfers
        n_assert(queuesProps[state.drawQueueFamily].queueFlags & VK_QUEUE_TRANSFER_BIT);
        state.transferQueueFamily = state.drawQueueFamily;
        state.transferQueueIdx = state.drawQueueIdx;
    }

    // could not find pure transfer queue
    if (state.sparseQueueIdx == UINT32_MAX)
    {
        if (queuesProps[state.transferQueueFamily].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
        {
            state.sparseQueueFamily = state.transferQueueFamily;

            // if we have an extra transfer queue, use it for sparse bindings
            if (queuesProps[state.sparseQueueFamily].queueCount > indexMap[state.sparseQueueFamily])
                state.sparseQueueIdx = indexMap[state.sparseQueueFamily]++;
            else
                state.sparseQueueIdx = state.transferQueueIdx;
        }
        else
        {
            n_warn2(queuesProps[state.drawQueueFamily].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT, "VkGraphicsDevice: No sparse binding queue could be found!\n");
            state.sparseQueueFamily = state.drawQueueFamily;
            state.sparseQueueIdx = state.drawQueueIdx;
        }
    }

    if (state.drawQueueFamily == UINT32_MAX)		n_error("VkGraphicsDevice: Could not find a queue for graphics and present.\n");
    if (state.computeQueueFamily == UINT32_MAX)		n_error("VkGraphicsDevice: Could not find a queue for compute.\n");
    if (state.transferQueueFamily == UINT32_MAX)	n_error("VkGraphicsDevice: Could not find a queue for transfers.\n");
    if (state.sparseQueueFamily == UINT32_MAX)		n_warning("VkGraphicsDevice: Could not find a queue for sparse binding.\n");

    // create device
    Util::FixedArray<Util::FixedArray<float>> prios;
    Util::Array<VkDeviceQueueCreateInfo> queueInfos;
    prios.Resize(numQueues);

    for (i = 0; i < numQueues; i++)
    {
        if (indexMap[i] == 0) continue;
        prios[i].Resize(indexMap[i]);
        prios[i].Fill(1.0f);
        queueInfos.Append(
            {
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                nullptr,
                0,
                i,
                indexMap[i],
                &prios[i][0]
            });
    }

    n_delete_array(queuesProps);

    // get physical device features
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(state.physicalDevices[state.currentDevice], &features);


    VkPhysicalDeviceHostQueryResetFeatures hostQueryReset =
    {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES,
        nullptr,
        true
    };

    VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphores =
    {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
        &hostQueryReset,
        true
    };

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures =
    {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
        &timelineSemaphores
    };
    descriptorIndexingFeatures.descriptorBindingPartiallyBound = true;

    VkDeviceCreateInfo deviceInfo =
    {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        &descriptorIndexingFeatures,
        0,
        (uint32_t)queueInfos.Size(),
        &queueInfos[0],
        (uint32_t)numLayers,
        layers,
        state.numCaps[state.currentDevice],
        state.deviceFeatureStrings[state.currentDevice].Begin(),
        &features
    };

    // create device
    res = vkCreateDevice(state.physicalDevices[state.currentDevice], &deviceInfo, NULL, &state.devices[state.currentDevice]);
    n_assert(res == VK_SUCCESS);

    // setup queue handler
    Util::FixedArray<uint> families(4);
    families[GraphicsQueueType] = state.drawQueueFamily;
    families[ComputeQueueType] = state.computeQueueFamily;
    families[TransferQueueType] = state.transferQueueFamily;
    families[SparseQueueType] = state.sparseQueueFamily;
    state.subcontextHandler.Setup(state.devices[state.currentDevice], indexMap, families);

    state.usedQueueFamilies.Add(state.drawQueueFamily);
    state.usedQueueFamilies.Add(state.computeQueueFamily);
    state.usedQueueFamilies.Add(state.transferQueueFamily);
    state.usedQueueFamilies.Add(state.sparseQueueFamily);
    state.queueFamilyMap.Resize(NumQueueTypes);
    state.queueFamilyMap[GraphicsQueueType] = state.drawQueueFamily;
    state.queueFamilyMap[ComputeQueueType] = state.computeQueueFamily;
    state.queueFamilyMap[TransferQueueType] = state.transferQueueFamily;
    state.queueFamilyMap[SparseQueueType] = state.sparseQueueFamily;

    VkPipelineCacheCreateInfo cacheInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        nullptr,
        0,
        0,
        nullptr
    };

    // create cache
    res = vkCreatePipelineCache(state.devices[state.currentDevice], &cacheInfo, NULL, &state.cache);
    n_assert(res == VK_SUCCESS);

    // setup our own pipeline database
    state.database.Setup(state.devices[state.currentDevice], state.cache);

    // setup the empty descriptor set
    SetupEmptyDescriptorSetLayout();

    // setup memory pools
    SetupMemoryPools(
        info.memoryHeaps[MemoryPool_DeviceLocal],
        info.memoryHeaps[MemoryPool_HostLocal],
        info.memoryHeaps[MemoryPool_HostToDevice],
        info.memoryHeaps[MemoryPool_DeviceToHost]
        );

    state.constantBufferRings.Resize(info.numBufferedFrames);

#ifdef CreateSemaphore
#pragma push_macro("CreateSemaphore")
#undef CreateSemaphore
#endif

    for (i = 0; i < info.numBufferedFrames; i++)
    {
        Vulkan::GraphicsDeviceState::ConstantsRingBuffer& cboRing = state.constantBufferRings[i];

        IndexT j;
        for (j = 0; j < NumConstantBufferTypes; j++)
        {
            cboRing.cboComputeStartAddress[j] = cboRing.cboComputeEndAddress[j] = 0;
            cboRing.cboGfxStartAddress[j] = cboRing.cboGfxEndAddress[j] = 0;
        }
    }

    for (i = 0; i < NumConstantBufferTypes; i++)
    {
        static const Util::String threadName[] = { "Main Thread ", "Visibility Thread " };
        static const Util::String systemName[] = { "Staging ", "Device " };
        static const Util::String queueName[] = { "Graphics Constant Buffer", "Compute Constant Buffer" };
        BufferCreateInfo cboInfo;

        
        cboInfo.byteSize = info.globalGraphicsConstantBufferMemorySize[i] * info.numBufferedFrames;
        state.globalGraphicsConstantBufferMaxValue[i] = info.globalGraphicsConstantBufferMemorySize[i];
        if (cboInfo.byteSize > 0)
        {
            cboInfo.name = systemName[0] + threadName[i] + queueName[0];
            cboInfo.mode = CoreGraphics::BufferAccessMode::HostLocal;
            cboInfo.usageFlags = CoreGraphics::TransferBufferSource;
            state.globalGraphicsConstantStagingBuffer[i] = CreateBuffer(cboInfo);

            cboInfo.name = systemName[1] + threadName[i] + queueName[0];
            cboInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
            cboInfo.usageFlags = CoreGraphics::TransferBufferDestination | CoreGraphics::ConstantBuffer;
            state.globalGraphicsConstantBuffer[i] = CreateBuffer(cboInfo);
        }

        cboInfo.byteSize = info.globalComputeConstantBufferMemorySize[i] * info.numBufferedFrames;
        state.globalComputeConstantBufferMaxValue[i] = info.globalComputeConstantBufferMemorySize[i];
        if (cboInfo.byteSize > 0)
        {
            cboInfo.name = systemName[0] + threadName[i] + queueName[1];
            cboInfo.mode = CoreGraphics::BufferAccessMode::HostLocal;
            cboInfo.usageFlags = CoreGraphics::TransferBufferSource;
            state.globalComputeConstantStagingBuffer[i] = CreateBuffer(cboInfo);

            cboInfo.name = systemName[1] + threadName[i] + queueName[1];
            cboInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
            cboInfo.usageFlags = CoreGraphics::TransferBufferDestination | CoreGraphics::ConstantBuffer;
            state.globalComputeConstantBuffer[i] = CreateBuffer(cboInfo);
        }
    }

    state.submissionTransferGraphicsHandoverCmdPool = CreateCommandBufferPool({CoreGraphics::GraphicsQueueType, false, true});
    state.submissionTransferCmdPool = CreateCommandBufferPool({CoreGraphics::TransferQueueType, false, true});
    state.submissionGraphicsCmdPool = CreateCommandBufferPool({CoreGraphics::GraphicsQueueType, false, true});
    state.submissionComputeCmdPool = CreateCommandBufferPool({CoreGraphics::ComputeQueueType, false, true});
    CommandBufferCreateInfo cmdCreateInfo =
    {
        false,
        state.submissionGraphicsCmdPool
    };

    // setup main submission context (Graphics)
    state.gfxSubmission = CreateSubmissionContext({ cmdCreateInfo, info.numBufferedFrames, false });
    state.gfxCmdBuffer = InvalidCommandBufferId;

    // setup compute submission context
    cmdCreateInfo.pool = state.submissionComputeCmdPool;
    state.computeSubmission = CreateSubmissionContext({ cmdCreateInfo, info.numBufferedFrames, false });
    state.computeCmdBuffer = InvalidCommandBufferId;

    CommandBufferBeginInfo beginInfo{ true, false, false };

    // create transfer submission context
    cmdCreateInfo.pool = state.submissionTransferCmdPool;
    state.resourceSubmissionContext = CreateSubmissionContext({ cmdCreateInfo, info.numBufferedFrames, true });
    state.resourceSubmissionActive = false;

    cmdCreateInfo.pool = state.submissionTransferGraphicsHandoverCmdPool;
    state.handoverSubmissionContext = CreateSubmissionContext({ cmdCreateInfo, info.numBufferedFrames, true });
    state.handoverSubmissionActive = false;

    state.sparseSubmitActive = false;
    state.sparseWaitHandled = true;

    // create main-queue setup submission context (forced to be beginning of frame when relevant)
    cmdCreateInfo.pool = state.submissionGraphicsCmdPool;
    state.setupSubmissionContext = CreateSubmissionContext({ cmdCreateInfo, info.numBufferedFrames, false });
    state.setupSubmissionActive = false;

    // setup the special submission context for graphics queries
    state.queryGraphicsSubmissionContext = CreateSubmissionContext({ cmdCreateInfo, info.numBufferedFrames, false });

    // setup special submission context for compute queries
    cmdCreateInfo.pool = state.submissionComputeCmdPool;
    state.queryComputeSubmissionContext = CreateSubmissionContext({ cmdCreateInfo, info.numBufferedFrames, false });

    state.propagateDescriptorSets.Resize(NEBULA_NUM_GROUPS);
    for (i = 0; i < NEBULA_NUM_GROUPS; i++)
    {
        state.propagateDescriptorSets[i].baseSet = -1;
    }

    state.drawThread = nullptr;

#pragma pop_macro("CreateSemaphore")

    state.maxNumBufferedFrames = info.numBufferedFrames;

    state.delayedDeleteBuffers.Resize(state.maxNumBufferedFrames);
    state.delayedDeleteImages.Resize(state.maxNumBufferedFrames);
    state.delayedDeleteImageViews.Resize(state.maxNumBufferedFrames);
    state.delayedFreeMemories.Resize(state.maxNumBufferedFrames);

#ifdef CreateSemaphore
#pragma push_macro("CreateSemaphore")
#undef CreateSemaphore
#endif

    state.presentFences.Resize(info.numBufferedFrames);
    state.renderingFinishedSemaphores.Resize(info.numBufferedFrames);
    for (i = 0; i < info.numBufferedFrames; i++)
    {
        state.presentFences[i] = CreateFence({true});
        state.renderingFinishedSemaphores[i] = CreateSemaphore({ SemaphoreType::Binary });
    }

#pragma pop_macro("CreateSemaphore")

    state.waitForPresentSemaphore = VK_NULL_HANDLE;
    state.mainSubmitQueue = CoreGraphics::GraphicsQueueType; // main queue to submit is on graphics

    state.passInfo =
    {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
        nullptr,
        0,
    };

    // setup pipeline construction struct
    state.currentPipelineInfo.pNext = nullptr;
    state.currentPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    state.currentPipelineInfo.flags = 0;
    state.currentPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    state.currentPipelineInfo.basePipelineIndex = -1;
    state.currentPipelineInfo.pColorBlendState = &state.blendInfo;
    state.currentPipelineInfo.pVertexInputState = nullptr;
    state.currentPipelineInfo.pInputAssemblyState = nullptr;
    state.currentPipelineBits = PipelineBuildBits::NoInfoSet;
    state.currentShaderPrograms.Resize(NumQueueTypes); // resize to fit all queues, even if only compute and graphics can use shaders...
    state.currentShaderPrograms.Fill(CoreGraphics::InvalidShaderProgramId);

    // construct queues
    VkQueryPoolCreateInfo queryInfos[CoreGraphics::NumQueryTypes];

    for (i = 0; i < CoreGraphics::NumQueryTypes; i++)
    {
        queryInfos[i] = {
            VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
            nullptr,
            0,
            VK_QUERY_TYPE_MAX_ENUM,
            (uint32_t)state.MaxQueriesPerFrame * info.numBufferedFrames,  // create 1024 queries per frame
            0
        };
    }

    queryInfos[CoreGraphics::QueryType::OcclusionQuery].queryType = VK_QUERY_TYPE_OCCLUSION;
    queryInfos[CoreGraphics::QueryType::GraphicsTimestampQuery].queryType = VK_QUERY_TYPE_TIMESTAMP;
    queryInfos[CoreGraphics::QueryType::PipelineStatisticsGraphicsQuery].queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
    queryInfos[CoreGraphics::QueryType::PipelineStatisticsGraphicsQuery].pipelineStatistics =
        VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
        //VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
        //VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT;
    queryInfos[CoreGraphics::QueryType::ComputeTimestampQuery].queryType = VK_QUERY_TYPE_TIMESTAMP;
    queryInfos[CoreGraphics::QueryType::PipelineStatisticsComputeQuery].queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
    queryInfos[CoreGraphics::QueryType::PipelineStatisticsComputeQuery].pipelineStatistics =
        VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
    
    state.queries.Resize(info.numBufferedFrames * state.MaxQueriesPerFrame);
    for (i = 0; i < CoreGraphics::NumQueryTypes; i++)
    {
        state.queryStartIndexByType[i].Resize(info.numBufferedFrames);
        state.queryCountsByType[i].Resize(info.numBufferedFrames);
        for (IndexT j = 0; j < info.numBufferedFrames; j++)
        {
            state.queryStartIndexByType[i][j] = state.MaxQueriesPerFrame * j;
            state.queryCountsByType[i][j] = 0;
        }

        VkResult res = vkCreateQueryPool(state.devices[state.currentDevice], &queryInfos[i], nullptr, &state.queryPoolsByType[i]);
        n_assert(res == VK_SUCCESS);

        // reset queries
        vkResetQueryPool(state.devices[state.currentDevice], state.queryPoolsByType[i], 0, state.MaxQueriesPerFrame * info.numBufferedFrames);

        uint32_t queue = Vulkan::GetQueueFamily((i > QueryType::QueryGraphicsMax) ? ComputeQueueType : GraphicsQueueType);
        uint32_t modifier = (i == CoreGraphics::PipelineStatisticsGraphicsQuery || i == CoreGraphics::PipelineStatisticsComputeQuery) ? 2 : 1;

        CoreGraphics::BufferCreateInfo queryBufferInfo;
        queryBufferInfo.name = "QueryBuffer";
        queryBufferInfo.byteSize = state.MaxQueriesPerFrame * sizeof(uint64) * modifier * state.maxNumBufferedFrames;
        queryBufferInfo.mode = BufferAccessMode::DeviceToHost;
        queryBufferInfo.usageFlags = CoreGraphics::TransferBufferDestination;
        state.queryResultBuffers[i] = CoreGraphics::CreateBuffer(queryBufferInfo);
    }

#if NEBULA_ENABLE_PROFILING
    state.profilingMarkersPerFrame.Resize(info.numBufferedFrames);
#endif

    state.inputInfo.pNext = nullptr;
    state.inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    state.inputInfo.flags = 0;

    state.blendInfo.pNext = nullptr;
    state.blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    state.blendInfo.flags = 0;

    // reset state
    state.inputInfo.topology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;    

    state.inBeginAsyncCompute = false;
    state.inBeginBatch = false;
    state.inBeginCompute = false;
    state.inBeginFrame = false;
    state.inBeginPass = false;
    state.inBeginGraphicsSubmission = false;
    state.inBeginComputeSubmission = false;

    _setup_grouped_timer(state.DebugTimer, "GraphicsDevice");
    _setup_grouped_counter(state.NumImageBytesAllocated, "GraphicsDevice");
    _begin_counter(state.NumImageBytesAllocated);
    _setup_grouped_counter(state.NumBufferBytesAllocated, "GraphicsDevice");
    _begin_counter(state.NumBufferBytesAllocated);
    _setup_grouped_counter(state.NumBytesAllocated, "GraphicsDevice");
    _begin_counter(state.NumBytesAllocated);
    _setup_grouped_counter(state.NumPipelinesBuilt, "GraphicsDevice");
    _begin_counter(state.NumPipelinesBuilt);
    _setup_grouped_counter(state.GraphicsDeviceNumComputes, "GraphicsDevice");
    _begin_counter(state.GraphicsDeviceNumComputes);
    _setup_grouped_counter(state.GraphicsDeviceNumPrimitives, "GraphicsDevice");
    _begin_counter(state.GraphicsDeviceNumPrimitives);
    _setup_grouped_counter(state.GraphicsDeviceNumDrawCalls, "GraphicsDevice");
    _begin_counter(state.GraphicsDeviceNumDrawCalls);

    // yay, Vulkan!
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyGraphicsDevice()
{
    _discard_timer(state.DebugTimer);
    _end_counter(state.GraphicsDeviceNumDrawCalls);
    _discard_counter(state.GraphicsDeviceNumDrawCalls);
    _end_counter(state.GraphicsDeviceNumPrimitives);
    _discard_counter(state.GraphicsDeviceNumPrimitives);
    _end_counter(state.GraphicsDeviceNumComputes);
    _discard_counter(state.GraphicsDeviceNumComputes);
    _end_counter(state.NumImageBytesAllocated);
    _discard_counter(state.NumImageBytesAllocated);
    _end_counter(state.NumBufferBytesAllocated);
    _discard_counter(state.NumBufferBytesAllocated);
    _end_counter(state.NumBytesAllocated);
    _discard_counter(state.NumBytesAllocated);
    _end_counter(state.NumPipelinesBuilt);
    _discard_counter(state.NumPipelinesBuilt);

    // save pipeline cache
    size_t size;
    vkGetPipelineCacheData(state.devices[0], state.cache, &size, nullptr);
    uint8_t* data = (uint8_t*)Memory::Alloc(Memory::ScratchHeap, size);
    vkGetPipelineCacheData(state.devices[0], state.cache, &size, data);
    Util::String path = Util::String::Sprintf("bin:%s_vkpipelinecache", App::Application::Instance()->GetAppTitle().AsCharPtr());
    Ptr<IO::Stream> cachedData = IO::IoServer::Instance()->CreateStream(path);
    cachedData->SetAccessMode(IO::Stream::WriteAccess);
    if (cachedData->Open())
    {
        cachedData->Write(data, (IO::Stream::Size)size);
        cachedData->Close();
    }

    // wait for all commands to be done first
    state.subcontextHandler.WaitIdle(GraphicsQueueType);
    state.subcontextHandler.WaitIdle(ComputeQueueType);
    state.subcontextHandler.WaitIdle(TransferQueueType);
    state.subcontextHandler.WaitIdle(SparseQueueType);

    // wait for queues and run all pending commands
    state.subcontextHandler.Discard();

    // clean up delayed delete objects
    uint j;
    for (j = 0; j < state.maxNumBufferedFrames; j++)
    {
        IndexT i;
        for (i = 0; i < state.delayedDeleteBuffers[j].Size(); i++)
            vkDestroyBuffer(state.devices[state.currentDevice], state.delayedDeleteBuffers[j][i], nullptr);
        state.delayedDeleteBuffers[j].Clear();
        for (i = 0; i < state.delayedDeleteImages[j].Size(); i++)
            vkDestroyImage(state.devices[state.currentDevice], state.delayedDeleteImages[j][i], nullptr);
        state.delayedDeleteImages[j].Clear();
        for (i = 0; i < state.delayedDeleteImageViews[j].Size(); i++)
            vkDestroyImageView(state.devices[state.currentDevice], state.delayedDeleteImageViews[j][i], nullptr);
        state.delayedDeleteImageViews[j].Clear();
        for (i = 0; i < state.delayedFreeMemories[j].Size(); i++)
            FreeMemory(state.delayedFreeMemories[j][i]);
        state.delayedFreeMemories[j].Clear();
    }

    DestroySubmissionContext(state.gfxSubmission);
    DestroySubmissionContext(state.computeSubmission);
    DestroySubmissionContext(state.setupSubmissionContext);
    DestroySubmissionContext(state.resourceSubmissionContext);
    DestroySubmissionContext(state.queryComputeSubmissionContext);
    DestroySubmissionContext(state.queryGraphicsSubmissionContext);

    DestroyCommandBufferPool(state.submissionGraphicsCmdPool);
    DestroyCommandBufferPool(state.submissionComputeCmdPool);
    DestroyCommandBufferPool(state.submissionTransferCmdPool);

    DiscardMemoryPools(state.devices[state.currentDevice]);

    // clean up global constant buffers
    IndexT i;
    for (i = 0; i < NumConstantBufferTypes; i++)
    {
        if (state.globalGraphicsConstantBufferMaxValue[i] > 0)
        {
            DestroyBuffer(state.globalGraphicsConstantBuffer[i]);
            DestroyBuffer(state.globalGraphicsConstantStagingBuffer[i]);
        }
        state.globalGraphicsConstantBuffer[i] = InvalidBufferId;
        state.globalGraphicsConstantStagingBuffer[i] = InvalidBufferId;

        if (state.globalComputeConstantBufferMaxValue[i] > 0)
        {
            DestroyBuffer(state.globalComputeConstantBuffer[i]);
            DestroyBuffer(state.globalComputeConstantStagingBuffer[i]);
        }
        state.globalComputeConstantBuffer[i] = InvalidBufferId;
        state.globalComputeConstantStagingBuffer[i] = InvalidBufferId;
    }
    state.database.Discard();

    // destroy query stuff
    for (i = 0; i < CoreGraphics::NumQueryTypes; i++)
    {
        CoreGraphics::DestroyBuffer(state.queryResultBuffers[i]);

        // destroy actual pool
        vkDestroyQueryPool(state.devices[state.currentDevice], state.queryPoolsByType[i], nullptr);
    }
    // destroy pipeline
    vkDestroyPipelineCache(state.devices[state.currentDevice], state.cache, nullptr);

    for (i = 0; i < state.renderingFinishedSemaphores.Size(); i++)
    {
        DestroyFence(state.presentFences[i]);
        DestroySemaphore(state.renderingFinishedSemaphores[i]);
    }

#if NEBULA_VULKAN_DEBUG
    VkDestroyDebugMessenger(state.instance, VkErrorDebugMessageHandle, nullptr);
#endif

    vkDestroyDevice(state.devices[0], nullptr);
    vkDestroyInstance(state.instance, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
GetNumBufferedFrames()
{
    return state.maxNumBufferedFrames;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
GetBufferedFrameIndex()
{
    return state.currentBufferedFrameIndex;
}

//------------------------------------------------------------------------------
/**
*/
void
AttachEventHandler(const Ptr<CoreGraphics::RenderEventHandler>& h)
{
    n_assert(h.isvalid());
    n_assert(InvalidIndex == state.eventHandlers.FindIndex(h));
    n_assert(!state.inNotifyEventHandlers);
    state.eventHandlers.Append(h);
    h->OnAttach();
}

//------------------------------------------------------------------------------
/**
*/
void
RemoveEventHandler(const Ptr<CoreGraphics::RenderEventHandler>& h)
{
    n_assert(h.isvalid());
    n_assert(!state.inNotifyEventHandlers);
    IndexT index = state.eventHandlers.FindIndex(h);
    n_assert(InvalidIndex != index);
    state.eventHandlers.EraseIndex(index);
    h->OnRemove();
}

//------------------------------------------------------------------------------
/**
*/
bool
NotifyEventHandlers(const CoreGraphics::RenderEvent& e)
{
    n_assert(!state.inNotifyEventHandlers);
    bool handled = false;
    state.inNotifyEventHandlers = true;
    IndexT i;
    for (i = 0; i < state.eventHandlers.Size(); i++)
    {
        handled |= state.eventHandlers[i]->PutEvent(e);
    }
    state.inNotifyEventHandlers = false;
    return handled;
}

//------------------------------------------------------------------------------
/**
*/
void
AddBackBufferTexture(const CoreGraphics::TextureId tex)
{
    state.backBuffers.Append(tex);
}

//------------------------------------------------------------------------------
/**
*/
void
RemoveBackBufferTexture(const CoreGraphics::TextureId tex)
{
    IndexT i = state.backBuffers.FindIndex(tex);
    n_assert(i != InvalidIndex);
    state.backBuffers.EraseIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
void 
SetDrawThread(CoreGraphics::DrawThread* thread)
{
    if (thread != nullptr)
    {
        state.drawThreads.Push(state.drawThread);
        state.drawThread = thread;
    }
    else
    {
        n_assert(!state.drawThreads.IsEmpty());
        state.drawThread = state.drawThreads.Pop();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
BeginFrame(IndexT frameIndex)
{
    n_assert(!state.inBeginFrame);
    n_assert(!state.inBeginPass);
    n_assert(!state.inBeginBatch);

    if (frameIndex != state.currentFrameIndex)
    {
        _begin_counter(state.GraphicsDeviceNumComputes);
        _begin_counter(state.GraphicsDeviceNumPrimitives);
        _begin_counter(state.GraphicsDeviceNumDrawCalls);
    }
    state.inBeginFrame = true;

    // clean up delayed delete objects
    IndexT i;
    for (i = 0; i < state.delayedDeleteBuffers[state.currentBufferedFrameIndex].Size(); i++)
        vkDestroyBuffer(state.devices[state.currentDevice], state.delayedDeleteBuffers[state.currentBufferedFrameIndex][i], nullptr);
    state.delayedDeleteBuffers[state.currentBufferedFrameIndex].Clear();
    for (i = 0; i < state.delayedDeleteImages[state.currentBufferedFrameIndex].Size(); i++)
        vkDestroyImage(state.devices[state.currentDevice], state.delayedDeleteImages[state.currentBufferedFrameIndex][i], nullptr);
    state.delayedDeleteImages[state.currentBufferedFrameIndex].Clear();
    for (i = 0; i < state.delayedDeleteImageViews[state.currentBufferedFrameIndex].Size(); i++)
        vkDestroyImageView(state.devices[state.currentDevice], state.delayedDeleteImageViews[state.currentBufferedFrameIndex][i], nullptr);
    state.delayedDeleteImageViews[state.currentBufferedFrameIndex].Clear();

    N_MARKER_BEGIN(WaitForPresent, Render);

    // slight limitation to only using one back buffer, so really we should do one begin and end frame per window...
    n_assert(state.backBuffers.Size() == 1);
    CoreGraphics::TextureSwapBuffers(state.backBuffers[0]);
    state.currentBufferedFrameIndex = (state.currentBufferedFrameIndex + 1) % state.maxNumBufferedFrames;
    state.queriesRingOffset = state.MaxQueriesPerFrame * state.currentBufferedFrameIndex;

    N_MARKER_END();

    N_MARKER_BEGIN(WaitForLastFrame, Render);

    // cycle submissions, will wait for the fence to finish
    CoreGraphics::SubmissionContextNextCycle(state.gfxSubmission, [](uint64 index)
        {
            state.subcontextHandler.Wait(GraphicsQueueType, index);
        }); 

    CoreGraphics::SubmissionContextNextCycle(state.computeSubmission, [](uint64 index)
        {
            state.subcontextHandler.Wait(ComputeQueueType, index);
        });

    CoreGraphics::SubmissionContextPoll(state.resourceSubmissionContext);
    CoreGraphics::SubmissionContextPoll(state.handoverSubmissionContext);

    N_MARKER_END();

    _ProcessQueriesBeginFrame();

    // update constant buffer offsets
    Vulkan::GraphicsDeviceState::ConstantsRingBuffer& nextCboRing = state.constantBufferRings[state.currentBufferedFrameIndex];
    for (IndexT i = 0; i < CoreGraphics::GlobalConstantBufferType::NumConstantBufferTypes; i++)
    {
        nextCboRing.cboGfxStartAddress[i] = state.globalGraphicsConstantBufferMaxValue[i] * state.currentBufferedFrameIndex;
        nextCboRing.cboGfxEndAddress[i] = state.globalGraphicsConstantBufferMaxValue[i] * state.currentBufferedFrameIndex;
        nextCboRing.cboComputeStartAddress[i] = state.globalComputeConstantBufferMaxValue[i] * state.currentBufferedFrameIndex;
        nextCboRing.cboComputeEndAddress[i] = state.globalComputeConstantBufferMaxValue[i] * state.currentBufferedFrameIndex;
    }

    // reset current thread
    state.currentPipelineBits = PipelineBuildBits::NoInfoSet;

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
BeginSubmission(CoreGraphics::QueueType queue, CoreGraphics::QueueType waitQueue)
{
    n_assert(state.inBeginFrame);
    n_assert(queue == GraphicsQueueType || queue == ComputeQueueType);

    switch (queue)
    {
    case GraphicsQueueType:
        n_assert(!state.inBeginGraphicsSubmission);
        state.inBeginGraphicsSubmission = true;
        break;
    case ComputeQueueType:
        n_assert(!state.inBeginComputeSubmission);
        state.inBeginComputeSubmission = true;
        break;
    }

    Vulkan::GraphicsDeviceState::ConstantsRingBuffer& sub = state.constantBufferRings[state.currentBufferedFrameIndex];
    VkDevice dev = state.devices[state.currentDevice];

    CoreGraphics::SubmissionContextId ctx = queue == GraphicsQueueType ? state.gfxSubmission : state.computeSubmission;
    CoreGraphics::CommandBufferId& cmds = queue == GraphicsQueueType ? state.gfxCmdBuffer : state.computeCmdBuffer;

    // generate new buffer and semaphore
    CoreGraphics::SubmissionContextNewBuffer(ctx, cmds);

    // begin recording the new buffer
    const CommandBufferBeginInfo cmdInfo =
    {
        true, false, false
    };
    CommandBufferBeginRecord(cmds, cmdInfo);

#if NEBULA_GRAPHICS_DEBUG
    const char* names[] =
    {
        "Graphics",
        "Compute",
        "Transfer",
        "Sparse"
    };

    // insert some markers explaining the queue synchronization, the +1 is because it will be the submission index on EndSubmission
    if (waitQueue != InvalidQueueType)
    {
        Util::String fmt = Util::String::Sprintf(
            "Submit #%d, wait for %s queue: submit #%d",
            state.subcontextHandler.GetTimelineIndex(queue) + 1,
            names[waitQueue],
            state.subcontextHandler.GetTimelineIndex(waitQueue));
        CommandBufferBeginMarker(queue, NEBULA_MARKER_PURPLE, fmt.AsCharPtr());
    }
    else
    {
        Util::String fmt = Util::String::Sprintf(
            "Submit #%d",
            state.subcontextHandler.GetTimelineIndex(queue) + 1);
        CommandBufferBeginMarker(queue, NEBULA_MARKER_PURPLE, fmt.AsCharPtr());
    }
#endif

    AtomicCounter* cboStartAddress = queue == GraphicsQueueType ? sub.cboGfxStartAddress : sub.cboComputeStartAddress;
    AtomicCounter* cboEndAddress = queue == GraphicsQueueType ? sub.cboGfxEndAddress : sub.cboComputeEndAddress;
    CoreGraphics::BufferId* stagingCbo = queue == GraphicsQueueType ? state.globalGraphicsConstantStagingBuffer : state.globalComputeConstantStagingBuffer;
    CoreGraphics::BufferId* cbo = queue == GraphicsQueueType ? state.globalGraphicsConstantBuffer : state.globalComputeConstantBuffer;

    IndexT i;
    for (i = 0; i < NumConstantBufferTypes; i++)
    {
        int size = cboEndAddress[i] - cboStartAddress[i];
        if (size > 0)
        {
            VkMappedMemoryRange range;
            range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            range.pNext = nullptr;
            range.offset = cboStartAddress[i];
            range.size = Math::align(size, state.deviceProps[state.currentDevice].limits.nonCoherentAtomSize);
            range.memory = BufferGetVkMemory(stagingCbo[i]);
            VkResult res = vkFlushMappedMemoryRanges(dev, 1, &range);
            n_assert(res == VK_SUCCESS);
            cboEndAddress[i] = Math::align(cboEndAddress[i], state.deviceProps[state.currentDevice].limits.nonCoherentAtomSize);

            VkBufferCopy copy;
            copy.srcOffset = copy.dstOffset = cboStartAddress[i];
            copy.size = range.size;
            vkCmdCopyBuffer(
                CommandBufferGetVk(cmds),
                BufferGetVk(stagingCbo[i]),
                BufferGetVk(cbo[i]), 1, &copy);

            // make sure to put a barrier after the copy so that subsequent calls may wait for the copy to finish
            VkBufferMemoryBarrier barrier =
            {
                VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                nullptr,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                BufferGetVk(cbo[i]), copy.srcOffset, copy.size
            };
            
            VkPipelineStageFlagBits stageFlag = queue == GraphicsQueueType ? VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT : VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            vkCmdPipelineBarrier(
                CommandBufferGetVk(cmds),
                VK_PIPELINE_STAGE_TRANSFER_BIT, stageFlag, 0,
                0, nullptr,
                1, &barrier,
                0, nullptr
            );
        }
        cboStartAddress[i] = cboEndAddress[i];
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
BeginPass(const CoreGraphics::PassId pass, PassRecordMode mode)
{
    n_assert(state.inBeginFrame);
    n_assert(!state.inBeginPass);
    n_assert(!state.inBeginBatch);
    n_assert(state.pass == InvalidPassId);
    state.inBeginPass = true;
    state.pass = pass;

    const VkRenderPassBeginInfo& info = PassGetVkRenderPassBeginInfo(pass);
    state.passInfo.framebuffer = info.framebuffer;
    state.passInfo.renderPass = info.renderPass;
    state.passInfo.subpass = 0;
    state.passInfo.pipelineStatistics = 0;
    state.passInfo.queryFlags = 0;
    state.passInfo.occlusionQueryEnable = VK_FALSE;

    // set info
    SetFramebufferLayoutInfo(PassGetVkFramebufferInfo(pass));
    state.database.SetPass(pass);
    state.database.SetSubpass(0);

#if NEBULA_ENABLE_MT_DRAW
    const Util::FixedArray<VkViewport>& viewports = PassGetVkViewports(state.pass);
    CoreGraphics::SetVkViewports(viewports.Begin(), viewports.Size());
    const Util::FixedArray<VkRect2D>& scissors = PassGetVkRects(state.pass);
    CoreGraphics::SetVkScissorRects(scissors.Begin(), scissors.Size());

    switch (mode)
    {
    case PassRecordMode::ExecuteInline:
        vkCmdBeginRenderPass(GetMainBuffer(GraphicsQueueType), &info, VK_SUBPASS_CONTENTS_INLINE);
        break;
    case PassRecordMode::ExecuteRecorded:
        vkCmdBeginRenderPass(GetMainBuffer(GraphicsQueueType), &info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        break;
    case PassRecordMode::Record:
    default:
        break;
    }
#else
    const Util::FixedArray<VkViewport>& viewports = PassGetVkViewports(state.pass);
    CoreGraphics::SetVkViewports(viewports.Begin(), viewports.Size());
    const Util::FixedArray<VkRect2D>& scissors = PassGetVkRects(state.pass);
    CoreGraphics::SetVkScissorRects(scissors.Begin(), scissors.Size());
    vkCmdBeginRenderPass(GetMainBuffer(GraphicsQueueType), &info, VK_SUBPASS_CONTENTS_INLINE);
#endif

}

//------------------------------------------------------------------------------
/**
*/
void
BeginSubpassCommands(CoreGraphics::CommandBufferId buf)
{
    n_assert(state.drawThread);
    state.drawThreadCommands = buf;

    // tell thread to begin command buffer recording
    VkCommandBufferBeginInfo begin =
    {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        NULL,
        VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr
    };

    IndexT i;
    for (i = 0; i < state.MaxVertexStreams; i++)
    {
        state.vboStreams[i] = CoreGraphics::InvalidBufferId;
        state.vboStreamOffsets[i] = -1;
    }
    state.ibo = CoreGraphics::InvalidBufferId;
    state.iboOffset = -1;

    VkCommandBufferThread::VkCommandBufferBeginCommand cmd;
    cmd.buf = CommandBufferGetVk(state.drawThreadCommands);
    cmd.inheritInfo = state.passInfo;
    cmd.info = begin;
    state.drawThread->Push(cmd);
}

//------------------------------------------------------------------------------
/**
*/
void 
SetToNextSubpass(PassRecordMode mode)
{
    n_assert(state.inBeginFrame);
    n_assert(state.inBeginPass);
    n_assert(!state.inBeginBatch);
    n_assert(state.pass != InvalidPassId);
    SetFramebufferLayoutInfo(PassGetVkFramebufferInfo(state.pass));
    state.database.SetSubpass(state.currentPipelineInfo.subpass);
    state.passInfo.subpass = state.currentPipelineInfo.subpass;

#if NEBULA_ENABLE_MT_DRAW
    const Util::FixedArray<VkViewport>& viewports = PassGetVkViewports(state.pass);
    CoreGraphics::SetVkViewports(viewports.Begin(), viewports.Size());
    const Util::FixedArray<VkRect2D>& scissors = PassGetVkRects(state.pass);
    CoreGraphics::SetVkScissorRects(scissors.Begin(), scissors.Size());

    switch (mode)
    {
    case PassRecordMode::ExecuteInline:
        vkCmdNextSubpass(GetMainBuffer(GraphicsQueueType), VK_SUBPASS_CONTENTS_INLINE);
        break;
    case PassRecordMode::ExecuteRecorded:
        vkCmdNextSubpass(GetMainBuffer(GraphicsQueueType), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        break;
    case PassRecordMode::Record:
    default:
        break;
    }
#else
    const Util::FixedArray<VkViewport>& viewports = PassGetVkViewports(state.pass);
    CoreGraphics::SetVkViewports(viewports.Begin(), viewports.Size());
    const Util::FixedArray<VkRect2D>& scissors = PassGetVkRects(state.pass);
    CoreGraphics::SetVkScissorRects(scissors.Begin(), scissors.Size());
    vkCmdNextSubpass(GetMainBuffer(GraphicsQueueType), VK_SUBPASS_CONTENTS_INLINE);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void 
BeginBatch(Frame::FrameBatchType::Code batchType)
{
    //n_assert(state.inBeginPass);
    n_assert(!state.inBeginBatch);
    //n_assert(state.pass != InvalidPassId);

    state.inBeginBatch = true;
}

//------------------------------------------------------------------------------
/**
*/
void 
ResetClipSettings()
{
    PassApplyClipSettings(state.pass);
}

//------------------------------------------------------------------------------
/**
*/
void 
SetStreamVertexBuffer(IndexT streamIndex, const CoreGraphics::BufferId& buffer, IndexT offsetVertexIndex)
{
    if (state.vboStreams[streamIndex] != buffer || state.vboStreamOffsets[streamIndex] != offsetVertexIndex)
    {
        if (state.drawThread)
        {
            n_assert(state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId);
            VkCommandBufferThread::VkVertexBufferCommand cmd;
            cmd.buffer = BufferGetVk(buffer);
            cmd.index = streamIndex;
            cmd.offset = offsetVertexIndex;
            state.drawThread->Push(cmd);
        }
        else
        {
            VkBuffer buf = BufferGetVk(buffer);
            VkDeviceSize offset = offsetVertexIndex;
            vkCmdBindVertexBuffers(GetMainBuffer(GraphicsQueueType), streamIndex, 1, &buf, &offset);
        }
        state.vboStreams[streamIndex] = buffer;
        state.vboStreamOffsets[streamIndex] = offsetVertexIndex;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SetVertexLayout(const CoreGraphics::VertexLayoutId& vl)
{
    n_assert(state.currentShaderPrograms[GraphicsQueueType] != CoreGraphics::InvalidShaderProgramId);
    if (state.currentVertexLayout != vl || state.currentVertexLayoutShader != state.currentShaderPrograms[GraphicsQueueType])
    {
        VkPipelineVertexInputStateCreateInfo* info = CoreGraphics::layoutPool->GetDerivativeLayout(vl, state.currentShaderPrograms[GraphicsQueueType]);
        SetVertexLayoutPipelineInfo(info);
        state.currentVertexLayout = vl;
        state.currentVertexLayoutShader = state.currentShaderPrograms[GraphicsQueueType];
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SetIndexBuffer(const CoreGraphics::BufferId& buffer, IndexT offsetIndex)
{
    IndexType::Code idxType = IndexType::ToIndexType(BufferGetElementSize(buffer));
    VkIndexType vkIdxType = idxType == IndexType::Index16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
    if (state.ibo != buffer || state.iboOffset != offsetIndex)
    {
        if (state.drawThread)
        {
            n_assert(state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId);
            VkCommandBufferThread::VkIndexBufferCommand cmd;
            cmd.buffer = BufferGetVk(buffer);
            cmd.indexType = vkIdxType;
            cmd.offset = offsetIndex;
            state.drawThread->Push(cmd);
        }
        else
        {
            vkCmdBindIndexBuffer(GetMainBuffer(GraphicsQueueType), BufferGetVk(buffer), offsetIndex, vkIdxType);
        }
        state.ibo = buffer;
        state.iboOffset = offsetIndex;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SetPrimitiveTopology(const CoreGraphics::PrimitiveTopology::Code topo)
{
    state.primitiveTopology = topo;
    VkPrimitiveTopology comp = VkTypes::AsVkPrimitiveType(topo);
    if (state.inputInfo.topology != comp)
    {
        state.inputInfo.topology = comp;
        state.inputInfo.primitiveRestartEnable = VK_FALSE;
        SetInputLayoutInfo(&state.inputInfo);
    }
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveTopology::Code& 
GetPrimitiveTopology()
{
    return state.primitiveTopology;
}

//------------------------------------------------------------------------------
/**
*/
void 
SetPrimitiveGroup(const CoreGraphics::PrimitiveGroup& pg)
{
    state.primitiveGroup = pg;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveGroup& 
GetPrimitiveGroup()
{
    return state.primitiveGroup;
}

//------------------------------------------------------------------------------
/**
*/
void
SetShaderProgram(const CoreGraphics::ShaderProgramId pro, const CoreGraphics::QueueType queue, const bool bindSharedResources)
{
    n_assert(pro != CoreGraphics::InvalidShaderProgramId);

    VkShaderProgramRuntimeInfo& info = CoreGraphics::shaderPool->shaderAlloc.Get<VkShaderCache::Shader_ProgramAllocator>(pro.shaderId).Get<ShaderProgram_RuntimeInfo>(pro.programId);
    info.colorBlendInfo.pAttachments = info.colorBlendAttachments;
    state.currentStencilFrontRef = info.stencilFrontRef;
    state.currentStencilBackRef = info.stencilBackRef;
    state.currentStencilReadMask = info.stencilReadMask;
    state.currentStencilWriteMask = info.stencilWriteMask;

    bool layoutChanged = false;

    // if we are compute, we can set the pipeline straight away, otherwise we have to accumulate the infos
    if (info.type == ComputePipeline)
    {
        n_assert(state.drawThreadCommands == CoreGraphics::InvalidCommandBufferId);

        // bind compute pipeline
        state.currentBindPoint = CoreGraphics::ComputePipeline;

        // bind pipeline
        vkCmdBindPipeline(GetMainBuffer(queue), VK_PIPELINE_BIND_POINT_COMPUTE, info.pipeline);

        layoutChanged = state.currentComputePipelineLayout != info.layout;
        state.currentComputePipelineLayout = info.layout;
    }
    else if (info.type == GraphicsPipeline)
    {
        // setup pipeline information regarding the shader state
        VkGraphicsPipelineCreateInfo ginfo =
        {
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            NULL,
            VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT,
            info.stageCount,
            info.shaderInfos,
            &info.vertexInfo,           // we only save how many vs inputs we allow here
            NULL,                       // this is input type related (triangles, patches etc)
            &info.tessInfo,
            NULL,                       // this is our viewport and is setup by the framebuffer
            &info.rasterizerInfo,
            &info.multisampleInfo,
            &info.depthStencilInfo,
            &info.colorBlendInfo,
            &info.dynamicInfo,
            info.layout,
            NULL,                           // pass specific stuff, keep as NULL, handled by the framebuffer
            0,
            VK_NULL_HANDLE, 0               // base pipeline is kept as NULL too, because this is the base for all derivatives
        };
        Vulkan::BindGraphicsPipelineInfo(ginfo, pro);
        state.currentBindPoint = CoreGraphics::ComputePipeline;

        layoutChanged = state.currentGraphicsPipelineLayout != info.layout;
        state.currentGraphicsPipelineLayout = info.layout;
    }
    else
        Vulkan::UnbindPipeline();

    // bind descriptors
    if (info.type == CoreGraphics::ComputePipeline)
    {
        if (layoutChanged && bindSharedResources)
        {
            CoreGraphics::SetResourceTable(state.tickResourceTable, NEBULA_TICK_GROUP, CoreGraphics::ComputePipeline, nullptr, queue);
            CoreGraphics::SetResourceTable(state.frameResourceTable, NEBULA_FRAME_GROUP, CoreGraphics::ComputePipeline, nullptr, queue);
        }
    }
    else // graphics queue
    {
        if (!state.drawThread && layoutChanged && bindSharedResources)
        {
            CoreGraphics::SetResourceTable(state.tickResourceTable, NEBULA_TICK_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
            CoreGraphics::SetResourceTable(state.frameResourceTable, NEBULA_FRAME_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SetResourceTable(const CoreGraphics::ResourceTableId table, const IndexT slot, ShaderPipeline pipeline, const Util::FixedArray<uint>& offsets, const CoreGraphics::QueueType queue)
{
    n_assert(table != CoreGraphics::InvalidResourceTableId);
    switch (pipeline)
    {
    case GraphicsPipeline:
        Vulkan::BindDescriptorsGraphics(&ResourceTableGetVkDescriptorSet(table),
            slot,
            1,
            offsets.IsEmpty() ? nullptr : offsets.Begin(),
            offsets.Size());
        break;
    case ComputePipeline:
        Vulkan::BindDescriptorsCompute(&ResourceTableGetVkDescriptorSet(table),
            slot,
            1,
            offsets.IsEmpty() ? nullptr : offsets.Begin(),
            offsets.Size(), queue);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SetResourceTable(const CoreGraphics::ResourceTableId table, const IndexT slot, ShaderPipeline pipeline, uint32 numOffsets, uint32* offsets, const CoreGraphics::QueueType queue)
{
    switch (pipeline)
    {
        case GraphicsPipeline:
        Vulkan::BindDescriptorsGraphics(&ResourceTableGetVkDescriptorSet(table),
                                        slot,
                                        1,
                                        offsets,
                                        numOffsets);
        break;
        case ComputePipeline:
        Vulkan::BindDescriptorsCompute(&ResourceTableGetVkDescriptorSet(table),
                                       slot,
                                       1,
                                       offsets,
                                       numOffsets, queue);
        break;
    }
}
//------------------------------------------------------------------------------
/**
*/
void 
SetResourceTablePipeline(const CoreGraphics::ResourcePipelineId layout)
{
    n_assert(layout != CoreGraphics::ResourcePipelineId::Invalid());
    state.currentGraphicsPipelineLayout = ResourcePipelineGetVk(layout);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourceTableId
SetTickResourceTable(const CoreGraphics::ResourceTableId table)
{
    n_assert(table != CoreGraphics::InvalidResourceTableId);
    CoreGraphics::ResourceTableId ret = state.tickResourceTable;
    state.tickResourceTable = table;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourceTableId
SetFrameResourceTable(const CoreGraphics::ResourceTableId table)
{
    n_assert(table != CoreGraphics::InvalidResourceTableId);
    CoreGraphics::ResourceTableId ret = state.frameResourceTable;
    state.frameResourceTable = table;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
PushConstants(ShaderPipeline pipeline, uint offset, uint size, byte* data)
{
    switch (pipeline)
    {
    case GraphicsPipeline:
        Vulkan::UpdatePushRanges(VK_SHADER_STAGE_ALL_GRAPHICS, state.currentGraphicsPipelineLayout, offset, size, data);
        break;
    case ComputePipeline:
        Vulkan::UpdatePushRanges(VK_SHADER_STAGE_COMPUTE_BIT, state.currentComputePipelineLayout, offset, size, data);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
int
SetGraphicsConstantsInternal(CoreGraphics::GlobalConstantBufferType type, const void* data, SizeT size)
{
    Vulkan::GraphicsDeviceState::ConstantsRingBuffer& sub = state.constantBufferRings[state.currentBufferedFrameIndex];

    // no matter how we spin it
    int alignedSize = Math::align(size, state.deviceProps[state.currentDevice].limits.minUniformBufferOffsetAlignment);
    int ret = Threading::Interlocked::Add(&sub.cboGfxEndAddress[type], alignedSize);

    // if we have to wrap around, or we are fingering on the range of the next frame submission buffer...
    if (ret + alignedSize >= state.globalGraphicsConstantBufferMaxValue[type] * int(state.currentBufferedFrameIndex + 1))
    {
        n_error("Over allocation of graphics constant memory! Memory will be overwritten!\n");

        // return the beginning of the buffer, will definitely stomp the memory!
        return ret;
    }

    // just bump the current frame submission pointer
    BufferUpdate(state.globalGraphicsConstantStagingBuffer[type], data, size, ret);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
int
SetComputeConstantsInternal(CoreGraphics::GlobalConstantBufferType type, const void* data, SizeT size)
{
    Vulkan::GraphicsDeviceState::ConstantsRingBuffer& sub = state.constantBufferRings[state.currentBufferedFrameIndex];

    // no matter how we spin it
    int alignedSize = Math::align(size, state.deviceProps[state.currentDevice].limits.minUniformBufferOffsetAlignment);
    int ret = Threading::Interlocked::Add(&sub.cboComputeEndAddress[type], alignedSize);

    // if we have to wrap around, or we are fingering on the range of the next frame submission buffer...
    if (ret + alignedSize >= state.globalComputeConstantBufferMaxValue[type] * int(state.currentBufferedFrameIndex + 1))
    {
        n_error("Over allocation of compute constant memory! Memory will be overwritten!\n");

        // return the beginning of the buffer, will definitely stomp the memory!
        return ret;
    }

    // just bump the current frame submission pointer
    BufferUpdate(state.globalComputeConstantStagingBuffer[type], data, size, ret);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
SetGraphicsConstantsInternal(CoreGraphics::GlobalConstantBufferType type, uint offset, const void* data, SizeT size)
{
    BufferUpdate(state.globalGraphicsConstantStagingBuffer[type], data, size, offset);
}

//------------------------------------------------------------------------------
/**
*/
void 
SetComputeConstantsInternal(CoreGraphics::GlobalConstantBufferType type, uint offset, const void* data, SizeT size)
{
    BufferUpdate(state.globalComputeConstantStagingBuffer[type], data, size, offset);
}

//------------------------------------------------------------------------------
/**
*/
uint 
AllocateGraphicsConstantBufferMemory(CoreGraphics::GlobalConstantBufferType type, uint size)
{
    n_assert(!state.inBeginGraphicsSubmission);
    Vulkan::GraphicsDeviceState::ConstantsRingBuffer& sub = state.constantBufferRings[state.currentBufferedFrameIndex];

    // no matter how we spin it
    uint ret = sub.cboGfxEndAddress[type];
    uint newEnd = Math::align(ret + size, state.deviceProps[state.currentDevice].limits.minUniformBufferOffsetAlignment);

    // if we have to wrap around, or we are fingering on the range of the next frame submission buffer...
    if (newEnd >= state.globalGraphicsConstantBufferMaxValue[type] * (state.currentBufferedFrameIndex + 1))
    {
        n_error("Over allocation of graphics constant memory! Memory will be overwritten!\n");

        // return the beginning of the buffer, will definitely stomp the memory!
        ret = state.globalGraphicsConstantBufferMaxValue[type] * state.currentBufferedFrameIndex;
        newEnd = Math::align(ret + size, state.deviceProps[state.currentDevice].limits.minUniformBufferOffsetAlignment);
    }

    // just bump the current frame submission pointer
    sub.cboGfxEndAddress[type] = newEnd;

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::BufferId
GetGraphicsConstantBuffer(CoreGraphics::GlobalConstantBufferType type)
{
    return state.globalGraphicsConstantBuffer[type];
}

//------------------------------------------------------------------------------
/**
*/
uint 
AllocateComputeConstantBufferMemory(CoreGraphics::GlobalConstantBufferType type, uint size)
{
    n_assert(!state.inBeginComputeSubmission);
    Vulkan::GraphicsDeviceState::ConstantsRingBuffer& sub = state.constantBufferRings[state.currentBufferedFrameIndex];

    // no matter how we spin it
    uint ret = sub.cboComputeEndAddress[type];
    uint newEnd = Math::align(ret + size, state.deviceProps[state.currentDevice].limits.minUniformBufferOffsetAlignment);

    // if we have to wrap around, or we are fingering on the range of the next frame submission buffer...
    if (newEnd >= state.globalComputeConstantBufferMaxValue[type] * (state.currentBufferedFrameIndex + 1))
    {
        n_error("Over allocation of compute constant memory! Memory will be overwritten!\n");

        // return the beginning of the buffer, will definitely stomp the memory!
        ret = state.globalComputeConstantBufferMaxValue[type] * state.currentBufferedFrameIndex;
        newEnd = Math::align(ret + size, state.deviceProps[state.currentDevice].limits.minUniformBufferOffsetAlignment);
    }

    // just bump the current frame submission pointer
    sub.cboComputeEndAddress[type] = newEnd;

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::BufferId
GetComputeConstantBuffer(CoreGraphics::GlobalConstantBufferType type)
{
    return state.globalComputeConstantBuffer[type];
}

//------------------------------------------------------------------------------
/**
*/
void 
LockResourceSubmission()
{
    state.resourceSubmissionCriticalSection.Enter();
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::SubmissionContextId 
GetResourceSubmissionContext()
{
    // if not active, issue a new resource submission (only done once per frame)
    if (!state.resourceSubmissionActive)
    {
        SubmissionContextNextCycle(state.resourceSubmissionContext, [](uint64 index)
            {
                state.subcontextHandler.Wait(TransferQueueType, index);
            });
        SubmissionContextNewBuffer(state.resourceSubmissionContext, state.resourceSubmissionCmdBuffer);

        // begin recording
        CommandBufferBeginInfo beginInfo{ true, false, false };
        CommandBufferBeginRecord(state.resourceSubmissionCmdBuffer, beginInfo);

        state.resourceSubmissionActive = true;
    }
    return state.resourceSubmissionContext;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::SubmissionContextId 
GetHandoverSubmissionContext()
{
    // if not active, issue a new resource submission (only done once per frame)
    if (!state.handoverSubmissionActive)
    {
        SubmissionContextNextCycle(state.handoverSubmissionContext, [](uint64 index)
            {
                // wait for the submission to finish
                state.subcontextHandler.Wait(GraphicsQueueType, index);
            });
        SubmissionContextNewBuffer(state.handoverSubmissionContext, state.handoverSubmissionCmdBuffer);

        // begin recording
        CommandBufferBeginInfo beginInfo{ true, false, false };
        CommandBufferBeginRecord(state.handoverSubmissionCmdBuffer, beginInfo);

        state.handoverSubmissionActive = true;
    }
    return state.handoverSubmissionContext;
}

//------------------------------------------------------------------------------
/**
*/
void 
UnlockResourceSubmission()
{
    state.resourceSubmissionCriticalSection.Leave();
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::SubmissionContextId 
GetSetupSubmissionContext()
{
    // if not active, issue a new resource submission (only done once per frame)
    if (!state.setupSubmissionActive)
    {
        SubmissionContextNextCycle(state.setupSubmissionContext, [](uint64 index)
            {
                // wait for the submission to finish
                state.subcontextHandler.Wait(GraphicsQueueType, index);
            });
        SubmissionContextNewBuffer(state.setupSubmissionContext, state.setupSubmissionCmdBuffer);

        // begin recording
        CommandBufferBeginInfo beginInfo{ true, false, false };
        CommandBufferBeginRecord(state.setupSubmissionCmdBuffer, beginInfo);

        state.setupSubmissionActive = true;
    }
    return state.setupSubmissionContext;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::CommandBufferId 
GetGfxCommandBuffer()
{
    return state.gfxCmdBuffer;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::CommandBufferId 
GetComputeCommandBuffer()
{
    return state.computeCmdBuffer;
}

//------------------------------------------------------------------------------
/**
*/
void 
SetGraphicsPipeline()
{
    n_assert((state.currentPipelineBits & PipelineBuildBits::AllInfoSet) != 0);
    state.currentBindPoint = CoreGraphics::GraphicsPipeline;
    if (!AllBits(state.currentPipelineBits, PipelineBuildBits::PipelineBuilt))
    {
        CreateAndBindGraphicsPipeline();
        state.currentPipelineBits |= PipelineBuildBits::PipelineBuilt;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ReloadShaderProgram(const CoreGraphics::ShaderProgramId& pro)
{
    state.database.Reload(pro);
}

//------------------------------------------------------------------------------
/**
*/
void 
InsertBarrier(const CoreGraphics::BarrierId barrier, const CoreGraphics::QueueType queue)
{
    n_assert(!state.inBeginPass);
    VkBarrierInfo& info = barrierAllocator.Get<0>(barrier.id24);
    if (queue == GraphicsQueueType && state.inBeginPass)
    {
        if (state.drawThread)
        {
            n_assert(state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId);         
            VkCommandBufferThread::VkBarrierCommand cmd;
            cmd.dep = info.dep;
            cmd.srcMask = info.srcFlags;
            cmd.dstMask = info.dstFlags;
            cmd.memoryBarrierCount = info.numMemoryBarriers;
            cmd.memoryBarriers = info.memoryBarriers;
            cmd.bufferBarrierCount = info.numBufferBarriers;
            cmd.bufferBarriers = info.bufferBarriers;
            cmd.imageBarrierCount = info.numImageBarriers;
            cmd.imageBarriers = info.imageBarriers;
            state.drawThread->Push(cmd);
        }
        else
        {
            vkCmdPipelineBarrier(GetMainBuffer(GraphicsQueueType),
                info.srcFlags, info.dstFlags, 
                info.dep, 
                info.numMemoryBarriers, info.memoryBarriers, 
                info.numBufferBarriers, info.bufferBarriers, 
                info.numImageBarriers, info.imageBarriers);
        }
    }
    else
    {
        VkCommandBuffer buf = GetMainBuffer(queue);
        vkCmdPipelineBarrier(buf,
            info.srcFlags,
            info.dstFlags,
            info.dep,
            info.numMemoryBarriers, info.memoryBarriers,
            info.numBufferBarriers, info.bufferBarriers,
            info.numImageBarriers, info.imageBarriers);
    }
}


//------------------------------------------------------------------------------
/**
*/
void 
SignalEvent(const CoreGraphics::EventId ev, const CoreGraphics::BarrierStage stage, const CoreGraphics::QueueType queue)
{
    VkEventInfo& info = eventAllocator.Get<1>(ev.id24);
    if (queue == GraphicsQueueType && state.inBeginPass)
    {
        if (state.drawThread)
        {
            n_assert(state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId);
            VkCommandBufferThread::VkSetEventCommand cmd;
            cmd.event = info.event;
            cmd.stages = VkTypes::AsVkPipelineFlags(stage);
            state.drawThread->Push(cmd);
        }
        else
        {
            vkCmdSetEvent(GetMainBuffer(queue), info.event, VkTypes::AsVkPipelineFlags(stage));
        }
    }
    else
    {
        VkCommandBuffer buf = GetMainBuffer(queue);
        vkCmdSetEvent(buf, info.event, VkTypes::AsVkPipelineFlags(stage));
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
WaitEvent(
    const EventId id,
    const CoreGraphics::BarrierStage waitStage,
    const CoreGraphics::BarrierStage signalStage,
    const CoreGraphics::QueueType queue
    )
{
    VkEventInfo& info = eventAllocator.Get<1>(id.id24);
    if (queue == GraphicsQueueType && state.inBeginPass)
    {
        if (state.drawThread)
        {
            n_assert(state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId);
            VkCommandBufferThread::VkWaitForEventCommand cmd;
            cmd.event = info.event;
            cmd.numEvents = 1;
            cmd.memoryBarrierCount = info.numMemoryBarriers;
            cmd.memoryBarriers = info.memoryBarriers;
            cmd.bufferBarrierCount = info.numBufferBarriers;
            cmd.bufferBarriers = info.bufferBarriers;
            cmd.imageBarrierCount = info.numImageBarriers;
            cmd.imageBarriers = info.imageBarriers;
            cmd.waitingStage = VkTypes::AsVkPipelineFlags(waitStage);
            cmd.signalingStage = VkTypes::AsVkPipelineFlags(signalStage);
            state.drawThread->Push(cmd);
        }
        else
        {
            vkCmdWaitEvents(GetMainBuffer(queue), 1, &info.event,
                VkTypes::AsVkPipelineFlags(waitStage),
                VkTypes::AsVkPipelineFlags(signalStage),
                info.numMemoryBarriers,
                info.memoryBarriers,
                info.numBufferBarriers,
                info.bufferBarriers,
                info.numImageBarriers,
                info.imageBarriers);
        }
    }
    else
    {
        VkCommandBuffer buf = GetMainBuffer(queue);
        vkCmdWaitEvents(buf, 1, &info.event,
            VkTypes::AsVkPipelineFlags(waitStage),
            VkTypes::AsVkPipelineFlags(signalStage),
            info.numMemoryBarriers,
            info.memoryBarriers,
            info.numBufferBarriers,
            info.bufferBarriers,
            info.numImageBarriers,
            info.imageBarriers);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ResetEvent(const CoreGraphics::EventId ev, const CoreGraphics::BarrierStage stage, const CoreGraphics::QueueType queue)
{
    VkEventInfo& info = eventAllocator.Get<1>(ev.id24);
    if (queue == GraphicsQueueType && state.inBeginPass)    
    {
        if (state.drawThread)
        {
            n_assert(state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId);
            VkCommandBufferThread::VkResetEventCommand cmd;
            cmd.event = info.event;
            cmd.stages = VkTypes::AsVkPipelineFlags(stage);
            state.drawThread->Push(cmd);
        }
        else
        {
            vkCmdResetEvent(GetMainBuffer(queue), info.event, VkTypes::AsVkPipelineFlags(stage));
        }
    }
    else
    {
        VkCommandBuffer buf = GetMainBuffer(queue);
        vkCmdResetEvent(buf, info.event, VkTypes::AsVkPipelineFlags(stage));
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
Draw()
{
    n_assert(state.inBeginPass);
    if (state.drawThread)
    {
        n_assert(state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId)
        VkCommandBufferThread::VkDrawCommand cmd;
        cmd.baseIndex = state.primitiveGroup.GetBaseIndex();
        cmd.baseVertex = state.primitiveGroup.GetBaseVertex();
        cmd.baseInstance = 0;
        cmd.numIndices = state.primitiveGroup.GetNumIndices();
        cmd.numVerts = state.primitiveGroup.GetNumVertices();
        cmd.numInstances = 1;
        state.drawThread->Push(cmd);
    }
    else
    {
        if (state.primitiveGroup.GetNumIndices() > 0)
            vkCmdDrawIndexed(GetMainBuffer(GraphicsQueueType), state.primitiveGroup.GetNumIndices(), 1, state.primitiveGroup.GetBaseIndex(), state.primitiveGroup.GetBaseVertex(), 0);
        else
            vkCmdDraw(GetMainBuffer(GraphicsQueueType), state.primitiveGroup.GetNumVertices(), 1, state.primitiveGroup.GetBaseVertex(), 0);
    }

    // go to next thread
    _incr_counter(state.GraphicsDeviceNumDrawCalls, 1);
    _incr_counter(state.GraphicsDeviceNumPrimitives, state.primitiveGroup.GetNumVertices() / 3);
}

//------------------------------------------------------------------------------
/**
*/
void
Draw(SizeT numInstances, IndexT baseInstance)
{
    n_assert(state.inBeginPass);

    if (state.drawThread)
    {
        n_assert(state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId);
        VkCommandBufferThread::VkDrawCommand cmd;
        cmd.baseIndex = state.primitiveGroup.GetBaseIndex();
        cmd.baseVertex = state.primitiveGroup.GetBaseVertex();
        cmd.baseInstance = baseInstance;
        cmd.numIndices = state.primitiveGroup.GetNumIndices();
        cmd.numVerts = state.primitiveGroup.GetNumVertices();
        cmd.numInstances = numInstances;
        state.drawThread->Push(cmd);
    }
    else
    {
        if (state.primitiveGroup.GetNumIndices() > 0)
            vkCmdDrawIndexed(GetMainBuffer(GraphicsQueueType), state.primitiveGroup.GetNumIndices(), numInstances, state.primitiveGroup.GetBaseIndex(), state.primitiveGroup.GetBaseVertex(), baseInstance);
        else
            vkCmdDraw(GetMainBuffer(GraphicsQueueType), state.primitiveGroup.GetNumVertices(), numInstances, state.primitiveGroup.GetBaseVertex(), baseInstance);
    }

    // increase counters
    _incr_counter(state.GraphicsDeviceNumDrawCalls, 1);
    _incr_counter(state.GraphicsDeviceNumPrimitives, state.primitiveGroup.GetNumIndices() * numInstances / 3);
}

//------------------------------------------------------------------------------
/**
*/
void
DrawIndirect(const CoreGraphics::BufferId buffer, IndexT bufferOffset, SizeT draws, IndexT stride)
{
    n_assert(state.inBeginPass);
    if (state.drawThread)
    {
        n_assert(state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId)
        VkCommandBufferThread::VkIndirectDrawCommand cmd;
        cmd.buffer = BufferGetVk(buffer);
        cmd.offset = bufferOffset;
        cmd.drawCount = draws;
        cmd.stride = stride;
        state.drawThread->Push(cmd);
    }
    else
    {
        vkCmdDrawIndirect(GetMainBuffer(GraphicsQueueType), BufferGetVk(buffer), bufferOffset, draws, stride);
    }

    // increase counters, we are missing the primitives here though :(
    _incr_counter(state.GraphicsDeviceNumDrawCalls, draws);
}

//------------------------------------------------------------------------------
/**
*/
void
DrawIndirectIndexed(const CoreGraphics::BufferId buffer, IndexT bufferOffset, SizeT draws, IndexT stride)
{
    n_assert(state.inBeginPass);
    if (state.drawThread)
    {
        n_assert(state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId)
        VkCommandBufferThread::VkIndirectIndexedDrawCommand cmd;
        cmd.buffer = BufferGetVk(buffer);
        cmd.offset = bufferOffset;
        cmd.drawCount = draws;
        cmd.stride = stride;
        state.drawThread->Push(cmd);
    }
    else
    {
        vkCmdDrawIndexedIndirect(GetMainBuffer(GraphicsQueueType), BufferGetVk(buffer), bufferOffset, draws, stride);
    }

    // increase counters, we are missing the primitives here though :(
    _incr_counter(state.GraphicsDeviceNumDrawCalls, draws);
}

//------------------------------------------------------------------------------
/**
*/
void 
Compute(int dimX, int dimY, int dimZ, const CoreGraphics::QueueType queue)
{
    n_assert(!state.inBeginPass);
    n_assert(dimX <= (int)state.deviceProps[state.currentDevice].limits.maxComputeWorkGroupCount[0]);
    n_assert(dimY <= (int)state.deviceProps[state.currentDevice].limits.maxComputeWorkGroupCount[1]);
    n_assert(dimZ <= (int)state.deviceProps[state.currentDevice].limits.maxComputeWorkGroupCount[2]);

    vkCmdDispatch(GetMainBuffer(queue), dimX, dimY, dimZ);
}

//------------------------------------------------------------------------------
/**
*/
void 
EndSubpassCommands()
{
    n_assert(state.drawThread);

    // push command to stop recording
    n_assert(state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId)
    VkCommandBufferThread::VkCommandBufferEndCommand cmd;
    state.drawThread->Push(cmd);

    // queue up the command buffer for clear next frame
    CoreGraphics::SubmissionContextClearCommandBuffer(state.gfxSubmission, state.drawThreadCommands);
    state.drawThreadCommands = CoreGraphics::InvalidCommandBufferId;
}

//------------------------------------------------------------------------------
/**
*/
void 
ExecuteCommands(const CoreGraphics::CommandBufferId cmds)
{
    VkCommandBuffer buf = CommandBufferGetVk(cmds);
    vkCmdExecuteCommands(GetMainBuffer(GraphicsQueueType), 1, &buf);
}

//------------------------------------------------------------------------------
/**
*/
void
EndBatch()
{
    n_assert(state.inBeginBatch);
    //n_assert(state.pass != InvalidPassId);

    state.currentShaderPrograms[CoreGraphics::GraphicsQueueType] = CoreGraphics::InvalidShaderProgramId;
    state.inBeginBatch = false;
}

//------------------------------------------------------------------------------
/**
*/
void
EndPass(PassRecordMode mode)
{
    n_assert(state.inBeginPass);
    n_assert(state.pass != InvalidPassId);

    state.pass = InvalidPassId;
    state.inBeginPass = false;

    //this->currentPipelineBits = 0;
    for (IndexT i = 0; i < NEBULA_NUM_GROUPS; i++)
        state.propagateDescriptorSets[i].baseSet = -1;
    state.currentShaderPrograms[CoreGraphics::GraphicsQueueType] = CoreGraphics::InvalidShaderProgramId;

    // end render pass
    switch (mode)
    {
    case PassRecordMode::ExecuteInline:
    case PassRecordMode::ExecuteRecorded:
        vkCmdEndRenderPass(GetMainBuffer(GraphicsQueueType));
        break;
    case PassRecordMode::Record:
    default:
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
EndSubmission(CoreGraphics::QueueType queue, CoreGraphics::QueueType waitQueue)
{
    n_assert(waitQueue != queue);

    switch (queue)
    {
    case GraphicsQueueType:
        n_assert(state.inBeginGraphicsSubmission);
        state.inBeginGraphicsSubmission = false;
        break;
    case ComputeQueueType:
        n_assert(state.inBeginComputeSubmission);
        state.inBeginComputeSubmission = false;
        break;
    }

    CoreGraphics::CommandBufferId commandBuffer = queue == GraphicsQueueType ? state.gfxCmdBuffer : state.computeCmdBuffer;

    LockResourceSubmission();
    if (queue == GraphicsQueueType && state.setupSubmissionActive)
    {
        // end recording and add this command buffer for submission
        // end recording and add this command buffer for submission
        CommandBufferEndRecord(state.setupSubmissionCmdBuffer);

        // submit to graphics without waiting for any previous commands
        uint64 index = state.subcontextHandler.AppendSubmissionTimeline(
            GraphicsQueueType,
            CommandBufferGetVk(state.setupSubmissionCmdBuffer)
        );
        SubmissionContextSetTimelineIndex(state.setupSubmissionContext, index);
        state.setupSubmissionActive = false;
    }
    UnlockResourceSubmission();

    // append a submission, and wait for the previous submission on the same queue
    uint64 index = state.subcontextHandler.AppendSubmissionTimeline(
        queue,
        CommandBufferGetVk(commandBuffer)
    );
    SubmissionContextSetTimelineIndex(queue == GraphicsQueueType ? state.gfxSubmission : state.computeSubmission, index);

#if NEBULA_GRAPHICS_DEBUG
    // put end marker
    CommandBufferEndMarker(queue);
#endif

    // stop recording
    CommandBufferEndRecord(commandBuffer);

    // if we have a presentation semaphore, also wait for it
    if (queue == state.mainSubmitQueue && state.waitForPresentSemaphore)
    {
        state.subcontextHandler.AppendWaitTimeline(
            queue,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            state.waitForPresentSemaphore
        );
        state.waitForPresentSemaphore = VK_NULL_HANDLE;
    }

    // if we have a queue that is blocking us, wait for it
    if (waitQueue != InvalidQueueType)
    {
        state.subcontextHandler.AppendWaitTimeline(
            queue,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            waitQueue
        );
    }
}

//------------------------------------------------------------------------------
/**
*/
void
EndFrame(IndexT frameIndex)
{
    n_assert(state.inBeginFrame);

    if (state.currentFrameIndex != frameIndex)
    {
        _end_counter(state.GraphicsDeviceNumComputes);
        _end_counter(state.GraphicsDeviceNumPrimitives);
        _end_counter(state.GraphicsDeviceNumDrawCalls);
        state.currentFrameIndex = frameIndex;
    }

    state.inBeginFrame = false;

    _ProcessQueriesEndFrame();

    if (state.sparseSubmitActive)
    {
#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::QueueBeginMarker(SparseQueueType, NEBULA_MARKER_ORANGE, "Sparse Bindings");
#endif

        state.subcontextHandler.FlushSparseBinds(nullptr);
        state.sparseSubmitActive = false;

#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::QueueEndMarker(SparseQueueType);
#endif
    }

    // if we have an active resource submission, submit it!
    LockResourceSubmission();

    // do transfer-graphics handovers
    if (state.handoverSubmissionActive)
    {
        // finish up the resource submission and setup submissions
        CommandBufferEndRecord(state.handoverSubmissionCmdBuffer);

        // append submission for doing handover from transfer
        uint64 index = state.subcontextHandler.AppendSubmissionTimeline(
            GraphicsQueueType,
            CommandBufferGetVk(state.handoverSubmissionCmdBuffer)
        );
        SubmissionContextSetTimelineIndex(state.handoverSubmissionContext, index);

        CoreGraphics::FenceId fence = SubmissionContextGetFence(state.handoverSubmissionContext);
        state.subcontextHandler.InsertFence(GraphicsQueueType, FenceGetVk(fence));

        state.handoverSubmissionActive = false;
    }

    // if we have a pending sparse submit, let the graphics wait for the sparse to finish
    if (state.sparseWaitHandled)
    {
        state.subcontextHandler.AppendWaitTimeline(
            CoreGraphics::GraphicsQueueType,
            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
            CoreGraphics::SparseQueueType
        );

        state.sparseWaitHandled = true;
    }

    if (state.resourceSubmissionActive)
    {
        // finish up the resource submission and setup submissions
        CommandBufferEndRecord(state.resourceSubmissionCmdBuffer);

        // finish by creating a singular submission for all transfers
        uint64 index = state.subcontextHandler.AppendSubmissionTimeline(
            TransferQueueType,
            CommandBufferGetVk(state.resourceSubmissionCmdBuffer)
        );
        SubmissionContextSetTimelineIndex(state.resourceSubmissionContext, index);

#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::QueueBeginMarker(TransferQueueType, NEBULA_MARKER_ORANGE, "Transfer");
#endif

        // submit transfers
        CoreGraphics::FenceId fence = SubmissionContextGetFence(state.resourceSubmissionContext);
        state.subcontextHandler.FlushSubmissionsTimeline(TransferQueueType, FenceGetVk(fence));

#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::QueueEndMarker(TransferQueueType);
#endif

        // make sure to allow the graphics queue to wait for this command buffer to finish, because we might need to wait for resource ownership handovers
        state.subcontextHandler.AppendWaitTimeline(
            GraphicsQueueType,
            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
            TransferQueueType
        );
        state.resourceSubmissionActive = false;
    }

    UnlockResourceSubmission();

#if NEBULA_GRAPHICS_DEBUG
    CoreGraphics::QueueBeginMarker(ComputeQueueType, NEBULA_MARKER_ORANGE, "Compute");
#endif

    N_MARKER_BEGIN(ComputeSubmit, Render);

    // submit compute, wait for this frames resource submissions
    state.subcontextHandler.FlushSubmissionsTimeline(ComputeQueueType, nullptr);

    N_MARKER_END();

#if NEBULA_GRAPHICS_DEBUG
    CoreGraphics::QueueEndMarker(ComputeQueueType);
    CoreGraphics::QueueBeginMarker(GraphicsQueueType, NEBULA_MARKER_ORANGE, "Graphics");
#endif

    // add signal for binary semaphore used by the presentation system
    state.subcontextHandler.AppendSignalTimeline(
        GraphicsQueueType,
        SemaphoreGetVk(state.renderingFinishedSemaphores[state.currentBufferedFrameIndex])
    );

    N_MARKER_BEGIN(GraphicsSubmit, Render);

    // submit graphics, since this is our main queue, we use this submission to get the semaphore wait index
    state.subcontextHandler.FlushSubmissionsTimeline(GraphicsQueueType, nullptr);

    N_MARKER_END();

#if NEBULA_GRAPHICS_DEBUG
    CoreGraphics::QueueEndMarker(GraphicsQueueType);
#endif

    // reset state
    state.inputInfo.topology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    state.currentShaderPrograms[CoreGraphics::GraphicsQueueType] = CoreGraphics::InvalidShaderProgramId;
    state.currentPipelineInfo.pVertexInputState = nullptr;
    state.currentPipelineInfo.pInputAssemblyState = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
bool
IsInBeginFrame()
{
    return state.inBeginFrame;
}

//------------------------------------------------------------------------------
/**
*/
void 
WaitForQueue(CoreGraphics::QueueType queue)
{
    state.subcontextHandler.WaitIdle(queue);
}

//------------------------------------------------------------------------------
/**
*/
void 
WaitForAllQueues()
{
    state.subcontextHandler.WaitIdle(GraphicsQueueType);
    state.subcontextHandler.WaitIdle(ComputeQueueType);
    state.subcontextHandler.WaitIdle(TransferQueueType);
    state.subcontextHandler.WaitIdle(SparseQueueType);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ImageFileFormat::Code
SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream)
{
    return CoreGraphics::ImageFileFormat::InvalidImageFileFormat;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ImageFileFormat::Code 
SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream, const Math::rectangle<int>& rect, int x, int y)
{
    return CoreGraphics::ImageFileFormat::InvalidImageFileFormat;
}

//------------------------------------------------------------------------------
/**
*/
bool 
GetVisualizeMipMaps()
{
    return state.visualizeMipMaps;
}

//------------------------------------------------------------------------------
/**
*/
void 
SetVisualizeMipMaps(bool val)
{
    state.visualizeMipMaps = val;
}

//------------------------------------------------------------------------------
/**
*/
bool
GetRenderWireframe()
{
    return state.renderWireframe;
}

//------------------------------------------------------------------------------
/**
*/
void 
SetRenderWireframe(bool b)
{
    state.renderWireframe = b;
}

#if NEBULA_ENABLE_PROFILING
//------------------------------------------------------------------------------
/**
*/
IndexT 
Timestamp(CoreGraphics::QueueType queue, const CoreGraphics::BarrierStage stage, const char* name)
{
    // convert to vulkan flags, force bits set to only be 1
    VkPipelineStageFlags flags = VkTypes::AsVkPipelineFlags(stage);
    n_assert(Util::CountBits(flags) == 1);

    // chose query based on queue
    QueryType type = (queue == GraphicsQueueType) ? GraphicsTimestampQuery : ComputeTimestampQuery;

    // get current query, and get the index
    SizeT& count = state.queryCountsByType[type][state.currentBufferedFrameIndex];
    IndexT idx = state.queryStartIndexByType[type][state.currentBufferedFrameIndex] + count;
    count++;
    n_assert(idx < state.MaxQueriesPerFrame * (SizeT)(state.currentBufferedFrameIndex + 1));

    CoreGraphics::Query query;
    query.type = type;
    query.idx = idx;
    state.queries[state.queriesRingOffset + state.numUsedQueries++] = query;

    // write time stamp
    if (state.drawThread)
    {
        n_assert(state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId);
        VkCommandBufferThread::VkWriteTimestampCommand cmd;
        cmd.flags = flags;
        cmd.pool = state.queryPoolsByType[type];
        cmd.index = idx;
        state.drawThread->Push(cmd);
    }
    else
    {
        VkCommandBuffer buf = GetMainBuffer(queue);
        vkCmdWriteTimestamp(buf, (VkPipelineStageFlagBits)flags, state.queryPoolsByType[type], idx);
    }

    return idx;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<FrameProfilingMarker>&
GetProfilingMarkers()
{
    return state.frameProfilingMarkers;
}

//------------------------------------------------------------------------------
/**
*/
SizeT 
GetNumDrawCalls()
{
    return state.GraphicsDeviceNumDrawCalls->GetSample();
}
#endif

//------------------------------------------------------------------------------
/**
*/
IndexT 
BeginQuery(CoreGraphics::QueueType queue, CoreGraphics::QueryType type)
{
    n_assert(type != CoreGraphics::QueryType::GraphicsTimestampQuery);

    // get current query, and get the index
    SizeT& count = state.queryCountsByType[type][state.currentBufferedFrameIndex];
    IndexT idx = state.queryStartIndexByType[type][state.currentBufferedFrameIndex] + count;
    count++;
    n_assert(idx < state.MaxQueriesPerFrame * (SizeT)(state.currentBufferedFrameIndex + 1));

    CoreGraphics::Query query;
    query.type = type;
    query.idx = idx;
    state.queries[state.queriesRingOffset + state.numUsedQueries++] = query;

    // start query
    if (state.drawThread)
    {
        n_assert(state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId);
        VkCommandBufferThread::VkBeginQueryCommand cmd;
        cmd.flags = VK_QUERY_CONTROL_PRECISE_BIT;
        cmd.pool = state.queryPoolsByType[type];
        cmd.index = idx;
        state.drawThread->Push(cmd);
    }
    else
    {
        VkCommandBuffer buf = GetMainBuffer(queue);
        vkCmdBeginQuery(buf, state.queryPoolsByType[type], idx, VK_QUERY_CONTROL_PRECISE_BIT);
    }
    
    return idx;
}

//------------------------------------------------------------------------------
/**
*/
void 
EndQuery(CoreGraphics::QueueType queue, CoreGraphics::QueryType type)
{
    n_assert(type != CoreGraphics::QueryType::GraphicsTimestampQuery);

    // get current query, and get the index
    uint32_t index = state.queryCountsByType[type][state.currentBufferedFrameIndex];

    // start query
    if (state.drawThread)
    {
        n_assert(state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId);
        VkCommandBufferThread::VkEndQueryCommand cmd;
        cmd.pool = state.queryPoolsByType[type];
        cmd.index = index;
        state.drawThread->Push(cmd);
    }
    else
    {
        VkCommandBuffer buf = GetMainBuffer(queue);
        vkCmdEndQuery(buf, state.queryPoolsByType[type], index);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
Copy(
    const CoreGraphics::QueueType queue,
    const CoreGraphics::TextureId fromTexture,
    const Util::Array<CoreGraphics::TextureCopy>& from,
    const CoreGraphics::TextureId toTexture,
    const Util::Array<CoreGraphics::TextureCopy>& to,
    const CoreGraphics::SubmissionContextId sub)
{
    n_assert(fromTexture != CoreGraphics::InvalidTextureId && toTexture != CoreGraphics::InvalidTextureId);
    n_assert(from.Size() == to.Size());

    bool isDepth = PixelFormat::IsDepthFormat(CoreGraphics::TextureGetPixelFormat(fromTexture));
    VkImageAspectFlags aspect = isDepth ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : VK_IMAGE_ASPECT_COLOR_BIT;
    Util::FixedArray<VkImageCopy> copies(from.Size());
    for (IndexT i = 0; i < copies.Size(); i++)
    {
        VkImageCopy& copy = copies[i];
        copy.dstOffset = { to[i].region.left, to[i].region.top, 0 };
        copy.dstSubresource = { aspect, (uint32_t)to[i].mip, (uint32_t)to[i].layer, 1 };
        copy.extent = { (uint32_t)to[i].region.width(), (uint32_t)to[i].region.height(), 1 };
        copy.srcOffset = { from[i].region.left, from[i].region.top, 0 };
        copy.srcSubresource = { aspect, (uint32_t)from[i].mip, (uint32_t)from[i].layer, 1 };
    }

    VkCommandBuffer buf;
    if (sub == CoreGraphics::InvalidSubmissionContextId)
    {
        n_assert(!state.inBeginPass);
        n_assert(state.drawThreadCommands == CoreGraphics::InvalidCommandBufferId);
        buf = GetMainBuffer(queue);
    }
    else
        buf = Vulkan::CommandBufferGetVk(CoreGraphics::SubmissionContextGetCmdBuffer(sub));
    vkCmdCopyImage(buf, TextureGetVkImage(fromTexture), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, TextureGetVkImage(toTexture), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copies.Size(), copies.Begin());
}

//------------------------------------------------------------------------------
/**
*/
void 
Copy(
    const CoreGraphics::QueueType queue,
    const CoreGraphics::BufferId fromBuffer,
    const Util::Array<CoreGraphics::BufferCopy>& from,
    const CoreGraphics::BufferId toBuffer,
    const Util::Array<CoreGraphics::BufferCopy>& to,
    SizeT size,
    const CoreGraphics::SubmissionContextId sub)
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
    
    VkCommandBuffer buf;
    if (sub == CoreGraphics::InvalidSubmissionContextId)
    {
        n_assert(!state.inBeginPass);
        n_assert(state.drawThreadCommands == CoreGraphics::InvalidCommandBufferId);
        buf = GetMainBuffer(queue);
    }
    else
        buf = Vulkan::CommandBufferGetVk(CoreGraphics::SubmissionContextGetCmdBuffer(sub));

    vkCmdCopyBuffer(buf, BufferGetVk(fromBuffer), BufferGetVk(toBuffer), copies.Size(), copies.Begin());
}


//------------------------------------------------------------------------------
/**
*/
void 
Copy(
    const CoreGraphics::QueueType queue,
    const CoreGraphics::BufferId fromBuffer,
    const Util::Array<CoreGraphics::BufferCopy>& from,
    const CoreGraphics::TextureId toTexture,
    const Util::Array<CoreGraphics::TextureCopy>& to,
    const CoreGraphics::SubmissionContextId sub)
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
    
    VkCommandBuffer buf;
    if (sub == CoreGraphics::InvalidSubmissionContextId)
    {
        n_assert(!state.inBeginPass);
        n_assert(state.drawThreadCommands == CoreGraphics::InvalidCommandBufferId);
        buf = GetMainBuffer(queue);
    }
    else
        buf = Vulkan::CommandBufferGetVk(CoreGraphics::SubmissionContextGetCmdBuffer(sub));

    vkCmdCopyBufferToImage(buf,
        BufferGetVk(fromBuffer),
        TextureGetVkImage(toTexture),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        copies.Size(),
        copies.Begin());
}

//------------------------------------------------------------------------------
/**
*/
void
Copy(
    const CoreGraphics::QueueType queue,
    const CoreGraphics::TextureId fromTexture,
    const Util::Array<CoreGraphics::TextureCopy>& from,
    const CoreGraphics::BufferId toBuffer,
    const Util::Array<CoreGraphics::BufferCopy>& to,
    const CoreGraphics::SubmissionContextId sub)
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
    
    VkCommandBuffer buf;
    if (sub == CoreGraphics::InvalidSubmissionContextId)
    {
        n_assert(!state.inBeginPass);
        n_assert(state.drawThreadCommands == CoreGraphics::InvalidCommandBufferId);
        buf = GetMainBuffer(queue);
    }
    else
        buf = Vulkan::CommandBufferGetVk(CoreGraphics::SubmissionContextGetCmdBuffer(sub));

    vkCmdCopyImageToBuffer(buf,
        TextureGetVkImage(fromTexture),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        BufferGetVk(toBuffer),
        copies.Size(),
        copies.Begin());
}

//------------------------------------------------------------------------------
/**
*/
void 
Blit(const CoreGraphics::TextureId from, const Math::rectangle<SizeT>& fromRegion, IndexT fromMip, IndexT fromLayer, const CoreGraphics::TextureId to, const Math::rectangle<SizeT>& toRegion, IndexT toMip, IndexT toLayer)
{
    n_assert(from != CoreGraphics::InvalidTextureId && to != CoreGraphics::InvalidTextureId);
    n_assert(!state.inBeginPass);
    n_assert(state.drawThreadCommands == CoreGraphics::InvalidCommandBufferId);

    bool isDepth = PixelFormat::IsDepthFormat(CoreGraphics::TextureGetPixelFormat(from));
    VkImageAspectFlags aspect = isDepth ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageBlit blit;
    blit.srcOffsets[0] = { fromRegion.left, fromRegion.top, 0 };
    blit.srcOffsets[1] = { fromRegion.right, fromRegion.bottom, 1 };
    blit.srcSubresource = { aspect, (uint32_t)fromMip, (uint32_t)fromLayer, 1 };
    blit.dstOffsets[0] = { toRegion.left, toRegion.top, 0 };
    blit.dstOffsets[1] = { toRegion.right, toRegion.bottom, 1 };
    blit.dstSubresource = { aspect, (uint32_t)toMip, (uint32_t)toLayer, 1 };
    vkCmdBlitImage(GetMainBuffer(CoreGraphics::GraphicsQueueType), TextureGetVkImage(from), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, TextureGetVkImage(to), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
}

//------------------------------------------------------------------------------
/**
*/
void 
SetUsePatches(bool b)
{
    state.usePatches = b;
}

//------------------------------------------------------------------------------
/**
*/
bool 
GetUsePatches()
{
    return state.usePatches;
}

//------------------------------------------------------------------------------
/**
*/
void 
SetViewport(const Math::rectangle<int>& rect, int index)
{
    // copy here is on purpose, because we don't want to modify the state viewports (they are pointers to the pass)
    VkViewport vp;
    vp.width = (float)rect.width();
    vp.height = (float)rect.height();
    vp.x = (float)rect.left;
    vp.y = (float)rect.top;
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;

    // only apply to batch or command buffer if we have a program bound
    n_assert(state.currentShaderPrograms[CoreGraphics::GraphicsQueueType] != CoreGraphics::InvalidShaderProgramId);
    if (state.drawThread)
    {
        if (state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId)
        {
            VkCommandBufferThread::VkViewportCommand cmd;
            cmd.index = index;
            cmd.vp = vp;
            state.drawThread->Push(cmd);
        }
    }
    else
    {
        vkCmdSetViewport(GetMainBuffer(GraphicsQueueType), index, 1, &vp);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SetScissorRect(const Math::rectangle<int>& rect, int index)
{
    // copy here is on purpose, because we don't want to modify the state scissors (they are pointers to the pass)
    VkRect2D sc;
    sc.extent.width = rect.width();
    sc.extent.height = rect.height();
    sc.offset.x = rect.left;
    sc.offset.y = rect.top;
    n_assert(state.currentShaderPrograms[CoreGraphics::GraphicsQueueType] != CoreGraphics::InvalidShaderProgramId);
    if (state.drawThread)
    {
        if (state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId)
        {
            VkCommandBufferThread::VkScissorRectCommand cmd;
            cmd.index = index;
            cmd.sc = sc;
            state.drawThread->Push(cmd);
        }
    }
    else
    {
        vkCmdSetScissor(GetMainBuffer(GraphicsQueueType), index, 1, &sc);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SetStencilRef(const uint frontRef, const uint backRef)
{
    state.currentStencilFrontRef = frontRef;
    state.currentStencilBackRef = backRef;
    if (state.drawThread)
    {
        if (state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId)
        {
            VkCommandBufferThread::VkStencilRefCommand cmd;
            cmd.frontRef = frontRef;
            cmd.backRef = backRef;
            state.drawThread->Push(cmd);
        }
    }
    else
    {
        vkCmdSetStencilReference(GetMainBuffer(GraphicsQueueType), VK_STENCIL_FACE_FRONT_BIT, frontRef);
        vkCmdSetStencilReference(GetMainBuffer(GraphicsQueueType), VK_STENCIL_FACE_BACK_BIT, backRef);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SetStencilReadMask(const uint readMask)
{
    state.currentStencilReadMask = readMask;
    if (state.drawThread)
    {
        if (state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId)
        {
            VkCommandBufferThread::VkStencilReadMaskCommand cmd;
            cmd.mask = readMask;
            state.drawThread->Push(cmd);
        }
    }
    else
    {
        vkCmdSetStencilCompareMask(GetMainBuffer(GraphicsQueueType), VK_STENCIL_FACE_FRONT_AND_BACK, readMask);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SetStencilWriteMask(const uint writeMask)
{
    state.currentStencilWriteMask = writeMask;
    if (state.drawThread)
    {
        if (state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId)
        {
            VkCommandBufferThread::VkStencilWriteMaskCommand cmd;
            cmd.mask = writeMask;
            state.drawThread->Push(cmd);
        }
    }
    else
    {
        vkCmdSetStencilWriteMask(GetMainBuffer(GraphicsQueueType), VK_STENCIL_FACE_FRONT_AND_BACK, writeMask);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateBuffer(const CoreGraphics::BufferId buffer, uint offset, uint size, const void* data, CoreGraphics::QueueType queue)
{
    if (state.drawThread)
    {
        if (state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId)
        {
            VkCommandBufferThread::VkUpdateBufferCommand cmd;
            cmd.buf = BufferGetVk(buffer);
            cmd.data = data;
            cmd.offset = offset;
            cmd.size = size;
            state.drawThread->Push(cmd);
        }
    }
    else
    {
        vkCmdUpdateBuffer(GetMainBuffer(queue), Vulkan::BufferGetVk(buffer), offset, size, data);
    }
}

#if NEBULA_GRAPHICS_DEBUG

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const CoreGraphics::BufferId id, const char* name)
{
    VkDebugUtilsObjectNameInfoEXT info =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_OBJECT_TYPE_BUFFER,
        (uint64_t)Vulkan::BufferGetVk(id),
        name
    };
    VkDevice dev = GetCurrentDevice();
    VkResult res = VkDebugObjectName(dev, &info);
    n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const CoreGraphics::TextureId id, const char* name)
{
    VkDebugUtilsObjectNameInfoEXT info =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_OBJECT_TYPE_IMAGE,
        (uint64_t)Vulkan::TextureGetVkImage(id),
        name
    };
    VkDevice dev = GetCurrentDevice();
    VkResult res = VkDebugObjectName(dev, &info);
    n_assert(res == VK_SUCCESS);

    info.objectHandle = (uint64_t)Vulkan::TextureGetVkImageView(id);
    info.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
    Util::String str = Util::String::Sprintf("%s - View", name);
    info.pObjectName = str.AsCharPtr();
    res = VkDebugObjectName(dev, &info);
    n_assert(res == VK_SUCCESS);    
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const CoreGraphics::ResourceTableLayoutId id, const char* name)
{
    VkDebugUtilsObjectNameInfoEXT info =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
        (uint64_t)Vulkan::ResourceTableLayoutGetVk(id),
        name
    };
    VkDevice dev = GetCurrentDevice();
    VkResult res = VkDebugObjectName(dev, &info);
    n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const CoreGraphics::ResourcePipelineId id, const char* name)
{
    VkDebugUtilsObjectNameInfoEXT info =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_OBJECT_TYPE_PIPELINE_LAYOUT,
        (uint64_t)Vulkan::ResourcePipelineGetVk(id),
        name
    };
    VkDevice dev = GetCurrentDevice();
    VkResult res = VkDebugObjectName(dev, &info);
    n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const CoreGraphics::ResourceTableId id, const char* name)
{
    VkDebugUtilsObjectNameInfoEXT info =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_OBJECT_TYPE_DESCRIPTOR_SET,
        (uint64_t)Vulkan::ResourceTableGetVkDescriptorSet(id),
        name
    };
    VkDevice dev = GetCurrentDevice();
    VkResult res = VkDebugObjectName(dev, &info);
    n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const CoreGraphics::CommandBufferId id, const char* name)
{
    VkDebugUtilsObjectNameInfoEXT info =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_OBJECT_TYPE_COMMAND_BUFFER,
        (uint64_t)Vulkan::CommandBufferGetVk(id),
        name
    };
    VkDevice dev = GetCurrentDevice();
    VkResult res = VkDebugObjectName(dev, &info);
    n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const VkShaderModule id, const char* name)
{
    VkDebugUtilsObjectNameInfoEXT info =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_OBJECT_TYPE_SHADER_MODULE,
        (uint64_t)id,
        name
    };
    VkDevice dev = GetCurrentDevice();
    VkResult res = VkDebugObjectName(dev, &info);
    n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ObjectSetName(const SemaphoreId id, const char* name)
{
    VkDebugUtilsObjectNameInfoEXT info =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_OBJECT_TYPE_SEMAPHORE,
        (uint64_t)SemaphoreGetVk(id.id24),
        name
    };
    VkDevice dev = GetCurrentDevice();
    VkResult res = VkDebugObjectName(dev, &info);
    n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void 
QueueBeginMarker(const CoreGraphics::QueueType queue, const Math::vec4& color, const char* name)
{
    VkQueue vkqueue = state.subcontextHandler.GetQueue(queue);
    alignas(16) float col[4];
    color.store(col);
    VkDebugUtilsLabelEXT info =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        nullptr,
        name,
        { col[0], col[1], col[2], col[3] }
    };
    VkQueueBeginLabel(vkqueue, &info);
}

//------------------------------------------------------------------------------
/**
*/
void 
QueueEndMarker(const CoreGraphics::QueueType queue)
{
    VkQueue vkqueue = state.subcontextHandler.GetQueue(queue);
    VkQueueEndLabel(vkqueue);
}

//------------------------------------------------------------------------------
/**
*/
void 
QueueInsertMarker(const CoreGraphics::QueueType queue, const Math::vec4& color, const char* name)
{
    VkQueue vkqueue = state.subcontextHandler.GetQueue(queue);
    alignas(16) float col[4];
    color.store(col);
    VkDebugUtilsLabelEXT info =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        nullptr,
        name,
        { col[0], col[1], col[2], col[3] }
    };
    VkQueueInsertLabel(vkqueue, &info);
}

//------------------------------------------------------------------------------
/**
*/
void 
CommandBufferBeginMarker(const CoreGraphics::QueueType queue, const Math::vec4& color, const char* name)
{
    // if batching, draws goes to thread
    if (state.drawThread)
    {
        if (state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId)
        {
            VkCommandBufferThread::VkBeginMarkerCommand cmd;
            cmd.text = name;
            color.storeu(cmd.values);
            state.drawThread->Push(cmd);
        }
    }
    else
    {
        VkCommandBuffer buf = GetMainBuffer(queue);
        alignas(16) float col[4];
        color.store(col);
        VkDebugUtilsLabelEXT info =
        {
            VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
            nullptr,
            name,
            { col[0], col[1], col[2], col[3] }
        };
        VkCmdDebugMarkerBegin(buf, &info);
    }

#if NEBULA_ENABLE_PROFILING
    N_MARKER_DYN_BEGIN(name, Render);
    FrameProfilingMarker marker;
    marker.queue = queue;
    marker.color = color;
    marker.name = name;
    marker.gpuBegin = CoreGraphics::Timestamp(queue, CoreGraphics::BarrierStage::Top, name);
    state.profilingMarkerStack[queue].Push(marker);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void 
CommandBufferEndMarker(const CoreGraphics::QueueType queue)
{

#if NEBULA_ENABLE_PROFILING
    FrameProfilingMarker marker = state.profilingMarkerStack[queue].Pop();
    N_MARKER_END();

    marker.gpuEnd = CoreGraphics::Timestamp(queue, CoreGraphics::BarrierStage::Bottom, nullptr);
    if (state.profilingMarkerStack[queue].IsEmpty())
        state.profilingMarkersPerFrame[state.currentBufferedFrameIndex].Append(marker);
    else
        state.profilingMarkerStack[queue].Peek().children.Append(marker);

#endif

    // if batching, draws goes to thread
    if (state.drawThread)
    {
        if (state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId)
        {
            VkCommandBufferThread::VkEndMarkerCommand cmd;
            state.drawThread->Push(cmd);
        }       
    }
    else
    {
        VkCommandBuffer buf = GetMainBuffer(queue);
        VkCmdDebugMarkerEnd(buf);
    }   
}

//------------------------------------------------------------------------------
/**
*/
void 
CommandBufferInsertMarker(const CoreGraphics::QueueType queue, const Math::vec4& color, const char* name)
{
    // if batching, draws goes to thread
    if (state.drawThread)
    {
        if (state.drawThreadCommands != CoreGraphics::InvalidCommandBufferId)
        {
            VkCommandBufferThread::VkInsertMarkerCommand cmd;
            cmd.text = name;
            color.storeu(cmd.values);
            state.drawThread->Push(cmd);
        }       
    }
    else
    {
        VkCommandBuffer buf = GetMainBuffer(queue);
        alignas(16) float col[4];
        color.store(col);
        VkDebugUtilsLabelEXT info =
        {
            VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
            nullptr,
            name,
            { col[0], col[1], col[2], col[3] }
        };
        VkCmdDebugMarkerInsert(buf, &info);
    }
}
#endif

} // namespace Vulkan

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::GraphicsDeviceState const* const
CoreGraphics::GetGraphicsDeviceState()
{
    return (CoreGraphics::GraphicsDeviceState*)&Vulkan::state;
}
