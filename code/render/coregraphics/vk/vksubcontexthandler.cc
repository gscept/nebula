//------------------------------------------------------------------------------
//  vksubcontexthandler.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vksubcontexthandler.h"
#include "coregraphics/config.h"

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
	this->drawQueues.Resize(indexMap[families[GraphicsQueueType]]);
	this->drawQueueStages.Resize(indexMap[families[GraphicsQueueType]]);
	this->computeQueues.Resize(indexMap[families[ComputeQueueType]]);
	this->computeQueueStages.Resize(indexMap[families[ComputeQueueType]]);
	this->transferQueues.Resize(indexMap[families[TransferQueueType]]);
	this->transferQueueStages.Resize(indexMap[families[TransferQueueType]]);
	this->sparseQueues.Resize(indexMap[families[SparseQueueType]]);
	this->sparseQueueStages.Resize(indexMap[families[SparseQueueType]]);

	this->queueFamilies[GraphicsQueueType] = families[GraphicsQueueType];
	this->queueFamilies[ComputeQueueType] = families[ComputeQueueType];
	this->queueFamilies[TransferQueueType] = families[TransferQueueType];
	this->queueFamilies[SparseQueueType] = families[SparseQueueType];

	uint i;
	for (i = 0; i < indexMap[families[GraphicsQueueType]]; i++)
	{
		vkGetDeviceQueue(dev, families[GraphicsQueueType], i, &this->drawQueues[i]);
		this->drawQueueStages[i] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}

	for (i = 0; i < indexMap[families[ComputeQueueType]]; i++)
	{
		vkGetDeviceQueue(dev, families[ComputeQueueType], i, &this->computeQueues[i]);
		this->computeQueueStages[i] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}

	for (i = 0; i < indexMap[families[TransferQueueType]]; i++)
	{
		vkGetDeviceQueue(dev, families[TransferQueueType], i, &this->transferQueues[i]);
		this->transferQueueStages[i] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}

	for (i = 0; i < indexMap[families[SparseQueueType]]; i++)
	{
		vkGetDeviceQueue(dev, families[SparseQueueType], i, &this->sparseQueues[i]);
		this->sparseQueueStages[i] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}

	this->currentDrawQueue = 0;
	this->currentComputeQueue = 0;
	this->currentTransferQueue = 0;
	this->currentSparseQueue = 0;

	this->semaphoreCache.Resize(NumQueueTypes);

	for (i = 0; i < NumQueueTypes; i++)
	{
		VkSemaphoreCreateInfo info =
		{
			VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			nullptr,
			0
		};

		const uint numQueues = indexMap[families[i]];
		this->semaphoreCache[i].Resize(numQueues);
		for (uint j = 0; j < numQueues; j++)
		{
			VkResult res = vkCreateSemaphore(dev, &info, nullptr, &this->semaphoreCache[i][j]);
			n_assert(res == VK_SUCCESS);
		}
	}

	this->buffers.Resize(NumQueueTypes);
	this->waitSemaphores.Resize(NumQueueTypes);
	this->waitFlags.Resize(NumQueueTypes);
	this->signalSemaphores.Resize(NumQueueTypes);
	this->signalFlags.Resize(NumQueueTypes);

	// setup wait within queues
	for (i = 0; i < NumQueueTypes; i++)
	{
		const uint numQueues = indexMap[families[i]];
		for (uint j = 0; j < numQueues; j++)
		{
			this->signalSemaphores[i].Append(this->semaphoreCache[i][j]);
			this->signalFlags[i].Append(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
		}
		
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::Discard()
{
	IndexT i;
	for (i = 0; i < NumQueueTypes; i++)
	{
		for (IndexT j = 0; j < this->semaphoreCache[i].Size(); j++)
		{
			vkDestroySemaphore(this->device, this->semaphoreCache[i][j], nullptr);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::SetToNextContext(const CoreGraphicsQueueType type)
{
	Util::FixedArray<VkQueue>* list = nullptr;
	uint* currentQueue = nullptr;
	switch (type)
	{
	case GraphicsQueueType:
		list = &this->drawQueues;
		currentQueue = &this->currentDrawQueue;
		break;
	case ComputeQueueType:
		list = &this->computeQueues;
		currentQueue = &this->currentComputeQueue;
		break;
	case TransferQueueType:
		list = &this->transferQueues;
		currentQueue = &this->currentTransferQueue;
		break;
	case SparseQueueType:
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
VkSubContextHandler::InsertDependency(const Util::FixedArray<CoreGraphicsQueueType> dependers, const CoreGraphicsQueueType dependee, VkPipelineStageFlags waitFlags)
{

	/*
	uint* dependeeQueue = nullptr;
	switch (dependee)
	{
	case GraphicsQueueType:
		dependeeQueue = &this->currentDrawQueue;
		break;
	case ComputeQueueType:
		dependeeQueue = &this->currentComputeQueue;
		break;
	case TransferQueueType:
		dependeeQueue = &this->currentTransferQueue;
		break;
	case SparseQueueType:
		dependeeQueue = &this->currentSparseQueue;
		break;
	}

	// the depender will have to wait for the dependee, which is why we select the same semaphore from the stack
	this->signalSemaphores[dependee].Append(this->semaphoreCache[dependee][*dependeeQueue]);

	IndexT i;
	for (i = 0; i < dependers.Size(); i++)
	{
		uint* dependerQueue = nullptr;
		switch (dependers[i])
		{
		case GraphicsQueueType:
			dependerQueue = &this->currentDrawQueue;
			break;
		case ComputeQueueType:
			dependerQueue = &this->currentComputeQueue;
			break;
		case TransferQueueType:
			dependerQueue = &this->currentTransferQueue;
			break;
		case SparseQueueType:
			dependerQueue = &this->currentSparseQueue;
			break;
		}

		this->waitFlags[dependers[i]].Append(waitFlags);
		this->waitSemaphores[dependers[i]].Append(this->semaphoreCache[dependee][*dependerQueue]);
	}
	*/
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::InsertCommandBuffer(const CoreGraphicsQueueType type, const VkCommandBuffer buf)
{
	this->buffers[type].Append(buf);
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::Submit(const CoreGraphicsQueueType type, VkFence fence, bool waitImmediately)
{
	VkQueue queue = VK_NULL_HANDLE;
	switch (type)
	{
	case GraphicsQueueType:
		queue = this->drawQueues[this->currentDrawQueue];
		break;
	case ComputeQueueType:
		queue = this->computeQueues[this->currentComputeQueue];
		break;
	case TransferQueueType:
		queue = this->transferQueues[this->currentTransferQueue];
		break;
	case SparseQueueType:
		queue = this->sparseQueues[this->currentSparseQueue];
		break;
	}
	n_assert(queue != VK_NULL_HANDLE);
	Util::Array<VkCommandBuffer>* buffers = &this->buffers[type];
	Util::Array<VkSemaphore>* waitSemaphores = &this->waitSemaphores[type];
	Util::Array<VkPipelineStageFlags>* waitFlags = &this->waitFlags[type];
	Util::Array<VkSemaphore>* signalSemaphores = &this->signalSemaphores[type];
	Util::Array<VkPipelineStageFlags>* signalFlags = &this->signalFlags[type];

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
	VkResult res = vkQueueSubmit(queue, 1, &info, fence);
	n_assert(res == VK_SUCCESS);

	if (waitImmediately)
	{
		res = vkWaitForFences(this->device, 1, &fence, true, UINT64_MAX);
		n_assert(res == VK_SUCCESS);
	}

	waitSemaphores->Clear();
	waitSemaphores->AppendArray(*signalSemaphores);
	waitFlags->Clear();
	waitFlags->AppendArray(*signalFlags);
	// copy signaling semaphores to waiting for next time we encounter them
	/*
	
	signalSemaphores->Clear();
	signalFlags->Clear();
	*/

	// clear rest
	buffers->Clear();	
	
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::WaitIdle(const CoreGraphicsQueueType type)
{
	Util::FixedArray<VkQueue>* list = nullptr;
	switch (type)
	{
	case GraphicsQueueType:
		list = &this->drawQueues;
		break;
	case ComputeQueueType:
		list = &this->computeQueues;
		break;
	case TransferQueueType:
		list = &this->transferQueues;
		break;
	case SparseQueueType:
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
VkSubContextHandler::GetQueue(const CoreGraphicsQueueType type)
{
	switch (type)
	{
	case GraphicsQueueType:
		return this->drawQueues[this->currentDrawQueue];
	case ComputeQueueType:
		return this->computeQueues[this->currentComputeQueue];
	case TransferQueueType:
		return this->transferQueues[this->currentTransferQueue];
	case SparseQueueType:
		return this->sparseQueues[this->currentSparseQueue];
	default:
		n_error("Invalid queue type %d", type);
		return VK_NULL_HANDLE;
	}
}

} // namespace Vulkan
