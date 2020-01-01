#pragma once
//------------------------------------------------------------------------------
/**
	The Vulkan implementation of the graphics device.

	All functions in the Vulkan namespace are internal helper functions specifically for Vulkan,
	the other functions implement the abstraction layer.

	(C) 2018-2020 Individual contributors, see AUTHORS file
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
/// get pipeline cache
VkPipelineCache GetPipelineCache();
/// get memory properties
VkPhysicalDeviceMemoryProperties GetMemoryProperties();
/// get main command buffer
VkCommandBuffer GetMainBuffer(const CoreGraphics::QueueType queue);
/// get final graphics semaphore
VkSemaphore GetPresentSemaphore();
/// get final rendering semaphore
VkSemaphore GetRenderingSemaphore();
/// set to wait for present semaphore this submission
void WaitForPresent(VkSemaphore sem);

/// get queue families
const Util::Set<uint32_t>& GetQueueFamilies();
/// get specific queue family
const uint32_t GetQueueFamily(const CoreGraphics::QueueType type);
/// get queue from index and family
const VkQueue GetQueue(const CoreGraphics::QueueType type, const IndexT index);
/// get currently active queue of type
const VkQueue GetCurrentQueue(const CoreGraphics::QueueType type);

/// insert barrier not created from a barrier object
void InsertBarrier(
	VkPipelineStageFlags srcFlags,
	VkPipelineStageFlags dstFlags,
	VkDependencyFlags dep,
	uint32_t numMemoryBarriers,
	VkMemoryBarrier* memoryBarriers,
	uint32_t numBufferBarriers,
	VkBufferMemoryBarrier* bufferBarriers,
	uint32_t numImageBarriers,
	VkImageMemoryBarrier* imageBarriers,
	const CoreGraphics::QueueType queue);

/// do actual copy (see coregraphics namespace for helper functions)
void Copy(const VkImage from, Math::rectangle<SizeT> fromRegion, const VkImage to, Math::rectangle<SizeT> toRegion);
/// perform actual blit (see coregraphics namespace for helper functions)
void Blit(const VkImage from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const VkImage to, Math::rectangle<SizeT> toRegion, IndexT toMip);

/// update descriptors
void BindDescriptorsGraphics(const VkDescriptorSet* descriptors, uint32_t baseSet, uint32_t setCount, const uint32_t* offsets, uint32_t offsetCount, bool shared = false);
/// update descriptors
void BindDescriptorsCompute(const VkDescriptorSet* descriptors, uint32_t baseSet, uint32_t setCount, const uint32_t* offsets, uint32_t offsetCount, const CoreGraphics::QueueType queue);
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
void BindComputePipeline(const VkPipeline& pipeline, const VkPipelineLayout& layout, const CoreGraphics::QueueType queue);
/// bind no pipeline (effectively making all descriptor binds happen on both graphics and compute)
void UnbindPipeline();
/// set array of viewports directly
void SetVkViewports(VkViewport* viewports, SizeT num);
/// set array of scissors directly
void SetVkScissorRects(VkRect2D* scissors, SizeT num);

/// start up new draw thread
void BeginDrawThread();
/// finish current draw threads
void EndDrawThreads();
/// add command to thread
void PushToThread(const VkCommandBufferThread::Command& cmd, const IndexT& index, bool allowStaging = true);
/// flush remaining staging thread commands
void FlushToThread(const IndexT& index);

/// begin command buffer marker (directly on vkcommandbuffer)
void CommandBufferBeginMarker(VkCommandBuffer buf, const Math::float4& color, const char* name);
/// end command buffer marker (directly on vkcommandbuffer)
void CommandBufferEndMarker(VkCommandBuffer buf);

} // namespace Vulkan
