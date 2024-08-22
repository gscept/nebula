//------------------------------------------------------------------------------
//  vksubcontexthandler.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "vksubcontexthandler.h"
#include "coregraphics/config.h"
#include "vkgraphicsdevice.h"
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
VkSubContextHandler::Setup(VkDevice dev, const Util::FixedArray<Util::Pair<uint, uint>> queueMap)
{
    // store device
    this->device = dev;

    this->queues.Resize(queueMap.Size());
    this->semaphores.Resize(queueMap.Size());
    this->semaphoreSubmissionIds.Resize(queueMap.Size());
    this->currentQueue.Resize(queueMap.Size());
    this->submissions.Resize(queueMap.Size());

    for (int i = 0; i < queueMap.Size(); i++)
    {
        auto& [family, count] = queueMap[i];
        this->queues[i].Resize(count);
        this->semaphores[i].Resize(count);
        this->semaphoreSubmissionIds[i].Resize(count);

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
#endif
        for (int j = 0; j < count; j++)
        {
            vkGetDeviceQueue(dev, family, j, &this->queues[i][j]);

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
            VkResult res = vkCreateSemaphore(this->device, &inf, nullptr, &this->semaphores[i][j]);
            n_assert(res == VK_SUCCESS);
            this->semaphoreSubmissionIds[i][j] = 0;
        
#if NEBULA_GRAPHICS_DEBUG
            VkDebugUtilsObjectNameInfoEXT info =
            {
                VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                nullptr,
                VK_OBJECT_TYPE_SEMAPHORE,
                (uint64_t)this->semaphores[i][j],
                name
            };
            VkResult res2 = VkDebugObjectName(this->device, &info);
            n_assert(res2 == VK_SUCCESS);
#endif
        }
        this->currentQueue[i] = 0;
    }
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
    Util::Array<VkQueue>* list = &this->queues[type];
    uint* currentQueue = &this->currentQueue[type];

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

    Util::Array<TimelineSubmission2>& submissionsForQueue = this->submissions[type];
    TimelineSubmission2& sub = submissionsForQueue.Emplace();
    sub.buffers.Append(cmds);
    sub.signalSemaphores.Append(this->semaphores[type][this->currentQueue[type]]);
    sub.signalIndices.Append(ret);
    sub.queue = type;
#if NEBULA_GRAPHICS_DEBUG
    sub.name = name;
#endif

    // Progress the semaphore counter
    this->semaphoreSubmissionIds[type][this->currentQueue[type]] = ret;
    
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
uint64
VkSubContextHandler::AppendSubmissionTimeline(CoreGraphics::QueueType type, Util::Array<VkCommandBuffer> cmds,
#if NEBULA_GRAPHICS_DEBUG
    const char* name
#endif
)
{
    n_assert(!cmds.IsEmpty());
    uint64 ret = GetNextTimelineIndex(type);

    Util::Array<TimelineSubmission2>& submissionsForQueue = this->submissions[type];
    TimelineSubmission2& sub = submissionsForQueue.Emplace();
    sub.buffers.AppendArray(cmds);
    sub.signalSemaphores.Append(this->semaphores[type][this->currentQueue[type]]);
    sub.signalIndices.Append(ret);
    sub.queue = type;
#if NEBULA_GRAPHICS_DEBUG
    sub.name = name;
#endif

    // Progress the semaphore counter
    this->semaphoreSubmissionIds[type][this->currentQueue[type]] = ret;
    
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
uint64
VkSubContextHandler::GetNextTimelineIndex(CoreGraphics::QueueType type)
{
    return this->semaphoreSubmissionIds[type][this->currentQueue[type]] + 1;
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::AppendWaitTimeline(uint64 index, CoreGraphics::QueueType type, VkPipelineStageFlags waitFlags, CoreGraphics::QueueType waitType)
{
    n_assert(type != CoreGraphics::InvalidQueueType);
    n_assert(index != UINT64_MAX);

    Util::Array<TimelineSubmission2>& submissionsForQueue = this->submissions[type];
    TimelineSubmission2& sub = submissionsForQueue.Back();
    sub.waitIndices.Append(index);
    sub.waitSemaphores.Append(this->semaphores[waitType][this->currentQueue[waitType]]);
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
    submission.signalSemaphores.Append(this->GetSemaphore(type));
    this->IncrementSemaphoreId(type);
    submission.signalIndices.Append(this->GetSemaphoreId(type));

    // add wait
    if (this->semaphoreSubmissionIds[CoreGraphics::GraphicsQueueType][this->currentQueue[CoreGraphics::GraphicsQueueType]] > 0)
    {
        submission.waitSemaphores.Append(this->GetSemaphore(CoreGraphics::GraphicsQueueType));
        submission.waitIndices.Append(this->GetSemaphoreId(CoreGraphics::GraphicsQueueType));
    }

    return this->GetSemaphoreId(type);
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
    submission.signalSemaphores.Append(this->GetSemaphore(type));
    this->IncrementSemaphoreId(type);
    submission.signalIndices.Append(this->GetSemaphoreId(type));

    return this->GetSemaphoreId(type);
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::AppendPresentSignal(CoreGraphics::QueueType type, VkSemaphore sem)
{
    Util::Array<TimelineSubmission2>& submissionsForQueue = this->submissions[type];
    TimelineSubmission2& sub = submissionsForQueue.Back();
    sub.signalIndices.Append(0);
    sub.signalSemaphores.Append(sem);
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::FlushSubmissions(VkFence fence)
{
    CoreGraphics::QueueType submissionOrder[] =
    {
        CoreGraphics::TransferQueueType,
        CoreGraphics::SparseQueueType,
        CoreGraphics::GraphicsQueueType,
        CoreGraphics::ComputeQueueType
    };
    for (auto queueIndex : submissionOrder)
    {
        Util::Array<TimelineSubmission2>& submissionsForQueue = this->submissions[queueIndex];
        if (submissionsForQueue.IsEmpty())
            continue;
        Util::FixedArray<VkSubmitInfo> submitInfos(submissionsForQueue.Size());
        Util::FixedArray<VkTimelineSemaphoreSubmitInfo> timelineInfos(submissionsForQueue.Size());

        for (int i = 0; i < submissionsForQueue.Size(); i++)
        {
            auto& sub = submissionsForQueue[i];
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

        CoreGraphics::QueueType type = CoreGraphics::QueueType(queueIndex);
        VkQueue queue = this->GetQueue(type);
        switch (type)
        {
            case CoreGraphics::ComputeQueueType:
                CoreGraphics::QueueBeginMarker(type, NEBULA_MARKER_COMPUTE, "Compute");
                break;
            case CoreGraphics::GraphicsQueueType:
                CoreGraphics::QueueBeginMarker(type, NEBULA_MARKER_GRAPHICS, "Graphics");
                break;
            case CoreGraphics::TransferQueueType:
                CoreGraphics::QueueBeginMarker(type, NEBULA_MARKER_TRANSFER, "Transfer");
                break;
            case CoreGraphics::SparseQueueType:
                CoreGraphics::QueueBeginMarker(type, NEBULA_MARKER_TRANSFER, "Sparse");
                break;
        }
        VkResult res = vkQueueSubmit(queue, submitInfos.Size(), submitInfos.Begin(), fence);
        n_assert(res == VK_SUCCESS);

        CoreGraphics::QueueEndMarker(type);
        submissionsForQueue.Clear();
    }
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
        VkSemaphore sem = this->GetSemaphore(type);
        VkSemaphoreWaitInfo waitInfo =
        {
            VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            nullptr,
            0,
            1,
            &sem,
            &index
        };
        VkResult res = vkWaitSemaphores(this->device, &waitInfo, UINT64_MAX);
        if (res == VK_ERROR_DEVICE_LOST)
            Vulkan::DeviceLost();
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
    VkResult res = vkGetSemaphoreCounterValue(this->device, this->GetSemaphore(type), &lastPayload);
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
    Util::Array<VkQueue>* list = &this->queues[type];
    for (IndexT i = 0; i < list->Size(); i++)
    {
        VkResult res = vkQueueWaitIdle((*list)[i]);
        if (res == VK_ERROR_DEVICE_LOST)
            Vulkan::DeviceLost();
        n_assert(res == VK_SUCCESS);
    }
}

//------------------------------------------------------------------------------
/**
*/
VkQueue
VkSubContextHandler::GetQueue(const CoreGraphics::QueueType type)
{
    return this->queues[type][this->currentQueue[type]];
}

//------------------------------------------------------------------------------
/**
*/
VkSemaphore
VkSubContextHandler::GetSemaphore(const CoreGraphics::QueueType type)
{
    return this->semaphores[type][this->currentQueue[type]];
}

//------------------------------------------------------------------------------
/**
*/
uint64
VkSubContextHandler::GetSemaphoreId(const CoreGraphics::QueueType type)
{
    return this->semaphoreSubmissionIds[type][this->currentQueue[type]];
}

//------------------------------------------------------------------------------
/**
*/
void
VkSubContextHandler::IncrementSemaphoreId(const CoreGraphics::QueueType type)
{
    this->semaphoreSubmissionIds[type][this->currentQueue[type]]++;
}

} // namespace Vulkan
