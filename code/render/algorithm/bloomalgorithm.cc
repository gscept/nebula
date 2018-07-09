//------------------------------------------------------------------------------
// bloomalgorithm.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "bloomalgorithm.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/shaderrwtexture.h"
#include "coregraphics/barrier.h"

using namespace CoreGraphics;
namespace Algorithms
{
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
	this->brightPassShader = ShaderGet("shd:brightpass");
	this->blurShader = ShaderGet("shd:blur_2d_rgb16f_cs");
	this->brightPass = ShaderCreateState(this->brightPassShader, { NEBULAT_BATCH_GROUP }, false);
	this->blur = ShaderCreateState(this->blurShader, { NEBULAT_BATCH_GROUP }, false);

	TextureDimensions dims = ShaderRWTextureGetDimensions(this->readWriteTextures[0]);
	ShaderRWTextureCreateInfo tinfo = 
	{
		"Bloom-Internal0",
		Texture2D,
		CoreGraphics::PixelFormat::R16G16B16A16F,
		dims.width, dims.height, dims.depth,
		1, 1,
		false, false
	};
	this->internalTargets[0] = CreateShaderRWTexture(tinfo);

	CoreGraphics::BarrierCreateInfo binfo =
	{
		BarrierDomain::Global,
		BarrierDependency::ComputeShader,
		BarrierDependency::ComputeShader
	};
	ImageSubresourceInfo subres;
	subres.aspect = ImageAspect::ColorBits;
	
	binfo.shaderRWTextures.Append(std::make_tuple(this->internalTargets[0], subres, ImageLayout::General, ImageLayout::General, BarrierAccess::ShaderWrite, BarrierAccess::ShaderRead));
	this->barriers[0] = CreateBarrier(binfo);

	this->brightPassColor = ShaderStateGetConstant(this->brightPass, "ColorSource");
	this->brightPassLuminance = ShaderStateGetConstant(this->brightPass, "LuminanceTexture");
	ShaderResourceSetRenderTexture(this->brightPassColor, this->brightPass, this->renderTextures[0]);
	ShaderResourceSetRenderTexture(this->brightPassLuminance, this->brightPass, this->renderTextures[1]);
	// get brightpass variables and set them up

	// get blur variables and set them up
	this->blurInputX = ShaderStateGetConstant(this->blur, "InputImageX");
	this->blurInputY = ShaderStateGetConstant(this->blur, "InputImageY");
	this->blurOutputX = ShaderStateGetConstant(this->blur, "BlurImageX");
	this->blurOutputY = ShaderStateGetConstant(this->blur, "BlurImageY");

	this->blurX = ShaderGetProgram(this->blurShader, ShaderFeatureFromString("Alt0"));
	this->blurY = ShaderGetProgram(this->blurShader, ShaderFeatureFromString("Alt1"));
	this->brightPassProgram = ShaderGetProgram(this->brightPassShader, ShaderFeatureFromString("Alt0"));

	// setup blur, we start by feeding the bloom rendered, then we just use the internal target (which should work, since we use barriers to block cross-tile execution)
	ShaderResourceSetRenderTexture(this->blurInputX, this->blur, this->renderTextures[2]);
	ShaderResourceSetReadWriteTexture(this->blurOutputX, this->blur, this->internalTargets[0]);
	ShaderResourceSetReadWriteTexture(this->blurInputY, this->blur, this->internalTargets[0]);
	ShaderResourceSetReadWriteTexture(this->blurOutputY, this->blur, this->readWriteTextures[0]);

	dims = ShaderRWTextureGetDimensions(this->readWriteTextures[0]);

	// get size of target texture
	this->fsq.Setup(dims.width, dims.height);

	this->AddFunction("BrightnessLowpass", Algorithm::Graphics, [this](IndexT)
	{
		CoreGraphics::SetShaderProgram(this->brightPassProgram);
		CoreGraphics::BeginBatch(Frame::FrameBatchType::System);
		this->fsq.ApplyMesh();		
		CoreGraphics::SetShaderState(this->brightPass);
		this->fsq.Draw();
		CoreGraphics::EndBatch();
	});

	this->AddFunction("BrightnessBlur", Algorithm::Compute, [this, dims](IndexT)
	{

#define TILE_WIDTH 320
#define DivAndRoundUp(a, b) (a % b != 0) ? (a / b + 1) : (a / b)

		// calculate execution dimensions
		uint numGroupsX1 = DivAndRoundUp(dims.width, TILE_WIDTH);
		uint numGroupsX2 = dims.width;
		uint numGroupsY1 = DivAndRoundUp(dims.height, TILE_WIDTH);
		uint numGroupsY2 = dims.height;

		CoreGraphics::SetShaderProgram(this->blurX);
		CoreGraphics::SetShaderState(this->blur);
		CoreGraphics::Compute(numGroupsX1, numGroupsY2, 1);

		// insert barrier between passes
		CoreGraphics::InsertBarrier(this->barriers[0], ComputeQueueType);

		CoreGraphics::SetShaderProgram(this->blurY);
		CoreGraphics::SetShaderState(this->blur);
		CoreGraphics::Compute(numGroupsY1, numGroupsX2, 1);

	});
}

//------------------------------------------------------------------------------
/**
*/
void
BloomAlgorithm::Discard()
{
	ShaderDestroyState(this->blur);
	ShaderDestroyState(this->brightPass);
	DestroyBarrier(this->barriers[0]);
	DestroyShaderRWTexture(this->internalTargets[0]);
	this->fsq.Discard();
}

} // namespace Algorithm