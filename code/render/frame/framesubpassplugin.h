#pragma once
//------------------------------------------------------------------------------
/**
    Executes RT plugins within a subpass
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"

namespace Frame
{
class FrameSubpassPlugin : public FrameOp
{
public:
    /// constructor
    FrameSubpassPlugin();
    /// destructor
    virtual ~FrameSubpassPlugin();

    /// setup plugin pass
    void Setup();
    /// discard operation
    void Discard();

    struct CompiledImpl : public FrameOp::Compiled
    {
        void Run(const IndexT frameIndex, const IndexT bufferIndex) override;

        std::function<void(IndexT, IndexT)> func;
    };

    FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);
    std::function<void(IndexT, IndexT)> func;

private:

};

} // namespace Frame2
