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
/// get final rendering semaphore
VkSemaphore GetRenderingSemaphore();
/// get the present fence
VkFence GetPresentFence();

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

/// update descriptors
void BindDescriptorsGraphics(const VkDescriptorSet* descriptors, uint32_t baseSet, uint32_t setCount, const uint32_t* offsets, uint32_t offsetCount, bool shared = false);
/// update descriptors
void BindDescriptorsCompute(const VkDescriptorSet* descriptors, uint32_t baseSet, uint32_t setCount, const uint32_t* offsets, uint32_t offsetCount, const CoreGraphics::QueueType queue);
/// update push ranges
void UpdatePushRanges(const VkShaderStageFlags& stages, const VkPipelineLayout& layout, uint32_t offset, uint32_t size, const byte* data);

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

/// begin command buffer marker (directly on vkcommandbuffer)
void CommandBufferBeginMarker(VkCommandBuffer buf, const Math::vec4& color, const char* name);
/// end command buffer marker (directly on vkcommandbuffer)
void CommandBufferEndMarker(VkCommandBuffer buf);

/// add buffer for delayed delete
void DelayedDeleteBuffer(const VkBuffer buf);
/// add image for delayed delete
void DelayedDeleteImage(const VkImage img);
/// add image view for delayed delete
void DelayedDeleteImageView(const VkImageView view);
/// add memory for delayed delete
void DelayedDeleteMemory(const VkDeviceMemory mem);

/// handle queries at beginning of frame
void _ProcessQueriesBeginFrame();
/// handle queries on ending the frame
void _ProcessQueriesEndFrame();

#if NEBULA_ENABLE_PROFILING
/// insert timestamp, returns handle to timestamp, which can be retreived on the next N'th frame where N is the number of buffered frames
void __Timestamp(CoreGraphics::CommandBufferId buf, CoreGraphics::QueueType queue, const CoreGraphics::BarrierStage stage, const char* name);
#endif

} // namespace Vulkan

namespace CoreGraphics
{

} // namespace CoreGraphics
