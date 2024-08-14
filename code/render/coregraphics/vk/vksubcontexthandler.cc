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

#if NEBULA_GRAPHICS_DEBUG
        const char* name = nullptr;
        switch (i)
        {
            case CoreGraphics::ComputeQueueType:
                name = "Compute Semaphore";
                break;
            case CoreGraphics::GraphicsQueueType:
                name = "Graphics Semaphore";
                break;
            case CoreGraphics::TransferQueueType:
                name = "Transfer Semaphore";
                break;
            case CoreGraphics::SparseQueueType:
                name = "Sparse Semaphore";
                break;
        }
        VkDebugUtilsObjectNameInfoEXT info =
        {
            VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            nullptr,
            VK_OBJECT_TYPE_SEMAPHORE,
            (uint64_t)semaphores[i],
            name
        };
        VkResult res2 = VkDebugObjectName(this->device, &info);
        n_assert(res2 == VK_SUCCESS);
#endif

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
VkSubContextHandler::AppendSubmissionTimeline(
    CoreGraphics::QueueType type
    , VkCommandBuffer cmds
#if NEBULA_GRAPHICS_DEBUG
    , const char* name
#endif
)
{
    n_assert(cmds != VK_NULL_HANDLE);

    uint64 ret = GetNextTimelineIndex(type);

    SubmissionList* lastList = nullptr;
    if (!orderedSubmissions.IsEmpty())
    {
        lastList = &this->orderedSubmissions.Back();
    }
    
    if (lastList != nullptr && lastList->queue == type)
    {
        TimelineSubmission2& sub = lastList->submissions.Emplace();
        sub.buffers.Append(cmds);
        sub.signalSemaphores.Append(this->semaphores[type]);
        sub.signalIndices.Append(ret);
    }
    else
    {
        SubmissionList& list = this->orderedSubmissions.Emplace();
        list.queue = type;

        // If command buffer is present, add it
        TimelineSubmission2& sub2 = list.submissions.Emplace();
        sub2.buffers.Append(cmds);
        sub2.signalSemaphores.Append(this->semaphores[type]);
        sub2.signalIndices.Append(ret);
        sub2.queue = type;
#if NEBULA_GRAPHICS_DEBUG
        sub2.name = name;
#endif
    }

    // Progress the semaphore counter
    this->semaphoreSubmissionIds[type] = ret;
    
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
uint64
VkSubContextHandler::GetNextTimelineIndex(CoreGraphics::QueueType type)
{
    return this->semaphoreSubmissionIds[type] + 1;
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::AppendWaitTimeline(uint64 index, CoreGraphics::QueueType type, VkPipelineStageFlags waitFlags, CoreGraphics::QueueType waitType)
{
    n_assert(type != CoreGraphics::InvalidQueueType);
    n_assert(index != UINT64_MAX);

    auto it = this->orderedSubmissions.End() - 1;
    for (; it != this->orderedSubmissions.Begin(); it--)
    {
        if (it->queue == type)
        {
            auto& sub = it->submissions.Back();
            sub.waitIndices.Append(index);
            sub.waitSemaphores.Append(this->semaphores[waitType]);
            sub.waitFlags.Append(waitFlags);
            break;
        }
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
uint64
VkSubContextHandler::AppendSparseBind(CoreGraphics::QueueType type, const VkBuffer buf, const Util::Array<VkSparseMemoryBind>& binds)
{
    this->sparseBindSubmissions.Append(SparseBindSubmission());
    SparseBindSubmission& submission = this->sparseBindSubmissions.Back();

    // setup bind structs, add support for ordinary buffer
    if (!binds.IsEmpty())
    {
        submission.bufferMemoryBinds.Resize(binds.Size());
        memcpy(submission.bufferMemoryBinds.Begin(), binds.Begin(), binds.Size() * sizeof(VkSparseMemoryBind));
        VkSparseBufferMemoryBindInfo bufferMemoryBindInfo =
        {
            buf,
            (uint32_t)submission.bufferMemoryBinds.Size(),
            submission.bufferMemoryBinds.Size() > 0 ? submission.bufferMemoryBinds.Begin() : nullptr
        };
        submission.bufferMemoryBindInfos.Append(bufferMemoryBindInfo);
    }

    // add signal
    submission.signalSemaphores.Append(this->semaphores[type]);
    this->semaphoreSubmissionIds[type]++;
    submission.signalIndices.Append(this->semaphoreSubmissionIds[type]);

    return this->semaphoreSubmissionIds[type];
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::AppendPresentSignal(CoreGraphics::QueueType type, VkSemaphore sem)
{
    auto it = this->orderedSubmissions.End() - 1;
    for (; it != this->orderedSubmissions.Begin(); it--)
    {
        if (it->queue == type)
        {
            auto& sub = it->submissions.Back();
            sub.signalIndices.Append(0);
            sub.signalSemaphores.Append(sem);
            break;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::FlushSubmissions(VkFence fence)
{
    for (auto& list : this->orderedSubmissions)
    {
        if (list.submissions.Size() == 0)
            continue;

        Util::FixedArray<VkSubmitInfo> submitInfos(list.submissions.Size());
        Util::FixedArray<VkTimelineSemaphoreSubmitInfo> timelineInfos(list.submissions.Size());

        for (int i = 0; i < list.submissions.Size(); i++)
        {
            auto& sub = list.submissions[i];
            timelineInfos[i] =
            {
                .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                .waitSemaphoreValueCount = (uint32_t)sub.waitIndices.Size(),
                .pWaitSemaphoreValues = sub.waitIndices.Size() > 0 ? sub.waitIndices.Begin() : nullptr,
                .signalSemaphoreValueCount = (uint32_t)sub.signalIndices.Size(),
                .pSignalSemaphoreValues = sub.signalIndices.Size() > 0 ? sub.signalIndices.Begin() : nullptr
            };

            submitInfos[i] =
            {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .pNext = &timelineInfos[i],
                .waitSemaphoreCount = (uint32_t)sub.waitSemaphores.Size(),
                .pWaitSemaphores = sub.waitSemaphores.Size() > 0 ? sub.waitSemaphores.Begin() : nullptr,
                .pWaitDstStageMask = sub.waitFlags.Size() > 0 ? sub.waitFlags.Begin() : nullptr,
                .commandBufferCount = (uint32_t)sub.buffers.Size(),
                .pCommandBuffers = sub.buffers.Size() > 0 ? sub.buffers.Begin() : nullptr,
                .signalSemaphoreCount = (uint32_t)sub.signalSemaphores.Size(),                              // if we have a finish semaphore, add it on the submit
                .pSignalSemaphores = sub.signalSemaphores.Size() > 0 ? sub.signalSemaphores.Begin() : nullptr
            };
        }

        VkQueue queue = this->GetQueue(list.queue);
        switch (list.queue)
        {
            case CoreGraphics::ComputeQueueType:
                CoreGraphics::QueueBeginMarker(list.queue, NEBULA_MARKER_COMPUTE, "Compute");
                break;
            case CoreGraphics::GraphicsQueueType:
                CoreGraphics::QueueBeginMarker(list.queue, NEBULA_MARKER_GRAPHICS, "Graphics");
                break;
            case CoreGraphics::TransferQueueType:
                CoreGraphics::QueueBeginMarker(list.queue, NEBULA_MARKER_TRANSFER, "Transfer");
                break;
            case CoreGraphics::SparseQueueType:
                CoreGraphics::QueueBeginMarker(list.queue, NEBULA_MARKER_TRANSFER, "Sparse");
                break;
        }
        VkResult res = vkQueueSubmit(queue, submitInfos.Size(), submitInfos.Begin(), fence);
        n_assert(res == VK_SUCCESS);

        CoreGraphics::QueueEndMarker(list.queue);
    }
    this->orderedSubmissions.Clear();
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
    return index <= lastPayload;
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
            .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreValueCount = (uint32_t)sub.waitIndices.Size(),
            .pWaitSemaphoreValues = sub.waitIndices.Begin(),
            .signalSemaphoreValueCount = (uint32_t)sub.signalIndices.Size(),
            .pSignalSemaphoreValues = sub.signalIndices.Begin()
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
