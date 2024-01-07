//------------------------------------------------------------------------------
//  vksubcontexthandler.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

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
    this->drawQueues.Resize(1);
    this->computeQueues.Resize(1);
    this->transferQueues.Resize(1);
    this->sparseQueues.Resize(1);

    this->queueFamilies[CoreGraphics::GraphicsQueueType] = families[CoreGraphics::GraphicsQueueType];
    this->queueFamilies[CoreGraphics::ComputeQueueType] = families[CoreGraphics::ComputeQueueType];
    this->queueFamilies[CoreGraphics::TransferQueueType] = families[CoreGraphics::TransferQueueType];
    this->queueFamilies[CoreGraphics::SparseQueueType] = families[CoreGraphics::SparseQueueType];

    Util::FixedArray<IndexT> queueUses(CoreGraphics::QueueType::NumQueueTypes, 0);

    SizeT i;
    for (i = 0; i < this->drawQueues.Size(); i++)
    {
        IndexT& queueIndex = queueUses[families[CoreGraphics::GraphicsQueueType]];
        vkGetDeviceQueue(dev, families[CoreGraphics::GraphicsQueueType], queueIndex++, &this->drawQueues[i]);
    }

    for (i = 0; i < this->computeQueues.Size(); i++)
    {
        IndexT& queueIndex = queueUses[families[CoreGraphics::ComputeQueueType]];
        vkGetDeviceQueue(dev, families[CoreGraphics::ComputeQueueType], queueIndex++, &this->computeQueues[i]);
    }

    for (i = 0; i < this->transferQueues.Size(); i++)
    {
        IndexT& queueIndex = queueUses[families[CoreGraphics::TransferQueueType]];
        vkGetDeviceQueue(dev, families[CoreGraphics::TransferQueueType], queueIndex++, &this->transferQueues[i]);
    }

    for (i = 0; i < this->sparseQueues.Size(); i++)
    {
        IndexT& queueIndex = queueUses[families[CoreGraphics::SparseQueueType]];
        vkGetDeviceQueue(dev, families[CoreGraphics::SparseQueueType], queueIndex++, &this->sparseQueues[i]);
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
    default: n_error("unhandled enum"); break;
    }

    // progress the queue index
    *currentQueue = (*currentQueue + 1) % list->Size();
}

//------------------------------------------------------------------------------
/**
*/
uint64 
VkSubContextHandler::AppendSubmissionTimeline(CoreGraphics::QueueType type, VkCommandBuffer cmds)
{
    n_assert(cmds != VK_NULL_HANDLE);
    Util::Array<TimelineSubmission>& submissions = this->timelineSubmissions[type];
    submissions.Append(TimelineSubmission());
    TimelineSubmission& sub = submissions.Back();

    uint64 ret = this->semaphoreSubmissionIds[type] + 1;
    
    // If command buffer is present, add it
    sub.buffers.Append(cmds);

    // Add signal
    sub.signalSemaphores.Append(this->semaphores[type]);
    sub.signalIndices.Append(ret);

    // Progress the semaphore counter
    this->semaphoreSubmissionIds[type] = ret;
    
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::AppendWaitTimeline(uint64 index, CoreGraphics::QueueType type, VkPipelineStageFlags waitFlags, CoreGraphics::QueueType waitType)
{
    n_assert(type != CoreGraphics::InvalidQueueType);
    n_assert(index != UINT64_MAX);
    TimelineSubmission& sub = this->timelineSubmissions[type].Back();
    sub.waitIndices.Append(index);
    sub.waitSemaphores.Append(this->semaphores[waitType]);
    sub.waitFlags.Append(waitFlags);
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
            (uint32_t)submission.imageMemoryBinds.Size(),
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
            (uint32_t)submission.opaqueMemoryBinds.Size(),
            submission.opaqueMemoryBinds.Size() > 0 ? submission.opaqueMemoryBinds.Begin() : nullptr
        };
        submission.imageOpaqueBindInfos.Append(opaqueMemoryBindInfo);
    }

    // add signal
    submission.signalSemaphores.Append(this->semaphores[type]);
    this->semaphoreSubmissionIds[type]++;
    submission.signalIndices.Append(this->semaphoreSubmissionIds[type]);

    // add wait
    if (this->semaphoreSubmissionIds[CoreGraphics::GraphicsQueueType] > 0)
    {
        submission.waitSemaphores.Append(this->semaphores[CoreGraphics::GraphicsQueueType]);
        submission.waitIndices.Append(this->semaphoreSubmissionIds[CoreGraphics::GraphicsQueueType]);
    }

    return this->semaphoreSubmissionIds[type];
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::AppendPresentSignal(CoreGraphics::QueueType type, VkSemaphore sem)
{
    TimelineSubmission& sub = this->timelineSubmissions[type].Back();

    sub.signalIndices.Append(0);
    sub.signalSemaphores.Append(sem);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkSubContextHandler::FlushSubmissionsTimeline(CoreGraphics::QueueType type, VkFence fence)
{
    Util::Array<TimelineSubmission>& submissions = this->timelineSubmissions[type];

    // skip flush if submission list is empty
    if (submissions.IsEmpty())
        return;

    Util::FixedArray<VkSubmitInfo> submitInfos(submissions.Size());
    Util::FixedArray<VkTimelineSemaphoreSubmitInfo> extensions(submissions.Size());
    for (IndexT i = 0; i < submissions.Size(); i++)
    {
        TimelineSubmission& sub = submissions[i];

        // if we have no work, return
        if (sub.buffers.Size() == 0)
            continue;

        VkTimelineSemaphoreSubmitInfo ext =
        {
            VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
            nullptr,
            (uint32_t)sub.waitIndices.Size(),
            sub.waitIndices.Size() > 0 ? sub.waitIndices.Begin() : nullptr,
            (uint32_t)sub.signalIndices.Size(),
            sub.signalIndices.Size() > 0 ? sub.signalIndices.Begin() : nullptr
        };
        extensions[i] = ext;

        VkSubmitInfo info =
        {
            VK_STRUCTURE_TYPE_SUBMIT_INFO,
            &extensions[i],
            (uint32_t)sub.waitSemaphores.Size(),
            sub.waitSemaphores.Size() > 0 ? sub.waitSemaphores.Begin() : nullptr,
            sub.waitFlags.Size() > 0 ? sub.waitFlags.Begin() : nullptr,
            (uint32_t)sub.buffers.Size(),
            sub.buffers.Size() > 0 ? sub.buffers.Begin() : nullptr,
            (uint32_t)sub.signalSemaphores.Size(),                              // if we have a finish semaphore, add it on the submit
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
}

//------------------------------------------------------------------------------
/**
*/
void 
VkSubContextHandler::Wait(CoreGraphics::QueueType type, uint64 index)
{
    // we can't really signal index UINT64_MAX, so skip it
    if (index != UINT64_MAX)
    {
        // setup wait
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
        n_assert(res == VK_SUCCESS);
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
VkSubContextHandler::Poll(CoreGraphics::QueueType type, uint64_t index)
{
    uint64_t lastPayload;
    VkResult res = vkGetSemaphoreCounterValue(this->device, this->semaphores[type], &lastPayload);
    n_assert(res == VK_SUCCESS);
    return index < lastPayload;
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
            (uint32_t)sub.waitIndices.Size(),
            sub.waitIndices.Begin(),
            (uint32_t)sub.signalIndices.Size(),
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
VkSubContextHandler::InsertFence(CoreGraphics::QueueType type, VkFence fence)
{
    VkQueue queue = this->GetQueue(type);
    VkResult res = vkQueueSubmit(queue, 0, nullptr, fence);
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
    default: n_error("unhandled enum"); break;
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
