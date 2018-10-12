#pragma once
//------------------------------------------------------------------------------
/**
	Implements a deferred delegate, which is used to perform an action whenever a fence object is done.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/fixedarray.h"
#include "coregraphics/config.h"
#include "coregraphics/barrier.h"
#include <vulkan/vulkan.h>
namespace Vulkan
{
struct VkDeferredCommand
{
	/// constructor
	VkDeferredCommand();
	/// destructor
	~VkDeferredCommand();

	enum DelegateType
	{
		FreeCmdBuffers,
		FreeMemory,
		FreeBuffer,
		FreeImage,
		__RunAfterFence,		// don't use this flag, but all delegates prior to this flag requires a frame to be complete before it can occur

		BindDescriptorSets,		// this can actually be done outside of a frame (views, custom code, etc)

		UpdateBuffer,
		UpdateImage,

		DestroyPipeline,

		ClearColorImage,
		ClearDepthStencilImage,
		ImageOwnershipChange,
		ImageLayoutTransition
	};

	struct Delegate
	{
		DelegateType type;
		CoreGraphicsQueueType queue;
		VkFence fence;
		union
		{
			struct // FreeCmdBuffers
			{
				VkCommandBuffer buffers[64];
				VkCommandPool pool;
				uint32_t numBuffers;
			} cmdbufferfree;

			struct // FreeMemory
			{
				void* data;
			} memory;

			struct // FreeBuffer
			{
				VkBuffer buf;
				VkDeviceMemory mem;
			} buffer;

			struct // FreeImage
			{
				VkImage img;
				VkDeviceMemory mem;
			} image;

			struct // UpdateBuffer
			{
				VkBuffer buf;
				VkDeviceSize offset;
				VkDeviceSize size;
				uint32_t* data;
			} bufferUpd;

			struct // UpdateImage
			{
				VkImage img;
				VkImageCreateInfo info;
				uint32_t mip;
				uint32_t face;
				VkDeviceSize size;
				uint32_t* data;
			} imageUpd;

			struct // DestroyPipeline
			{
				VkPipeline pipeline;
			} pipelineDestroy;

			struct // ImageLayoutTransition
			{
				VkPipelineStageFlags left;
				VkPipelineStageFlags right;
				VkImageMemoryBarrier barrier;
			} imgBarrier;

			struct // ClearColorImage
			{
				VkImage img;
				VkImageLayout layout;
				VkClearColorValue clearValue;
				VkImageSubresourceRange region;
			} imgColorClear;

			struct // ClearDepthStencilImage
			{
				VkImage img;
				VkImageLayout layout;
				VkClearDepthStencilValue clearValue;
				VkImageSubresourceRange region;
			} imgDepthStencilClear;

			struct // ImageOwnershipChange
			{
				VkPipelineStageFlags left;
				VkPipelineStageFlags right;
				VkImageMemoryBarrier barrier;
			} imgOwnerChange;

			struct // BindDescriptorSets
			{
				VkPipelineBindPoint type;
				VkPipelineLayout layout;
				uint32_t baseSet;
				uint32_t numSets;
				const VkDescriptorSet* sets;
				uint32_t numOffsets;
				const uint32_t* offsets;
			} descSetBind;
		};
	} del;

	VkDevice dev;

	/// run delegate action
	void RunDelegate();
};


} // namespace Vulkan