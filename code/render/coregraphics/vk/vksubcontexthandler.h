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

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
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

    struct TimelineSubmission2
    {
#if NEBULA_GRAPHICS_DEBUG
        const char* name = nullptr;
#endif
        CoreGraphics::QueueType queue;
        Util::Array<uint64, 16> signalIndices;
        Util::Array<VkSemaphore, 16> signalSemaphores;
        Util::Array<VkCommandBuffer, 16> buffers;
        Util::Array<VkPipelineStageFlags, 16> waitFlags;
        Util::Array<VkSemaphore, 16> waitSemaphores;
        Util::Array<uint64, 16> waitIndices;
    };

    struct SubmissionList
    {
        CoreGraphics::QueueType queue;
        Util::Array<TimelineSubmission2> submissions;
    };

    struct SparseBindSubmission
    {
        VkBindSparseInfo bindInfo;
        Util::Array<uint64, 16> signalIndices;
        Util::Array<VkSemaphore, 16> signalSemaphores;
        Util::FixedArray<VkSparseMemoryBind> opaqueMemoryBinds;
        Util::FixedArray<VkSparseImageMemoryBind> imageMemoryBinds;
        Util::FixedArray<VkSparseMemoryBind> bufferMemoryBinds;
        Util::Array<VkSparseBufferMemoryBindInfo, 16> bufferMemoryBindInfos;
        Util::Array<VkSparseImageOpaqueMemoryBindInfo, 16> imageOpaqueBindInfos;
        Util::Array<VkSparseImageMemoryBindInfo, 16> imageMemoryBindInfos;
        Util::Array<VkSemaphore, 16> waitSemaphores;
        Util::Array<uint64, 16> waitIndices;
    };

    /// setup subcontext handler
    void Setup(VkDevice dev, const Util::FixedArray<Util::Pair<uint, uint>> indexMap);
    /// discard
    void Discard();
    /// set to next context of type
    void SetToNextContext(const CoreGraphics::QueueType type);

    /// append submission to context to execute later, supports waiting for a queue
    uint64 AppendSubmissionTimeline(
        CoreGraphics::QueueType type
        , VkCommandBuffer cmds
        , Util::Array<CoreGraphics::SubmissionWaitEvent, 8> waitEvents
#if NEBULA_GRAPHICS_DEBUG
        , const char* name = nullptr
#endif
    );

    /// append submissions to context to execute later, supports waiting for a queue
    uint64 AppendSubmissionTimeline(
        CoreGraphics::QueueType type
        , Util::Array<VkCommandBuffer, 16> cmds
        , Util::Array<CoreGraphics::SubmissionWaitEvent, 8> waitEvents
#if NEBULA_GRAPHICS_DEBUG
        , const char* name = nullptr
#endif
    );
    /// Gets the next submission id for a specific queue
    uint64 GetNextTimelineIndex(CoreGraphics::QueueType type);
    /// append a sparse image bind timeline operation
    uint64 AppendSparseBind(CoreGraphics::QueueType type, const VkImage img, const Util::Array<VkSparseMemoryBind>& opaqueBinds, const Util::Array<VkSparseImageMemoryBind>& pageBinds);
    /// append a sparse buffer bind timeline operation
    uint64 AppendSparseBind(CoreGraphics::QueueType type, const VkBuffer buf, const Util::Array<VkSparseMemoryBind>& binds);
    /// Append present signal
    void AppendPresentSignal(CoreGraphics::QueueType type, VkSemaphore sem);
    /// Flush submissions
    void FlushSubmissions(VkFence fence);
    /// wait for timeline index
    void Wait(CoreGraphics::QueueType type, uint64 index);
    /// check to see if timeline index has passed
    bool Poll(CoreGraphics::QueueType type, uint64_t index);

    /// flush sparse binds
    void FlushSparseBinds(VkFence fence);
    /// insert fence
    void InsertFence(CoreGraphics::QueueType type, VkFence fence);

    /// wait for a queue to finish working
    void WaitIdle(const CoreGraphics::QueueType type);

    /// get current queue
    VkQueue GetQueue(const CoreGraphics::QueueType type);

private:
    friend const VkQueue GetQueue(const CoreGraphics::QueueType type, const IndexT index);

    VkSemaphore GetSemaphore(const CoreGraphics::QueueType type);
    uint64 GetSemaphoreId(const CoreGraphics::QueueType type);
    void IncrementSemaphoreId(const CoreGraphics::QueueType type);

    VkDevice device;
    Util::FixedArray<Util::Array<VkQueue>> queues;
    Util::FixedArray<uint> currentQueue;
    Util::FixedArray<Util::Array<VkSemaphore>> semaphores;
    Util::FixedArray<Util::Array<uint64>> semaphoreSubmissionIds;
    Util::FixedArray<Util::Array<TimelineSubmission2, 16>> submissions;
    Util::Array<SparseBindSubmission> sparseBindSubmissions;
    Threading::CriticalSection submissionLock;
};

} // namespace Vulkan
