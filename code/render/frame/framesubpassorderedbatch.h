#pragma once
//------------------------------------------------------------------------------
/**
    A subpass sorted batch renders the same geometry as the ordinary batch, however
    it prioritizes Z-order instead shader, making it potentially detrimental for performance.
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
namespace Frame
{
class FrameSubpassOrderedBatch : public FrameOp
{
public:
    /// constructor
    FrameSubpassOrderedBatch();
    /// destructor
    virtual ~FrameSubpassOrderedBatch();

    struct CompiledImpl : public FrameOp::Compiled
    {
        void Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex) override;

        MaterialTemplates::BatchGroup batch;
    };

    FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);

    MaterialTemplates::BatchGroup batch;
};

} // namespace Frame2
