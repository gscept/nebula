//------------------------------------------------------------------------------
//  vksubcontexthandler.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vksubcontexthandler.h"
namespace Vulkan
{

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::Setup(VkDevice dev, const Util::FixedArray<uint> indexMap, const Util::FixedArray<uint> families)
{
	// store device
	this->device = dev;

	// get all queues related to their respective family (we can have more draw queues than 1, for example)
	this->drawQueues.Resize(indexMap[families[DrawContextType]]);
	this->computeQueues.Resize(indexMap[families[ComputeContextType]]);
	this->transferQueues.Resize(indexMap[families[TransferContextType]]);
	this->sparseQueues.Resize(indexMap[families[SparseContextType]]);

	uint i;
	for (i = 0; i < indexMap[families[DrawContextType]]; i++)
	{
		vkGetDeviceQueue(dev, families[DrawContextType], i, &this->drawQueues[i]);
	}

	for (i = 0; i < indexMap[families[ComputeContextType]]; i++)
	{
		vkGetDeviceQueue(dev, families[ComputeContextType], i, &this->computeQueues[i]);
	}

	for (i = 0; i < indexMap[families[TransferContextType]]; i++)
	{
		vkGetDeviceQueue(dev, families[TransferContextType], i, &this->transferQueues[i]);
	}

	for (i = 0; i < indexMap[families[SparseContextType]]; i++)
	{
		vkGetDeviceQueue(dev, families[SparseContextType], i, &this->sparseQueues[i]);
	}

	this->currentDrawQueue = 0;
	this->currentComputeQueue = 0;
	this->currentTransferQueue = 0;
	this->currentSparseQueue = 0;

	this->semaphoreCache.Resize(NumSubContextTypes);

	for (i = 0; i < NumSubContextTypes; i++)
	{
		VkSemaphoreCreateInfo info =
		{
			VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			nullptr,
			0
		};

		const uint numQueues = indexMap[families[i]];
		this->semaphoreCache.Resize(numQueues);
		for (uint j = 0; j < numQueues; j++)
		{
			VkResult res = vkCreateSemaphore(dev, &info, nullptr, &this->semaphoreCache[i][j]);
			n_assert(res == VK_SUCCESS);
		}
	}

	this->buffers.Resize(NumSubContextTypes);
	this->waitSemaphores.Resize(NumSubContextTypes);
	this->waitFlags.Resize(NumSubContextTypes);
	this->signalSemaphores.Resize(NumSubContextTypes);
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::SetToNextContext(const SubContextType type)
{
	Util::FixedArray<VkQueue>* list = nullptr;
	uint* currentQueue = nullptr;
	switch (type)
	{
	case DrawContextType:
		list = &this->drawQueues;
		currentQueue = &this->currentDrawQueue;
		break;
	case ComputeContextType:
		list = &this->computeQueues;
		currentQueue = &this->currentComputeQueue;
		break;
	case TransferContextType:
		list = &this->transferQueues;
		currentQueue = &this->currentTransferQueue;
		break;
	case SparseContextType:
		list = &this->sparseQueues;
		currentQueue = &this->currentSparseQueue;
		break;
	}

	// progress the queue index
	*currentQueue = (*currentQueue + 1) % list->Size();
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::InsertDependency(const SubContextType depender, const SubContextType dependee, VkPipelineStageFlags waitFlags)
{
	uint* dependerQueue = nullptr;
	switch (depender)
	{
	case DrawContextType:
		dependerQueue = &this->currentDrawQueue;
		break;
	case ComputeContextType:
		dependerQueue = &this->currentComputeQueue;
		break;
	case TransferContextType:
		dependerQueue = &this->currentTransferQueue;
		break;
	case SparseContextType:
		dependerQueue = &this->currentSparseQueue;
		break;
	}

	uint* dependeeQueue = nullptr;
	switch (dependee)
	{
	case DrawContextType:
		dependeeQueue = &this->currentDrawQueue;
		break;
	case ComputeContextType:
		dependeeQueue = &this->currentComputeQueue;
		break;
	case TransferContextType:
		dependeeQueue = &this->currentTransferQueue;
		break;
	case SparseContextType:
		dependeeQueue = &this->currentSparseQueue;
		break;
	}

	// the depender will have to wait for the dependee, which is why we select the same semaphore from the stack
	this->waitFlags[depender].Append(waitFlags);
	this->signalSemaphores[dependee].Append(this->semaphoreCache[dependee][*dependeeQueue]);
	this->waitSemaphores[depender].Append(this->semaphoreCache[dependee][*dependerQueue]);
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::InsertCommandBuffer(const SubContextType type, const VkCommandBuffer buf)
{
	this->buffers[type].Append(buf);
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::Submit(const SubContextType type, VkFence fence, bool waitImmediately)
{
	Util::FixedArray<VkQueue>* list = nullptr;
	uint* currentQueue = nullptr;
	switch (type)
	{
	case DrawContextType:
		list = &this->drawQueues;
		currentQueue = &this->currentDrawQueue;
		break;
	case ComputeContextType:
		list = &this->computeQueues;
		currentQueue = &this->currentComputeQueue;
		break;
	case TransferContextType:
		list = &this->transferQueues;
		currentQueue = &this->currentTransferQueue;
		break;
	case SparseContextType:
		list = &this->sparseQueues;
		currentQueue = &this->currentSparseQueue;
		break;
	}
	Util::Array<VkCommandBuffer>* buffers = this->buffers[type].IsEmpty() ? nullptr : &this->buffers[type];
	Util::Array<VkSemaphore>* waitSemaphores = this->waitSemaphores[type].IsEmpty() ? nullptr : &this->waitSemaphores[type];
	Util::Array<VkPipelineStageFlags>* waitFlags = this->waitFlags[type].IsEmpty() ? nullptr : &this->waitFlags[type];
	Util::Array<VkSemaphore>* signalSemaphores = this->signalSemaphores[type].IsEmpty() ? nullptr : &this->signalSemaphores[type];

	VkSubmitInfo info =
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		nullptr,
		waitSemaphores->Size(),
		waitSemaphores->IsEmpty()	? nullptr : waitSemaphores->Begin(),
		waitFlags->IsEmpty()		? nullptr : waitFlags->Begin(),
		buffers->Size(),
		buffers->IsEmpty()			? nullptr : buffers->Begin(),
		signalSemaphores->Size(),
		signalSemaphores->IsEmpty() ? nullptr : signalSemaphores->Begin(),

	};
	VkResult res = vkQueueSubmit((*list)[*currentQueue], 1, &info, fence);
	n_assert(res == VK_SUCCESS);

	if (waitImmediately)
	{
		res = vkWaitForFences(this->device, 1, &fence, true, UINT64_MAX);
		n_assert(res == VK_SUCCESS);
	}

	buffers->Clear();
	waitSemaphores->Clear();
	waitFlags->Clear();
	signalSemaphores->Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::WaitIdle(const SubContextType type)
{
	Util::FixedArray<VkQueue>* list = nullptr;
	switch (type)
	{
	case DrawContextType:
		list = &this->drawQueues;
		break;
	case ComputeContextType:
		list = &this->computeQueues;
		break;
	case TransferContextType:
		list = &this->transferQueues;
		break;
	case SparseContextType:
		list = &this->sparseQueues;
		break;
	}
	for (IndexT i = 0; i < list->Size(); i++)
	{
		VkResult res = vkQueueWaitIdle((*list)[i]);
		n_assert(res == VK_SUCCESS);
	}
}

//------------------------------------------------------------------------------
/**
*/
VkQueue
VkSubContextHandler::GetQueue(const SubContextType type)
{
	switch (type)
	{
	case DrawContextType:
		return this->drawQueues[this->currentDrawQueue];
	case ComputeContextType:
		return this->computeQueues[this->currentComputeQueue];
	case TransferContextType:
		return this->transferQueues[this->currentTransferQueue];
	case SparseContextType:
		return this->sparseQueues[this->currentSparseQueue];
	default:
		n_error("Invalid queue type %d", type);
		return VK_NULL_HANDLE;
	}
}

} // namespace Vulkan
