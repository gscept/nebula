#pragma once
//------------------------------------------------------------------------------
/**
    Performs an image copy without any filtering or image conversion.
    
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
namespace Frame
{
class FrameCopy : public FrameOp
{
public:
    /// constructor
    FrameCopy();
    /// destructor
    virtual ~FrameCopy();

    struct CompiledImpl : public FrameOp::Compiled
    {
        void Run(const IndexT frameIndex, const IndexT bufferIndex) override;

#if NEBULA_GRAPHICS_DEBUG
        Util::StringAtom name;
#endif

        CoreGraphics::TextureId from, to;
    };

    FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);

    CoreGraphics::TextureId from, to;
};

} // namespace Frame2
