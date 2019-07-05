#pragma once
//------------------------------------------------------------------------------
/**
	The Vulkan sub-context handler is responsible for negotiating queue submissions.

	In Vulkan, this directly maps to the VkQueue, but also implicitly support semaphore
	synchronization of required.

	To more efficiently utilize the GPU, call SetToNextContext after a Submit if 
	the subsequent Submits can run in parallel. If synchronization is a problem, 
	defer from using SetToNextContext such that we can run all commands on a single
	submission. 

	Submit also allows for waiting on the entire command buffer execution on the CPU.
	By either submitting true to the Submit function argument 'wait for fence', or 
	chose where to wait for it in the main code. Avoid waiting as well if possible,
	since it will stall the CPU.

	It also supports inserting a dependency on different queues, wherein the depender
	is the queue depending on another to finish working, will wait for the other to signal.

	TODO: We should be able to use this handler across devices too...

	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "vulkan/vulkan.h"
#include "util/fixedarray.h"
#include "coregraphics/config.h"
namespace Vulkan
{

class VkSubContextHandler
{
public:

	/// setup subcontext handler
	void Setup(VkDevice dev, const Util::FixedArray<uint> indexMap, const Util::FixedArray<uint> families);
	/// discard
	void Discard();
	/// set to next context of type
	void SetToNextContext(const CoreGraphicsQueueType type);

	/// add submission to context, but don't really execute
	void AddSubmission(CoreGraphicsQueueType type, VkCommandBuffer cmds, VkSemaphore waitSemaphore, VkPipelineStageFlags waitFlag, VkSemaphore signalSemaphore);
	/// add another wait to the previous submission
	void AddWaitSemaphore(CoreGraphicsQueueType type, VkSemaphore waitSemaphore, VkPipelineStageFlags waitFlag);
	/// flush submissions and send to GPU as one submit call
	void FlushSubmissions(CoreGraphicsQueueType type, VkFence fence, bool waitImmediately, VkSemaphore queueSemaphore = VK_NULL_HANDLE, VkPipelineStageFlags queueWaitFlags = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM);

	/// submit immediately
	void SubmitImmediate(CoreGraphicsQueueType type, VkCommandBuffer cmds, VkSemaphore waitSemaphore, VkPipelineStageFlags waitFlags, VkSemaphore signalSemaphore, VkFence fence, bool waitImmediately);

	/// wait for a queue to finish working
	void WaitIdle(const CoreGraphicsQueueType type);

	/// get current queue
	VkQueue GetQueue(const CoreGraphicsQueueType type);

private:
	friend const VkQueue GetQueue(const CoreGraphicsQueueType type, const IndexT index);

	VkDevice device;
	Util::FixedArray<VkQueue> drawQueues;
	Util::FixedArray<VkPipelineStageFlags> drawQueueStages;
	Util::FixedArray<VkQueue> computeQueues;
	Util::FixedArray<VkPipelineStageFlags> computeQueueStages;
	Util::FixedArray<VkQueue> transferQueues;
	Util::FixedArray<VkPipelineStageFlags> transferQueueStages;
	Util::FixedArray<VkQueue> sparseQueues;
	Util::FixedArray<VkPipelineStageFlags> sparseQueueStages;
	uint currentDrawQueue;
	uint currentComputeQueue;
	uint currentTransferQueue;
	uint currentSparseQueue;
	uint queueFamilies[NumQueueTypes];

	Util::FixedArray<Util::Array<VkCommandBuffer>> buffers;
	Util::FixedArray<Util::Array<VkSemaphore>> waitSemaphores;
	Util::FixedArray<Util::Array<VkPipelineStageFlags>> waitFlags;
	Util::FixedArray<Util::Array<VkSemaphore>> signalSemaphores;
	Util::FixedArray<Util::Array<VkSubmitInfo>> submitInfos;
};

} // namespace Vulkan
