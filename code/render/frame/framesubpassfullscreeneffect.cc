//------------------------------------------------------------------------------
// framefullscreeneffect.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "framesubpassfullscreeneffect.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameSubpassFullscreenEffect::FrameSubpassFullscreenEffect()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameSubpassFullscreenEffect::~FrameSubpassFullscreenEffect()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassFullscreenEffect::Setup()
{
    n_assert(this->tex != InvalidTextureId);
    this->program = ShaderGetProgram(this->shader, ShaderFeatureMask(SHADER_POSTEFFECT_DEFAULT_FEATURE_MASK));
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassFullscreenEffect::Discard()
{
    FrameOp::Discard();

    this->tex = InvalidTextureId;
    DestroyResourceTable(this->resourceTable);
    IndexT i;
    for (i = 0; i < this->constantBuffers.Size(); i++)
        DestroyBuffer(this->constantBuffers.ValueAtIndex(i));
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassFullscreenEffect::OnWindowResized()
{
    FrameOp::OnWindowResized();
    for (int i = 0; i < this->textures.Size(); i++)
    {
        const Util::Tuple<IndexT, CoreGraphics::BufferId, CoreGraphics::TextureId>& texture = this->textures[i];
        if (Util::Get<1>(texture) == CoreGraphics::InvalidBufferId)
        {
            ResourceTableSetTexture(this->resourceTable, { Util::Get<2>(texture), Util::Get<0>(texture), 0, CoreGraphics::InvalidSamplerId, false });
        }
    }
    ResourceTableCommitChanges(this->resourceTable);
    /*
    TextureWindowResized(this->tex);

    IndexT i;
    for (i = 0; i < this->textures.Size(); i++)
    {
        const Util::Tuple<IndexT, CoreGraphics::BufferId, CoreGraphics::TextureId>& tuple = this->textures[i];
        if (Util::Get<1>(tuple) != CoreGraphics::InvalidBufferId)
        {
            CoreGraphics::BufferUpdate(Util::Get<1>(tuple), CoreGraphics::TextureGetBindlessHandle(Util::Get<2>(tuple)), Util::Get<0>(tuple));
        }
        else
        {
            ResourceTableSetTexture(this->resourceTable, { Util::Get<2>(tuple), Util::Get<0>(tuple), 0, CoreGraphics::InvalidSamplerId, false });
        }
    }
    */
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameSubpassFullscreenEffect::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
    CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
    ret->program = this->program;
    ret->resourceTable = this->resourceTable;

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassFullscreenEffect::CompiledImpl::Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex)
{
    // activate shader
    CoreGraphics::CmdSetShaderProgram(cmdBuf, this->program);

    // draw
    RenderUtil::DrawFullScreenQuad::ApplyMesh(cmdBuf);
    CoreGraphics::CmdSetGraphicsPipeline(cmdBuf);
    CoreGraphics::CmdSetResourceTable(cmdBuf, this->resourceTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
    CoreGraphics::CmdDraw(cmdBuf, RenderUtil::DrawFullScreenQuad::GetPrimitiveGroup());
}

} // namespace Frame
