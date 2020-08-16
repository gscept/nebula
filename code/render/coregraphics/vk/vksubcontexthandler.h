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

	(C)2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "vkloader.h"
#include "util/fixedarray.h"
#include "coregraphics/config.h"
namespace Vulkan
{

class VkSubContextHandler
{
public:

	/// constructor
	VkSubContextHandler();
	/// destructor
	~VkSubContextHandler();

	struct TimelineSubmission
	{
		Util::Array<uint64> signalIndices;
		Util::Array<VkSemaphore> signalSemaphores;
		Util::Array<VkCommandBuffer> buffers;
		Util::Array<VkPipelineStageFlags> waitFlags;
		Util::Array<VkSemaphore> waitSemaphores;
		Util::Array<uint64> waitIndices;
	};

	struct SparseBindSubmission
	{
		VkBindSparseInfo bindInfo;
		Util::Array<uint64> signalIndices;
		Util::Array<VkSemaphore> signalSemaphores;
		Util::FixedArray<VkSparseMemoryBind> opaqueMemoryBinds;
		Util::FixedArray<VkSparseImageMemoryBind> imageMemoryBinds;
		Util::FixedArray<VkSparseMemoryBind> bufferMemoryBinds;
		Util::Array<VkSparseBufferMemoryBindInfo> bufferMemoryBindInfos;
		Util::Array<VkSparseImageOpaqueMemoryBindInfo> imageOpaqueBindInfos;
		Util::Array<VkSparseImageMemoryBindInfo> imageMemoryBindInfos;
		Util::Array<VkSemaphore> waitSemaphores;
		Util::Array<uint64> waitIndices;
	};

	/// setup subcontext handler
	void Setup(VkDevice dev, const Util::FixedArray<uint> indexMap, const Util::FixedArray<uint> families);
	/// discard
	void Discard();
	/// set to next context of type
	void SetToNextContext(const CoreGraphics::QueueType type);

	/// append submission to context to execute later, supports waiting for a queue
	uint64 AppendSubmissionTimeline(CoreGraphics::QueueType type, VkCommandBuffer cmds, bool semaphore = true);
	/// append a wait on the current submission of a specific queue
	void AppendWaitTimeline(CoreGraphics::QueueType type, VkPipelineStageFlags waitFlags, CoreGraphics::QueueType waitQueue);
	/// append a sparse bind timeline operation
	uint64 AppendSparseBind(CoreGraphics::QueueType type, const VkImage img, const Util::Array<VkSparseMemoryBind>& opaqueBinds, const Util::Array<VkSparseImageMemoryBind>& pageBinds);
	/// append a wait on a binary semaphore
	void AppendWaitTimeline(CoreGraphics::QueueType type, VkPipelineStageFlags waitFlags, VkSemaphore waitSemaphore);
	/// append a binary semaphore to signal when done
	void AppendSignalTimeline(CoreGraphics::QueueType type, VkSemaphore signalSemaphore);
	/// flush submissions
	uint64 FlushSubmissionsTimeline(CoreGraphics::QueueType type, VkFence fence);
	/// get timeline index
	uint64 GetTimelineIndex(CoreGraphics::QueueType type);
	/// wait for timeline index
	void Wait(CoreGraphics::QueueType type, uint64 index);
	/// check to see if timeline index has passed
	bool Poll(CoreGraphics::QueueType type, uint64 index);

	/// flush sparse binds
	void FlushSparseBinds(VkFence fence);

	/// wait for a queue to finish working
	void WaitIdle(const CoreGraphics::QueueType type);

	/// get current queue
	VkQueue GetQueue(const CoreGraphics::QueueType type);

private:
	friend const VkQueue GetQueue(const CoreGraphics::QueueType type, const IndexT index);

	VkDevice device;
	Util::FixedArray<VkQueue> drawQueues;
	Util::FixedArray<VkQueue> computeQueues;
	Util::FixedArray<VkQueue> transferQueues;
	Util::FixedArray<VkQueue> sparseQueues;
	uint currentDrawQueue;
	uint currentComputeQueue;
	uint currentTransferQueue;
	uint currentSparseQueue;
	uint queueFamilies[CoreGraphics::NumQueueTypes];

	VkSemaphore semaphores[CoreGraphics::NumQueueTypes];
	uint64 semaphoreSubmissionIds[CoreGraphics::NumQueueTypes];

	Util::FixedArray<Util::Array<TimelineSubmission>> timelineSubmissions;
	Util::Array<SparseBindSubmission> sparseBindSubmissions;
};

} // namespace Vulkan
