#pragma once
//------------------------------------------------------------------------------
/**
    Performs an image copy without any filtering or image conversion.
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
namespace Frame
{
class FrameSwap : public FrameOp
{
public:
    /// constructor
    FrameSwap();
    /// destructor
    virtual ~FrameSwap();

    struct CompiledImpl : public FrameSwap::Compiled
    {
        void Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex) override;

#if NEBULA_GRAPHICS_DEBUG
        Util::StringAtom name;
#endif
    };

    FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);
};

} // namespace Frame2
