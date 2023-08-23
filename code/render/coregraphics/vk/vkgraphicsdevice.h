#pragma once
//------------------------------------------------------------------------------
/**
    The Vulkan implementation of the graphics device.

    All functions in the Vulkan namespace are internal helper functions specifically for Vulkan,
    the other functions implement the abstraction layer.

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/graphicsdevice.h"
#include "vkbarrier.h"
#include "vksubcontexthandler.h"
#include "vkcommandbufferthread.h"
#include "vkmemory.h"

namespace Vulkan
{


/// setup graphics adapter
void SetupAdapter();

/// get vk instance
VkInstance GetInstance();
/// get the currently activated device
VkDevice GetCurrentDevice();
/// get the currently activated physical device
VkPhysicalDevice GetCurrentPhysicalDevice();
/// get the current device properties
VkPhysicalDeviceProperties GetCurrentProperties();
/// get the current device features
VkPhysicalDeviceFeatures GetCurrentFeatures();
/// get pipeline cache
VkPipelineCache GetPipelineCache();
/// get memory properties
VkPhysicalDeviceMemoryProperties GetMemoryProperties();
/// get main command buffer
VkCommandBuffer GetMainBuffer(const CoreGraphics::QueueType queue);
/// get final rendering semaphore
VkSemaphore GetRenderingSemaphore();
/// get the present fence
VkFence GetPresentFence();

/// Add VkBuffer for late delete
void DelayedDeleteVkBuffer(const VkDevice dev, const VkBuffer buf);

/// Get query pool
VkQueryPool GetQueryPool(const CoreGraphics::QueryType query);

/// get queue from index and family
const VkQueue GetQueue(const CoreGraphics::QueueType type, const IndexT index);
/// get currently active queue of type
const VkQueue GetCurrentQueue(const CoreGraphics::QueueType type);

/// Generate or return cached VkPipeline
VkPipeline GetOrCreatePipeline(CoreGraphics::PassId pass, uint subpass, CoreGraphics::ShaderProgramId program, CoreGraphics::InputAssemblyKey inputAssembly, const VkGraphicsPipelineCreateInfo& info);

/// perform a set of sparse bind operations
void SparseTextureBind(const VkImage img, const Util::Array<VkSparseMemoryBind>& opaqueBinds, const Util::Array<VkSparseImageMemoryBind>& pageBinds);

/// Clear pending resources
void ClearPending();

} // namespace Vulkan

namespace CoreGraphics
{

struct PipelineId
{
    VkPipeline pipeline;
};

} // namespace CoreGraphics
