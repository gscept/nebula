//------------------------------------------------------------------------------
//  vksubcontexthandler.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
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
	lastSubmissions[CoreGraphics::GraphicsQueueType] = nullptr;
	lastSubmissions[CoreGraphics::ComputeQueueType] = nullptr;
	lastSubmissions[CoreGraphics::TransferQueueType] = nullptr;
	lastSubmissions[CoreGraphics::SparseQueueType] = nullptr;
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
	this->drawQueues.Resize(indexMap[families[CoreGraphics::GraphicsQueueType]]);
	this->drawQueueStages.Resize(indexMap[families[CoreGraphics::GraphicsQueueType]]);
	this->computeQueues.Resize(indexMap[families[CoreGraphics::ComputeQueueType]]);
	this->computeQueueStages.Resize(indexMap[families[CoreGraphics::ComputeQueueType]]);
	this->transferQueues.Resize(indexMap[families[CoreGraphics::TransferQueueType]]);
	this->transferQueueStages.Resize(indexMap[families[CoreGraphics::TransferQueueType]]);
	this->sparseQueues.Resize(indexMap[families[CoreGraphics::SparseQueueType]]);
	this->sparseQueueStages.Resize(indexMap[families[CoreGraphics::SparseQueueType]]);

	this->queueFamilies[CoreGraphics::GraphicsQueueType] = families[CoreGraphics::GraphicsQueueType];
	this->queueFamilies[CoreGraphics::ComputeQueueType] = families[CoreGraphics::ComputeQueueType];
	this->queueFamilies[CoreGraphics::TransferQueueType] = families[CoreGraphics::TransferQueueType];
	this->queueFamilies[CoreGraphics::SparseQueueType] = families[CoreGraphics::SparseQueueType];

	uint i;
	for (i = 0; i < indexMap[families[CoreGraphics::GraphicsQueueType]]; i++)
	{
		vkGetDeviceQueue(dev, families[CoreGraphics::GraphicsQueueType], i, &this->drawQueues[i]);
		this->drawQueueStages[i] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}

	for (i = 0; i < indexMap[families[CoreGraphics::ComputeQueueType]]; i++)
	{
		vkGetDeviceQueue(dev, families[CoreGraphics::ComputeQueueType], i, &this->computeQueues[i]);
		this->computeQueueStages[i] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}

	for (i = 0; i < indexMap[families[CoreGraphics::TransferQueueType]]; i++)
	{
		vkGetDeviceQueue(dev, families[CoreGraphics::TransferQueueType], i, &this->transferQueues[i]);
		this->transferQueueStages[i] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}

	for (i = 0; i < indexMap[families[CoreGraphics::SparseQueueType]]; i++)
	{
		vkGetDeviceQueue(dev, families[CoreGraphics::SparseQueueType], i, &this->sparseQueues[i]);
		this->sparseQueueStages[i] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}

	// setup timeline semaphores
	this->timelineSubmissions.Resize(CoreGraphics::NumQueueTypes);
	for (IndexT i = 0; i < CoreGraphics::NumQueueTypes; i++)
	{
		VkSemaphoreTypeCreateInfo ext =
		{
			VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
			nullptr,
			VkSemaphoreType::VK_SEMAPHORE_TYPE_TIMELINE,
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

	this->submissions.Resize(CoreGraphics::NumQueueTypes);
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
VkSubContextHandler::SetToNextContext(const CoreGraphics::QueueType type)
{
	Util::FixedArray<VkQueue>* list = nullptr;
	uint* currentQueue = nullptr;
	switch (type)
	{
	case CoreGraphics::GraphicsQueueType:
		list = &this->drawQueues;
		currentQueue = &this->currentDrawQueue;
		break;
	case CoreGraphics::ComputeQueueType:
		list = &this->computeQueues;
		currentQueue = &this->currentComputeQueue;
		break;
	case CoreGraphics::TransferQueueType:
		list = &this->transferQueues;
		currentQueue = &this->currentTransferQueue;
		break;
	case CoreGraphics::SparseQueueType:
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
uint64 
VkSubContextHandler::AppendSubmissionTimeline(CoreGraphics::QueueType type, VkCommandBuffer cmds, bool semaphore)
{
	n_assert(cmds != VK_NULL_HANDLE);
	Util::Array<TimelineSubmission>& submissions = this->timelineSubmissions[type];
	submissions.Append(TimelineSubmission());
	TimelineSubmission& sub = submissions.Back();
	sub.queue = type;

	// if command buffer is present, add to 
	sub.buffers.Append(cmds);

	if (semaphore)
	{
		// add signal
		sub.signalSemaphores.Append(this->semaphores[type]);
		this->semaphoreSubmissionIds[type]++;
		sub.signalIndices.Append(this->semaphoreSubmissionIds[type]);
	}
	
	this->queueEmpty[type] = false;
	return this->semaphoreSubmissionIds[type];
}

//------------------------------------------------------------------------------
/**
*/
void 
VkSubContextHandler::AppendWaitTimeline(CoreGraphics::QueueType type, VkPipelineStageFlags waitFlags, CoreGraphics::QueueType waitQueue)
{
	TimelineSubmission& sub = this->timelineSubmissions[this->queueFamilies[type]].Back();

	n_assert(waitQueue != CoreGraphics::InvalidQueueType);
	uint payload = this->semaphoreSubmissionIds[this->queueFamilies[waitQueue]];
	if (payload > 0)
	{
		sub.waitIndices.Append(payload);
		sub.waitSemaphores.Append(this->semaphores[this->queueFamilies[waitQueue]]);
		sub.waitFlags.Append(waitFlags);
	}	
}

//------------------------------------------------------------------------------
/**
*/
void 
VkSubContextHandler::AppendWaitTimeline(CoreGraphics::QueueType type, VkPipelineStageFlags waitFlags, VkSemaphore waitSemaphore)
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
VkSubContextHandler::AppendSignalTimeline(CoreGraphics::QueueType type, VkSemaphore signalSemaphore)
{
	TimelineSubmission& sub = this->timelineSubmissions[type].Back();

	sub.signalIndices.Append(0);
	sub.signalSemaphores.Append(signalSemaphore);
}

//------------------------------------------------------------------------------
/**
*/
uint64 
VkSubContextHandler::FlushSubmissionsTimeline(CoreGraphics::QueueType type, VkFence fence)
{
	Util::Array<TimelineSubmission>& submissions = this->timelineSubmissions[type];
	uint64 ret = 0;

	// skip flush if submission list is empty
	if (submissions.IsEmpty())
		return ret;

	Util::FixedArray<VkSubmitInfo> submitInfos(submissions.Size());
	Util::FixedArray<VkTimelineSemaphoreSubmitInfo> extensions(submissions.Size());
	for (IndexT i = 0; i < submissions.Size(); i++)
	{
		TimelineSubmission& sub = submissions[i];

		// if we have no work, return
		if (sub.buffers.Size() == 0)
			continue;

		// save last signal index for return
		for (IndexT j = 0; j < sub.signalIndices.Size(); j++)
			ret = ret > sub.signalIndices[j] ? ret : sub.signalIndices[j];

		VkTimelineSemaphoreSubmitInfo ext =
		{
			VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
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
VkSubContextHandler::GetTimelineIndex(CoreGraphics::QueueType type)
{
	return this->semaphoreSubmissionIds[type];
}

//------------------------------------------------------------------------------
/**
*/
void 
VkSubContextHandler::Wait(CoreGraphics::QueueType type, uint64 index)
{
	// skip the undefined submission
	VkSemaphoreWaitInfo waitInfo =
	{
		VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
		nullptr,
		0,
		1,
		&this->semaphores[type],
		&index
	};
	VkResult res = vkWaitSemaphores(this->device, &waitInfo, UINT64_MAX);
	n_assert(res == VK_SUCCESS || res == VK_TIMEOUT);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkSubContextHandler::FlushSubmissions(CoreGraphics::QueueType type, VkFence fence)
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
VkSubContextHandler::SubmitBindSparseTimeline(CoreGraphics::QueueType type, const VkBindSparseInfo& info)
{
	VkQueue queue = this->GetQueue(type);

	// wait for graphics to finish before we perform our sparse bindings
	this->semaphoreSubmissionIds[this->queueFamilies[type]]++;
	VkTimelineSemaphoreSubmitInfo timelineSubmitInfo =
	{
		VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
		nullptr,
		1,
		&this->semaphoreSubmissionIds[CoreGraphics::GraphicsQueueType],
		1,
		&this->semaphoreSubmissionIds[this->queueFamilies[type]]
	};

	// patch up bind info
	VkBindSparseInfo modInfo = info;
	modInfo.pNext = &timelineSubmitInfo;
	modInfo.waitSemaphoreCount = 1;
	modInfo.pWaitSemaphores = &this->semaphores[CoreGraphics::GraphicsQueueType];
	modInfo.signalSemaphoreCount = 1;
	modInfo.pSignalSemaphores = &this->semaphores[this->queueFamilies[type]];

	// submit
	VkResult res = vkQueueBindSparse(queue, 1, &modInfo, nullptr);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::WaitIdle(const CoreGraphics::QueueType type)
{
	Util::FixedArray<VkQueue>* list = nullptr;
	switch (type)
	{
	case CoreGraphics::GraphicsQueueType:
		list = &this->drawQueues;
		break;
	case CoreGraphics::ComputeQueueType:
		list = &this->computeQueues;
		break;
	case CoreGraphics::TransferQueueType:
		list = &this->transferQueues;
		break;
	case CoreGraphics::SparseQueueType:
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
VkSubContextHandler::GetQueue(const CoreGraphics::QueueType type)
{
	switch (type)
	{
	case CoreGraphics::GraphicsQueueType:
		return this->drawQueues[this->currentDrawQueue];
	case CoreGraphics::ComputeQueueType:
		return this->computeQueues[this->currentComputeQueue];
	case CoreGraphics::TransferQueueType:
		return this->transferQueues[this->currentTransferQueue];
	case CoreGraphics::SparseQueueType:
		return this->sparseQueues[this->currentSparseQueue];
	default:
		n_error("Invalid queue type %d", type);
		return VK_NULL_HANDLE;
	}
}

} // namespace Vulkan
