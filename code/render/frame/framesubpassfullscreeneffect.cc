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
	n_assert(this->tex != TextureId::Invalid());
	TextureDimensions dims = TextureGetDimensions(this->tex);
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
	this->tex = TextureId::Invalid();
	DestroyResourceTable(this->resourceTable);
	IndexT i;
	for (i = 0; i < this->constantBuffers.Size(); i++)
		DestroyConstantBuffer(this->constantBuffers.ValueAtIndex(i));
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
		const std::tuple<IndexT, CoreGraphics::ConstantBufferId, CoreGraphics::TextureId>& tuple = this->textures[i];
		if (std::get<1>(tuple) != CoreGraphics::ConstantBufferId::Invalid())
		{
			CoreGraphics::ConstantBufferUpdate(std::get<1>(tuple), CoreGraphics::TextureGetBindlessHandle(std::get<2>(tuple)), std::get<0>(tuple));
		}
		else
		{
			ResourceTableSetTexture(this->resourceTable, { std::get<2>(tuple), std::get<0>(tuple), 0, CoreGraphics::SamplerId::Invalid(), false });
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