//------------------------------------------------------------------------------
// bloomalgorithm.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "bloomalgorithm.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/framebatchtype.h"
#include "coregraphics/shaderreadwritetexture.h"
#include "coregraphics/barrier.h"

namespace Algorithms
{
__ImplementClass(Algorithms::BloomAlgorithm, 'BLOO', Algorithms::Algorithm);
//------------------------------------------------------------------------------
/**
*/
BloomAlgorithm::BloomAlgorithm()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
BloomAlgorithm::~BloomAlgorithm()
{
	// empty
}


//------------------------------------------------------------------------------
/**
*/
void
BloomAlgorithm::Setup()
{
	Algorithm::Setup();
	n_assert(this->renderTextures.Size() == 3);
	n_assert(this->readWriteTextures.Size() == 1);

	// setup shaders
	this->brightPassShader = CoreGraphics::ShaderServer::Instance()->CreateShaderState("shd:brightpass", { NEBULAT_DEFAULT_GROUP });
	this->blurShader = CoreGraphics::ShaderServer::Instance()->CreateShaderState("shd:blur_2d_rgb16f_cs", { NEBULAT_DEFAULT_GROUP });

	this->internalTargets[0] = CoreGraphics::ShaderReadWriteTexture::Create();
	this->internalTargets[0]->Setup(this->readWriteTextures[0]->GetWidth(), this->readWriteTextures[0]->GetHeight(), CoreGraphics::PixelFormat::R16G16B16A16F, "Bloom-Internal0");

	this->barriers[0] = CoreGraphics::Barrier::Create();
	this->barriers[0]->SetLeftDependency(CoreGraphics::Barrier::Dependency::ComputeShader);
	this->barriers[0]->SetRightDependency(CoreGraphics::Barrier::Dependency::ComputeShader);
	this->barriers[0]->SetDomain(CoreGraphics::Barrier::Domain::Global);
	this->barriers[0]->AddReadWriteTexture(this->internalTargets[0], CoreGraphics::Barrier::Access::ShaderWrite, CoreGraphics::Barrier::Access::ShaderRead);
	this->barriers[0]->Setup();

	// get brightpass variables and set them up
	this->brightPassColor = this->brightPassShader->GetVariableByName("ColorSource");
	this->brightPassLuminance = this->brightPassShader->GetVariableByName("LuminanceTexture");
	this->brightPassColor->SetTexture(this->renderTextures[0]->GetTexture());
	this->brightPassLuminance->SetTexture(this->renderTextures[1]->GetTexture());

	// get blur variables and set them up
	this->blurInputX = this->blurShader->GetVariableByName("InputImageX");
	this->blurInputY = this->blurShader->GetVariableByName("InputImageY");
	this->blurOutputX = this->blurShader->GetVariableByName("BlurImageX");
	this->blurOutputY = this->blurShader->GetVariableByName("BlurImageY");

	this->blurX = CoreGraphics::ShaderServer::Instance()->FeatureStringToMask("Alt0");
	this->blurY = CoreGraphics::ShaderServer::Instance()->FeatureStringToMask("Alt1");

	// setup blur, we start by feeding the bloom rendered, then we just use the internal target (which should work, since we use barriers to block cross-tile execution)
	this->blurInputX->SetTexture(this->renderTextures[2]->GetTexture());
	this->blurInputY->SetTexture(this->internalTargets[0]->GetTexture());
	this->blurOutputX->SetShaderReadWriteTexture(this->internalTargets[0]);
	this->blurOutputY->SetShaderReadWriteTexture(this->readWriteTextures[0]);

	// get size of target texture
	uint width = this->readWriteTextures[0]->GetWidth();
	uint height = this->readWriteTextures[0]->GetHeight();
	this->fsq.Setup(width, height);

	// get render device
	CoreGraphics::RenderDevice* dev = CoreGraphics::RenderDevice::Instance();

	this->AddFunction("BrightnessLowpass", Algorithm::Graphics, [this, dev](IndexT)
	{
		this->brightPassShader->Apply();
		dev->BeginBatch(CoreGraphics::FrameBatchType::System);
		this->fsq.ApplyMesh();		
		this->brightPassShader->Commit();
		this->fsq.Draw();
		dev->EndBatch();
	});

	this->AddFunction("BrightnessBlur", Algorithm::Compute, [this, dev, width, height](IndexT)
	{

#define TILE_WIDTH 320
#define DivAndRoundUp(a, b) (a % b != 0) ? (a / b + 1) : (a / b)

		// calculate execution dimensions
		uint numGroupsX1 = DivAndRoundUp(width, TILE_WIDTH);
		uint numGroupsX2 = width;
		uint numGroupsY1 = DivAndRoundUp(height, TILE_WIDTH);
		uint numGroupsY2 = height;

		this->blurShader->SelectActiveVariation(this->blurX);
		this->blurShader->Apply();
		this->blurShader->Commit();
		dev->Compute(numGroupsX1, numGroupsY2, 1);

		// just ensure the first compute is done before the other... dunno if we actually need it
		dev->InsertBarrier(this->barriers[0]);

		this->blurShader->SelectActiveVariation(this->blurY);
		this->blurShader->Apply();
		this->blurShader->Commit();
		dev->Compute(numGroupsY1, numGroupsX2, 1);

	});
}

//------------------------------------------------------------------------------
/**
*/
void
BloomAlgorithm::Discard()
{
	this->blurShader->Discard();
	this->brightPassShader->Discard();
	this->barriers[0]->Discard();
	this->internalTargets[0]->Discard();
	this->brightPassColor = nullptr;
	this->brightPassLuminance = nullptr;
	this->blurInputX = nullptr;
	this->blurInputY = nullptr;
	this->blurOutputX = nullptr;
	this->blurOutputY = nullptr;
	this->fsq.Discard();
}

} // namespace Algorithm