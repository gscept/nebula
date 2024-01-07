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
        void Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex) override;

        std::function<void(const CoreGraphics::CmdBufferId cmdBuf, IndexT, IndexT)> func;
    };

    FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);
    std::function<void(const CoreGraphics::CmdBufferId cmdBuf, IndexT, IndexT)> func;

private:

        /// build operation
    void Build(
        Memory::ArenaAllocator<BIG_CHUNK>& allocator,
        Util::Array<FrameOp::Compiled*>& compiledOps,
        Util::Array<CoreGraphics::EventId>& events,
        Util::Array<CoreGraphics::BarrierId>& barriers,
        Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& rwBuffers,
        Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures
    );

};

} // namespace Frame2
