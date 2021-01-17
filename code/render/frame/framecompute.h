#pragma once
//------------------------------------------------------------------------------
/**
    Executes compute shader.
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/shader.h"
#include "coregraphics/resourcetable.h"
namespace Frame
{
class FrameCompute : public FrameOp
{
public:
    /// constructor
    FrameCompute();
    /// destructor
    virtual ~FrameCompute();

    /// discard
    void Discard();

    struct CompiledImpl : public FrameOp::Compiled
    {
        void Run(const IndexT frameIndex, const IndexT bufferIndex) override;

        CoreGraphics::ShaderProgramId program;
        CoreGraphics::ResourceTableId resourceTable;
        SizeT x, y, z;
    };

    FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);

    Util::Dictionary<Util::StringAtom, CoreGraphics::BufferId> constantBuffers;
    Util::Array<Util::Tuple<IndexT, CoreGraphics::BufferId, CoreGraphics::TextureId>> textures;
    CoreGraphics::ResourceTableId resourceTable;
    CoreGraphics::ShaderId shader;
    CoreGraphics::ShaderProgramId program;
    SizeT x, y, z;
};

} // namespace Frame2
