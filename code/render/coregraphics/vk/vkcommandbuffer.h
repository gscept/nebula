#pragma once
//------------------------------------------------------------------------------
/**
    Implements a Vulkan command buffer

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "vkloader.h"
#include "coregraphics/commandbuffer.h"
#include "coregraphics/pass.h"
#include "coregraphics/shader.h"


namespace CoreGraphics
{
struct PassId;
struct ShaderProgramId;
};

namespace Vulkan
{

    
#if NEBULA_GRAPHICS_DEBUG
struct NvidiaAftermathCheckpoint
{
    Util::String name;
    NvidiaAftermathCheckpoint* prev;
    bool push : 1;
};
#endif

extern PFN_vkCmdBeginDebugUtilsLabelEXT VkCmdDebugMarkerBegin;
extern PFN_vkCmdEndDebugUtilsLabelEXT VkCmdDebugMarkerEnd;
extern PFN_vkCmdInsertDebugUtilsLabelEXT VkCmdDebugMarkerInsert;

/// get vk command buffer pool
const VkCommandPool CmdBufferPoolGetVk(const CoreGraphics::CmdBufferPoolId id);
/// get vk device that created the pool
const VkDevice CmdBufferPoolGetVkDevice(const CoreGraphics::CmdBufferPoolId id);

enum
{
    CommandBufferPool_VkDevice,
    CommandBufferPool_VkCommandPool,
};
typedef Ids::IdAllocator<VkDevice, VkCommandPool> VkCommandBufferPoolAllocator;

//------------------------------------------------------------------------------
/**
*/
static const uint NumPoolTypes = 4;
struct CommandBufferPools
{
    VkCommandPool pools[CoreGraphics::QueueType::NumQueueTypes][NumPoolTypes];
    uint queueFamilies[CoreGraphics::QueueType::NumQueueTypes];
    VkDevice dev;
};

/// Get vk command buffer
const VkCommandBuffer CmdBufferGetVk(const CoreGraphics::CmdBufferId id);
/// Get vk command buffer pool
const VkCommandPool CmdBufferGetVkPool(const CoreGraphics::CmdBufferId id);
/// Get vk device 
const VkDevice CmdBufferGetVkDevice(const CoreGraphics::CmdBufferId id);
#if NEBULA_GRAPHICS_DEBUG
/// Get nvidia checkpoints
Util::Array<NvidiaAftermathCheckpoint> CmdBufferMoveVkNvCheckpoints(const CoreGraphics::CmdBufferId id);
#endif

enum
{
    CmdBuffer_VkDevice
    , CmdBuffer_VkCommandBuffer
    , CmdBuffer_VkCommandPool
    , CmdBuffer_PipelineBuildBits
    , CmdBuffer_VkPipelineBundle
    , CmdBuffer_PendingViewports
    , CmdBuffer_PendingScissors
    , CmdBuffer_Usage
#if NEBULA_ENABLE_PROFILING
    , CmdBuffer_ProfilingMarkers
    , CmdBuffer_Query
#endif
#if NEBULA_GRAPHICS_DEBUG
    , CmdBuffer_NVCheckpoints
#endif
};

struct VkPipelineBundle
{
    VkGraphicsPipelineCreateInfo pipelineInfo;
    VkPipelineColorBlendStateCreateInfo blendInfo;
    VkPipelineMultisampleStateCreateInfo multisampleInfo;
    CoreGraphics::InputAssemblyKey inputAssembly;
    VkPipelineLayout computeLayout;
    VkPipelineLayout graphicsLayout;
    VkPipelineLayout raytracingLayout;
    CoreGraphics::PassId pass;
    CoreGraphics::ShaderProgramId program;
};

struct QueryBundle
{
    bool enabled[CoreGraphics::QueryType::NumQueryTypes];

    struct QueryChunk
    {
        uint offset;
        uint queryCount;
    };

    Util::Array<QueryChunk> chunks[CoreGraphics::QueryType::NumQueryTypes];

    struct QueryState
    {
        uint currentChunk;
        uint chunkSize;
    };
    QueryState states[CoreGraphics::QueryType::NumQueryTypes];


    QueryChunk& GetChunk(CoreGraphics::QueryType queryType)
    {
        Util::Array<QueryChunk>& chunks = this->chunks[queryType];
        QueryState& state = this->states[queryType];

        auto chunkCreator = [&state, &chunks](CoreGraphics::QueryType type) -> QueryChunk&
        {
            QueryChunk& newChunk = chunks.Emplace();
            newChunk.offset = CoreGraphics::AllocateQueries(type, state.chunkSize);
            newChunk.queryCount = 0;
            return newChunk;
        };

        if (!chunks.IsEmpty())
        {
            QueryChunk& currentChunk = chunks[state.currentChunk];

            // If chunk is full, get new
            if (currentChunk.queryCount == state.chunkSize)
            {
                QueryChunk& newChunk = chunkCreator(queryType);
                state.currentChunk++;
                return newChunk;
            }
            return currentChunk;
        }
        else
        {
            return chunkCreator(queryType);
        }
    };
};

struct ViewportBundle
{
    Util::FixedArray<VkViewport> viewports;
    uint numPending;
};

struct ScissorBundle
{
    Util::FixedArray<VkRect2D> scissors;
    uint numPending;
};

typedef Ids::IdAllocatorSafe<
    0xFFF
    , VkDevice
    , VkCommandBuffer
    , VkCommandPool
    , CoreGraphics::CmdPipelineBuildBits
    , VkPipelineBundle
    , ViewportBundle
    , ScissorBundle
    , CoreGraphics::QueueType
#if NEBULA_ENABLE_PROFILING
    , CoreGraphics::CmdBufferMarkerBundle
    , QueryBundle
#endif
#if NEBULA_GRAPHICS_DEBUG
    , Util::Array<NvidiaAftermathCheckpoint>
#endif
> VkCommandBufferAllocator;

} // Vulkan
