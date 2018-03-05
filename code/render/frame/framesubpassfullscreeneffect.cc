//------------------------------------------------------------------------------
// framefullscreeneffect.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "framesubpassfullscreeneffect.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/shaderserver.h"

using namespace CoreGraphics;
namespace Frame
{

__ImplementClass(Frame::FrameSubpassFullscreenEffect, 'FFLE', Frame::FrameOp);
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
	shaderServer->SetActiveShader(this->shaderState->GetShader());
	this->shaderState->Apply();

	// draw
	renderDevice->BeginBatch(FrameBatchType::System);
	this->fsq.ApplyMesh();
	this->shaderState->Commit();
	this->fsq.Draw();
	renderDevice->EndBatch();
}

} // namespace Frame2