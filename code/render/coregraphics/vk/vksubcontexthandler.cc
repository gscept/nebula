//------------------------------------------------------------------------------
//  vksubcontexthandler.cc
//  (C)2017-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vksubcontexthandler.h"
#include "coregraphics/config.h"

namespace Vulkan
{

//------------------------------------------------------------------------------
/**
*/
VkSubContextHandler::VkSubContextHandler()
{
	lastSubmissions[GraphicsQueueType] = nullptr;
	lastSubmissions[ComputeQueueType] = nullptr;
	lastSubmissions[TransferQueueType] = nullptr;
	lastSubmissions[SparseQueueType] = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
VkSubContextHandler::~VkSubContextHandler()
{
	// empty
}

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

	// setup timeline semaphores
	this->timelineSubmissions.Resize(NumQueueTypes);
	for (IndexT i = 0; i < NumQueueTypes; i++)
	{
		VkSemaphoreTypeCreateInfoKHR ext =
		{
			VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR,
			nullptr,
			VkSemaphoreTypeKHR::VK_SEMAPHORE_TYPE_TIMELINE_KHR,
			0
		};
		VkSemaphoreCreateInfo inf =
		{
			VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			&ext,
			0
		};
		VkResult res = vkCreateSemaphore(this->device, &inf, nullptr, &semaphores[i]);
		n_assert(res == VK_SUCCESS);

		semaphoreSubmissionIds[i] = 0;
		this->queueEmpty[i] = true;
	}

	this->currentDrawQueue = 0;
	this->currentComputeQueue = 0;
	this->currentTransferQueue = 0;
	this->currentSparseQueue = 0;

	this->submissions.Resize(NumQueueTypes);
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::Discard()
{
	this->submissions.Clear();
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
VkSubContextHandler::AppendSubmissionTimeline(CoreGraphicsQueueType type, VkCommandBuffer cmds)
{
	n_assert(cmds != VK_NULL_HANDLE);
	Util::Array<TimelineSubmission>& submissions = this->timelineSubmissions[type];
	submissions.Append(TimelineSubmission());
	TimelineSubmission& sub = submissions.Back();
	sub.queue = type;

	// if command buffer is present, add to 
	sub.buffers.Append(cmds);

	// add signal
	sub.signalSemaphores.Append(this->semaphores[type]);
	this->semaphoreSubmissionIds[type]++;
	sub.signalIndices.Append(this->semaphoreSubmissionIds[type]);
	this->queueEmpty[type] = false;
}

//------------------------------------------------------------------------------
/**
*/
void 
VkSubContextHandler::AppendWaitTimeline(CoreGraphicsQueueType type, VkPipelineStageFlags waitFlags, CoreGraphicsQueueType waitQueue)
{
	TimelineSubmission& sub = this->timelineSubmissions[type].Back();

	n_assert(waitQueue != InvalidQueueType);
	sub.waitIndices.Append(this->semaphoreSubmissionIds[waitQueue]);
	sub.waitSemaphores.Append(this->semaphores[waitQueue]);
	sub.waitFlags.Append(waitFlags);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkSubContextHandler::AppendWaitTimeline(CoreGraphicsQueueType type, VkPipelineStageFlags waitFlags, CoreGraphicsQueueType waitQueue, const uint64 index)
{
	TimelineSubmission& sub = this->timelineSubmissions[type].Back();

	n_assert(waitQueue != InvalidQueueType);
	sub.waitIndices.Append(index);
	sub.waitSemaphores.Append(this->semaphores[waitQueue]);
	sub.waitFlags.Append(waitFlags);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkSubContextHandler::AppendWaitTimeline(CoreGraphicsQueueType type, VkPipelineStageFlags waitFlags, VkSemaphore waitSemaphore)
{
	TimelineSubmission& sub = this->timelineSubmissions[type].Back();

	sub.waitIndices.Append(0);
	sub.waitSemaphores.Append(waitSemaphore);
	sub.waitFlags.Append(waitFlags);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkSubContextHandler::AppendSignalTimeline(CoreGraphicsQueueType type, VkSemaphore signalSemaphore)
{
	TimelineSubmission& sub = this->timelineSubmissions[type].Back();

	sub.signalIndices.Append(0);
	sub.signalSemaphores.Append(signalSemaphore);
}

//------------------------------------------------------------------------------
/**
*/
uint64 
VkSubContextHandler::FlushSubmissionsTimeline(CoreGraphicsQueueType type, VkFence fence)
{
	Util::Array<TimelineSubmission>& submissions = this->timelineSubmissions[type];
	uint64 ret = UINT64_MAX;

	// skip flush if submission list is empty
	if (submissions.IsEmpty())
		return ret;

	Util::FixedArray<VkSubmitInfo> submitInfos(submissions.Size());
	Util::FixedArray<VkTimelineSemaphoreSubmitInfoKHR> extensions(submissions.Size());
	for (IndexT i = 0; i < submissions.Size(); i++)
	{
		TimelineSubmission& sub = submissions[i];

		// if we have no work, return
		if (sub.buffers.Size() == 0)
			continue;

		// save last signal index for return
		ret = sub.signalIndices.Back();
		VkTimelineSemaphoreSubmitInfoKHR ext =
		{
			VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO_KHR,
			nullptr,
			sub.waitIndices.Size(),
			sub.waitIndices.Size() > 0 ? sub.waitIndices.Begin() : nullptr,
			sub.signalIndices.Size(),
			sub.signalIndices.Size() > 0 ? sub.signalIndices.Begin() : nullptr
		};
		extensions[i] = ext;

		VkSubmitInfo info =
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			&extensions[i],
			sub.waitSemaphores.Size(),
			sub.waitSemaphores.Size() > 0 ? sub.waitSemaphores.Begin() : nullptr,
			sub.waitFlags.Size() > 0 ? sub.waitFlags.Begin() : nullptr,
			sub.buffers.Size(),
			sub.buffers.Size() > 0 ? sub.buffers.Begin() : nullptr,
			sub.signalSemaphores.Size(),								// if we have a finish semaphore, add it on the submit
			sub.signalSemaphores.Size() > 0 ? sub.signalSemaphores.Begin() : nullptr
		};
		submitInfos[i] = info;
	}

	// execute all commands
	VkQueue queue = this->GetQueue(type);
	VkResult res = vkQueueSubmit(queue, submitInfos.Size(), submitInfos.Begin(), fence);
	n_assert(res == VK_SUCCESS);

	// clear submissions
	submissions.Clear();

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
uint64 
VkSubContextHandler::GetTimelineIndex(CoreGraphicsQueueType type)
{
	return this->semaphoreSubmissionIds[type];
}

//------------------------------------------------------------------------------
/**
*/
void 
VkSubContextHandler::AppendSubmission(CoreGraphicsQueueType type, VkCommandBuffer cmds, VkSemaphore waitSemaphore, VkPipelineStageFlags waitFlag, VkSemaphore signalSemaphore)
{
	Util::Array<Submission>& submissions = this->submissions[type];

	// append new submission
	submissions.Append(Submission{});
	Submission& sub = submissions.Back();
	this->lastSubmissions[type] = &sub;
	sub.queue = type;

	// if command buffer is present, add to 
	if (cmds != VK_NULL_HANDLE)
	{
		sub.buffers.Append(cmds);
	}
		
	// if we have wait semaphores, add both flags and the semaphore it self
	if (waitSemaphore != VK_NULL_HANDLE)
	{
		sub.waitSemaphores.Append(waitSemaphore);
		sub.waitFlags.Append(waitFlag);
	}

	// finally add signal semaphore if present
	if (signalSemaphore != VK_NULL_HANDLE)
	{
		sub.signalSemaphores.Append(signalSemaphore);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
VkSubContextHandler::AddWaitSemaphore(CoreGraphicsQueueType type, VkSemaphore waitSemaphore, VkPipelineStageFlags waitFlag)
{
	Util::Array<Submission>& submissions = this->submissions[type];
	Submission& sub = submissions.Back();

	// if we have wait semaphores, add both flags and the semaphore itself
	if (waitSemaphore != VK_NULL_HANDLE)
	{
		sub.waitSemaphores.Append(waitSemaphore);
		sub.waitFlags.Append(waitFlag);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
VkSubContextHandler::AddSignalSemaphore(CoreGraphicsQueueType type, VkSemaphore signalSemaphore)
{
	Util::Array<Submission>& submissions = this->submissions[type];
	Submission& sub = submissions.Back();

	// if we have wait semaphores, add both flags and the semaphore itself
	if (signalSemaphore != VK_NULL_HANDLE)
	{
		sub.signalSemaphores.Append(signalSemaphore);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
VkSubContextHandler::FlushSubmissions(CoreGraphicsQueueType type, VkFence fence)
{
	Util::Array<Submission>& submissions = this->submissions[type];

	if (submissions.Size() > 0)
	{
		VkQueue queue = this->GetQueue(type);
		Util::FixedArray<VkSubmitInfo> submitInfos(submissions.Size());
		for (IndexT i = 0; i < submissions.Size(); i++)
		{
			VkSubmitInfo info =
			{
				VK_STRUCTURE_TYPE_SUBMIT_INFO,
				nullptr,
				submissions[i].waitSemaphores.Size(),
				submissions[i].waitSemaphores.Size() > 0 ? submissions[i].waitSemaphores.Begin() : nullptr,
				submissions[i].waitFlags.Size() > 0 ? submissions[i].waitFlags.Begin() : nullptr,
				submissions[i].buffers.Size(),
				submissions[i].buffers.Size() > 0 ? submissions[i].buffers.Begin() : nullptr,
				submissions[i].signalSemaphores.Size(),
				submissions[i].signalSemaphores.Size() > 0 ? submissions[i].signalSemaphores.Begin() : nullptr
			};
			submitInfos[i] = info;
		}

		VkResult res = vkQueueSubmit(queue, submitInfos.Size(), submitInfos.Begin(), fence);
		n_assert(res == VK_SUCCESS);
	}

	// clear the submit infos
	this->submissions[type].Clear();
}

//------------------------------------------------------------------------------
/**
*/
void 
VkSubContextHandler::SubmitFence(CoreGraphicsQueueType type, VkFence fence)
{
	VkQueue queue = this->GetQueue(type);
	VkResult res = vkQueueSubmit(queue, 0, nullptr, fence);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void VkSubContextHandler::SubmitImmediate(CoreGraphicsQueueType type, VkCommandBuffer cmds, VkSemaphore waitSemaphore, VkPipelineStageFlags waitFlags, VkSemaphore signalSemaphore, VkFence fence, bool waitImmediately)
{
	VkQueue queue = this->GetQueue(type);
	VkSubmitInfo info =
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		nullptr,
		0, nullptr, nullptr,	// wait
		0, nullptr,				// cmd buf
		0, nullptr				// signal
	};


	// if command buffer is present, add to 
	if (cmds != VK_NULL_HANDLE)
	{
		info.commandBufferCount = 1;
		info.pCommandBuffers = &cmds;
	}

	// if we have wait semaphores, add both flags and the semaphore it self
	if (waitSemaphore != VK_NULL_HANDLE)
	{
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &waitSemaphore;
		info.pWaitDstStageMask = &waitFlags;
	}

	// finally add signal semaphore if present
	if (signalSemaphore != VK_NULL_HANDLE)
	{
		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores = &signalSemaphore;
	}

	// submit
	VkResult res = vkQueueSubmit(queue, 1, &info, fence);
	n_assert(res == VK_SUCCESS);

	if (waitImmediately && fence != VK_NULL_HANDLE)
	{
		res = vkWaitForFences(this->device, 1, &fence, true, UINT64_MAX);
		n_assert(res == VK_SUCCESS);
	}
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

	this->queueEmpty[type] = true;
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
