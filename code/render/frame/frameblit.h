#pragma once
//------------------------------------------------------------------------------
/**
    A frame blit performs an image copy with optional filtering and image type conversions.
    
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
namespace Frame
{
class FrameBlit : public FrameOp
{
public:
    /// constructor
    FrameBlit();
    /// destructor
    virtual ~FrameBlit();

    struct CompiledImpl : public FrameOp::Compiled
    {
        void Run(const IndexT frameIndex, const IndexT bufferIndex) override;
        void Discard();

#if NEBULA_GRAPHICS_DEBUG
        Util::StringAtom name;
#endif

        CoreGraphics::TextureId from, to;
    };

    FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);

    CoreGraphics::TextureId from, to;
};

} // namespace Frame2
