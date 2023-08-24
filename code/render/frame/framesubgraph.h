#pragma once
//------------------------------------------------------------------------------
/**
    Runs a programmatically constructed subgraph

    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "util/stringatom.h"
namespace Frame
{
class FrameSubgraph : public FrameOp
{
public:
    /// constructor
    FrameSubgraph();
    /// destructor
    virtual ~FrameSubgraph();

    /// Handle display resizing
    void OnWindowResized() override;

    struct CompiledImpl : public FrameOp::Compiled
    {
        void Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex) override;

#if NEBULA_GRAPHICS_DEBUG
        Util::StringAtom name;
#endif
        Util::Array<FrameOp::Compiled*> subgraphOps;
    };

    FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);

private:

    void Build(const BuildContext& ctx) override;
};

/// Add a subgraph by name for the framescript
extern void AddSubgraph(const Util::StringAtom name, const Util::Array<FrameOp*> ops);
/// Get subgraph by name
const Util::Array<FrameOp*> GetSubgraph(const Util::StringAtom name);

extern Util::Dictionary<Util::StringAtom, Util::Array<FrameOp*>> nameToSubgraph;

} // namespace Frame2
