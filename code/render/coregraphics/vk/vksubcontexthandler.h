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

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "vulkan/vulkan.h"
#include "util/fixedarray.h"
namespace Vulkan
{

class VkSubContextHandler
{
public:

	enum SubContextType
	{
		DrawContextType,
		ComputeContextType,
		TransferContextType,
		SparseContextType,

		NumSubContextTypes
	};

	/// setup subcontext handler
	void Setup(VkDevice dev, const Util::FixedArray<uint> indexMap, const Util::FixedArray<uint> families);
	/// set to next context of type
	void SetToNextContext(const SubContextType type);
	/// insert dependency between the current two queues of given types
	void InsertDependency(const SubContextType depender, const SubContextType dependee, VkPipelineStageFlags waitFlags);
	/// insert command buffer
	void InsertCommandBuffer(const SubContextType type, const VkCommandBuffer buf);
	/// submit commands to subcontext, also consumes the dependencies put on this queue
	void Submit(const SubContextType type, VkFence fence, bool waitImmediately);
	/// wait for a queue to finish working
	void WaitIdle(const SubContextType type);

	/// get current queue
	VkQueue GetQueue(const SubContextType type);

private:
	friend class VkRenderDevice;

	VkDevice device;
	Util::FixedArray<VkQueue> drawQueues;
	Util::FixedArray<VkQueue> computeQueues;
	Util::FixedArray<VkQueue> transferQueues;
	Util::FixedArray<VkQueue> sparseQueues;
	uint currentDrawQueue;
	uint currentComputeQueue;
	uint currentTransferQueue;
	uint currentSparseQueue;

	Util::FixedArray<Util::FixedArray<VkSemaphore>> semaphoreCache;

	Util::FixedArray<Util::Array<VkCommandBuffer>> buffers;
	Util::FixedArray<Util::Array<VkSemaphore>> waitSemaphores;
	Util::FixedArray<Util::Array<VkPipelineStageFlags>> waitFlags;
	Util::FixedArray<Util::Array<VkSemaphore>> signalSemaphores;
};

} // namespace Vulkan
