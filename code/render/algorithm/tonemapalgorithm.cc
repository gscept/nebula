//------------------------------------------------------------------------------
// tonemapalgorithm.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "tonemapalgorithm.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/renderdevice.h"
#include "framesync/framesynctimer.h"

namespace Algorithms
{

__ImplementClass(Algorithms::TonemapAlgorithm, 'TONE', Algorithms::Algorithm);
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

	// setup downsample target
	this->downsample2x2 = CoreGraphics::RenderTexture::Create();
	this->downsample2x2->SetResourceId("Tonemapping-Downsample2x2");
	this->downsample2x2->SetIsWindowTexture(false);
	this->downsample2x2->SetIsScreenRelative(false);
	this->downsample2x2->SetIsDynamicScaled(false);
	this->downsample2x2->SetLayers(1);
	this->downsample2x2->SetDimensions(2, 2);
	this->downsample2x2->SetEnableMSAA(false);
	this->downsample2x2->SetPixelFormat(this->renderTextures[0]->GetPixelFormat());
	this->downsample2x2->SetTextureType(CoreGraphics::Texture::Texture2D);
	this->downsample2x2->SetUsage(CoreGraphics::RenderTexture::ColorAttachment);
	this->downsample2x2->Setup();

	// setup copy target
	this->copy = CoreGraphics::RenderTexture::Create();
	this->copy->SetResourceId("Tonemapping-Copy");
	this->copy->SetIsWindowTexture(false);
	this->copy->SetIsScreenRelative(false);
	this->copy->SetIsDynamicScaled(false);
	this->copy->SetLayers(1);
	this->copy->SetDimensions(1, 1);
	this->copy->SetEnableMSAA(false);
	this->copy->SetPixelFormat(this->renderTextures[1]->GetPixelFormat());
	this->copy->SetTextureType(CoreGraphics::Texture::Texture2D);
	this->copy->SetUsage(CoreGraphics::RenderTexture::ColorAttachment);
	this->copy->Setup();

	// get render device
	CoreGraphics::RenderDevice* dev = CoreGraphics::RenderDevice::Instance();

	// create shader
	this->shader = CoreGraphics::ShaderServer::Instance()->CreateShaderState("shd:averagelum", { NEBULAT_DEFAULT_GROUP });
	this->timevar = this->shader->GetVariableByName("TimeDiff");
	this->colorvar = this->shader->GetVariableByName("ColorSource");
	this->prevvar = this->shader->GetVariableByName("PreviousLum");
	this->prevvar->SetTexture(this->copy->GetTexture());
	this->colorvar->SetTexture(this->downsample2x2->GetTexture());
	this->fsq.Setup(2, 2);

	// begin by copying and mipping down to a 2x2 texture
	this->AddFunction("Downsample", Algorithm::Compute, [this](IndexT)
	{
		this->renderTextures[0]->Blit(0, 0, this->downsample2x2);
	});

	// this pass calculates tonemapping from 2x2 cluster down to single pixel, called from the script
	this->AddFunction("AverageLum", Algorithm::Graphics, [this, dev](IndexT)
	{
		Timing::Time time = FrameSync::FrameSyncTimer::Instance()->GetFrameTime();
		this->shader->Apply();
		dev->BeginBatch(CoreGraphics::FrameBatchType::System);
		this->fsq.ApplyMesh();
		this->timevar->SetFloat((float)time);
		this->shader->Commit();
		this->fsq.Draw();
		dev->EndBatch();
	});

	// last pass, copy from render target to copy
	this->AddFunction("Copy", Algorithm::Compute, [this](IndexT)
	{
		this->renderTextures[1]->Blit(0, 0, this->copy);
	});
}

//------------------------------------------------------------------------------
/**
*/
void
TonemapAlgorithm::Discard()
{
	Algorithm::Discard();
	this->downsample2x2->Discard();
	this->copy->Discard();
	this->shader->Discard();
	this->prevvar = nullptr;
	this->colorvar = nullptr;
	this->timevar = nullptr;
	this->fsq.Discard();
}

} // namespace Algorithms
