//------------------------------------------------------------------------------
//  vkgraphicsdevice.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "coregraphics/config.h"
#include "vkgraphicsdevice.h"
#include "coregraphics/commandbuffer.h"
#include "vkpipelinedatabase.h"
#include "vkcommandbuffer.h"
#include "vkresourcetable.h"
#include "vkpass.h"
#include "vkbuffer.h"
#include "vktexture.h"
#include "coregraphics/displaydevice.h"
#include "app/application.h"
#include "io/ioserver.h"
#include "vkfence.h"
#include "vkshader.h"
#include "vkvertexlayout.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/vk/vksemaphore.h"
#include "coregraphics/vk/vkfence.h"
#include "coregraphics/vk/vktextureview.h"
#include "coregraphics/vk/vkaccelerationstructure.h"
#include "coregraphics/graphicsdevice.h"
#include "profiling/profiling.h"

#include "threading/criticalsection.h"

namespace Vulkan
{
static Threading::CriticalSection delayedDeleteSection;
static Threading::CriticalSection transferLock;
static Threading::CriticalSection setupLock;
static Threading::CriticalSection submitLock;

struct GraphicsDeviceState : CoreGraphics::GraphicsDeviceState
{
    uint32_t adapter;
    uint32_t frameId;
    VkPhysicalDeviceMemoryProperties memoryProps;

    VkInstance instance;
    VkDescriptorPool descPool;
    VkPipelineCache cache;
    VkAllocationCallbacks alloc;

    static const uint MaxVertexStreams = 16;
    CoreGraphics::BufferId vboStreams[MaxVertexStreams];
    IndexT vboStreamOffsets[MaxVertexStreams];
    CoreGraphics::BufferId ibo;
    IndexT iboOffset;

    struct ConstantsRingBuffer
    {
        // handle global constant memory
        Threading::AtomicCounter endAddress;
        struct FlushedRanges
        {
            SizeT flushedStart;
        } gfx, cmp;
        bool allowConstantAllocation;
    };
    Util::FixedArray<ConstantsRingBuffer> constantBufferRings;

    Util::Array<Memory::RangeAllocation> uploadAllocationsToFlush;
    Util::FixedArray<Util::Array<Memory::RangeAllocation>> uploadAllocationsToFree;
    CoreGraphics::BufferId uploadBuffer;

    struct SparseImageBind
    {
        VkImage img;
        Util::Array<VkSparseMemoryBind> opaqueBinds;
        Util::Array<VkSparseImageMemoryBind> pageBinds;
    };
    Util::Array<SparseImageBind> sparseImageBinds;

    struct SparseBufferBind
    {
        VkBuffer buf;
        Util::Array<VkSparseMemoryBind> binds;
    };
    Util::Array<SparseBufferBind> sparseBufferBinds;

    struct PendingDeletes
    {
        Util::Array<Util::Tuple<VkDevice, VkImageView, VkImage>> textures;
        Util::Array<Util::Tuple<VkDevice, VkImageView>> textureViews;
        Util::Array<Util::Tuple<VkDevice, VkBuffer>> buffers;
        Util::Array<Util::Tuple<VkDevice, VkCommandPool, VkCommandBuffer
            #if NEBULA_GRAPHICS_DEBUG
            , Util::Array<NvidiaAftermathCheckpoint
            #endif
            >>> commandBuffers;
        Util::Array<Util::Tuple<VkDevice, VkDescriptorPool, VkDescriptorSet, uint*>> resourceTables;
        Util::Array<Util::Tuple<VkDevice, VkFramebuffer, VkRenderPass>> passes;
        Util::Array<Util::Tuple<VkDevice, VkAccelerationStructureKHR>> ases;
        Util::Array<CoreGraphics::Alloc> allocs;
    };
    Util::FixedArray<PendingDeletes> pendingDeletes;
    Util::Array<PendingDeletes> deletesPerSubmission;
    Util::FixedArray<Util::Array<CoreGraphics::SubmissionWaitEvent>> waitEvents;

    struct PendingMarkers
    {
        Util::Array<Util::Array<CoreGraphics::FrameProfilingMarker>> markers;
        Util::Array<uint> baseOffset;
    };
    Util::FixedArray<Util::FixedArray<PendingMarkers>> pendingMarkers;

    struct Queries
    {
        CoreGraphics::BufferId queryBuffer[(uint)CoreGraphics::QueryType::NumQueryTypes];
        VkQueryPool queryPools[(uint)CoreGraphics::QueryType::NumQueryTypes];
        Threading::AtomicCounter queryFreeCount[(uint)CoreGraphics::QueryType::NumQueryTypes];
        uint queryMaxCount[(uint)CoreGraphics::QueryType::NumQueryTypes];
    };
    Util::FixedArray<Queries> queries;

    VkSemaphore waitForPresentSemaphore;

    VkExtensionProperties physicalExtensions[64];

    uint32_t usedPhysicalExtensions;
    const char* deviceExtensionStrings[64];

    uint32_t usedExtensions;
    const char* extensions[64];

    uint32_t drawQueueFamily, computeQueueFamily, transferQueueFamily, sparseQueueFamily;
    uint32_t drawQueueCount, computeQueueCount, transferQueueCount, sparseQueueCount;
    Util::Set<uint32_t> usedQueueFamilies;
    Util::FixedArray<uint32_t> queueFamilyMap;

    // setup management classes
    VkSubContextHandler queueHandler;
    VkPipelineDatabase database;

    // device handling (multi GPU?!?!)
    Util::FixedArray<VkDevice> devices;
    Util::FixedArray<VkPhysicalDevice> physicalDevices;
    Util::FixedArray<VkPhysicalDeviceProperties2> deviceProps;
    Util::FixedArray<VkPhysicalDeviceAccelerationStructurePropertiesKHR> accelerationStructureDeviceProps;
    Util::FixedArray<VkPhysicalDeviceRayTracingPipelinePropertiesKHR> raytracingDeviceProps;
    Util::FixedArray<VkPhysicalDeviceFeatures> deviceFeatures;
    Util::FixedArray<uint32_t> numCaps;
    Util::FixedArray<Util::FixedArray<const char*>> deviceFeatureStrings;
    IndexT currentDevice;

    _declare_counter(NumPipelinesBuilt);
    _declare_timer(DebugTimer);

} state;

VkDebugUtilsMessengerEXT VkErrorDebugMessageHandle = nullptr;
PFN_vkCreateDebugUtilsMessengerEXT VkCreateDebugMessenger = nullptr;
PFN_vkDestroyDebugUtilsMessengerEXT VkDestroyDebugMessenger = nullptr;

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
SetupAdapter(CoreGraphics::GraphicsDeviceCreateInfo::Features features)
{
    // retrieve available GPUs
    uint32_t gpuCount;
    VkResult res;
    res = vkEnumeratePhysicalDevices(state.instance, &gpuCount, NULL);
    n_assert(res == VK_SUCCESS);

    state.devices.Resize(gpuCount);
    state.physicalDevices.Resize(gpuCount);
    state.numCaps.Resize(gpuCount);
    state.deviceFeatureStrings.Resize(gpuCount);
    state.deviceProps.Resize(gpuCount);
    state.accelerationStructureDeviceProps.Resize(gpuCount);
    state.raytracingDeviceProps.Resize(gpuCount);
    state.deviceFeatures.Resize(gpuCount);

    if (gpuCount > 0)
    {
        res = vkEnumeratePhysicalDevices(state.instance, &gpuCount, state.physicalDevices.Begin());
        n_assert(res == VK_SUCCESS);

        if (gpuCount > 1)
            n_printf("Found %d GPUs, which is more than 1! Perhaps the Render Device should be able to use it?\n", gpuCount);

        IndexT i;
        for (i = 0; i < (IndexT)1; i++)
        {
            // Get device props and features
            state.accelerationStructureDeviceProps[i] =
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR,
                .pNext = nullptr
            };
            state.raytracingDeviceProps[i] =
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
                .pNext = &state.accelerationStructureDeviceProps[i]
            };

            state.deviceProps[i] =
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
                .pNext = &state.raytracingDeviceProps[i]
            };
            vkGetPhysicalDeviceProperties2(state.physicalDevices[i], &state.deviceProps[i]);
            vkGetPhysicalDeviceFeatures(state.physicalDevices[i], &state.deviceFeatures[i]);

            res = vkEnumerateDeviceExtensionProperties(state.physicalDevices[i], nullptr, &state.numCaps[i], nullptr);
            n_assert(res == VK_SUCCESS);

            bool validDevice = true;
            if (state.numCaps[i] > 0)
            {
                Util::FixedArray<VkExtensionProperties> caps;
                caps.Resize(state.numCaps[i]);
                state.deviceFeatureStrings[i].Resize(state.numCaps[i]);

                res = vkEnumerateDeviceExtensionProperties(state.physicalDevices[i], nullptr, &state.numCaps[i], caps.Begin());
                n_assert(res == VK_SUCCESS);

                Util::Set<Util::String> existingExtensions;
                for (const VkExtensionProperties& extension : caps)
                    existingExtensions.Add(extension.extensionName);

                uint32_t newNumCaps = 0;

                static const Util::String requiredExtensions[] =
                {
                    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                    VK_KHR_MAINTENANCE_1_EXTENSION_NAME,
                    VK_KHR_MAINTENANCE_2_EXTENSION_NAME,
                    VK_KHR_MAINTENANCE_3_EXTENSION_NAME,
                    VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
                    VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME,
                };

                for (int j = 0; j < lengthof(requiredExtensions); j++)
                {
                    if (existingExtensions.FindIndex(requiredExtensions[j]) != InvalidIndex)
                    {
                        if (requiredExtensions[j] == VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME)
                        {
                            CoreGraphics::DynamicVertexInputSupported = true;
                            n_printf("[Graphics Device] Dynamic Vertex Input is enabled\n");
                        }

                        state.deviceFeatureStrings[i][newNumCaps++] = requiredExtensions[j].AsCharPtr();
                    }
                    else
                        validDevice = false;
                }

                static const Util::String wantedExtensions[] =
                {
                    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                    VK_EXT_MESH_SHADER_EXTENSION_NAME,
                    VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
                    VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
                    VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
#if NEBULA_GRAPHICS_DEBUG
                    VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME,
                    VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME
#endif
                };

                for (uint32_t j = 0; j < lengthof(wantedExtensions); j++)
                {
                    if (existingExtensions.FindIndex(wantedExtensions[j]) != InvalidIndex)
                    {
                        if (features.enableRayTracing)
                        {
                            if (wantedExtensions[j] == VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
                            {
                                CoreGraphics::RayTracingSupported = true;
                                n_printf("[Graphics Device] Ray Tracing is enabled\n");
                            }
                        }
                        if (features.enableMeshShaders)
                        {
                            if (wantedExtensions[j] == VK_EXT_MESH_SHADER_EXTENSION_NAME)
                            {
                                CoreGraphics::MeshShadersSupported = true;
                                n_printf("[Graphics Device] Mesh Shaders are enabled\n");
                            }
                        }

                        if (features.enableVariableRateShading)
                        {
                            if (wantedExtensions[j] == VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME)
                            {
                                CoreGraphics::VariableRateShadingSupported = true;
                                n_printf("[Graphics Device] Variable Rate Shading is enabled\n");
                            }
                        }
                        if (features.enableGPUCrashAnalytics)
                        {
                            if (wantedExtensions[j] == VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME)
                            {
                                CoreGraphics::NvidiaCheckpointsSupported = true;
                                _IMP_VK_DYN(vkCmdSetCheckpointNV, state.instance);
                                _IMP_VK_DYN(vkGetQueueCheckpointDataNV, state.instance);
                                n_printf("[Graphics Device] Nvidia Checkpoints are enabled\n");
                            }
                        }
                        state.deviceFeatureStrings[i][newNumCaps++] = wantedExtensions[j].AsCharPtr();
                    }
                }

                if (!CoreGraphics::RayTracingSupported)
                    n_printf("[Graphics Device] Ray Tracing is disabled\n");
                if (!CoreGraphics::DynamicVertexInputSupported)
                    n_printf("[Graphics Device] Dynamic Vertex Input is disabled\n");
                if (!CoreGraphics::MeshShadersSupported)
                    n_printf("[Graphics Device] Mesh Shaders are disabled\n");
                if (!CoreGraphics::VariableRateShadingSupported)
                    n_printf("[Graphics Device] Variable Rate Shading is disabled\n");
                
                state.numCaps[i] = newNumCaps;

                if (validDevice)
                {
                    n_printf("[Graphics Device] Using '%s' as primary graphics adapter", state.deviceProps[i].properties.deviceName);
                    state.currentDevice = i;
                    CoreGraphics::ReadWriteBufferAlignment = state.deviceProps[i].properties.limits.minStorageBufferOffsetAlignment;
                    CoreGraphics::ConstantBufferAlignment = state.deviceProps[i].properties.limits.minUniformBufferOffsetAlignment;
                    CoreGraphics::MaxConstantBufferSize = state.deviceProps[i].properties.limits.maxUniformBufferRange;
                    CoreGraphics::SparseAddressSize = state.deviceProps[i].properties.limits.sparseAddressSpaceSize;

                    CoreGraphics::MaxPerStageConstantBuffers = state.deviceProps[i].properties.limits.maxPerStageDescriptorUniformBuffers;
                    CoreGraphics::MaxPerStageReadWriteBuffers = state.deviceProps[i].properties.limits.maxPerStageDescriptorStorageBuffers;
                    CoreGraphics::MaxPerStageSampledImages = state.deviceProps[i].properties.limits.maxPerStageDescriptorSampledImages;
                    CoreGraphics::MaxPerStageReadWriteImages = state.deviceProps[i].properties.limits.maxPerStageDescriptorStorageImages;
                    CoreGraphics::MaxPerStageSamplers = state.deviceProps[i].properties.limits.maxPerStageDescriptorSamplers;
                    CoreGraphics::MaxPerStageInputAttachments = state.deviceProps[i].properties.limits.maxPerStageDescriptorInputAttachments;

                    CoreGraphics::MaxPushConstantSize = state.deviceProps[i].properties.limits.maxPushConstantsSize;
                    CoreGraphics::MaxResourceTableConstantBuffers = state.deviceProps[i].properties.limits.maxDescriptorSetUniformBuffers;
                    CoreGraphics::MaxResourceTableDynamicOffsetConstantBuffers = state.deviceProps[i].properties.limits.maxDescriptorSetUniformBuffersDynamic;
                    CoreGraphics::MaxResourceTableReadWriteBuffers = state.deviceProps[i].properties.limits.maxDescriptorSetStorageBuffers;
                    CoreGraphics::MaxResourceTableDynamicOffsetReadWriteBuffers = state.deviceProps[i].properties.limits.maxDescriptorSetStorageBuffersDynamic;
                    CoreGraphics::MaxResourceTableSampledImages = state.deviceProps[i].properties.limits.maxDescriptorSetSampledImages;
                    CoreGraphics::MaxResourceTableReadWriteImages = state.deviceProps[i].properties.limits.maxDescriptorSetStorageImages;
                    CoreGraphics::MaxResourceTableSamplers = state.deviceProps[i].properties.limits.maxDescriptorSetSamplers;
                    CoreGraphics::MaxResourceTableInputAttachments = state.deviceProps[i].properties.limits.maxDescriptorSetInputAttachments;

                    CoreGraphics::MemoryRangeGranularity = state.deviceProps[i].properties.limits.nonCoherentAtomSize;
                    CoreGraphics::TimestampPeriod = state.deviceProps[i].properties.limits.timestampPeriod;

                    CoreGraphics::AccelerationStructureScratchAlignment = state.accelerationStructureDeviceProps[i].minAccelerationStructureScratchOffsetAlignment;
                    CoreGraphics::ShaderGroupAlignment = state.raytracingDeviceProps[i].shaderGroupBaseAlignment;
                    CoreGraphics::ShaderGroupSize = state.raytracingDeviceProps[i].shaderGroupHandleSize;
                    CoreGraphics::MaxRecursionDepth = state.raytracingDeviceProps[i].maxRayRecursionDepth;

                    break;
                }
            }
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
VkPhysicalDeviceFeatures 
GetCurrentFeatures()
{
    return state.deviceFeatures[state.currentDevice];
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
DelayedDeleteVkBuffer(const VkDevice dev, const VkBuffer buf)
{
    Threading::CriticalScope scope(&delayedDeleteSection);
    n_assert(dev != VK_NULL_HANDLE);
    n_assert(buf != VK_NULL_HANDLE);
    state.pendingDeletes[state.currentBufferedFrameIndex].buffers.Append(Util::MakeTuple(dev, buf));
}

//------------------------------------------------------------------------------
/**
*/
VkQueryPool
GetQueryPool(const CoreGraphics::QueryType query)
{
    return state.queries[state.currentBufferedFrameIndex].queryPools[query];
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
const VkQueue 
GetQueue(const CoreGraphics::QueueType type, const IndexT index)
{
    return state.queueHandler.queues[type][index];
}

//------------------------------------------------------------------------------
/**
*/
const VkQueue 
GetCurrentQueue(const CoreGraphics::QueueType type)
{
    return state.queueHandler.GetQueue(type);
}

static Threading::CriticalSection pipelineMutex;
//------------------------------------------------------------------------------
/**
*/
VkPipeline
GetOrCreatePipeline(
    CoreGraphics::PassId pass
    , uint subpass
    , CoreGraphics::ShaderProgramId program
    , const CoreGraphics::InputAssemblyKey inputAssembly
    , const VkGraphicsPipelineCreateInfo& info)
{
    Threading::CriticalScope scope(&pipelineMutex);
    VkPipeline pipeline = state.database.GetCompiledPipeline(pass, subpass, program, inputAssembly, info);
    _incr_counter(state.NumPipelinesBuilt, 1);
    return pipeline;
}

//------------------------------------------------------------------------------
/**
*/
void 
SparseTextureBind(const VkImage img, const Util::Array<VkSparseMemoryBind>& opaqueBinds, const Util::Array<VkSparseImageMemoryBind>& pageBinds)
{
    CoreGraphics::SubmissionWaitEvent sparseWait;
    sparseWait.timelineIndex = state.queueHandler.AppendSparseBind(CoreGraphics::SparseQueueType, img, opaqueBinds, pageBinds);
    sparseWait.queue = CoreGraphics::SparseQueueType;

    // Set wait events in graphics device
    state.waitEvents[state.currentBufferedFrameIndex].Append(sparseWait);

    state.sparseImageBinds.Append(GraphicsDeviceState::SparseImageBind{.img = img, .opaqueBinds = opaqueBinds, .pageBinds = pageBinds});
    //state.queueHandler.AppendSparseBind(CoreGraphics::SparseQueueType, img, opaqueBinds, pageBinds);
}

//------------------------------------------------------------------------------
/**
*/
void
SparseBufferBind(const VkBuffer buf, const Util::Array<VkSparseMemoryBind>& binds)
{
    state.sparseBufferBinds.Append(GraphicsDeviceState::SparseBufferBind{.buf = buf, .binds = binds});
    //state.queueHandler.AppendSparseBind(CoreGraphics::SparseQueueType, buf, binds);
}

//------------------------------------------------------------------------------
/**
*/
void
ClearPending()
{
    GraphicsDeviceState::PendingDeletes& pendingDeletes = state.pendingDeletes[state.currentBufferedFrameIndex];

    // Clear up any pending deletes
    for (auto& [dev, pool, buf
#if NEBULA_GRAPHICS_DEBUG
        , nvCheckpoints
#endif

    ] : pendingDeletes.commandBuffers)
    {
        vkFreeCommandBuffers(dev, pool, 1, &buf);
#if NEBULA_GRAPHICS_DEBUG
        nvCheckpoints.Clear();
#endif
    }
    pendingDeletes.commandBuffers.Clear();

    for (const auto& [dev, buf] : pendingDeletes.buffers)
    {
        n_assert(dev != nullptr);
        n_assert(buf != nullptr);
        vkDestroyBuffer(dev, buf, nullptr);
    }
    pendingDeletes.buffers.Clear();

    for (const auto& [dev, view, img] : pendingDeletes.textures)
    {
        vkDestroyImage(dev, img, nullptr);
        vkDestroyImageView(dev, view, nullptr);
    }
    pendingDeletes.textures.Clear();

    for (const auto& [dev, view] : pendingDeletes.textureViews)
    {
        vkDestroyImageView(dev, view, nullptr);
    }
    pendingDeletes.textureViews.Clear();

    for (const auto& [dev, pool, set, size] : pendingDeletes.resourceTables)
    {
        vkFreeDescriptorSets(dev, pool, 1, &set);
        (*size)++;
    }
    pendingDeletes.resourceTables.Clear();

    for (const auto& [dev, fbo, pass] : pendingDeletes.passes)
    {
        vkDestroyRenderPass(dev, pass, nullptr);
        vkDestroyFramebuffer(dev, fbo, nullptr);
    }
    pendingDeletes.passes.Clear();

    for (const auto& [dev, as] : pendingDeletes.ases)
    {
        vkDestroyAccelerationStructureKHR(dev, as, nullptr);
    }
    pendingDeletes.ases.Clear();

    for (const auto& alloc : pendingDeletes.allocs)
    {
        CoreGraphics::FreeMemory(alloc);
    }
    pendingDeletes.allocs.Clear();

    for (auto& markers : state.pendingMarkers)
    {
        markers[state.currentBufferedFrameIndex].markers.Clear();
        markers[state.currentBufferedFrameIndex].baseOffset.Clear();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DeviceLost()
{
    #if NEBULA_GRAPHICS_DEBUG
    if (CoreGraphics::NvidiaCheckpointsSupported)
    {
        n_printf("******** NVIDIA CRASH REPORT ********\n");
        for (int i = 0; i < CoreGraphics::QueueType::NumQueueTypes; i++)
        {
            switch (i)
            {
            case CoreGraphics::QueueType::GraphicsQueueType:
                n_printf("Graphics Queue: \n");
                break;
            case CoreGraphics::QueueType::ComputeQueueType:
                n_printf("Compute Queue: \n");
                break;
            case CoreGraphics::QueueType::TransferQueueType:
                n_printf("Transfer Queue: \n");
                break;
            case CoreGraphics::QueueType::SparseQueueType:
                n_printf("Sparse Queue: \n");
                break;
            }
            VkQueue queue = state.queueHandler.GetQueue((CoreGraphics::QueueType)i);
            uint32_t numCheckpoints;
            vkGetQueueCheckpointDataNV(queue, &numCheckpoints, nullptr);
            Util::FixedArray<VkCheckpointDataNV> checkpoints(numCheckpoints);
            for (auto& checkpoint : checkpoints)
            {
                checkpoint.sType = VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;
                checkpoint.pNext = nullptr;
            }
            vkGetQueueCheckpointDataNV(queue, &numCheckpoints, checkpoints.Begin());

            // Print GPU stack trace
            int indentation = 0;
            for (int j = 0; j < numCheckpoints; j++)
            {
                const VkCheckpointDataNV& data = checkpoints[j];
                NvidiaAftermathCheckpoint* userData = (NvidiaAftermathCheckpoint*)data.pCheckpointMarker;

                if (userData != nullptr)
                {
                    NvidiaAftermathCheckpoint* next = userData;
                    while (next != nullptr)
                    {
                        if (!next->push)
                            indentation--;

                        if (next->name != nullptr)
                        {
                            n_printf("%*s", indentation * 4, "-");
                            n_printf(" %s\n", next->name.AsCharPtr());
                        }

                        if (next->push)
                            indentation++;

                        NvidiaAftermathCheckpoint* prev = next->prev;
                        next = prev;
                    }
                }
            }
        }
        n_printf("******** END OF CRASH REPORT ********\n");

    }
    #endif
}

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
    n_warning("%s\n", callbackData->pMessage);
    return VK_FALSE;
}

namespace CoreGraphics
{

bool RayTracingSupported = false;
bool DynamicVertexInputSupported = false;
bool MeshShadersSupported = false;
bool VariableRateShadingSupported = false;
bool NvidiaCheckpointsSupported = false;

uint ReadWriteBufferAlignment = UINT_MAX;
uint ConstantBufferAlignment = UINT_MAX;
uint64 MaxConstantBufferSize = UINT_MAX;
uint MaxPushConstantSize = UINT_MAX;
uint64 SparseAddressSize = UINT_MAX;

uint MaxPerStageConstantBuffers = UINT_MAX;
uint MaxPerStageReadWriteBuffers = UINT_MAX;
uint MaxPerStageSampledImages = UINT_MAX;
uint MaxPerStageReadWriteImages = UINT_MAX;
uint MaxPerStageSamplers = UINT_MAX;
uint MaxPerStageInputAttachments = UINT_MAX;

uint MaxResourceTableConstantBuffers = UINT_MAX;
uint MaxResourceTableDynamicOffsetConstantBuffers = UINT_MAX;
uint MaxResourceTableReadWriteBuffers = UINT_MAX;
uint MaxResourceTableDynamicOffsetReadWriteBuffers = UINT_MAX;
uint MaxResourceTableSampledImages = UINT_MAX;
uint MaxResourceTableReadWriteImages = UINT_MAX;
uint MaxResourceTableSamplers = UINT_MAX;
uint MaxResourceTableInputAttachments = UINT_MAX;

uint MemoryRangeGranularity = UINT_MAX;
uint TimestampPeriod = UINT_MAX;

uint AccelerationStructureScratchAlignment = UINT_MAX;
uint ShaderGroupAlignment = UINT_MAX;
uint64 ShaderGroupSize = UINT_MAX;
uint MaxRecursionDepth = UINT_MAX;

using namespace Vulkan;

#if NEBULA_GRAPHICS_DEBUG
template<> void ObjectSetName(const CoreGraphics::CmdBufferId id, const char* name);
#endif

N_DECLARE_COUNTER(N_CONSTANT_MEMORY, Graphics Constant Memory);
N_DECLARE_COUNTER(N_VERTEX_MEMORY, Vertex Memory);
N_DECLARE_COUNTER(N_INDEX_MEMORY, Index Memory);
N_DECLARE_COUNTER(N_UPLOAD_MEMORY, Upload Memory);

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
        VK_API_VERSION_1_3											// API version
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
    state.currentDevice = 0;
    SetupAdapter(info.features);

    if (info.enableValidation)
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
    VkQueueFamilyProperties* queuesProps = new VkQueueFamilyProperties[numQueues];
    vkGetPhysicalDeviceQueueFamilyProperties(state.physicalDevices[state.currentDevice], &numQueues, queuesProps);
    vkGetPhysicalDeviceMemoryProperties(state.physicalDevices[state.currentDevice], &state.memoryProps);

    state.drawQueueFamily = UINT32_MAX;
    state.computeQueueFamily = UINT32_MAX;
    state.transferQueueFamily = UINT32_MAX;
    state.sparseQueueFamily = UINT32_MAX;

    // create three queues for each family
    for (i = 0; i < numQueues; i++)
    {
        int numQueues = queuesProps[i].queueCount;
        // just pick whichever queue supports graphics, it will most likely only be 1
        if (AllBits(queuesProps[i].queueFlags, VK_QUEUE_GRAPHICS_BIT)
            && state.drawQueueFamily == UINT32_MAX)
        {
            state.drawQueueFamily = i;
            state.drawQueueCount = numQueues;
            continue;
        }

        // find a compute queue which is not for graphics
        if (AllBits(queuesProps[i].queueFlags, VK_QUEUE_COMPUTE_BIT)
            && state.computeQueueFamily == UINT32_MAX)
        {
            state.computeQueueFamily = i;
            state.computeQueueCount = numQueues;
            continue;
        }

        // find a transfer queue that is purely for transfers
        if (AllBits(queuesProps[i].queueFlags, VK_QUEUE_TRANSFER_BIT)
            && state.transferQueueFamily == UINT32_MAX)
        {
            state.transferQueueFamily = i;
            state.transferQueueCount = numQueues;
            continue;
        }

        // find a sparse or transfer queue that supports sparse binding
        if (AllBits(queuesProps[i].queueFlags, VK_QUEUE_SPARSE_BINDING_BIT)
            && state.sparseQueueFamily == UINT32_MAX)
        {
            state.sparseQueueFamily = i;
            state.sparseQueueCount = numQueues;
            continue;
        }
    }

    // could not find pure compute queue
    if (state.computeQueueFamily == UINT32_MAX)
    {
        // assert that the graphics queue can handle computes
        n_assert(queuesProps[state.drawQueueFamily].queueFlags & VK_QUEUE_COMPUTE_BIT);
        state.computeQueueFamily = state.drawQueueFamily;
        state.computeQueueCount = 1;
    }

    // could not find pure transfer queue
    if (state.transferQueueFamily == UINT32_MAX)
    {
        // assert the draw queue can handle transfers
        n_assert(queuesProps[state.drawQueueFamily].queueFlags & VK_QUEUE_TRANSFER_BIT);
        state.transferQueueFamily = state.drawQueueFamily;
        state.transferQueueCount = 1;
    }

    // could not find pure transfer queue
    if (state.sparseQueueFamily == UINT32_MAX)
    {
        if (queuesProps[state.transferQueueFamily].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
        {
            state.sparseQueueFamily = state.transferQueueFamily;
            state.sparseQueueCount = state.transferQueueCount;
        }
        else
        {
            n_warn2(queuesProps[state.drawQueueFamily].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT, "VkGraphicsDevice: No sparse binding queue could be found!\n");
            state.sparseQueueFamily = state.drawQueueFamily;
            state.sparseQueueCount = state.drawQueueCount;
        }
    }

    if (state.drawQueueFamily == UINT32_MAX)		n_error("VkGraphicsDevice: Could not find a queue for graphics and present.\n");
    if (state.computeQueueFamily == UINT32_MAX)		n_error("VkGraphicsDevice: Could not find a queue for compute.\n");
    if (state.transferQueueFamily == UINT32_MAX)	n_error("VkGraphicsDevice: Could not find a queue for transfers.\n");
    if (state.sparseQueueFamily == UINT32_MAX)		n_warning("VkGraphicsDevice: Could not find a queue for sparse binding.\n");

    // This setup must match the enum CoreGraphics::QueueType
    Util::FixedArray<Util::Pair<uint32_t, uint32_t>> queueSetup =
    {
        { state.drawQueueFamily, state.drawQueueCount }
        , { state.computeQueueFamily, state.computeQueueCount }
        , { state.transferQueueFamily, state.transferQueueCount }
        , { state.sparseQueueFamily, state.sparseQueueCount }
    };

    Util::Dictionary<uint32_t, uint32_t> uniqueQueues;
    for (auto& [family, count] : queueSetup)
    {
        uniqueQueues.Emplace(family) = count;
    }

    // create device
    Util::FixedArray<Util::FixedArray<float>> prios;
    Util::Array<VkDeviceQueueCreateInfo> queueInfos;
    prios.Resize(uniqueQueues.Size());

    for (i = 0; i < uniqueQueues.Size(); i++)
    {
        auto& [family, count] = uniqueQueues.KeyValuePairAtIndex(i);
        prios[i].Resize(count);
        prios[i].Fill(1.0f);
        queueInfos.Append(
            {
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                nullptr,
                0,
                family,
                count,
                &prios[i][0]
            });
    }

    delete[] queuesProps;

    // get physical device features
    //VkPhysicalDeviceFeatures features;
    //vkGetPhysicalDeviceFeatures(state.physicalDevices[state.currentDevice], &features);

    VkPhysicalDeviceDiagnosticsConfigFeaturesNV nvidiaDeviceDiagnosticsFeature =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DIAGNOSTICS_CONFIG_FEATURES_NV,
        .pNext = nullptr,
        .diagnosticsConfig = false
    };

    VkPhysicalDeviceVulkan11Features vk11Features =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .pNext = &nvidiaDeviceDiagnosticsFeature,
    };

    VkPhysicalDeviceVulkan12Features vk12Features =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &vk11Features,
    };

    VkPhysicalDeviceFeatures2 features2 =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &vk12Features
    };
    vkGetPhysicalDeviceFeatures2(state.physicalDevices[state.currentDevice], &features2);

    vk11Features.pNext = nullptr;
    VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT dynamicVertexFeatures =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT,
        .pNext = &vk12Features,
        .vertexInputDynamicState = true
    };
    void* lastExtension = &dynamicVertexFeatures;

#pragma region Mesh Shader Features
    VkPhysicalDeviceMeshShaderFeaturesEXT meshShadersFeatures =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
        .pNext = lastExtension,
        .taskShader = true,
        .meshShader = true,
        .multiviewMeshShader = false,
        .primitiveFragmentShadingRateMeshShader = info.features.enableVariableRateShading,
        .meshShaderQueries = false,
    };
    if (info.features.enableMeshShaders)
    {
        lastExtension = &meshShadersFeatures;
    }
#pragma endregion

#pragma region Variable Rate Shading Features
    VkPhysicalDeviceFragmentShadingRateFeaturesKHR variableShadingFeatures =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR,
        .pNext = lastExtension,
        .pipelineFragmentShadingRate = true,
        .primitiveFragmentShadingRate = true,
        .attachmentFragmentShadingRate = true,
    };
    if (info.features.enableVariableRateShading)
    {
        lastExtension = &variableShadingFeatures;
    }
#pragma endregion

#pragma region Ray Tracing Features
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .pNext = lastExtension,
        .accelerationStructure = true,
        .accelerationStructureCaptureReplay = true,
        .accelerationStructureIndirectBuild = false,
        .accelerationStructureHostCommands = false,
        .descriptorBindingAccelerationStructureUpdateAfterBind = false,
    };

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytracingFeatures =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        .pNext = &accelerationStructureFeatures,
        .rayTracingPipeline = true,
        .rayTracingPipelineShaderGroupHandleCaptureReplay = false,
        .rayTracingPipelineShaderGroupHandleCaptureReplayMixed = false,
        .rayTracingPipelineTraceRaysIndirect = false,
        .rayTraversalPrimitiveCulling = false
    };

    if (info.features.enableRayTracing)
    {
        lastExtension = &raytracingFeatures;
    }
#pragma endregion

#pragma region Nvidia extensions
#if NEBULA_GRAPHICS_DEBUG

    VkDeviceDiagnosticsConfigCreateInfoNV nvidiaCheckpointConfig =
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV,
        .pNext = lastExtension,
        .flags = VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV  | VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_ERROR_REPORTING_BIT_NV
    };
    nvidiaDeviceDiagnosticsFeature =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DIAGNOSTICS_CONFIG_FEATURES_NV,
        .pNext = &nvidiaCheckpointConfig,
        .diagnosticsConfig = true
    };
    if (CoreGraphics::NvidiaCheckpointsSupported)
    {
        lastExtension = &nvidiaDeviceDiagnosticsFeature;
    }
#endif
#pragma endregion

    VkDeviceCreateInfo deviceInfo =
    {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        lastExtension,
        0,
        (uint32_t)queueInfos.Size(),
        queueInfos.Begin(),
        (uint32_t)numLayers,
        layers,
        state.numCaps[state.currentDevice],
        state.deviceFeatureStrings[state.currentDevice].Begin(),
        &features2.features
    };

    // create device
    res = vkCreateDevice(state.physicalDevices[state.currentDevice], &deviceInfo, NULL, &state.devices[state.currentDevice]);
    n_assert(res == VK_SUCCESS);

    // setup queue handler
    state.queueHandler.Setup(state.devices[state.currentDevice], queueSetup);

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
        info.memoryHeaps[MemoryPool_HostCached],
        info.memoryHeaps[MemoryPool_DeviceAndHost]
        );

    state.constantBufferRings.Resize(info.numBufferedFrames);

    N_BUDGET_COUNTER_SETUP(N_CONSTANT_MEMORY, info.globalConstantBufferMemorySize);
    N_BUDGET_COUNTER_SETUP(N_VERTEX_MEMORY, info.globalVertexBufferMemorySize);
    N_BUDGET_COUNTER_SETUP(N_INDEX_MEMORY, info.globalIndexBufferMemorySize);
    N_BUDGET_COUNTER_SETUP(N_UPLOAD_MEMORY, info.globalUploadMemorySize);

    for (i = 0; i < info.numBufferedFrames; i++)
    {
        Vulkan::GraphicsDeviceState::ConstantsRingBuffer& cboRing = state.constantBufferRings[i];
        cboRing.allowConstantAllocation = true;
        cboRing.endAddress = 0;
        cboRing.gfx.flushedStart = cboRing.cmp.flushedStart = 0;
    }

    BufferCreateInfo cboInfo;
        
    state.globalConstantBufferMaxValue = info.globalConstantBufferMemorySize;

    cboInfo.name = "Global Constant Buffer";
    cboInfo.byteSize = info.globalConstantBufferMemorySize;
    cboInfo.mode = CoreGraphics::BufferAccessMode::DeviceAndHost;
    cboInfo.usageFlags = CoreGraphics::ConstantBuffer | CoreGraphics::TransferBufferDestination;
    state.globalConstantBuffer.Resize(info.numBufferedFrames);
    for (IndexT i = 0; i < info.numBufferedFrames; i++)
    {
        auto gfxCboInfo = cboInfo;
        gfxCboInfo.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
        state.globalConstantBuffer[i] = CreateBuffer(gfxCboInfo);
    }

    state.maxNumBufferedFrames = info.numBufferedFrames;

#ifdef CreateSemaphore
#pragma push_macro("CreateSemaphore")
#undef CreateSemaphore
#endif

    state.presentFences.Resize(info.numBufferedFrames);
    state.renderingFinishedSemaphores.Resize(info.numBufferedFrames);
    for (i = 0; i < info.numBufferedFrames; i++)
    {
        state.presentFences[i] = CreateFence({true});
        state.renderingFinishedSemaphores[i] = CreateSemaphore({
#if NEBULA_GRAPHICS_DEBUG
            .name = "Present", 
#endif
            .type = SemaphoreType::Binary });
    }

#pragma pop_macro("CreateSemaphore")


    // Setup "special" command buffers
    CoreGraphics::CmdBufferPoolCreateInfo setupResourcePoolInfo;
    setupResourcePoolInfo.queue = CoreGraphics::QueueType::GraphicsQueueType;
    setupResourcePoolInfo.resetable = true;
    setupResourcePoolInfo.shortlived = true;
    state.setupGraphicsCommandBufferPool = CoreGraphics::CreateCmdBufferPool(setupResourcePoolInfo);
    setupResourcePoolInfo.queue = CoreGraphics::QueueType::TransferQueueType;
    state.setupTransferCommandBufferPool = CoreGraphics::CreateCmdBufferPool(setupResourcePoolInfo);

    state.pendingDeletes.Resize(info.numBufferedFrames);
    state.waitEvents.Resize(info.numBufferedFrames);
    state.pendingMarkers.Resize(CoreGraphics::QueueType::NumQueueTypes);
    for (auto& markers : state.pendingMarkers)
        markers.Resize(info.numBufferedFrames);
    state.queries.Resize(info.numBufferedFrames);

    VkQueryPoolCreateInfo queryPoolInfo =
    {
        VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        nullptr,
        0
    };

    VkQueryPipelineStatisticFlags statistics =
        VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT
        | VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT
        | VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT
        | VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT
        | VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT
        | VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT
        | VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT;

    for (auto& queries : state.queries)
    {
        for (IndexT i = 0; i < CoreGraphics::QueryType::NumQueryTypes; i++)
        {
            SizeT baseSize = sizeof(uint);
            SizeT byteSize = 0;
            switch (i)
            {
                case CoreGraphics::QueryType::OcclusionQueryType:
                    queryPoolInfo.queryType = VK_QUERY_TYPE_OCCLUSION;
                    queryPoolInfo.queryCount = info.maxOcclusionQueries; // With occlusion culling, we might have thousands of objects
                    byteSize = baseSize * queryPoolInfo.queryCount;
                    break;
                case CoreGraphics::QueryType::TimestampsQueryType:
                    queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
                    queryPoolInfo.queryCount = info.maxTimestampQueries;   // Timestamps will be quite a lot fewer
                    baseSize = sizeof(uint64_t);
                    byteSize = baseSize * queryPoolInfo.queryCount;
                    break;
                case CoreGraphics::QueryType::StatisticsQueryType:
                    queryPoolInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
                    queryPoolInfo.pipelineStatistics = statistics;
                    queryPoolInfo.queryCount = info.maxStatisticsQueries;    // Statistics will be just a handful over the whole command buffer
                    byteSize = 7 * baseSize * queryPoolInfo.queryCount;
                    break;
            }

            // Create query pool, and do an initial reset
            VkResult res = vkCreateQueryPool(state.devices[state.currentDevice], &queryPoolInfo, nullptr, &queries.queryPools[i]);
            n_assert(res == VK_SUCCESS);
            vkResetQueryPool(state.devices[state.currentDevice], queries.queryPools[i], 0, queryPoolInfo.queryCount);
            queries.queryFreeCount[i] = 0;
            queries.queryMaxCount[i] = queryPoolInfo.queryCount;

            // Create results buffer
            CoreGraphics::BufferCreateInfo bufInfo;
            bufInfo.byteSize = byteSize;
            bufInfo.mode = CoreGraphics::HostLocal;
            bufInfo.queueSupport = GraphicsQueueSupport | ComputeQueueSupport;
            bufInfo.usageFlags = CoreGraphics::TransferBufferDestination;
            queries.queryBuffer[i] = CoreGraphics::CreateBuffer(bufInfo);
        }
    }

    state.waitForPresentSemaphore = VK_NULL_HANDLE;

    CoreGraphics::BufferCreateInfo vboInfo;
    vboInfo.name = "Global Vertex Cache";
    vboInfo.byteSize = info.globalVertexBufferMemorySize;
    vboInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
    vboInfo.queueSupport = CoreGraphics::BufferQueueSupport::GraphicsQueueSupport | CoreGraphics::BufferQueueSupport::ComputeQueueSupport | CoreGraphics::BufferQueueSupport::TransferQueueSupport;
    vboInfo.usageFlags =
        CoreGraphics::BufferUsageFlag::VertexBuffer
        | CoreGraphics::BufferUsageFlag::TransferBufferDestination
        | CoreGraphics::BufferUsageFlag::ReadWriteBuffer
        | CoreGraphics::BufferUsageFlag::ShaderAddress
        | CoreGraphics::BufferUsageFlag::AccelerationStructureInput;
    state.vertexBuffer = CoreGraphics::CreateBuffer(vboInfo);
    state.vertexAllocator = Memory::RangeAllocator(info.globalVertexBufferMemorySize, 2048);

    CoreGraphics::BufferCreateInfo iboInfo;
    iboInfo.name = "Global Index Cache";
    iboInfo.byteSize = info.globalIndexBufferMemorySize;
    iboInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
    iboInfo.queueSupport = CoreGraphics::BufferQueueSupport::GraphicsQueueSupport | CoreGraphics::BufferQueueSupport::ComputeQueueSupport | CoreGraphics::BufferQueueSupport::TransferQueueSupport;
    iboInfo.usageFlags =
        CoreGraphics::BufferUsageFlag::IndexBuffer
        | CoreGraphics::BufferUsageFlag::TransferBufferDestination
        | CoreGraphics::BufferUsageFlag::ReadWriteBuffer
        | CoreGraphics::BufferUsageFlag::ShaderAddress
        | CoreGraphics::BufferUsageFlag::AccelerationStructureInput;
    state.indexBuffer = CoreGraphics::CreateBuffer(iboInfo);
    state.indexAllocator = Memory::RangeAllocator(info.globalIndexBufferMemorySize, 2048);

    CoreGraphics::BufferCreateInfo uploadInfo;
    uploadInfo.name = "Global Upload Buffer";
    uploadInfo.byteSize = Math::align(info.globalUploadMemorySize, CoreGraphics::MemoryRangeGranularity);
    uploadInfo.mode = CoreGraphics::BufferAccessMode::HostLocal;
    uploadInfo.queueSupport = CoreGraphics::BufferQueueSupport::GraphicsQueueSupport | CoreGraphics::BufferQueueSupport::ComputeQueueSupport | CoreGraphics::BufferQueueSupport::TransferQueueSupport;
    uploadInfo.usageFlags = CoreGraphics::BufferUsageFlag::TransferBufferSource;

    state.uploadAllocationsToFree.Resize(info.numBufferedFrames);
    state.uploadBuffer = CoreGraphics::CreateBuffer(uploadInfo);
    state.uploadAllocator = Memory::RangeAllocator(info.globalUploadMemorySize, 2048);

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
    state.queueHandler.WaitIdle(GraphicsQueueType);
    state.queueHandler.WaitIdle(ComputeQueueType);
    state.queueHandler.WaitIdle(TransferQueueType);
    state.queueHandler.WaitIdle(SparseQueueType);

    Vulkan::ClearPending();

    // wait for queues and run all pending commands
    state.queueHandler.Discard();

    DiscardMemoryPools(state.devices[state.currentDevice]);

    if (state.globalConstantBufferMaxValue > 0)
    {
        for (IndexT i = 0; i < state.maxNumBufferedFrames; i++)
        {
            DestroyBuffer(state.globalConstantBuffer[i]);
        }
    }

    state.database.Discard();

    IndexT i;
    for (i = 0; i < state.renderingFinishedSemaphores.Size(); i++)
    {
        FenceWait(state.presentFences[i], UINT64_MAX);
        DestroyFence(state.presentFences[i]);
        DestroySemaphore(state.renderingFinishedSemaphores[i]);
    }

#if NEBULA_VULKAN_DEBUG
    VkDestroyDebugMessenger(state.instance, VkErrorDebugMessageHandle, nullptr);
#endif

    vkDestroyDevice(state.devices[state.currentDevice], nullptr);
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
const CoreGraphics::CmdBufferId
LockTransferSetupCommandBuffer()
{
    transferLock.Enter();
    CoreGraphics::CmdBufferCreateInfo cmdCreateInfo;
    cmdCreateInfo.pool = state.setupTransferCommandBufferPool;
    cmdCreateInfo.usage = CoreGraphics::QueueType::TransferQueueType;
    cmdCreateInfo.queryTypes = CoreGraphics::CmdBufferQueryBits::NoQueries;
    cmdCreateInfo.name = "Setup Transfer";
    CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::CreateCmdBuffer(cmdCreateInfo);

    CoreGraphics::CmdBeginRecord(cmdBuf, { true, false, false });
    CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_PURPLE, "Transfer");
    return cmdBuf;
}

//------------------------------------------------------------------------------
/**
*/
void
UnlockTransferSetupCommandBuffer(CoreGraphics::CmdBufferId cmdBuf)
{
    CoreGraphics::CmdEndMarker(cmdBuf);
    CoreGraphics::CmdEndRecord(cmdBuf);
    state.setupTransferCommandBuffers.Append(cmdBuf);
    CoreGraphics::CmdBufferIdRelease(cmdBuf);
    transferLock.Leave();
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::CmdBufferId
LockGraphicsSetupCommandBuffer(const char* name)
{
    setupLock.Enter();
    CoreGraphics::CmdBufferCreateInfo cmdCreateInfo;
    cmdCreateInfo.pool = state.setupGraphicsCommandBufferPool;
    cmdCreateInfo.usage = CoreGraphics::QueueType::GraphicsQueueType;
    cmdCreateInfo.queryTypes = CoreGraphics::CmdBufferQueryBits::NoQueries;
    cmdCreateInfo.name = name == nullptr ? "Setup" : name;
    CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::CreateCmdBuffer(cmdCreateInfo);

    CoreGraphics::CmdBeginRecord(cmdBuf, { true, false, false });
    CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_PURPLE, name == nullptr ? "Setup" : name);
    return cmdBuf;
}

//------------------------------------------------------------------------------
/**
*/
void
UnlockGraphicsSetupCommandBuffer(CoreGraphics::CmdBufferId cmdBuf)
{
    CoreGraphics::CmdEndMarker(cmdBuf);
    CoreGraphics::CmdEndRecord(cmdBuf);
    state.setupGraphicsCommandBuffers.Append(cmdBuf);
    CoreGraphics::CmdBufferIdRelease(cmdBuf);
    setupLock.Leave();
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::CmdBufferId
LockTransferHandoverSetupCommandBuffer()
{
    transferLock.Enter();
    CoreGraphics::CmdBufferCreateInfo cmdCreateInfo;
    cmdCreateInfo.pool = state.setupTransferCommandBufferPool;
    cmdCreateInfo.usage = CoreGraphics::QueueType::TransferQueueType;
    cmdCreateInfo.queryTypes = CoreGraphics::CmdBufferQueryBits::NoQueries;
    cmdCreateInfo.name = "Transfer Handover";
    CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::CreateCmdBuffer(cmdCreateInfo);

    CoreGraphics::CmdBeginRecord(cmdBuf, { true, false, false });
    CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_PURPLE, "Transfer Handover");
    return cmdBuf;
}

//------------------------------------------------------------------------------
/**
*/
void
UnlockTransferHandoverSetupCommandBuffer(CoreGraphics::CmdBufferId cmdBuf)
{
    CoreGraphics::CmdEndMarker(cmdBuf);
    CoreGraphics::CmdEndRecord(cmdBuf);
    state.handoverTransferCommandBuffers.Append(cmdBuf);
    CoreGraphics::CmdBufferIdRelease(cmdBuf);
    transferLock.Leave();
}

//------------------------------------------------------------------------------
/**
*/
bool
PollSubmissionIndex(const CoreGraphics::QueueType queue, uint64 index)
{
    return state.queueHandler.Poll(queue, index);
}

//------------------------------------------------------------------------------
/**
*/
void 
AddSubmissionEvent(const CoreGraphics::SubmissionWaitEvent& event)
{
    state.waitEvents[state.currentBufferedFrameIndex].Append(event);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::SubmissionWaitEvent
SubmitCommandBuffers(
    const Util::Array<CoreGraphics::CmdBufferId>& cmds
    , CoreGraphics::QueueType type
    , Util::Array<CoreGraphics::SubmissionWaitEvent> waitEvents
#if NEBULA_GRAPHICS_DEBUG
    , const char* name
#endif
)
{
    // Append submission
    Threading::CriticalScope _0(&submitLock);
    Util::Array<VkCommandBuffer> vkBufs;
    vkBufs.Reserve(cmds.Size());
    for (auto cmd : cmds)
        vkBufs.Append(CmdBufferGetVk(cmd));

    CoreGraphics::SubmissionWaitEvent ret;
    ret.timelineIndex = state.queueHandler.AppendSubmissionTimeline(
        type
        , vkBufs
        , waitEvents
#if NEBULA_GRAPHICS_DEBUG
        , name
#endif
    );
    ret.queue = type;

    // Add wait event
    AddSubmissionEvent(ret);

#if NEBULA_ENABLE_PROFILING
    for (auto cmdBuf : cmds)
    {
        if (CoreGraphics::CmdRecordsMarkers(cmdBuf))
        {
            Util::Array<CoreGraphics::FrameProfilingMarker> markers = CmdCopyProfilingMarkers(cmdBuf);
            state.pendingMarkers[type][state.currentBufferedFrameIndex].markers.Append(std::move(markers));
            state.pendingMarkers[type][state.currentBufferedFrameIndex].baseOffset.Append(CmdGetMarkerOffset(cmdBuf));
        }
    }
#endif
    
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
SubmitImmediateCommandBuffers()
{
    CoreGraphics::SubmissionWaitEvent handoverWait, graphicsWait, uploadWait;

    CoreGraphics::FlushUploads(state.uploadAllocationsToFlush);
    state.uploadAllocationsToFlush.Clear();

    // Submit transfer and graphics commands from this frame
    transferLock.Enter();
    if (!state.setupTransferCommandBuffers.IsEmpty())
    {
        Util::Array<VkCommandBuffer> vkBufs;
        vkBufs.Reserve(state.setupTransferCommandBuffers.Size());
        for (auto cmdBuf : state.setupTransferCommandBuffers)
            vkBufs.Append(CmdBufferGetVk(cmdBuf));

        uploadWait.timelineIndex = state.queueHandler.AppendSubmissionTimeline(CoreGraphics::TransferQueueType, vkBufs, nullptr, "Transfer");
        uploadWait.queue = CoreGraphics::TransferQueueType;

        // Set wait events in graphics device
        AddSubmissionEvent(uploadWait);

        for (int i = 0; i < state.setupTransferCommandBuffers.Size(); i++)
        {
            DeferredDestroyCmdBuffer(state.setupTransferCommandBuffers[i]);
        }
        state.setupTransferCommandBuffers.Clear();
    }

    if (!state.handoverTransferCommandBuffers.IsEmpty())
    {
        Util::Array<VkCommandBuffer> vkBufs;
        vkBufs.Reserve(state.handoverTransferCommandBuffers.Size());
        for (auto cmdBuf : state.handoverTransferCommandBuffers)
            vkBufs.Append(CmdBufferGetVk(cmdBuf));

        handoverWait.timelineIndex = state.queueHandler.AppendSubmissionTimeline(CoreGraphics::TransferQueueType, vkBufs, { uploadWait }, "Handover");
        handoverWait.queue = CoreGraphics::TransferQueueType;

        // Set wait events in graphics device
        AddSubmissionEvent(handoverWait);

        for (int i = 0; i < state.handoverTransferCommandBuffers.Size(); i++)
        {
            DeferredDestroyCmdBuffer(state.handoverTransferCommandBuffers[i]);
        }
        state.handoverTransferCommandBuffers.Clear();
    }
    transferLock.Leave();

    setupLock.Enter();
    if (!state.setupGraphicsCommandBuffers.IsEmpty())
    {
        Util::Array<VkCommandBuffer> vkBufs;
        vkBufs.Reserve(state.setupGraphicsCommandBuffers.Size());
        for (auto cmdBuf : state.setupGraphicsCommandBuffers)
            vkBufs.Append(CmdBufferGetVk(cmdBuf));

        graphicsWait.timelineIndex = state.queueHandler.AppendSubmissionTimeline(CoreGraphics::GraphicsQueueType, vkBufs, { handoverWait }, "Setup");
        graphicsWait.queue = CoreGraphics::GraphicsQueueType;

        // Add wait event
        AddSubmissionEvent(graphicsWait);

        for (int i = 0; i < state.setupGraphicsCommandBuffers.Size(); i++)
        {
            DeferredDestroyCmdBuffer(state.setupGraphicsCommandBuffers[i]);
        }
        state.setupGraphicsCommandBuffers.Clear();
    }
    setupLock.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
UnlockConstantUpdates()
{
    Vulkan::GraphicsDeviceState::ConstantsRingBuffer& sub = state.constantBufferRings[state.currentBufferedFrameIndex];
    sub.allowConstantAllocation = true;
}

//------------------------------------------------------------------------------
/**
*/
void
LockConstantUpdates()
{
    Vulkan::GraphicsDeviceState::ConstantsRingBuffer& sub = state.constantBufferRings[state.currentBufferedFrameIndex];
    sub.allowConstantAllocation = false;
}

//------------------------------------------------------------------------------
/**
    Set constants for preallocated memory
*/
void
SetConstantsInternal(ConstantBufferOffset offset, const void* data, SizeT size)
{
    BufferUpdate(state.globalConstantBuffer[state.currentBufferedFrameIndex], data, size, offset);
}

//------------------------------------------------------------------------------
/**
*/
ConstantBufferOffset
AllocateConstantBufferMemory(uint size)
{
    Vulkan::GraphicsDeviceState::ConstantsRingBuffer& sub = state.constantBufferRings[state.currentBufferedFrameIndex];
    n_assert(sub.allowConstantAllocation);

    // Calculate aligned upper bound
    int alignedSize = Math::align(size, state.deviceProps[state.currentDevice].properties.limits.minUniformBufferOffsetAlignment);
    N_BUDGET_COUNTER_INCR(N_CONSTANT_MEMORY, alignedSize);

    // Allocate the memory range
    int ret = Threading::Interlocked::Add(&sub.endAddress, alignedSize);

    // If we have to wrap around, or we are fingering on the range of the next frame submission buffer...
    if (ret + alignedSize >= state.globalConstantBufferMaxValue)
    {
        n_error("Over allocation of constant memory! Memory will be overwritten!\n");

        // Return dummy value
        return ret;
    }
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::BufferId
GetConstantBuffer(IndexT i)
{
    return state.globalConstantBuffer[i];
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
FinishFrame(IndexT frameIndex)
{
    if (state.currentFrameIndex != frameIndex)
    {
        _end_counter(state.GraphicsDeviceNumComputes);
        _end_counter(state.GraphicsDeviceNumPrimitives);
        _end_counter(state.GraphicsDeviceNumDrawCalls);
        state.currentFrameIndex = frameIndex;
    }

    // Signal rendering finished semaphore just before submitting graphics queue
    state.queueHandler.AppendPresentSignal(
        ComputeQueueType,
        SemaphoreGetVk(state.renderingFinishedSemaphores[state.currentBufferedFrameIndex])
    );

    state.queueHandler.FlushSubmissions(nullptr);

    if (!state.sparseBufferBinds.IsEmpty() || !state.sparseImageBinds.IsEmpty())
    {
        // Bind sparse buffers at the end of the frame
        for (auto& bind : state.sparseBufferBinds)
        {
            state.queueHandler.AppendSparseBind(CoreGraphics::SparseQueueType, bind.buf, bind.binds);
        }

        // Do the same for images
        for (auto& bind : state.sparseImageBinds)
        {
            state.queueHandler.AppendSparseBind(CoreGraphics::SparseQueueType, bind.img, bind.opaqueBinds, bind.pageBinds);
        }

        // And flush queues
        state.queueHandler.FlushSparseBinds(nullptr);
        state.sparseBufferBinds.Clear();
        state.sparseImageBinds.Clear();
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
WaitForQueue(CoreGraphics::QueueType queue)
{
    state.queueHandler.WaitIdle(queue);
}

//------------------------------------------------------------------------------
/**
*/
void 
WaitAndClearPendingCommands()
{
    state.queueHandler.WaitIdle(GraphicsQueueType);
    state.queueHandler.WaitIdle(ComputeQueueType);
    state.queueHandler.WaitIdle(TransferQueueType);
    state.queueHandler.WaitIdle(SparseQueueType);
    vkDeviceWaitIdle(state.devices[state.currentDevice]);

    for (int buffer = 0; buffer < state.maxNumBufferedFrames; buffer++)
    {
        state.currentBufferedFrameIndex = buffer;
        Vulkan::ClearPending();

        // Reset queries
        for (IndexT i = 0; i < CoreGraphics::QueryType::NumQueryTypes; i++)
        {
            state.queries[buffer].queryFreeCount[i] = 0;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DelayedDeleteBuffer(const BufferId id)
{
    Threading::CriticalScope scope(&delayedDeleteSection);
    VkBuffer buf = BufferGetVk(id);
    VkDevice dev = BufferGetVkDevice(id);
    n_assert(dev != VK_NULL_HANDLE);
    n_assert(buf != VK_NULL_HANDLE);
    state.pendingDeletes[state.currentBufferedFrameIndex].buffers.Append(Util::MakeTuple(dev, buf));
}

//------------------------------------------------------------------------------
/**
*/
void
DelayedDeleteTexture(const TextureId id)
{
    Threading::CriticalScope scope(&delayedDeleteSection);
    VkImage img = TextureGetVkImage(id);
    VkImageView view = TextureGetVkImageView(id);
    VkDevice dev = TextureGetVkDevice(id);

    state.pendingDeletes[state.currentBufferedFrameIndex].textures.Append(Util::MakeTuple(dev, view, img));
}

//------------------------------------------------------------------------------
/**
*/
void
DelayedDeleteTextureView(const TextureViewId id)
{
    Threading::CriticalScope scope(&delayedDeleteSection);
    VkImageView view = TextureViewGetVk(id);
    VkDevice dev = TextureViewGetVkDevice(id);
    state.pendingDeletes[state.currentBufferedFrameIndex].textureViews.Append(Util::MakeTuple(dev, view));
}

//------------------------------------------------------------------------------
/**
*/
void
DelayedDeleteCommandBuffer(const CoreGraphics::CmdBufferId id)
{
    Threading::CriticalScope scope(&delayedDeleteSection);
    VkDevice dev = CmdBufferGetVkDevice(id);
    VkCommandPool pool = CmdBufferGetVkPool(id);
    VkCommandBuffer buf = CmdBufferGetVk(id);
#if NEBULA_GRAPHICS_DEBUG
    Util::Array<NvidiaAftermathCheckpoint> checkpoints = CmdBufferMoveVkNvCheckpoints(id);
#endif
    state.pendingDeletes[state.currentBufferedFrameIndex].commandBuffers.Append(Util::MakeTuple(dev, pool, buf, std::move(checkpoints)));
}

//------------------------------------------------------------------------------
/**
*/
void
DelayedFreeMemory(const CoreGraphics::Alloc alloc)
{
    Threading::CriticalScope scope(&delayedDeleteSection);
    state.pendingDeletes[state.currentBufferedFrameIndex].allocs.Append(alloc);
}

//------------------------------------------------------------------------------
/**
*/
void
DelayedDeleteDescriptorSet(const ResourceTableId id)
{
    Threading::CriticalScope scope(&delayedDeleteSection);
    VkDescriptorSet set = ResourceTableGetVkDescriptorSet(id);
    ResourceTableLayoutId layout = ResourceTableGetLayout(id);
    IndexT index = ResourceTableGetVkPoolIndex(id);
    VkDescriptorPool pool = ResourceTableLayoutGetVkDescriptorPool(layout, index);
    VkDevice dev = ResourceTableGetVkDevice(id);
    uint* counter = ResourceTableLayoutGetFreeItemsCounter(layout, index);
    state.pendingDeletes[state.currentBufferedFrameIndex].resourceTables.Append(Util::MakeTuple(dev, pool, set, counter));
}

//------------------------------------------------------------------------------
/**
*/
void
DelayedDeletePass(const CoreGraphics::PassId id)
{
    Threading::CriticalScope scope(&delayedDeleteSection);
    VkDevice dev = CoreGraphics::PassGetVkDevice(id);
    VkRenderPass pass = CoreGraphics::PassGetVkRenderPass(id);
    VkFramebuffer fbo = CoreGraphics::PassGetVkFramebuffer(id);
    state.pendingDeletes[state.currentBufferedFrameIndex].passes.Append(Util::MakeTuple(dev, fbo, pass));
}

//------------------------------------------------------------------------------
/**
*/
void
DelayedDeleteBlas(const CoreGraphics::BlasId id)
{
    Threading::CriticalScope scope(&delayedDeleteSection);
    VkDevice dev = CoreGraphics::BlasGetVkDevice(id);
    VkAccelerationStructureKHR as = BlasGetVk(id);
    state.pendingDeletes[state.currentBufferedFrameIndex].ases.Append(Util::MakeTuple(dev, as));
}

//------------------------------------------------------------------------------
/**
*/
void
DelayedDeleteTlas(const CoreGraphics::TlasId id)
{
    Threading::CriticalScope scope(&delayedDeleteSection);
    VkDevice dev = CoreGraphics::TlasGetVkDevice(id);
    VkAccelerationStructureKHR as = TlasGetVk(id);
    state.pendingDeletes[state.currentBufferedFrameIndex].ases.Append(Util::MakeTuple(dev, as));
}

//------------------------------------------------------------------------------
/**
*/
uint
AllocateQueries(const CoreGraphics::QueryType type, uint numQueries)
{
    uint ret = Threading::Interlocked::Add(&state.queries[state.currentBufferedFrameIndex].queryFreeCount[type], numQueries);
    if (ret + numQueries > state.queries[state.currentBufferedFrameIndex].queryMaxCount[type])
    {
        n_error("Over allocation of queries");

        return 0;
    }
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FinishQueries(const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueryType type, IndexT* starts, SizeT* counts, SizeT numCopies)
{
    VkCommandBuffer vkCmd = CmdBufferGetVk(cmdBuf);
    CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::HostRead, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::BarrierDomain::Global);
    for (IndexT i = 0; i < numCopies; i++)
    {
        vkCmdCopyQueryPoolResults(vkCmd, state.queries[state.currentBufferedFrameIndex].queryPools[type], starts[i], counts[i], BufferGetVk(state.queries[state.currentBufferedFrameIndex].queryBuffer[type]), starts[i] * sizeof(uint64_t), sizeof(uint64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

        // Finally reset the query pool
        vkCmdResetQueryPool(vkCmd, state.queries[state.currentBufferedFrameIndex].queryPools[type], starts[i], counts[i]);
    }
    CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::PipelineStage::HostRead, CoreGraphics::BarrierDomain::Global);
}

//------------------------------------------------------------------------------
/**
*/
IndexT
GetQueueIndex(const QueueType queue)
{
    return state.queueFamilyMap[queue];
}

//------------------------------------------------------------------------------
/**
*/
const Util::Set<uint32_t>&
GetQueueIndices()
{
    return state.usedQueueFamilies;
}

Threading::CriticalSection vertexAllocationMutex;

//------------------------------------------------------------------------------
/**
*/
const VertexAlloc
AllocateVertices(const SizeT numVertices, const SizeT vertexSize)
{
    Threading::CriticalScope scope(&vertexAllocationMutex);
    const uint size = numVertices * vertexSize;
    Memory::RangeAllocation alloc = state.vertexAllocator.Alloc(size);
    n_assert(alloc.offset != alloc.OOM);
    N_BUDGET_COUNTER_INCR(N_VERTEX_MEMORY, size);
    return VertexAlloc{ .size = size, .offset = alloc.offset, .node = alloc.node };
}

//------------------------------------------------------------------------------
/**
*/
const VertexAlloc
AllocateVertices(const SizeT bytes)
{
    Threading::CriticalScope scope(&vertexAllocationMutex);
    Memory::RangeAllocation alloc = state.vertexAllocator.Alloc(bytes);
    n_assert(alloc.offset != alloc.OOM);
    N_BUDGET_COUNTER_INCR(N_VERTEX_MEMORY, bytes);
    return VertexAlloc{ .size = (uint)bytes, .offset = alloc.offset, .node = alloc.node };
}

//------------------------------------------------------------------------------
/**
*/
void
DeallocateVertices(const VertexAlloc& alloc)
{
    Threading::CriticalScope scope(&vertexAllocationMutex);
    state.vertexAllocator.Dealloc(Memory::RangeAllocation{.offset = (uint)alloc.offset, .size = 0, .node = alloc.node});
    N_BUDGET_COUNTER_DECR(N_VERTEX_MEMORY, alloc.size);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::BufferId
GetVertexBuffer()
{
    return state.vertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const VertexAlloc
AllocateIndices(const SizeT numIndices, const IndexType::Code indexType)
{
    Threading::CriticalScope scope(&vertexAllocationMutex);
    uint indexSize = IndexType::SizeOf(indexType);
    uint size = numIndices * indexSize;
    Memory::RangeAllocation alloc = state.indexAllocator.Alloc(size, indexSize);
    n_assert(alloc.offset != alloc.OOM);
    N_BUDGET_COUNTER_INCR(N_INDEX_MEMORY, numIndices * IndexType::SizeOf(indexType));
    return VertexAlloc{ .size = size, .offset = alloc.offset, .node = alloc.node };
}

//------------------------------------------------------------------------------
/**
*/
const VertexAlloc
AllocateIndices(const SizeT bytes)
{
    Threading::CriticalScope scope(&vertexAllocationMutex);
    Memory::RangeAllocation alloc = state.indexAllocator.Alloc(bytes, 4);
    n_assert(alloc.offset != alloc.OOM);
    N_BUDGET_COUNTER_INCR(N_INDEX_MEMORY, bytes);
    return VertexAlloc{ .size = (uint)bytes, .offset = alloc.offset, .node = alloc.node };
}

//------------------------------------------------------------------------------
/**
*/
void
DeallocateIndices(const VertexAlloc& alloc)
{
    Threading::CriticalScope scope(&vertexAllocationMutex);
    state.indexAllocator.Dealloc(Memory::RangeAllocation{.offset = (uint)alloc.offset, .size = 0, .node = alloc.node});
    N_BUDGET_COUNTER_DECR(N_INDEX_MEMORY, alloc.size);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::BufferId
GetIndexBuffer()
{
    return state.indexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
Threading::CriticalSection UploadLock;
Util::Pair<Memory::RangeAllocation, CoreGraphics::BufferId>
AllocateUpload(const SizeT numBytes, const SizeT alignment)
{
    Threading::CriticalScope _0(&UploadLock);

    // Calculate aligned upper bound
    SizeT adjustedAlignment = Math::max(alignment, (SizeT)CoreGraphics::MemoryRangeGranularity);
    const SizeT alignedBytes = numBytes + adjustedAlignment - 1;
    N_BUDGET_COUNTER_INCR(N_UPLOAD_MEMORY, alignedBytes);

    Memory::RangeAllocation alloc = state.uploadAllocator.Alloc(numBytes, adjustedAlignment);
    if (alloc.offset != alloc.OOM)
    {
        return Util::MakePair(alloc, state.uploadBuffer);
    }
    return Util::MakePair(Memory::RangeAllocation(), InvalidBufferId);
}

//------------------------------------------------------------------------------
/**
*/
void
UploadInternal(const CoreGraphics::BufferId buffer, const uint offset, const void* data, SizeT size)
{
    // For upload, we don't make any attempts acquiring the buffer
    CoreGraphics::BufferUpdate(buffer, data, size, offset);
}

//------------------------------------------------------------------------------
/**
*/
void
FreeUploads(const Util::Array<Memory::RangeAllocation>& allocations)
{
    for (const auto& alloc : allocations)
    {
        state.uploadAllocator.Dealloc(alloc);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FlushUploads(const Util::Array<Memory::RangeAllocation>& allocations)
{
    if (!allocations.IsEmpty())
    {
        Util::FixedArray<VkMappedMemoryRange> ranges(allocations.Size());
        for (IndexT i = 0; i < allocations.Size(); i++)
        {
            VkMappedMemoryRange& range = ranges[i];
            range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            range.pNext = nullptr;
            range.offset = allocations[i].offset; //uploadBuffer.interval.start;
            range.size = Math::align(allocations[i].size, CoreGraphics::MemoryRangeGranularity);// (DeviceSize)size;
            range.memory = BufferGetVkMemory(state.uploadBuffer);
        }

        if (ranges.Size() > 0)
        {
            VkResult res = vkFlushMappedMemoryRanges(state.devices[state.currentDevice], ranges.Size(), ranges.Begin());
            n_assert(res == VK_SUCCESS);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
EnqueueUploadsFlushAndFree(const Util::Array<Memory::RangeAllocation>& allocations)
{
    state.uploadAllocationsToFlush.AppendArray(allocations);
    state.uploadAllocationsToFree[state.currentBufferedFrameIndex].AppendArray(allocations);
}

//------------------------------------------------------------------------------
/**
*/
void
ParseMarkersAndTime(CoreGraphics::FrameProfilingMarker& marker, uint64* data, const uint64& offset)
{
    const SizeT timestampPeriod = state.deviceProps[state.currentDevice].properties.limits.timestampPeriod;
    uint64 begin = data[marker.gpuBegin];
    uint64 end = data[marker.gpuEnd];
    marker.start = (begin - offset) * timestampPeriod;
    marker.duration = (end - begin) * timestampPeriod;

    for (IndexT i = 0; i < marker.children.Size(); i++)
    {
        ParseMarkersAndTime(marker.children[i], data, offset);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
NewFrame()
{
    // We need to lock here so that we don't accidentally insert a delete resource inbetween updating
    // the current buffer index and deleting pending resources
    Threading::CriticalScope transferScope(&transferLock);
    Threading::CriticalScope setupScope(&setupLock);
    Threading::CriticalScope deleteScope(&delayedDeleteSection);

    // Progress to next frame and wait for that buffer
    state.currentBufferedFrameIndex = (state.currentBufferedFrameIndex + 1) % state.maxNumBufferedFrames;

    N_MARKER_BEGIN(WaitForBuffer, Wait);

    auto& waitEventsThisFrame = state.waitEvents[state.currentBufferedFrameIndex];
    for (const auto& waitEvent : waitEventsThisFrame)
    {
        state.queueHandler.Wait(waitEvent.queue, waitEvent.timelineIndex);
    }
    waitEventsThisFrame.Clear();

    // Go through the query timestamp results and set the time in the markers
    state.frameProfilingMarkers.Clear();
    for (IndexT queue = 0; queue < QueueType::NumQueueTypes; queue++)
    {
        if (!state.pendingMarkers[queue][state.currentBufferedFrameIndex].markers.IsEmpty())
        {
            CoreGraphics::BufferId buf = state.queries[state.currentBufferedFrameIndex].queryBuffer[CoreGraphics::QueryType::TimestampsQueryType];
            CoreGraphics::BufferInvalidate(buf);
            uint64* data = (uint64*)CoreGraphics::BufferMap(buf);

            const auto& markersPerBuffer = state.pendingMarkers[queue][state.currentBufferedFrameIndex].markers;
            uint64 offset = UINT64_MAX;

            offset = Math::min(offset, data[state.pendingMarkers[queue][state.currentBufferedFrameIndex].baseOffset[0]]);

            for (IndexT i = 0; i < markersPerBuffer.Size(); i++)
            {
                const auto& markers = markersPerBuffer[i];
                for (auto& marker : markers)
                {
                    ParseMarkersAndTime(marker, data, offset);

                    // Add to combined markers to graphics device for this frame
                    state.frameProfilingMarkers.Append(marker);
                }
            }
            CoreGraphics::BufferUnmap(buf);
        }
    }

    // Cleanup resources
    Vulkan::ClearPending();

    // Reset queries
    for (IndexT i = 0; i < CoreGraphics::QueryType::NumQueryTypes; i++)
    {
        state.queries[state.currentBufferedFrameIndex].queryFreeCount[i] = 0;
    }

    for (const auto& alloc : state.uploadAllocationsToFree[state.currentBufferedFrameIndex])
    {
        state.uploadAllocator.Dealloc(alloc);
    }
    state.uploadAllocationsToFree[state.currentBufferedFrameIndex].Clear();

    // update constant buffer offsets
    Vulkan::GraphicsDeviceState::ConstantsRingBuffer& nextCboRing = state.constantBufferRings[state.currentBufferedFrameIndex];
    nextCboRing.endAddress = 0;
    nextCboRing.gfx.flushedStart = nextCboRing.cmp.flushedStart = nextCboRing.endAddress;

    N_BUDGET_COUNTER_RESET(N_CONSTANT_MEMORY);
    N_BUDGET_COUNTER_RESET(N_UPLOAD_MEMORY);

    N_MARKER_END();
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
ObjectSetName(const CoreGraphics::TextureViewId id, const char* name)
{
    VkDebugUtilsObjectNameInfoEXT info =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_OBJECT_TYPE_IMAGE_VIEW,
        (uint64_t)Vulkan::TextureViewGetVk(id),
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
ObjectSetName(const CoreGraphics::CmdBufferId id, const char* name)
{
    VkDebugUtilsObjectNameInfoEXT info =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_OBJECT_TYPE_COMMAND_BUFFER,
        (uint64_t)Vulkan::CmdBufferGetVk(id),
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
ObjectSetName(const VkRenderPass id, const char* name)
{
    VkDebugUtilsObjectNameInfoEXT info =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_OBJECT_TYPE_RENDER_PASS,
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
        (uint64_t)SemaphoreGetVk(id.id),
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
    VkQueue vkqueue = state.queueHandler.GetQueue(queue);
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
    VkQueue vkqueue = state.queueHandler.GetQueue(queue);
    VkQueueEndLabel(vkqueue);
}

//------------------------------------------------------------------------------
/**
*/
void 
QueueInsertMarker(const CoreGraphics::QueueType queue, const Math::vec4& color, const char* name)
{
    VkQueue vkqueue = state.queueHandler.GetQueue(queue);
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
#endif

//------------------------------------------------------------------------------
/**
*/
SubmissionWaitEvent::SubmissionWaitEvent()
    : timelineIndex(UINT64_MAX)
{
}

//------------------------------------------------------------------------------
/**
*/
const bool
SubmissionWaitEvent::operator==(const std::nullptr_t) const
{
    return this->timelineIndex == UINT64_MAX;
}

//------------------------------------------------------------------------------
/**
*/
const bool
SubmissionWaitEvent::operator!=(const std::nullptr_t) const
{
    return this->timelineIndex != UINT64_MAX;
}

//------------------------------------------------------------------------------
/**
*/
void
SubmissionWaitEvent::operator=(const std::nullptr_t)
{
    this->timelineIndex = UINT64_MAX;
}

} // namespace CoreGraphics
