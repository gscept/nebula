#pragma once
//------------------------------------------------------------------------------
/**
    Handles frame submissions start and end

    @copyright
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/graphicsdevice.h"

namespace Frame
{

class FrameSubmission : public FrameOp
{
public:
    /// constructor
    FrameSubmission();
    /// destructor
    ~FrameSubmission();

    /// Handle display resizing
    void OnWindowResized();

    struct CompiledImpl : public FrameOp::Compiled
    {
        void Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex) override;
        void Discard() override;

        Util::Array<FrameSubmission::CompiledImpl*> waitSubmissions;
        Util::Array<CoreGraphics::QueueType> waitQueues;
        CoreGraphics::CmdBufferPoolId commandBufferPool;
        CoreGraphics::QueueType queue;
        Util::Array<CoreGraphics::BarrierId>* resourceResetBarriers;
        Util::Array<FrameOp::Compiled*> compiled;
        CoreGraphics::SubmissionWaitEvent submissionId;
#if NEBULA_GRAPHICS_DEBUG
        Util::StringAtom name;
#endif
    };

    /// allocate new instance
    FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);

        /// build operation
    virtual void Build(const BuildContext& ctx);

    Util::Array<FrameSubmission*> waitSubmissions;
    Util::Array<CoreGraphics::QueueType> waitQueues;
    CoreGraphics::CmdBufferPoolId commandBufferPool;
    CoreGraphics::QueueType queue;
    Util::Array<CoreGraphics::BarrierId>* resourceResetBarriers;
};

} // namespace Frame
