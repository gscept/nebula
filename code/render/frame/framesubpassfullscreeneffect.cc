//------------------------------------------------------------------------------
// framefullscreeneffect.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framesubpassfullscreeneffect.h"
#include "coregraphics/renderdevice.h"
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

	this->shader = ShaderGet(this->shaderState);
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
	this->shaderState = ShaderStateId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassFullscreenEffect::Run(const IndexT frameIndex)
{
	RenderDevice* renderDevice = RenderDevice::Instance();
	ShaderServer* shaderServer = ShaderServer::Instance();

	// activate shader
	ShaderProgramBind(this->program);

	// draw
	renderDevice->BeginBatch(FrameBatchType::System);
	this->fsq.ApplyMesh();
	ShaderStateApply(this->shaderState);
	this->fsq.Draw();
	renderDevice->EndBatch();
}

} // namespace Frame2