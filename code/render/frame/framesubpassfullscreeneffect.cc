//------------------------------------------------------------------------------
// framefullscreeneffect.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framesubpassfullscreeneffect.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/shaderserver.h"

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
    TextureDimensions dims = TextureGetDimensions(this->tex);

    this->program = ShaderGetProgram(this->shader, ShaderFeatureFromString(SHADER_POSTEFFECT_DEFAULT_FEATURE_MASK));
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
FrameSubpassFullscreenEffect::CompiledImpl::Run(const IndexT frameIndex, const IndexT bufferIndex)
{
    // activate shader
    CoreGraphics::SetShaderProgram(this->program);

    // draw
    CoreGraphics::BeginBatch(FrameBatchType::System);
    RenderUtil::DrawFullScreenQuad::ApplyMesh();
    CoreGraphics::SetResourceTable(this->resourceTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
    CoreGraphics::Draw();
    CoreGraphics::EndBatch();
}

} // namespace Frame2
