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
};

struct VkPipelineBundle
{
    VkGraphicsPipelineCreateInfo pipelineInfo;
    VkPipelineColorBlendStateCreateInfo blendInfo;
    VkPipelineMultisampleStateCreateInfo multisampleInfo;
    CoreGraphics::InputAssemblyKey inputAssembly;
    VkPipelineLayout computeLayout;
    VkPipelineLayout graphicsLayout;
    CoreGraphics::PassId pass;
    CoreGraphics::ShaderProgramId program;
};

struct QueryBundle
{
    IndexT offset[CoreGraphics::QueryType::NumQueryTypes];
    uint queryCount[CoreGraphics::QueryType::NumQueryTypes];
    bool enabled[CoreGraphics::QueryType::NumQueryTypes];
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
    VkDevice
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
> VkCommandBufferAllocator;

} // Vulkan
