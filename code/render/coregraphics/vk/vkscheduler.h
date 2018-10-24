#pragma once
//------------------------------------------------------------------------------
/**
	Implements a scheduler interface which serves to perform Vulkan operations which might be requested
	anywhere in the code to be executed when actually relevant.

	Calls to the scheduler will be executed dependent on the CommandPass, which denotes when the delayed
	command is supposed to be run. This is to make sure commands come in order on the command queue.

	For immediate calls to Vulkan, look at VkUtilities
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "vkdeferredcommand.h"
#include "core/singleton.h"
#include "util/dictionary.h"

namespace Vulkan
{
class VkScheduler
{
	__DeclareSingleton(VkScheduler);
public:

	enum CommandPass
	{
		OnBeginFrame,
		OnEndFrame,
		OnBeginPass,
		OnNextSubpass,
		OnEndPass,
		OnMainTransferSubmitted,
		OnMainDrawSubmitted,
		OnMainComputeSubmitted,
		OnBeginTransferThread,
		OnBeginDrawThread,
		OnBeginComputeThread,
		OnTransferThreadsSubmitted,
		OnDrawThreadsSubmitted,
		OnComputeThreadsSubmitted,

		OnHandleTransferFences,
		OnHandleDrawFences,
		OnHandleComputeFences,
		OnHandleSparseFences,
		OnHandleFences,

		OnBindGraphicsPipeline,
		OnBindComputePipeline,

		NumCommandPasses
	};

	/// constructor
	VkScheduler();
	/// destructor
	virtual ~VkScheduler();

	/// push image layout change
	void PushImageLayoutTransition(CoreGraphicsQueueType queue, CoreGraphics::BarrierStage left, CoreGraphics::BarrierStage right, VkImageMemoryBarrier barrier);
	/// push transition image ownership transition
	void PushImageOwnershipChange(CoreGraphicsQueueType queue, CoreGraphics::BarrierStage left, CoreGraphics::BarrierStage right, VkImageMemoryBarrier barrier);
	/// push image color clear
	void PushImageColorClear(const VkImage& image, const CoreGraphicsQueueType queue, VkImageLayout layout, VkClearColorValue clearValue, VkImageSubresourceRange subres);
	/// push image depth stencil clear
	void PushImageDepthStencilClear(const VkImage& image, const CoreGraphicsQueueType queue, VkImageLayout layout, VkClearDepthStencilValue clearValue, VkImageSubresourceRange subres);
	/// setup staging image update for later execution
	void PushImageUpdate(const VkImage& img, const VkImageCreateInfo& info, uint32_t mip, uint32_t face, VkDeviceSize size, uint32_t* data);

	/// execute stacked commands
	void ExecuteCommandPass(const CommandPass& pass);
	/// set device to be used by this scheduler
	void SetDevice(const VkDevice dev);

	/// begin new cycle for all queues
	void Begin();
	/// end cycle for transfers
	void EndTransfers();
	/// end cycle for draws
	void EndDraws();
	/// end cycle for computes
	void EndComputes();

	/// discard scheduler
	void Discard();

private:

	/// push command to sheduler
	void PushCommand(const VkDeferredCommand& cmd, const CommandPass& pass);

	friend class VkUtilities;

	friend void	BindDescriptorsGraphics(const VkDescriptorSet* descriptors, uint32_t baseSet, uint32_t setCount, const uint32_t* offsets, uint32_t offsetCount, bool shared);
	friend void	BindDescriptorsCompute(const VkDescriptorSet* descriptors, uint32_t baseSet, uint32_t setCount, const uint32_t* offsets, uint32_t offsetCount);
	friend void	EndDrawThreads();
	friend void	EndDrawSubpass();

	bool putTransferFenceThisFrame;
	bool putSparseFenceThisFrame;
	bool putComputeFenceThisFrame;
	bool putDrawFenceThisFrame;
	VkDevice dev;
	Util::FixedArray<Util::Array<VkDeferredCommand>> commands;
	Util::Dictionary<VkFence, Util::Array<VkDeferredCommand>> transferFenceCommands;
	Util::Dictionary<VkFence, Util::Array<VkDeferredCommand>> drawFenceCommands;
	Util::Dictionary<VkFence, Util::Array<VkDeferredCommand>> computeFenceCommands;
	Util::Dictionary<VkFence, Util::Array<VkDeferredCommand>> sparseFenceCommands;
};


//------------------------------------------------------------------------------
/**
*/
inline void
VkScheduler::SetDevice(const VkDevice dev)
{
	this->dev = dev;
}

} // namespace Vulkan