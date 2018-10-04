//------------------------------------------------------------------------------
// framefullscreeneffect.cc
// (C) 2016 Individual contributors, see AUTHORS file
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
	n_assert(this->tex != RenderTextureId::Invalid());
	TextureDimensions dims = RenderTextureGetDimensions(this->tex);
	this->fsq.Setup(dims.width, dims.height);

	this->program = ShaderGetProgram(this->shader, ShaderFeatureFromString(SHADER_POSTEFFECT_DEFAULT_FEATURE_MASK));
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassFullscreenEffect::Discard()
{
	FrameOp::Discard();

	this->fsq.Discard();
	this->tex = RenderTextureId::Invalid();
	DestroyResourceTable(this->resourceTable);
	IndexT i;
	for (i = 0; i < this->constantBuffers.Size(); i++)
		DestroyConstantBuffer(this->constantBuffers.ValueAtIndex(i));
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameSubpassFullscreenEffect::AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->fsq = this->fsq;
	ret->program = this->program;
	ret->resourceTable = this->resourceTable;

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassFullscreenEffect::CompiledImpl::Run(const IndexT frameIndex)
{
	// activate shader
	CoreGraphics::SetShaderProgram(this->program);

	// draw
	CoreGraphics::BeginBatch(FrameBatchType::System);
	this->fsq.ApplyMesh();
	CoreGraphics::SetResourceTable(this->resourceTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
	this->fsq.Draw();
	CoreGraphics::EndBatch();
}

} // namespace Frame2