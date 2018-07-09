//------------------------------------------------------------------------------
// tonemapalgorithm.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "tonemapalgorithm.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/graphicsdevice.h"
#include "framesync/framesynctimer.h"

using namespace CoreGraphics;
namespace Algorithms
{

//------------------------------------------------------------------------------
/**
*/
TonemapAlgorithm::TonemapAlgorithm()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
TonemapAlgorithm::~TonemapAlgorithm()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
TonemapAlgorithm::Setup()
{
	Algorithm::Setup();
	n_assert(this->renderTextures.Size() == 2);

	RenderTextureCreateInfo rtinfo = 
	{
		"Tonemapping-Downsample2x2",
		Texture2D,
		RenderTextureGetPixelFormat(this->renderTextures[0]),
		ColorAttachment,
		2, 2, 1,
		1, 1,
		false, false, false
	};
	this->downsample2x2 = CreateRenderTexture(rtinfo);

	rtinfo.name = "Tonemapping-Copy";
	rtinfo.width = 1;
	rtinfo.height = 1;
	rtinfo.format = RenderTextureGetPixelFormat(this->renderTextures[1]);
	this->copy = CreateRenderTexture(rtinfo);

	// create shader
	this->shader = ShaderGet("shd:averagelum");
	this->tonemapShader = ShaderCreateState(this->shader, { NEBULAT_BATCH_GROUP }, false);
	this->timevar = ShaderStateGetConstant(this->tonemapShader, "TimeDiff");
	this->colorvar = ShaderStateGetConstant(this->tonemapShader, "ColorSource");
	this->prevvar = ShaderStateGetConstant(this->tonemapShader, "PreviousLum");
	this->program = ShaderGetProgram(this->shader, ShaderFeatureFromString("Alt0"));
	ShaderResourceSetRenderTexture(this->prevvar, this->tonemapShader, this->copy);
	ShaderResourceSetRenderTexture(this->colorvar, this->tonemapShader, this->downsample2x2);
	this->fsq.Setup(2, 2);

	// begin by copying and mipping down to a 2x2 texture
	this->AddFunction("Downsample", Algorithm::Compute, [this](IndexT)
	{
		CoreGraphics::Blit(this->renderTextures[0], Math::rectangle<int>(0, 0, 512, 512), 0, this->downsample2x2, Math::rectangle<int>(0, 0, 2, 2), 0);
	});

	// this pass calculates tonemapping from 2x2 cluster down to single pixel, called from the script
	this->AddFunction("AverageLum", Algorithm::Graphics, [this](IndexT)
	{
		Timing::Time time = FrameSync::FrameSyncTimer::Instance()->GetFrameTime();
		CoreGraphics::SetShaderProgram(this->program);
		CoreGraphics::BeginBatch(Frame::FrameBatchType::System);
		this->fsq.ApplyMesh();
		ShaderConstantSet(this->timevar, this->tonemapShader, (float)time);
		CoreGraphics::SetShaderState(this->tonemapShader);
		this->fsq.Draw();
		CoreGraphics::EndBatch();
	});

	// last pass, copy from render target to copy
	this->AddFunction("Copy", Algorithm::Compute, [this](IndexT)
	{
		CoreGraphics::Copy(this->renderTextures[1], Math::rectangle<int>(0, 0, 1, 1), this->copy, Math::rectangle<int>(0, 0, 1, 1));
	});
}

//------------------------------------------------------------------------------
/**
*/
void
TonemapAlgorithm::Discard()
{
	Algorithm::Discard();
	DestroyRenderTexture(this->downsample2x2);
	DestroyRenderTexture(this->copy);
	ShaderDestroyState(this->tonemapShader);
	this->fsq.Discard();
}

} // namespace Algorithms
