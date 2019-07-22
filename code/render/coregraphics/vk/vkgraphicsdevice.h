#pragma once
//------------------------------------------------------------------------------
/**
	The Vulkan implementation of the graphics device.

	All functions in the Vulkan namespace are internal helper functions specifically for Vulkan,
	the other functions implement the abstraction layer.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/graphicsdevice.h"
#include "util/set.h"
#include "vksubcontexthandler.h"
#include "vkcommandbufferthread.h"

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
/// get the currently free pool
VkDescriptorPool GetCurrentDescriptorPool();
/// get pipeline cache
VkPipelineCache GetPipelineCache();
/// get memory properties
VkPhysicalDeviceMemoryProperties GetMemoryProperties();
/// get main command buffer
VkCommandBuffer GetMainBuffer(const CoreGraphicsQueueType queue);
/// get final graphics semaphore
VkSemaphore GetPresentSemaphore();
/// get final rendering semaphore
VkSemaphore GetRenderingSemaphore();
/// set to wait for present semaphore this submission
void WaitForPresent(VkSemaphore sem);

/// allocate range of graphics memory and set data, return offset
uint SetGraphicsConstants(CoreGraphicsGlobalConstantBufferType type, void* data, SizeT size);
/// allocate range of compute memory and set data, return offset
uint SetComputeConstants(CoreGraphicsGlobalConstantBufferType type, void* data, SizeT size);

/// get queue families
const Util::Set<uint32_t>& GetQueueFamilies();
/// get specific queue family
const uint32_t GetQueueFamily(const CoreGraphicsQueueType type);
/// get queue from index and family
const VkQueue GetQueue(const CoreGraphicsQueueType type, const IndexT index);
/// get currently active queue of type
const VkQueue GetCurrentQueue(const CoreGraphicsQueueType type);

/// do actual copy (see coregraphics namespace for helper functions)
void Copy(const VkImage from, Math::rectangle<SizeT> fromRegion, const VkImage to, Math::rectangle<SizeT> toRegion);
/// perform actual blit (see coregraphics namespace for helper functions)
void Blit(const VkImage from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const VkImage to, Math::rectangle<SizeT> toRegion, IndexT toMip);

/// update descriptors
void BindDescriptorsGraphics(const VkDescriptorSet* descriptors, uint32_t baseSet, uint32_t setCount, const uint32_t* offsets, uint32_t offsetCount, bool shared = false);
/// update descriptors
void BindDescriptorsCompute(const VkDescriptorSet* descriptors, uint32_t baseSet, uint32_t setCount, const uint32_t* offsets, uint32_t offsetCount);
/// update push ranges
void UpdatePushRanges(const VkShaderStageFlags& stages, const VkPipelineLayout& layout, uint32_t offset, uint32_t size, void* data);

/// sets the current shader pipeline information
void BindGraphicsPipelineInfo(const VkGraphicsPipelineCreateInfo& shader, const uint32_t programId);
/// sets the current vertex layout information
void SetVertexLayoutPipelineInfo(VkPipelineVertexInputStateCreateInfo* vertexLayout);
/// sets the current framebuffer layout information
void SetFramebufferLayoutInfo(const VkGraphicsPipelineCreateInfo& framebufferLayout);
/// sets the current primitive layout information
void SetInputLayoutInfo(VkPipelineInputAssemblyStateCreateInfo* inputLayout);
/// create a new pipeline (or fetch from cache) and bind to command queue
void CreateAndBindGraphicsPipeline();
/// bind compute pipeline
void BindComputePipeline(const VkPipeline& pipeline, const VkPipelineLayout& layout);
/// bind no pipeline (effectively making all descriptor binds happen on both graphics and compute)
void UnbindPipeline();
/// set array of viewports directly
void SetViewports(VkViewport* viewports, SizeT num);
/// set array of scissors directly
void SetScissorRects(VkRect2D* scissors, SizeT num);

/// helper function to submit a command buffer
void SubmitToQueue(VkQueue queue, VkPipelineStageFlags flags, uint32_t numBuffers, VkCommandBuffer* buffers);
/// helper function to submit a fence
void SubmitToQueue(VkQueue queue, VkFence fence);
/// wait for queue to finish execution using fence, also resets fence
void WaitForFences(VkFence* fences, uint32_t numFences, bool waitForAll);

/// start up new draw thread
void BeginDrawThread();
/// finish current draw threads
void EndDrawThreads();
/// add command to thread
void PushToThread(const VkCommandBufferThread::Command& cmd, const IndexT& index, bool allowStaging = true);
/// flush remaining staging thread commands
void FlushToThread(const IndexT& index);

/// binds common descriptors
void BindSharedDescriptorSets();

/// begin command buffer marker (directly on vkcommandbuffer)
void CommandBufferBeginMarker(VkCommandBuffer buf, const Math::float4& color, const char* name);
/// end command buffer marker (directly on vkcommandbuffer)
void CommandBufferEndMarker(VkCommandBuffer buf);

} // namespace Vulkan
