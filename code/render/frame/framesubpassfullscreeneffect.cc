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
	this->fsq.Setup(this->tex->GetWidth(), this->tex->GetHeight());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassFullscreenEffect::Discard()
{
	FrameOp::Discard();

	this->fsq.Discard();
	this->tex = 0;
	this->shaderState = 0;
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