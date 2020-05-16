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
	this->computeQueues.Resize(indexMap[families[CoreGraphics::ComputeQueueType]]);
	this->transferQueues.Resize(indexMap[families[CoreGraphics::TransferQueueType]]);
	this->sparseQueues.Resize(indexMap[families[CoreGraphics::SparseQueueType]]);

	this->queueFamilies[CoreGraphics::GraphicsQueueType] = families[CoreGraphics::GraphicsQueueType];
	this->queueFamilies[CoreGraphics::ComputeQueueType] = families[CoreGraphics::ComputeQueueType];
	this->queueFamilies[CoreGraphics::TransferQueueType] = families[CoreGraphics::TransferQueueType];
	this->queueFamilies[CoreGraphics::SparseQueueType] = families[CoreGraphics::SparseQueueType];

	uint i;
	for (i = 0; i < indexMap[families[CoreGraphics::GraphicsQueueType]]; i++)
	{
		vkGetDeviceQueue(dev, families[CoreGraphics::GraphicsQueueType], i, &this->drawQueues[i]);
	}

	for (i = 0; i < indexMap[families[CoreGraphics::ComputeQueueType]]; i++)
	{
		vkGetDeviceQueue(dev, families[CoreGraphics::ComputeQueueType], i, &this->computeQueues[i]);
	}

	for (i = 0; i < indexMap[families[CoreGraphics::TransferQueueType]]; i++)
	{
		vkGetDeviceQueue(dev, families[CoreGraphics::TransferQueueType], i, &this->transferQueues[i]);
	}

	for (i = 0; i < indexMap[families[CoreGraphics::SparseQueueType]]; i++)
	{
		vkGetDeviceQueue(dev, families[CoreGraphics::SparseQueueType], i, &this->sparseQueues[i]);
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
	}

	this->currentDrawQueue = 0;
	this->currentComputeQueue = 0;
	this->currentTransferQueue = 0;
	this->currentSparseQueue = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::Discard()
{
	this->timelineSubmissions.Clear();
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

	// if command buffer is present, add it
	sub.buffers.Append(cmds);

	if (semaphore)
	{
		// add signal
		sub.signalSemaphores.Append(this->semaphores[type]);
		this->semaphoreSubmissionIds[type]++;
		sub.signalIndices.Append(this->semaphoreSubmissionIds[type]);
	}
	
	return this->semaphoreSubmissionIds[type];
}

//------------------------------------------------------------------------------
/**
*/
void 
VkSubContextHandler::AppendWaitTimeline(CoreGraphics::QueueType type, VkPipelineStageFlags waitFlags, CoreGraphics::QueueType waitQueue)
{
	TimelineSubmission& sub = this->timelineSubmissions[type].Back();

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
uint64_t 
VkSubContextHandler::AppendSparseBind(CoreGraphics::QueueType type, const VkImage img, const Util::Array<VkSparseMemoryBind>& opaqueBinds, const Util::Array<VkSparseImageMemoryBind>& pageBinds)
{
	this->sparseBindSubmissions.Append(SparseBindSubmission());
	SparseBindSubmission& submission = this->sparseBindSubmissions.Back();

	// setup bind structs, add support for ordinary buffer
	if (!pageBinds.IsEmpty())
	{
		submission.imageMemoryBinds.Resize(pageBinds.Size());
		memcpy(submission.imageMemoryBinds.Begin(), pageBinds.Begin(), pageBinds.Size() * sizeof(VkSparseImageMemoryBind));
		VkSparseImageMemoryBindInfo imageMemoryBindInfo =
		{
			img,
			submission.imageMemoryBinds.Size(),
			submission.imageMemoryBinds.Size() > 0 ? submission.imageMemoryBinds.Begin() : nullptr
		};
		submission.imageMemoryBindInfos.Append(imageMemoryBindInfo);
	}

	if (!opaqueBinds.IsEmpty())
	{
		submission.opaqueMemoryBinds.Resize(opaqueBinds.Size());
		memcpy(submission.opaqueMemoryBinds.Begin(), opaqueBinds.Begin(), opaqueBinds.Size() * sizeof(VkSparseMemoryBind));
		VkSparseImageOpaqueMemoryBindInfo opaqueMemoryBindInfo =
		{
			img,
			submission.opaqueMemoryBinds.Size(),
			submission.opaqueMemoryBinds.Size() > 0 ? submission.opaqueMemoryBinds.Begin() : nullptr
		};
		submission.imageOpaqueBindInfos.Append(opaqueMemoryBindInfo);
	}

	// add signal
	submission.signalSemaphores.Append(this->semaphores[type]);
	this->semaphoreSubmissionIds[type]++;
	submission.signalIndices.Append(this->semaphoreSubmissionIds[type]);

	// add wait
	submission.waitSemaphores.Append(this->semaphores[CoreGraphics::GraphicsQueueType]);
	submission.waitIndices.Append(this->semaphoreSubmissionIds[CoreGraphics::GraphicsQueueType]);

	return this->semaphoreSubmissionIds[type];
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
VkSubContextHandler::FlushSparseBinds(VkFence fence)
{
	// abort early
	if (this->sparseBindSubmissions.IsEmpty())
		return;

	CoreGraphics::QueueType type = CoreGraphics::SparseQueueType;
	VkQueue queue = this->GetQueue(type);

	Util::FixedArray<VkTimelineSemaphoreSubmitInfo> extensions(this->sparseBindSubmissions.Size());
	Util::FixedArray<VkBindSparseInfo> bindInfo(this->sparseBindSubmissions.Size());
	for (IndexT i = 0; i < this->sparseBindSubmissions.Size(); i++)
	{
		const SparseBindSubmission& sub = this->sparseBindSubmissions[i];

		// wait for graphics to finish before we perform our sparse bindings
		VkTimelineSemaphoreSubmitInfo timelineSubmitInfo =
		{
			VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
			nullptr,
			sub.waitIndices.Size(),
			sub.waitIndices.Begin(),
			sub.signalIndices.Size(),
			sub.signalIndices.Begin()
		};
		extensions[i] = timelineSubmitInfo;

		// patch up bind info
		VkBindSparseInfo& modInfo = bindInfo[i];
		modInfo.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
		modInfo.pNext = &extensions[i];
		modInfo.waitSemaphoreCount = sub.waitSemaphores.Size();
		modInfo.pWaitSemaphores = sub.waitSemaphores.Begin();
		modInfo.signalSemaphoreCount = sub.signalSemaphores.Size();
		modInfo.pSignalSemaphores = sub.signalSemaphores.Begin();

		modInfo.bufferBindCount = sub.bufferMemoryBindInfos.Size();
		modInfo.pBufferBinds = sub.bufferMemoryBindInfos.Size() > 0 ? sub.bufferMemoryBindInfos.Begin() : nullptr;
		modInfo.imageOpaqueBindCount = sub.imageOpaqueBindInfos.Size();
		modInfo.pImageOpaqueBinds = sub.imageOpaqueBindInfos.Size() > 0 ? sub.imageOpaqueBindInfos.Begin() : nullptr;
		modInfo.imageBindCount = sub.imageMemoryBindInfos.Size();
		modInfo.pImageBinds = sub.imageMemoryBindInfos.Size() > 0 ? sub.imageMemoryBindInfos.Begin() : nullptr;
	}

	// submit
	VkResult res = vkQueueBindSparse(queue, bindInfo.Size(), bindInfo.Begin(), fence);
	n_assert(res == VK_SUCCESS);

	this->sparseBindSubmissions.Clear();
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
