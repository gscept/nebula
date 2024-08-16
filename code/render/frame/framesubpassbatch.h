#pragma once
//------------------------------------------------------------------------------
/**
    A subpass batch performs batch rendering of geometry.
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "materials/materialtemplates.h"
#include "graphics/graphicsentity.h"
namespace Frame
{
class FrameSubpassBatch : public FrameOp
{
public:
    /// constructor
    FrameSubpassBatch();
    /// destructor
    virtual ~FrameSubpassBatch();

    struct CompiledImpl : public FrameOp::Compiled
    {
        void Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex) override;

        MaterialTemplates::BatchGroup batch;
    };

    FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);
    MaterialTemplates::BatchGroup batch;

    /// Do the actual drawing
    static void DrawBatch(const CoreGraphics::CmdBufferId cmdBuf, MaterialTemplates::BatchGroup batch, const Graphics::GraphicsEntityId id, const IndexT bufferIndex);
    /// Do the actual drawing, but with duplicate instances
    static void DrawBatch(const CoreGraphics::CmdBufferId cmdBuf, MaterialTemplates::BatchGroup batch, const Graphics::GraphicsEntityId id, const SizeT numInstances, const IndexT baseInstance, const IndexT bufferIndex);
};

} // namespace Frame2
