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
#include "core/refcounted.h"
#include "core/singleton.h"

namespace Vulkan
{
class VkScheduler : public Core::RefCounted
{
	__DeclareClass(VkScheduler);
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
	void PushImageLayoutTransition(VkDeferredCommand::CommandQueueType queue, VkImageMemoryBarrier barrier);
	/// push transition image ownership transition
	void PushImageOwnershipChange(VkDeferredCommand::CommandQueueType queue, VkImageMemoryBarrier barrier);
	/// push image color clear
	void PushImageColorClear(const VkImage& image, const VkDeferredCommand::CommandQueueType& queue, VkImageLayout layout, VkClearColorValue clearValue, VkImageSubresourceRange subres);
	/// push image depth stencil clear
	void PushImageDepthStencilClear(const VkImage& image, const VkDeferredCommand::CommandQueueType& queue, VkImageLayout layout, VkClearDepthStencilValue clearValue, VkImageSubresourceRange subres);
	/// setup staging image update for later execution
	void PushImageUpdate(const VkImage& img, const VkImageCreateInfo& info, uint32_t mip, uint32_t face, VkDeviceSize size, uint32_t* data);

	/// execute stacked commands
	void ExecuteCommandPass(const CommandPass& pass);

	/// begin new cycle for all queues
	void Begin();
	/// end cycle for transfers
	void EndTransfers();
	/// end cycle for draws
	void EndDraws();
	/// end cycle for computes
	void EndComputes();

private:

	/// push command to sheduler
	void PushCommand(const VkDeferredCommand& cmd, const CommandPass& pass);


	friend class VkRenderDevice;
	friend class VkUtilities;

	bool putTransferFenceThisFrame;
	bool putDrawFenceThisFrame;
	bool putComputeFenceThisFrame;
	Util::FixedArray<Util::Array<VkDeferredCommand>> commands;
	Util::Dictionary<VkFence, Util::Array<VkDeferredCommand>> transferFenceCommands;
	Util::Dictionary<VkFence, Util::Array<VkDeferredCommand>> drawFenceCommands;
	Util::Dictionary<VkFence, Util::Array<VkDeferredCommand>> computeFenceCommands;
};
} // namespace Vulkan