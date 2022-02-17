#pragma once
//------------------------------------------------------------------------------
/**
    Runs code from frame operation created in code

    @copyright
    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "util/stringatom.h"
namespace Frame
{
class FrameCode : public FrameOp
{
public:
    /// constructor
    FrameCode();
    /// destructor
    virtual ~FrameCode();

    using FrameCodeFunc = void(*)(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex);
    struct CompiledImpl : public FrameOp::Compiled
    {
        void Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex) override;

#if NEBULA_GRAPHICS_DEBUG
        Util::StringAtom name;
#endif
        FrameCodeFunc func;
    };

    FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);

    FrameCodeFunc func;
private:

    void Build(
        Memory::ArenaAllocator<BIG_CHUNK>& allocator,
        Util::Array<FrameOp::Compiled*>& compiledOps,
        Util::Array<CoreGraphics::EventId>& events,
        Util::Array<CoreGraphics::BarrierId>& barriers,
        Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& rwBuffers,
        Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures) override;
};

} // namespace Frame2
