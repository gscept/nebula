#pragma once
//------------------------------------------------------------------------------
/**
    Handles frame submissions start and end

    @copyright
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
namespace Frame
{

class FrameSubmission : public FrameOp
{
public:
    /// constructor
    FrameSubmission();
    /// destructor
    ~FrameSubmission();

    struct CompiledImpl : public FrameOp::Compiled
    {
        void RunJobs(const IndexT frameIndex, const IndexT bufferIndex) override;
        void Run(const IndexT frameIndex, const IndexT bufferIndex) override;

        CoreGraphics::QueueType queue;
        CoreGraphics::QueueType waitQueue;
        Util::Array<CoreGraphics::BarrierId>* resourceResetBarriers;
        Util::Array<FrameOp::Compiled*> compiled;
#if NEBULA_GRAPHICS_DEBUG
        Util::StringAtom name;
#endif
    };

    /// allocate new instance
    FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);

        /// build operation
    virtual void Build(
        Memory::ArenaAllocator<BIG_CHUNK>& allocator,
        Util::Array<FrameOp::Compiled*>& compiledOps,
        Util::Array<CoreGraphics::EventId>& events,
        Util::Array<CoreGraphics::BarrierId>& barriers,
        Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& rwBuffers,
        Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures,
        CoreGraphics::CommandBufferPoolId commandBufferPool);

    CoreGraphics::QueueType queue;
    CoreGraphics::QueueType waitQueue;
    Util::Array<CoreGraphics::BarrierId>* resourceResetBarriers;
};

} // namespace Frame
