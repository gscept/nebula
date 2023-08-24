#pragma once
//------------------------------------------------------------------------------
/**
    A frame pass prepares a rendering sequence, draws and subpasses must reside
    within one of these objects.
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "framesubpass.h"
#include "coregraphics/pass.h"
namespace Threading
{
class Event;
}

namespace Frame
{
class FramePass : public FrameOp
{
public:
    /// constructor
    FramePass();
    /// destructor
    virtual ~FramePass();

    /// discard operation
    void Discard();
    /// handle display resizing
    void OnWindowResized() override;

    struct CompiledImpl : public FrameOp::Compiled
    {
        void Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex) override;
        void Discard();

#if NEBULA_GRAPHICS_DEBUG
        Util::StringAtom name;
#endif
        Util::Array<FrameOp::Compiled*> subpasses;
        CoreGraphics::PassId pass;
    };

    /// allocate new instance
    FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);

    CoreGraphics::PassId pass;

private:
    friend class FrameScript;

    void Build(const BuildContext& ctx) override;
};

} // namespace Frame2
