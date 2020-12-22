#pragma once
//------------------------------------------------------------------------------
/**
    Implements a global state which is initialized at the beginning of the frame.
    
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/shader.h"
namespace Frame
{
class FrameGlobalState : public FrameOp
{
public:
    /// constructor
    FrameGlobalState();
    /// destructor
    virtual ~FrameGlobalState();

    struct CompiledImpl : public FrameOp::Compiled
    {
        void Run(const IndexT frameIndex, const IndexT bufferIndex) override;

    };

    FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);

};

} // namespace Frame2
